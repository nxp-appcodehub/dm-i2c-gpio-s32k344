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

/**
*   @file main.c
*
*   @addtogroup main_module main module documentation
*   @{
*/

#ifdef __cplusplus
extern "C"{
#endif

#include "Mcal.h"
#include "Clock_Ip.h"
#include "Lpi2c_Ip.h"
#include "Lpi2c_Ip_Cfg.h"

#include "accelerometer.h"
#include "temperature.h"
#include "motion_detector.h"

volatile int exit_code = 0;

/* Buffers to store sensor data */
uint8 accelData[6];  /* X, Y, Z data (2 bytes each) */
uint8 tempData[2];   /* Temperature data */

int main(void) {
    Lpi2c_Ip_StatusType i2cStatus;
    float accelX, accelY, accelZ;
    float temperature;

    /* Init system clock */
    Clock_Ip_Init(&Clock_Ip_aClockConfig[0]);
    Siul2_Port_Ip_Init(
    NUM_OF_CONFIGURED_PINS_PortContainer_0_BOARD_InitPeripherals,
            g_pin_mux_InitConfigArr_PortContainer_0_BOARD_InitPeripherals);

    /* Initialize LPI2C Master */
    Lpi2c_Ip_MasterInit(LPI2C_INSTANCE, &I2c_Lpi2cMaster_HwChannel1_Channel0);

    TurnOffAllLEDS();
    SimpleDelay(100000U);

    /* Initialize accelerometer */
    i2cStatus = InitAccelerometer();
    if (i2cStatus != LPI2C_IP_SUCCESS_STATUS) {
        exit_code = 1;  // Initialization failed
        while (1);  // Halt
    }

    for (;;) {
        /* Read temperature data */
        i2cStatus = ReadTemperature(tempData);
        if (i2cStatus == LPI2C_IP_SUCCESS_STATUS) {
            temperature = ConvertTemperature_TMP102(tempData);

            /* High temperature alert - highest priority */
            while (temperature > TEMPERATURE_THRESHOLD) {
                TurnOnAllLEDS();
                SimpleDelay(500000U);
                TurnOffAllLEDS();
                SimpleDelay(500000U);

                /* Re-read temperature */
                i2cStatus = ReadTemperature(tempData);
                if (i2cStatus != LPI2C_IP_SUCCESS_STATUS){
                    exit_code = 1;
                    break;
                }
                temperature = ConvertTemperature_TMP102(tempData);
            }
        }

        if (exit_code != 0) {
            break;
        }

        /* Read accelerometer data */
        i2cStatus = ReadAccelerometer(accelData);

        if (i2cStatus == LPI2C_IP_SUCCESS_STATUS) {
            /* Convert acceleration data */
            ConvertAcceleration_FXLS8964(accelData, &accelX, &accelY, &accelZ);

            /* Priority 1: Check for shake - overrides tilt indicator */
            if (DetectShake(accelX, accelY, accelZ)) {
                HandleShakeAnimation();
            }

            /* Priority 2: Tilt indicator (only if not in shake cooldown) */
            if (GetShakeCooldown() > 0) {
                DecrementShakeCooldown();
                TurnOffAllLEDS();
            }
            else {
                /* Normal operation - update tilt indicator with different colors */
                UpdateTiltIndicator(accelX, accelY, accelZ);
            }
        }

        /* Wait before next iteration */
        SimpleDelay(100000U);

        if (exit_code != 0) {
            break;
        }
    }

    return exit_code;
}

#ifdef __cplusplus
}
#endif
