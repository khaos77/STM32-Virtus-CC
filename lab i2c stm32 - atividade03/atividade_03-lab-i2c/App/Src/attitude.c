/**
 * @file    attitude.c
 * @brief   Implementacao do filtro complementar de atitude.
 * @ingroup ATTITUDE
 */
#include "attitude.h"
#include <math.h>

#define DEG2RAD        0.01745329252f
#define RAD2DEG        57.2957795131f

/** @brief Faixa de |a| (em g) em que o acelerometro e considerado confiavel. */
#define ACC_NORM_MIN   0.5f
#define ACC_NORM_MAX   1.5f
/** @brief Abaixo deste valor de sqrt(ay^2+az^2) (g) o roll_acc e indefinido. */
#define ACC_YZ_MIN     0.15f
/** @brief Limite inferior de |cos(pitch)| nas taxas de Euler (gimbal lock). */
#define COS_PITCH_MIN  0.05f

/** @brief Reduz um angulo em graus para a faixa (-180, +180]. */
static float wrap180(float a)
{
    while (a > 180.0f)   { a -= 360.0f; }
    while (a <= -180.0f) { a += 360.0f; }
    return a;
}

void attitude_init(attitude_t *att, float alpha)
{
    att->roll        = 0.0f;
    att->pitch       = 0.0f;
    att->yaw         = 0.0f;
    att->alpha       = alpha;
    att->initialized = false;
}

void attitude_reset_yaw(attitude_t *att)
{
    att->yaw = 0.0f;
}

void attitude_update(attitude_t *att, const float accel_g[3],
                     const float gyro_dps[3], float dt)
{
    const float ax = accel_g[0], ay = accel_g[1], az = accel_g[2];
    const float p  = gyro_dps[0], q = gyro_dps[1], r = gyro_dps[2];

    /* --- 1. angulos observados pelo acelerometro ------------------------ */
    const float yz        = sqrtf(ay * ay + az * az);
    const float norm      = sqrtf(ax * ax + yz * yz);
    const float roll_acc  = atan2f(ay, az) * RAD2DEG;
    const float pitch_acc = atan2f(-ax, yz) * RAD2DEG;
    const bool  acc_ok    = (norm > ACC_NORM_MIN) && (norm < ACC_NORM_MAX);

    if (!att->initialized)
    {
        att->roll        = roll_acc;
        att->pitch       = pitch_acc;
        att->yaw         = 0.0f;
        att->initialized = true;
        return;
    }

    /* --- 2. taxas de Euler a partir das velocidades do corpo ------------ */
    const float sr = sinf(att->roll * DEG2RAD);
    const float cr = cosf(att->roll * DEG2RAD);
    float       cp = cosf(att->pitch * DEG2RAD);
    const float sp = sinf(att->pitch * DEG2RAD);
    if (fabsf(cp) < COS_PITCH_MIN)
    {
        cp = (cp < 0.0f) ? -COS_PITCH_MIN : COS_PITCH_MIN;
    }

    const float w         = q * sr + r * cr;
    const float roll_dot  = p + w * (sp / cp);
    const float pitch_dot = q * cr - r * sr;
    const float yaw_dot   = w / cp;

    /* --- 3. predicao pelo giroscopio ------------------------------------ */
    const float roll_gyro  = att->roll  + roll_dot  * dt;
    const float pitch_gyro = att->pitch + pitch_dot * dt;
    att->yaw = wrap180(att->yaw + yaw_dot * dt);

    /* --- 4. correcao pelo acelerometro (filtro complementar) ------------
     * angulo = alpha*giro + (1-alpha)*acc, escrito como
     * giro + (1-alpha)*(acc - giro) para tratar a descontinuidade em +-180. */
    const float k = 1.0f - att->alpha;

    if (acc_ok)
    {
        att->pitch = pitch_gyro + k * (pitch_acc - pitch_gyro);
        if (yz > ACC_YZ_MIN)
        {
            att->roll = wrap180(roll_gyro + k * wrap180(roll_acc - roll_gyro));
        }
        else
        {
            att->roll = wrap180(roll_gyro);   /* pitch ~ +-90: so giroscopio */
        }
    }
    else
    {
        /* aceleracao linear forte (ou queda livre): confia so no giroscopio */
        att->pitch = pitch_gyro;
        att->roll  = wrap180(roll_gyro);
    }

    if (att->pitch > 90.0f)  { att->pitch = 90.0f; }
    if (att->pitch < -90.0f) { att->pitch = -90.0f; }
}
