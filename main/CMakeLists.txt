# IMU手势可视化/main/CMakeLists.txt
idf_component_register(SRCS "main.c" "mpu6050.c" "display.c" "web_server.c" "dht11.c" "buzzer.c"
                       "touch.c" "input.c" "page_manager.c"
                       INCLUDE_DIRS ".")