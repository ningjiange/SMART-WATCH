# IMU手势可视化/main/CMakeLists.txt
idf_component_register(SRCS "main.c" "mpu6050.c" "display.c"
                       INCLUDE_DIRS ".")