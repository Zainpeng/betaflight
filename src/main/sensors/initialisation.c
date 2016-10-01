/*
 * This file is part of Cleanflight.
 *
 * Cleanflight is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Cleanflight is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Cleanflight.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "platform.h"

#include "build/build_config.h"

#include "common/axis.h"

#include "drivers/io.h"
#include "drivers/system.h"
#include "drivers/exti.h"

#include "drivers/sensor.h"

#include "drivers/accgyro.h"
#include "drivers/accgyro_adxl345.h"
#include "drivers/accgyro_bma280.h"
#include "drivers/accgyro_l3g4200d.h"
#include "drivers/accgyro_mma845x.h"
#include "drivers/accgyro_mpu.h"
#include "drivers/accgyro_mpu3050.h"
#include "drivers/accgyro_mpu6050.h"
#include "drivers/accgyro_mpu6500.h"
#include "drivers/accgyro_l3gd20.h"
#include "drivers/accgyro_lsm303dlhc.h"

#include "drivers/bus_spi.h"
#include "drivers/accgyro_spi_icm20689.h"
#include "drivers/accgyro_spi_mpu6000.h"
#include "drivers/accgyro_spi_mpu6500.h"
#include "drivers/accgyro_spi_mpu9250.h"
#include "drivers/gyro_sync.h"

#include "drivers/barometer.h"
#include "drivers/barometer_bmp085.h"
#include "drivers/barometer_bmp280.h"
#include "drivers/barometer_ms5611.h"

#include "drivers/compass.h"
#include "drivers/compass_hmc5883l.h"
#include "drivers/compass_ak8975.h"
#include "drivers/compass_ak8963.h"

#include "drivers/sonar_hcsr04.h"

#include "fc/runtime_config.h"

#include "sensors/sensors.h"
#include "sensors/acceleration.h"
#include "sensors/barometer.h"
#include "sensors/gyro.h"
#include "sensors/compass.h"
#include "sensors/sonar.h"
#include "sensors/initialisation.h"

#ifdef USE_HARDWARE_REVISION_DETECTION
#include "hardware_revision.h"
#endif

extern float magneticDeclination;

extern gyro_t gyro;
extern baro_t baro;
extern acc_t acc;
extern sensor_align_e gyroAlign;

uint8_t detectedSensors[SENSOR_INDEX_COUNT] = { GYRO_NONE, ACC_NONE, BARO_NONE, MAG_NONE };


const extiConfig_t *selectMPUIntExtiConfig(void)
{
#if defined(MPU_INT_EXTI)
    static const extiConfig_t mpuIntExtiConfig = { .tag = IO_TAG(MPU_INT_EXTI) };
    return &mpuIntExtiConfig;
#elif defined(USE_HARDWARE_REVISION_DETECTION)
    return selectMPUIntExtiConfigByHardwareRevision();
#else
    return NULL;
#endif
}

#ifdef USE_FAKE_GYRO
int16_t fake_gyro_values[XYZ_AXIS_COUNT] = { 0,0,0 };
static void fakeGyroInit(uint8_t lpf)
{
    UNUSED(lpf);
}

static bool fakeGyroRead(int16_t *gyroADC)
{
    for (int i = 0; i < XYZ_AXIS_COUNT; ++i) {
        gyroADC[i] = fake_gyro_values[i];
    }

    return true;
}

static bool fakeGyroReadTemp(int16_t *tempData)
{
    UNUSED(tempData);
    return true;
}


static bool fakeGyroInitStatus(void) {
    return true;
}

bool fakeGyroDetect(gyro_t *gyro)
{
    gyro->init = fakeGyroInit;
    gyro->intStatus = fakeGyroInitStatus;
    gyro->read = fakeGyroRead;
    gyro->temperature = fakeGyroReadTemp;
    gyro->scale = 1.0f / 16.4f;
    return true;
}
#endif

#ifdef USE_FAKE_ACC
int16_t fake_acc_values[XYZ_AXIS_COUNT] = {0,0,0};
static void fakeAccInit(acc_t *acc) {UNUSED(acc);}
static bool fakeAccRead(int16_t *accData) {
    for(int i=0;i<XYZ_AXIS_COUNT;++i) {
        accData[i] = fake_acc_values[i];
    }

    return true;
}

bool fakeAccDetect(acc_t *acc)
{
    acc->init = fakeAccInit;
    acc->read = fakeAccRead;
    acc->acc_1G = 512*8;
    acc->revisionCode = 0;
    return true;
}
#endif

bool detectGyro(void)
{
    gyroSensor_e gyroHardware = GYRO_DEFAULT;

    gyroAlign = ALIGN_DEFAULT;

    switch(gyroHardware) {
        case GYRO_DEFAULT:
            ; // fallthrough
        case GYRO_MPU6050:
#ifdef USE_GYRO_MPU6050
            if (mpu6050GyroDetect(&gyro)) {
                gyroHardware = GYRO_MPU6050;
#ifdef GYRO_MPU6050_ALIGN
                gyroAlign = GYRO_MPU6050_ALIGN;
#endif
                break;
            }
#endif
            ; // fallthrough
        case GYRO_L3G4200D:
#ifdef USE_GYRO_L3G4200D
            if (l3g4200dDetect(&gyro)) {
                gyroHardware = GYRO_L3G4200D;
#ifdef GYRO_L3G4200D_ALIGN
                gyroAlign = GYRO_L3G4200D_ALIGN;
#endif
                break;
            }
#endif
            ; // fallthrough

        case GYRO_MPU3050:
#ifdef USE_GYRO_MPU3050
            if (mpu3050Detect(&gyro)) {
                gyroHardware = GYRO_MPU3050;
#ifdef GYRO_MPU3050_ALIGN
                gyroAlign = GYRO_MPU3050_ALIGN;
#endif
                break;
            }
#endif
            ; // fallthrough

        case GYRO_L3GD20:
#ifdef USE_GYRO_L3GD20
            if (l3gd20Detect(&gyro)) {
                gyroHardware = GYRO_L3GD20;
#ifdef GYRO_L3GD20_ALIGN
                gyroAlign = GYRO_L3GD20_ALIGN;
#endif
                break;
            }
#endif
            ; // fallthrough

        case GYRO_MPU6000:
#ifdef USE_GYRO_SPI_MPU6000
            if (mpu6000SpiGyroDetect(&gyro)) {
                gyroHardware = GYRO_MPU6000;
#ifdef GYRO_MPU6000_ALIGN
                gyroAlign = GYRO_MPU6000_ALIGN;
#endif
                break;
            }
#endif
            ; // fallthrough

        case GYRO_MPU6500:
#if defined(USE_GYRO_MPU6500) || defined(USE_GYRO_SPI_MPU6500)
#ifdef USE_GYRO_SPI_MPU6500
            if (mpu6500GyroDetect(&gyro) || mpu6500SpiGyroDetect(&gyro))
#else
            if (mpu6500GyroDetect(&gyro))
#endif
            {
                gyroHardware = GYRO_MPU6500;
#ifdef GYRO_MPU6500_ALIGN
                gyroAlign = GYRO_MPU6500_ALIGN;
#endif

                break;
            }
#endif
            ; // fallthrough

    case GYRO_MPU9250:
#ifdef USE_GYRO_SPI_MPU9250

        if (mpu9250SpiGyroDetect(&gyro))
        {
            gyroHardware = GYRO_MPU9250;
#ifdef GYRO_MPU9250_ALIGN
            gyroAlign = GYRO_MPU9250_ALIGN;
#endif

            break;
        }
#endif
        ; // fallthrough
        
        case GYRO_ICM20689:
#ifdef USE_GYRO_SPI_ICM20689
            if (icm20689SpiGyroDetect(&gyro))
            {
                gyroHardware = GYRO_ICM20689;
#ifdef GYRO_ICM20689_ALIGN
                gyroAlign = GYRO_ICM20689_ALIGN;
#endif

                break;
            }
#endif
            ; // fallthrough

        case GYRO_FAKE:
#ifdef USE_FAKE_GYRO
            if (fakeGyroDetect(&gyro)) {
                gyroHardware = GYRO_FAKE;
                break;
            }
#endif
            ; // fallthrough
        case GYRO_NONE:
            gyroHardware = GYRO_NONE;
    }

    if (gyroHardware == GYRO_NONE) {
        return false;
    }

    detectedSensors[SENSOR_INDEX_GYRO] = gyroHardware;
    sensorsSet(SENSOR_GYRO);

    return true;
}

static bool detectAcc(accelerationSensor_e accHardwareToUse)
{
    accelerationSensor_e accHardware;

#ifdef USE_ACC_ADXL345
    drv_adxl345_config_t acc_params;
#endif

retry:
    accAlign = ALIGN_DEFAULT;

    switch (accHardwareToUse) {
        case ACC_DEFAULT:
            ; // fallthrough
        case ACC_ADXL345: // ADXL345
#ifdef USE_ACC_ADXL345
            acc_params.useFifo = false;
            acc_params.dataRate = 800; // unused currently
#ifdef NAZE
            if (hardwareRevision < NAZE32_REV5 && adxl345Detect(&acc_params, &acc)) {
#else
            if (adxl345Detect(&acc_params, &acc)) {
#endif
#ifdef ACC_ADXL345_ALIGN
                accAlign = ACC_ADXL345_ALIGN;
#endif
                accHardware = ACC_ADXL345;
                break;
            }
#endif
            ; // fallthrough
        case ACC_LSM303DLHC:
#ifdef USE_ACC_LSM303DLHC
            if (lsm303dlhcAccDetect(&acc)) {
#ifdef ACC_LSM303DLHC_ALIGN
                accAlign = ACC_LSM303DLHC_ALIGN;
#endif
                accHardware = ACC_LSM303DLHC;
                break;
            }
#endif
            ; // fallthrough
        case ACC_MPU6050: // MPU6050
#ifdef USE_ACC_MPU6050
            if (mpu6050AccDetect(&acc)) {
#ifdef ACC_MPU6050_ALIGN
                accAlign = ACC_MPU6050_ALIGN;
#endif
                accHardware = ACC_MPU6050;
                break;
            }
#endif
            ; // fallthrough
        case ACC_MMA8452: // MMA8452
#ifdef USE_ACC_MMA8452
#ifdef NAZE
            // Not supported with this frequency
            if (hardwareRevision < NAZE32_REV5 && mma8452Detect(&acc)) {
#else
            if (mma8452Detect(&acc)) {
#endif
#ifdef ACC_MMA8452_ALIGN
                accAlign = ACC_MMA8452_ALIGN;
#endif
                accHardware = ACC_MMA8452;
                break;
            }
#endif
            ; // fallthrough
        case ACC_BMA280: // BMA280
#ifdef USE_ACC_BMA280
            if (bma280Detect(&acc)) {
#ifdef ACC_BMA280_ALIGN
                accAlign = ACC_BMA280_ALIGN;
#endif
                accHardware = ACC_BMA280;
                break;
            }
#endif
            ; // fallthrough
        case ACC_MPU6000:
#ifdef USE_ACC_SPI_MPU6000
            if (mpu6000SpiAccDetect(&acc)) {
#ifdef ACC_MPU6000_ALIGN
                accAlign = ACC_MPU6000_ALIGN;
#endif
                accHardware = ACC_MPU6000;
                break;
            }
#endif
            ; // fallthrough
        case ACC_MPU6500:
#if defined(USE_ACC_MPU6500) || defined(USE_ACC_SPI_MPU6500)
#ifdef USE_ACC_SPI_MPU6500
            if (mpu6500AccDetect(&acc) || mpu6500SpiAccDetect(&acc))
#else
            if (mpu6500AccDetect(&acc))
#endif
            {
#ifdef ACC_MPU6500_ALIGN
                accAlign = ACC_MPU6500_ALIGN;
#endif
                accHardware = ACC_MPU6500;
                break;
            }
#endif
            ; // fallthrough
        case ACC_ICM20689:
#ifdef USE_ACC_SPI_ICM20689

            if (icm20689SpiAccDetect(&acc))
            {
#ifdef ACC_ICM20689_ALIGN
                accAlign = ACC_ICM20689_ALIGN;
#endif
                accHardware = ACC_ICM20689;
                break;
            }
#endif
            ; // fallthrough
        case ACC_FAKE:
#ifdef USE_FAKE_ACC
            if (fakeAccDetect(&acc)) {
                accHardware = ACC_FAKE;
                break;
            }
#endif
            ; // fallthrough
        case ACC_NONE: // disable ACC
            accHardware = ACC_NONE;
            break;

    }

    // Found anything? Check if error or ACC is really missing.
    if (accHardware == ACC_NONE && accHardwareToUse != ACC_DEFAULT && accHardwareToUse != ACC_NONE) {
        // Nothing was found and we have a forced sensor that isn't present.
        accHardwareToUse = ACC_DEFAULT;
        goto retry;
    }


    if (accHardware == ACC_NONE) {
        return false;
    }

    detectedSensors[SENSOR_INDEX_ACC] = accHardware;
    sensorsSet(SENSOR_ACC);
    return true;
}

#ifdef BARO
static bool detectBaro(baroSensor_e baroHardwareToUse)
{
    // Detect what pressure sensors are available. baro->update() is set to sensor-specific update function

    baroSensor_e baroHardware = baroHardwareToUse;

#ifdef USE_BARO_BMP085
    const bmp085Config_t *bmp085Config = NULL;

#if defined(BARO_XCLR_GPIO) && defined(BARO_EOC_GPIO)
    static const bmp085Config_t defaultBMP085Config = {
        .xclrIO = IO_TAG(BARO_XCLR_PIN),
        .eocIO = IO_TAG(BARO_EOC_PIN),
    };
    bmp085Config = &defaultBMP085Config;
#endif

#ifdef NAZE
    if (hardwareRevision == NAZE32) {
        bmp085Disable(bmp085Config);
    }
#endif

#endif

    switch (baroHardware) {
        case BARO_DEFAULT:
            ; // fallthough
        case BARO_BMP085:
#ifdef USE_BARO_BMP085
            if (bmp085Detect(bmp085Config, &baro)) {
                baroHardware = BARO_BMP085;
                break;
            }
#endif
            ; // fallthough
        case BARO_MS5611:
#ifdef USE_BARO_MS5611
            if (ms5611Detect(&baro)) {
                baroHardware = BARO_MS5611;
                break;
            }
#endif
            ; // fallthough
        case BARO_BMP280:
#ifdef USE_BARO_BMP280
            if (bmp280Detect(&baro)) {
                baroHardware = BARO_BMP280;
                break;
            }
#endif
            ; // fallthough
        case BARO_NONE:
            baroHardware = BARO_NONE;
            break;
    }

    if (baroHardware == BARO_NONE) {
        return false;
    }

    detectedSensors[SENSOR_INDEX_BARO] = baroHardware;
    sensorsSet(SENSOR_BARO);
    return true;
}
#endif

#ifdef MAG
static bool detectMag(magSensor_e magHardwareToUse)
{
    magSensor_e magHardware;

#ifdef USE_MAG_HMC5883
    const hmc5883Config_t *hmc5883Config = 0;

#ifdef NAZE // TODO remove this target specific define
    static const hmc5883Config_t nazeHmc5883Config_v1_v4 = {
            .intTag = IO_TAG(PB12) /* perhaps disabled? */
    };
    static const hmc5883Config_t nazeHmc5883Config_v5 = {
            .intTag = IO_TAG(MAG_INT_EXTI)
    };
    if (hardwareRevision < NAZE32_REV5) {
        hmc5883Config = &nazeHmc5883Config_v1_v4;
    } else {
        hmc5883Config = &nazeHmc5883Config_v5;
    }
#endif

#ifdef MAG_INT_EXTI
    static const hmc5883Config_t extiHmc5883Config = {
        .intTag = IO_TAG(MAG_INT_EXTI)
    };

    hmc5883Config = &extiHmc5883Config;
#endif

#endif

retry:

    magAlign = ALIGN_DEFAULT;

    switch(magHardwareToUse) {
        case MAG_DEFAULT:
            ; // fallthrough

        case MAG_HMC5883:
#ifdef USE_MAG_HMC5883
            if (hmc5883lDetect(&mag, hmc5883Config)) {
#ifdef MAG_HMC5883_ALIGN
                magAlign = MAG_HMC5883_ALIGN;
#endif
                magHardware = MAG_HMC5883;
                break;
            }
#endif
            ; // fallthrough

        case MAG_AK8975:
#ifdef USE_MAG_AK8975
            if (ak8975Detect(&mag)) {
#ifdef MAG_AK8975_ALIGN
                magAlign = MAG_AK8975_ALIGN;
#endif
                magHardware = MAG_AK8975;
                break;
            }
#endif
            ; // fallthrough

        case MAG_AK8963:
#ifdef USE_MAG_AK8963
            if (ak8963Detect(&mag)) {
#ifdef MAG_AK8963_ALIGN
                magAlign = MAG_AK8963_ALIGN;
#endif
                magHardware = MAG_AK8963;
                break;
            }
#endif
            ; // fallthrough

        case MAG_NONE:
            magHardware = MAG_NONE;
            break;
    }

    if (magHardware == MAG_NONE && magHardwareToUse != MAG_DEFAULT && magHardwareToUse != MAG_NONE) {
        // Nothing was found and we have a forced sensor that isn't present.
        magHardwareToUse = MAG_DEFAULT;
        goto retry;
    }

    if (magHardware == MAG_NONE) {
        return false;
    }

    detectedSensors[SENSOR_INDEX_MAG] = magHardware;
    sensorsSet(SENSOR_MAG);
    return true;
}
#endif

void reconfigureAlignment(sensorAlignmentConfig_t *sensorAlignmentConfig)
{
    if (sensorAlignmentConfig->gyro_align != ALIGN_DEFAULT) {
        gyroAlign = sensorAlignmentConfig->gyro_align;
    }
    if (sensorAlignmentConfig->acc_align != ALIGN_DEFAULT) {
        accAlign = sensorAlignmentConfig->acc_align;
    }
    if (sensorAlignmentConfig->mag_align != ALIGN_DEFAULT) {
        magAlign = sensorAlignmentConfig->mag_align;
    }
}

bool sensorsAutodetect(sensorAlignmentConfig_t *sensorAlignmentConfig,
        uint8_t accHardwareToUse,
        uint8_t magHardwareToUse,
        uint8_t baroHardwareToUse,
        int16_t magDeclinationFromConfig,
        uint8_t gyroLpf,
        uint8_t gyroSyncDenominator)
{
    memset(&acc, 0, sizeof(acc));
    memset(&gyro, 0, sizeof(gyro));

#if defined(USE_GYRO_MPU6050) || defined(USE_GYRO_MPU3050) || defined(USE_GYRO_MPU6500) || defined(USE_GYRO_SPI_MPU6500) || defined(USE_GYRO_SPI_MPU6000) || defined(USE_ACC_MPU6050) || defined(USE_GYRO_SPI_MPU9250) || defined(USE_GYRO_SPI_ICM20689)

    const extiConfig_t *extiConfig = selectMPUIntExtiConfig();

    mpuDetectionResult_t *mpuDetectionResult = detectMpu(extiConfig);
    UNUSED(mpuDetectionResult);
#endif

    if (!detectGyro()) {
        return false;
    }

    // Now time to init things
    // this is safe because either mpu6050 or mpu3050 or lg3d20 sets it, and in case of fail, we never get here.
    gyro.targetLooptime = gyroSetSampleRate(gyroLpf, gyroSyncDenominator);    // Set gyro sample rate before initialisation
    gyro.init(gyroLpf); // driver initialisation
    gyroInit(); // sensor initialisation

    if (detectAcc(accHardwareToUse)) {
        acc.acc_1G = 256; // set default
        acc.init(&acc); // driver initialisation
        accInit(gyro.targetLooptime); // sensor initialisation
    }


    magneticDeclination = 0.0f; // TODO investigate if this is actually needed if there is no mag sensor or if the value stored in the config should be used.
#ifdef MAG
    // FIXME extract to a method to reduce dependencies, maybe move to sensors_compass.c
    if (detectMag(magHardwareToUse)) {
        // calculate magnetic declination
        const int16_t deg = magDeclinationFromConfig / 100;
        const int16_t min = magDeclinationFromConfig % 100;
        magneticDeclination = (deg + ((float)min * (1.0f / 60.0f))) * 10; // heading is in 0.1deg units
    }
#else
    UNUSED(magHardwareToUse);
    UNUSED(magDeclinationFromConfig);
#endif

#ifdef BARO
    detectBaro(baroHardwareToUse);
#else
    UNUSED(baroHardwareToUse);
#endif

    reconfigureAlignment(sensorAlignmentConfig);

    return true;
}
