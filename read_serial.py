import serial
import sys

try:
    ser = serial.Serial('COM16', 115200, timeout=2)
    print("Connected to COM16 at 115200 baud")
    print("Reading serial data...\n")

    import time
    time.sleep(2)  # Wait for ESP32 reset

    count = 0
    while count < 100:
        if ser.in_waiting > 0:
            data = ser.read(ser.in_waiting)
            text = data.decode('utf-8', errors='replace')
            sys.stdout.write(text)
            sys.stdout.flush()
            count += 1
        time.sleep(0.1)

    ser.close()
    print("\nDone reading.")
except Exception as e:
    print(f"Error: {e}")
