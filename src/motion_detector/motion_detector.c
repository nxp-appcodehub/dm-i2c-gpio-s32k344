/*==================================================================================================
* Project : RTD AUTOSAR 4.9
* Platform : CORTEXM
* Peripheral : S32K3XX
* Dependencies : none
*
* Autosar Version : 4.9.0
* Autosar Revision : ASR_REL_4_9_REV_0000
* Autosar Conf.Variant :
* SW Version : 7.0.0
* Build Version : S32K3_RTD_7_0_0_QLP03_D2512_ASR_REL_4_9_REV_0000_20251210
*
* Copyright 2020 - 2026 NXP
*
*   NXP Proprietary. This software is owned or controlled by NXP and may only be
*   used strictly in accordance with the applicable license terms. By expressly
*   accepting such terms or by downloading, installing, activating and/or otherwise
*   using the software, you are agreeing that you have read, and that you agree to
*   comply with and are bound by, such license terms. If you do not agree to be
*   bound by the applicable license terms, then you may not retain, install,
*   activate or otherwise use the software.
==================================================================================================*/

#include "motion_detector.h"
#include "Siul2_Port_Ip.h"
#include <math.h>

/* Shake detection state */
static uint32 shake_cooldown = 0;  // Countdown timer for shake animation

void SimpleDelay(uint32 delay) {
    for (uint32 i = 0; i < delay; i++) {
        __asm("NOP");
    }
}

void TurnOffAllLEDS(void) {
    Siul2_Dio_Ip_WritePin(RED_LED_PORT, RED_LED_PIN, 0U);
    Siul2_Dio_Ip_WritePin(GREEN_LED_PORT, GREEN_LED_PIN, 0U);
    Siul2_Dio_Ip_WritePin(BLUE_LED_PORT, BLUE_LED_PIN, 0U);
}

void TurnOnAllLEDS(void) {
    Siul2_Dio_Ip_WritePin(RED_LED_PORT, RED_LED_PIN, 1U);
    Siul2_Dio_Ip_WritePin(GREEN_LED_PORT, GREEN_LED_PIN, 1U);
    Siul2_Dio_Ip_WritePin(BLUE_LED_PORT, BLUE_LED_PIN, 1U);
}

void SetLEDColor(uint8 red, uint8 green, uint8 blue) {
    Siul2_Dio_Ip_WritePin(RED_LED_PORT, RED_LED_PIN, red);
    Siul2_Dio_Ip_WritePin(GREEN_LED_PORT, GREEN_LED_PIN, green);
    Siul2_Dio_Ip_WritePin(BLUE_LED_PORT, BLUE_LED_PIN, blue);
}

uint8 DetectShake(float x, float y, float z) {
    static float prev_x = 0, prev_y = 0, prev_z = 0;
    static uint8 first_run = 1;

    /* Skip first run to initialize previous values */
    if (first_run) {
        prev_x = x;
        prev_y = y;
        prev_z = z;
        first_run = 0;
        return 0;
    }

    /* Calculate change in acceleration */
    float delta_x = fabsf(x - prev_x);
    float delta_y = fabsf(y - prev_y);
    float delta_z = fabsf(z - prev_z);

    float total_delta = delta_x + delta_y + delta_z;

    /* Store current values for next iteration */
    prev_x = x;
    prev_y = y;
    prev_z = z;

    /* Return TRUE if shake detected */
    if (total_delta > 15.0f) {
        return 1;  // Shake detected!
    }

    return 0;  // No shake
}

void HandleShakeAnimation(void) {
    /* Flash pattern: 3 quick flashes */
    for (uint8 i = 0; i < 3; i++) {
        TurnOnAllLEDS();
        SimpleDelay(100000U);
        TurnOffAllLEDS();
        SimpleDelay(100000U);
    }

    /* Set cooldown to prevent re-triggering immediately */
    shake_cooldown = 10;  // Skip next 10 tilt updates
}

void UpdateTiltIndicator(float x, float y, float z) {
    float accelX_abs = fabsf(x);
    float accelY_abs = fabsf(y);
    float accelZ_abs = fabsf(z);

    /* Turn off all LEDs first */
    TurnOffAllLEDS();

    /* Light LED based on dominant axis */
    if (accelX_abs > AXIS_THRESHOLD) {
        if (accelY_abs < AXIS_THRESHOLD && accelZ_abs < AXIS_THRESHOLD) {
            /* X-axis dominant */
            if (x > 0) {
                /* Positive X: Pure RED */
                SetLEDColor(1, 0, 0);
            } else {
                /* Negative X: ORANGE (Red + Green) */
                SetLEDColor(1, 1, 0);
            }
        }
    }
    else if (accelY_abs > AXIS_THRESHOLD) {
        if (accelX_abs < AXIS_THRESHOLD && accelZ_abs < AXIS_THRESHOLD) {
            /* Y-axis dominant */
            if (y > 0) {
                /* Positive Y: Pure GREEN */
                SetLEDColor(0, 1, 0);
            } else {
                /* Negative Y: CYAN (Green + Blue) */
                SetLEDColor(0, 1, 1);
            }
        }
    }
    else if (accelZ_abs > AXIS_THRESHOLD) {
        if (accelY_abs < AXIS_THRESHOLD && accelX_abs < AXIS_THRESHOLD) {
            /* Z-axis dominant */
            if (z > 0) {
                /* Positive Z: Pure BLUE */
                SetLEDColor(0, 0, 1);
            } else {
                /* Negative Z: MAGENTA (Red + Blue) */
                SetLEDColor(1, 0, 1);
            }
        }
    }
}

uint32 GetShakeCooldown(void) {
    return shake_cooldown;
}

void DecrementShakeCooldown(void) {
    if (shake_cooldown > 0) {
        shake_cooldown--;
    }
}
