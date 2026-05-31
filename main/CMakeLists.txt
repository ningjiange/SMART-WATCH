# IMU手势可视化/main/CMakeLists.txt
idf_component_register(SRCS "system_info.c" "weather.c" "ntp_sync.c" "main.c" "mpu6050.c" "display.c" "web_server.c" "dht11.c" "buzzer.c"
                       "touch.c" "input.c" "page_manager.c"
                       "stopwatch.c" "countdown.c"
                       "pedometer.c" "motion_detect.c"
                       "flashlight.c"
                       INCLUDE_DIRS ".")