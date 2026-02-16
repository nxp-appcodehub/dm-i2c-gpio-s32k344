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

#include "accelerometer.h"
#include "motion_detector.h"
#include "Lpi2c_Ip.h"

Lpi2c_Ip_StatusType InitAccelerometer(void) {
    Lpi2c_Ip_StatusType status;
    uint8 writeData[2];
    uint8 whoAmIReg = FXLS8964_WHO_AM_I;
    uint8 whoAmIValue = 0;

    /* Set slave address */
    Lpi2c_Ip_MasterSetSlaveAddr(LPI2C_INSTANCE, ACCEL_ADDR, (boolean) FALSE);

    /* Step 1: Verify communication with WHO_AM_I register */
    status = Lpi2c_Ip_MasterSendDataBlocking(LPI2C_INSTANCE, &whoAmIReg, 1U,
            (boolean) FALSE, TIMEOUT);
    if (status == LPI2C_IP_SUCCESS_STATUS) {
        status = Lpi2c_Ip_MasterReceiveDataBlocking(LPI2C_INSTANCE,
                &whoAmIValue, 1U, (boolean) TRUE, TIMEOUT);

        /* Check if it's FXLS8964 (0x86) or FXLS8974 (0x87) */
        if (whoAmIValue != 0x86U && whoAmIValue != 0x87U) {
            return LPI2C_IP_ERROR_STATUS;  // Wrong device
        }
    } else {
        return status;  // Communication failed
    }

    SimpleDelay(10000U);

    /* Step 2: Enter Standby mode (required for configuration) */
    writeData[0] = FXLS8964_SENS_CONFIG1;
    writeData[1] = 0x00U;  // ACTIVE=0, FSR=00b (±2g)
    status = Lpi2c_Ip_MasterSendDataBlocking(LPI2C_INSTANCE, writeData, 2U,
            (boolean) TRUE, TIMEOUT);
    if (status != LPI2C_IP_SUCCESS_STATUS)
        return status;

    SimpleDelay(10000U);

    /* Step 3: Configure data format - CRITICAL! */
    writeData[0] = FXLS8964_SENS_CONFIG2;
    writeData[1] = 0x00U;  // LE_BE=0 (little-endian), F_READ=0 (12-bit), LPM
    status = Lpi2c_Ip_MasterSendDataBlocking(LPI2C_INSTANCE, writeData, 2U,
            (boolean) TRUE, TIMEOUT);
    if (status != LPI2C_IP_SUCCESS_STATUS)
        return status;

    SimpleDelay(10000U);

    /* Step 4: Set ODR to 100 Hz */
    writeData[0] = FXLS8964_SENS_CONFIG3;
    writeData[1] = 0x55U;  // WAKE_ODR=100Hz, SLEEP_ODR=100Hz
    status = Lpi2c_Ip_MasterSendDataBlocking(LPI2C_INSTANCE, writeData, 2U,
            (boolean) TRUE, TIMEOUT);
    if (status != LPI2C_IP_SUCCESS_STATUS)
        return status;

    SimpleDelay(10000U);

    /* Step 5: Enter Active mode */
    writeData[0] = FXLS8964_SENS_CONFIG1;  // SENS_CONFIG1
    writeData[1] = 0x01U;  // ACTIVE=1, FSR=00b (±2g)
    status = Lpi2c_Ip_MasterSendDataBlocking(LPI2C_INSTANCE, writeData, 2U,
            (boolean) TRUE, TIMEOUT);

    SimpleDelay(50000U);  // Wait for sensor to stabilize

    return status;
}

Lpi2c_Ip_StatusType ReadAccelerometer(uint8 *accelData) {
    Lpi2c_Ip_StatusType status;
    uint8 regAddr = FXLS8964_OUT_X_LSB; /* Start at X-axis LSB register (0x04) */

    /* Step 1: Set slave address */
    Lpi2c_Ip_MasterSetSlaveAddr(LPI2C_INSTANCE, ACCEL_ADDR, (boolean) FALSE);

    /* Step 2: Send register address (write operation) */
    status = Lpi2c_Ip_MasterSendDataBlocking(
    LPI2C_INSTANCE, &regAddr, 1U, (boolean) FALSE, TIMEOUT);

    if (status == LPI2C_IP_SUCCESS_STATUS) {
        /* Step 3: Read 6 bytes (X, Y, Z acceleration data) */
        status = Lpi2c_Ip_MasterReceiveDataBlocking(
        LPI2C_INSTANCE, accelData, 6U, (boolean) TRUE, TIMEOUT);
    }
    return status;
}

void ConvertAcceleration_FXLS8964(uint8 *accelData, float *x, float *y, float *z) {
    // The sensor provides sign-extended 16-bit 2's complement values
    int16_t rawX = (int16_t) ((accelData[1] << 8) | accelData[0]);
    int16_t rawY = (int16_t) ((accelData[3] << 8) | accelData[2]);
    int16_t rawZ = (int16_t) ((accelData[5] << 8) | accelData[4]);

    if (rawX & 0x0800)
        rawX |= 0xF000;
    if (rawY & 0x0800)
        rawY |= 0xF000;
    if (rawZ & 0x0800)
        rawZ |= 0xF000;

    const float G_PER_LSB = 1 / 1024.0f;
    const float G_TO_MS2 = 9.80665f;

    *x = rawX * G_PER_LSB * G_TO_MS2;
    *y = rawY * G_PER_LSB * G_TO_MS2;
    *z = rawZ * G_PER_LSB * G_TO_MS2;
}
