import serial

port = '/dev/tty.usbserial-0001'
baud_rate = 115200

print(f"Listening on {port} at {baud_rate} baud rate...")

count = 0

with serial.Serial(port, baud_rate, timeout=1) as ser:
    while True:
        # Check if data is available to read
        if ser.in_waiting > 0:
            # Read one byte from the serial port
            data = ser.read()
            
            for byte in data:
                print(hex(byte), end=' ')
                count += 1
                if count % 16 == 0:
                    print('')