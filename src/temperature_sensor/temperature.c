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

#include "temperature.h"
#include "accelerometer.h"  // For LPI2C_INSTANCE and TIMEOUT

Lpi2c_Ip_StatusType ReadTemperature(uint8 *tempData) {
    Lpi2c_Ip_StatusType status;
    uint8 regAddr = 0x00U; /* Register address - check your sensor datasheet! */

    /* Step 1: Set slave address for temperature sensor */
    Lpi2c_Ip_MasterSetSlaveAddr(LPI2C_INSTANCE, TEMP_ADDR, (boolean) FALSE);

    /* Step 2: Send register address (blocking mode) */
    status = Lpi2c_Ip_MasterSendDataBlocking(
    LPI2C_INSTANCE, &regAddr, 1U, (boolean) FALSE, /* Don't send STOP yet */
    TIMEOUT);

    if (status == LPI2C_IP_SUCCESS_STATUS) {
        /* Step 3: Read 2 bytes (temperature value) */
        status = Lpi2c_Ip_MasterReceiveDataBlocking(
        LPI2C_INSTANCE, tempData, 2U, (boolean) TRUE, /* Send STOP after read */
        TIMEOUT);
    }

    return status;
}

float ConvertTemperature_TMP102(uint8 *tempData) {
    int16_t rawTemp;
    float temperature;

    /* Combine MSB and LSB */
    rawTemp = (int16_t) ((tempData[0] << 8) | tempData[1]);

    /* TMP102: 12-bit value in bits [15:4], right-shift by 4 */
    rawTemp = rawTemp >> 4;

    /* Check if temperature is negative (bit 11 set) */
    if (rawTemp & 0x800) {
        /* Extend sign for negative temperatures */
        rawTemp |= 0xF000;
    }

    /* Convert to Celsius: LSB = 0.0625°C */
    temperature = (float) rawTemp * 0.0625f;

    return temperature;
}
