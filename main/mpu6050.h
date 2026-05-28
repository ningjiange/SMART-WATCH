// IMU手势可视化/main/mpu6050.h
#ifndef MPU6050_H
#define MPU6050_H

#include "driver/i2c_master.h"

// MPU6050 I2C 地址（AD0 接 GND 时为 0x68）
#define MPU6050_ADDR 0x68

// 寄存器地址
#define MPU6050_REG_WHO_AM_I     0x75
#define MPU6050_REG_PWR_MGMT_1   0x6B
#define MPU6050_REG_SMPLRT_DIV   0x19
#define MPU6050_REG_CONFIG       0x1A
#define MPU6050_REG_ACCEL_CONFIG 0x1C
#define MPU6050_REG_GYRO_CONFIG  0x1B
#define MPU6050_REG_ACCEL_XOUT_H 0x3B
#define MPU6050_REG_GYRO_XOUT_H  0x43

// 姿态数据结构
typedef struct {
    float accel_x;    // 加速度计 X 轴 (m/s²)
    float accel_y;    // 加速度计 Y 轴 (m/s²)
    float accel_z;    // 加速度计 Z 轴 (m/s²)
    float gyro_x;     // 陀螺仪 X 轴 (°/s)
    float gyro_y;     // 陀螺仪 Y 轴 (°/s)
    float gyro_z;     // 陀螺仪 Z 轴 (°/s)
    float pitch;      // 俯仰角 (°)
    float roll;       // 横滚角 (°)
    // 注：MPU6050 无磁力计，无法独立计算 Yaw，需外接磁力计或用陀螺仪积分近似
} mpu6050_data_t;

// API 函数
esp_err_t mpu6050_init(i2c_master_bus_handle_t bus_handle, mpu6050_data_t *data);
esp_err_t mpu6050_read(mpu6050_data_t *data);

#endif