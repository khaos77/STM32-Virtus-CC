# Lab Timer/UART STM32 – Gerador senoidal por PWM

Formação em Sistemas Embarcados Virtus-CC – Curso 2 (Bare Metal).
Placa: **NUCLEO-F767ZI** (STM32F767ZIT6), projeto STM32CubeMX / STM32CubeIDE.

Gera uma senoide a partir de um PWM de 100 kHz cujo duty cycle varia
senoidalmente, filtrado por um passa-baixas RC. A frequência é ajustada pela
UART (Virtual COM do ST-LINK).

## Ligações

| Função       | Pino | Periférico | Onde na placa        |
|--------------|------|------------|----------------------|
| PWM senoidal | PA6  | TIM3_CH1   | CN7 pino 12 (D12)    |
| Console      | PD8/PD9 | USART3  | USB do ST-LINK, 115200 8N1 |
| LED status   | PB0  | GPIO       | LD1 (verde), pisca a 1 Hz |
| LED erro     | PB14 | GPIO       | LD3 (vermelho), comando inválido |

```
            R = 1 kΩ
 PA6 ──────/\/\/\──────┬────── Vout (osciloscópio)
 (PWM 100 kHz)         │
                      ═╪═ C = 100 nF
                       │
 GND ──────────────────┴────── GND
```

## Solução

### PWM (TIM3)

- Clock do timer: 108 MHz (HSE 8 MHz → PLL → SYSCLK 216 MHz, APB1 = 54 MHz, timers ×2)
- `PSC = 0`, `ARR = 1079` → `f_pwm = 108 MHz / 1080 = 100 kHz`, 1080 níveis de duty
- Preload de `CCR1` habilitado: o duty só muda no evento de update
- Interrupção de update a 100 kHz calcula o próximo duty

### Senoide por DDS

Em vez de alterar a frequência do PWM (seção 11.3.7.1 do *Mastering STM32*), a
portadora é fixa e a senoide é sintetizada por um acumulador de fase de 32 bits:

```
fase  += incremento               (a cada período de PWM, fs = 100 kHz)
CCR1   = tabela[fase >> 22]       (tabela de 1024 amostras)

incremento = f_out · 2^32 / fs
tabela[n]  = (P/2) · (1 + m · sen(2π·n/1024)),   P = 1080,  m = 0,90
```

- Resolução de frequência: `fs / 2^32 ≈ 23 µHz`
- Troca de frequência sem salto de fase (só o incremento muda)
- Filtro projetado uma única vez, pois a portadora não muda

### Filtro RC

```
fc = 1 / (2π·R·C) = 1 / (2π · 1 kΩ · 100 nF) ≈ 1591,5 Hz
|H(f)| = 1 / sqrt(1 + (f/fc)²)
```

| Frequência          | \|H\|  | dB       |
|---------------------|--------|----------|
| 60 Hz               | 0,999  | −0,006   |
| 1 kHz (máximo)      | 0,847  | −1,44    |
| 100 kHz (portadora) | 0,0159 | −36,0    |

Saída esperada: senoide de ≈ 1,49 V de pico (m · 3,3 V / 2 · |H|) sobre um
offset de 1,65 V.

### Limites de frequência (UART)

**1 Hz a 1000 Hz** (`App/Inc/app_config.h`). Acima de 1 kHz a fundamental passa
a ser atenuada em mais de 1,5 dB e sobram menos de 100 períodos de PWM por
ciclo da senoide; valores fora da faixa são rejeitados e a frequência anterior
é mantida.

### Redução de THD

- Portadora fixa e alta (≥ 100× a fundamental)
- Tabela de 1024 amostras, compatível com a resolução do PWM (1080 níveis)
- Atualização síncrona do duty (preload), sem glitches
- Índice de modulação 0,90: evita pulsos estreitos demais nos picos

O RC de 1ª ordem ainda deixa um ripple de portadora da ordem de 2 % da
amplitude (≈ 33 mV de pico). Para reduzir mais, cascatear um segundo estágio RC
(−72 dB em 100 kHz) ou aumentar `C` reduzindo o limite superior de frequência.
O THD ainda **não foi medido** em bancada.

## Comandos da UART

| Comando        | Ação                                   |
|----------------|----------------------------------------|
| `60` ou `f 60` | Define a frequência em Hz (aceita `59.5` ou `59,5`) |
| `on` / `off`   | Liga / desliga a saída                 |
| `?`            | Estado atual e ajuda                   |

Frequência inicial após o reset: 60 Hz.

## Roteiro de testes

1. Abrir o terminal serial (115200 8N1) e enviar a frequência inicial, ex.: `60`
2. Observar a forma de onda em `Vout` no osciloscópio
3. Observar o espectro (FFT) e avaliar o THD
4. Enviar nova frequência, ex.: `500`, e repetir os passos 2 e 3

## Arquitetura

```
Core/   código gerado pelo CubeMX (clock, MX_*_Init, vetores de interrupção)
BSP/    acesso ao hardware (única camada que chama a HAL)
  bsp.c          inicialização agregada, bsp_millis()
  bsp_pwm.c      TIM3_CH1: start/stop, compare, callback de update
  bsp_console.c  USART3: printf (TX) e buffer circular de RX por interrupção
  bsp_led.c      LEDs de status e erro
App/    lógica independente de hardware
  sine_gen.c     DDS: tabela de seno, acumulador de fase, limites
  cli.c          linha de comando (parser, eco, mensagens)
  app.c          app_init() / app_loop() chamados pelo main.c
```

Fluxo: `TIM3 update IRQ → bsp_pwm → sine_gen (novo CCR1)` e
`USART3 RX IRQ → buffer circular → cli_process() no superloop → sine_gen_set_frequency()`.

## Documentação (Doxygen)

```
doxygen Doxyfile
```

Saída em `docs/html/index.html`.
