import serial
import time

ser = serial.Serial('COM16', 115200, timeout=1)
time.sleep(2)  # Wait for ESP32 to reset

print("=== MPU6050 Serial Monitor ===")
print("Press Ctrl+C to exit\n")

try:
    while True:
        if ser.in_waiting > 0:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if line:
                print(line)
        time.sleep(0.01)
except KeyboardInterrupt:
    print("\nMonitor stopped.")
finally:
    ser.close()
