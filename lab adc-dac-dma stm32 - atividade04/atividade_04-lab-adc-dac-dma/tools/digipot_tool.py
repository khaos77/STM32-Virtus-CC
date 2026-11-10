#!/usr/bin/env python3
"""Ferramenta de PC para o digipot LED + LDR (Lab ADC-DAC-DMA).

Subcomandos:
  curve   executa 'sweep' (R x res) ou 'dsweep' (R x codigo DAC) e salva em CSV
  upload  monta a look-up-table a partir de um CSV (dac, ohms) e a envia pela
          UART; no microcontrolador a recepcao e feita por DMA
  rmse    calcula o RMSE de um CSV (res, r_nominal, r_medido)

Requer:  pip install pyserial      (matplotlib e opcional, para --plot)

Exemplos:
  python digipot_tool.py curve  --port COM5 --raw --out calib.csv
  python digipot_tool.py upload --port COM5 --csv calib.csv --rmin 1000 --rmax 50000
  python digipot_tool.py curve  --port COM5 --out curva.csv --plot
  python digipot_tool.py rmse   --csv teste.csv
"""
import argparse
import csv
import math
import struct
import sys
import time

MAGIC = b"\xA5\x5A"
LUT_SIZE = 256
DAC_MAX = 4095


def crc16_ccitt(data: bytes) -> int:
    """CRC16-CCITT-FALSE (polinomio 0x1021, inicial 0xFFFF)."""
    crc = 0xFFFF
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
    return crc


def build_lut(points, r_min=None, r_max=None):
    """Interpola a LUT res -> codigo DAC a partir de pontos (dac, ohms).

    Mantem apenas pontos com resistencia estritamente decrescente quando o
    codigo do DAC cresce (mais luz = menos resistencia).
    Retorna (lut, r_min, r_max).
    """
    mono = []
    for dac, ohms in sorted(points):
        if not mono or ohms < mono[-1][1]:
            mono.append((dac, ohms))
    if len(mono) < 2:
        raise ValueError("sao necessarios ao menos 2 pontos monotonicos (dac, ohms)")

    lo, hi = mono[-1][1], mono[0][1]
    r_min = lo if r_min is None else max(r_min, lo)
    r_max = hi if r_max is None else min(r_max, hi)
    r_min, r_max = int(round(r_min)), int(round(r_max))
    if r_min >= r_max:
        raise ValueError(f"faixa invalida: r_min={r_min} >= r_max={r_max}")

    lut = []
    for res in range(LUT_SIZE):
        target = r_min + (r_max - r_min) * res / 255.0
        if target >= mono[0][1]:
            code = mono[0][0]
        elif target <= mono[-1][1]:
            code = mono[-1][0]
        else:
            for (d0, r0), (d1, r1) in zip(mono, mono[1:]):
                if r0 >= target > r1:
                    # interpola na condutancia (1/R), quase linear com o DAC
                    code = d0 + (d1 - d0) * (r1 * (r0 - target)) / (target * (r0 - r1))
                    break
        lut.append(min(DAC_MAX, max(0, int(round(code)))))
    return lut, r_min, r_max


def build_frame(lut, r_min, r_max) -> bytes:
    """Quadro: A5 5A | r_min u32 | r_max u32 | 256 x u16 | CRC16 (little-endian)."""
    body = struct.pack("<II", r_min, r_max) + struct.pack(f"<{LUT_SIZE}H", *lut)
    return MAGIC + body + struct.pack("<H", crc16_ccitt(body))


def read_points(path):
    """Le (dac, ohms) de um CSV com ';' ou ','. Aceita a saida de 'dsweep'."""
    points = []
    with open(path, newline="", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            cols = line.replace(",", ";").split(";")
            try:
                if len(cols) >= 5:      # dsweep: dac;adc;mV;R;valido
                    if cols[4].strip() != "1":
                        continue
                    points.append((int(cols[0]), float(cols[3])))
                else:                   # manual: dac;ohms
                    points.append((int(cols[0]), float(cols[1])))
            except ValueError:
                continue                # cabecalho
    return points


def open_port(port, baud):
    import serial
    ser = serial.Serial(port, baud, timeout=0.5)
    time.sleep(0.2)
    ser.reset_input_buffer()
    return ser


def wait_line(ser, prefixes, timeout):
    """Devolve a primeira linha que contenha um dos prefixos."""
    end = time.time() + timeout
    while time.time() < end:
        line = ser.readline().decode("ascii", "replace").strip()
        for p in prefixes:
            if p in line:
                return line[line.index(p):]
    raise TimeoutError(f"sem resposta ({'/'.join(prefixes)}) em {timeout} s")


def cmd_curve(args):
    ser = open_port(args.port, args.baud)
    command = f"dsweep {args.points}" if args.raw else f"sweep {args.step}"
    ser.write((command + "\r").encode())
    header = wait_line(ser, ["# res;", "# dac;"], 10)
    rows = []
    end = time.time() + 300
    while time.time() < end:
        line = ser.readline().decode("ascii", "replace").strip()
        if "# fim" in line:
            break
        if line and line[0].isdigit() and ";" in line:
            rows.append(line.split(";"))
            print(line)
    ser.close()

    with open(args.out, "w", newline="", encoding="utf-8") as f:
        f.write(header + "\n")
        csv.writer(f, delimiter=";").writerows(rows)
    print(f"{len(rows)} pontos salvos em {args.out}")

    if args.plot:
        import matplotlib.pyplot as plt
        col_r = 3 if args.raw else 4
        ok = [r for r in rows if r[-1] == "1"]
        plt.semilogy([int(r[0]) for r in ok], [float(r[col_r]) for r in ok], "o-")
        plt.xlabel("codigo DAC" if args.raw else "uint8 res")
        plt.ylabel("Resistencia_digipot (ohm)")
        plt.grid(True, which="both")
        plt.savefig(args.out.rsplit(".", 1)[0] + ".png", dpi=150)
        plt.show()


def cmd_upload(args):
    lut, r_min, r_max = build_lut(read_points(args.csv), args.rmin, args.rmax)
    frame = build_frame(lut, r_min, r_max)
    print(f"LUT: {r_min} a {r_max} ohm, DAC {min(lut)} a {max(lut)}, {len(frame)} bytes")
    if args.dry_run:
        print(" ".join(str(v) for v in lut))
        return
    ser = open_port(args.port, args.baud)
    ser.write(b"load\r")
    print(wait_line(ser, ["READY"], 3))
    ser.write(frame)
    answer = wait_line(ser, ["OK", "ERRO"], 8)
    ser.close()
    print(answer)
    sys.exit(0 if answer.startswith("OK") else 1)


def cmd_rmse(args):
    errs = []
    with open(args.csv, newline="", encoding="utf-8") as f:
        for line in f:
            cols = line.strip().replace(",", ";").split(";")
            try:
                errs.append(float(cols[2]) - float(cols[1]))
            except (ValueError, IndexError):
                continue
    if not errs:
        sys.exit("nenhum ponto valido (esperado: res;r_nominal;r_medido)")
    rmse = math.sqrt(sum(e * e for e in errs) / len(errs))
    print(f"N = {len(errs)}   RMSE = {rmse:.1f} ohm")


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = ap.add_subparsers(dest="cmd", required=True)

    c = sub.add_parser("curve", help="captura a curva R x res (ou R x DAC com --raw)")
    c.add_argument("--port", required=True)
    c.add_argument("--baud", type=int, default=115200)
    c.add_argument("--raw", action="store_true", help="usa 'dsweep' (codigo DAC, sem LUT)")
    c.add_argument("--step", type=int, default=5, help="passo de res no 'sweep'")
    c.add_argument("--points", type=int, default=64, help="pontos do 'dsweep'")
    c.add_argument("--out", default="curva.csv")
    c.add_argument("--plot", action="store_true")
    c.set_defaults(func=cmd_curve)

    u = sub.add_parser("upload", help="gera a LUT de um CSV (dac, ohms) e envia")
    u.add_argument("--port")
    u.add_argument("--baud", type=int, default=115200)
    u.add_argument("--csv", required=True)
    u.add_argument("--rmin", type=float, help="ohms em res = 0 (padrao: menor medido)")
    u.add_argument("--rmax", type=float, help="ohms em res = 255 (padrao: maior medido)")
    u.add_argument("--dry-run", action="store_true", help="so calcula e imprime a LUT")
    u.set_defaults(func=cmd_upload)

    r = sub.add_parser("rmse", help="RMSE de um CSV res;r_nominal;r_medido")
    r.add_argument("--csv", required=True)
    r.set_defaults(func=cmd_rmse)

    args = ap.parse_args()
    if args.cmd == "upload" and not args.dry_run and not args.port:
        ap.error("upload requer --port (ou --dry-run)")
    args.func(args)


if __name__ == "__main__":
    main()
