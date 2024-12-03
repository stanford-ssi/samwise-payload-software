# Mimics the feather so it can be run on a laptop for testing

# Circuit python imports
import binascii
import time
import serial

from command_handler_host import SerialCommandHandlerHost
from serial_file_transfer import SerialFileTransfer
from serial_packet_handler import SerialPacketHandler

PORT_NAME = "/dev/tty.usbserial-0001"
BAUDRATE = 115200

TIMEOUT = 10

# ------------------------

# Main code entry point
print("Code running...")
with serial.Serial(PORT_NAME, BAUDRATE, timeout=TIMEOUT) as ser:

    while True:
        packet_handler = SerialPacketHandler(ser)
        packet, seq_num = packet_handler.read_packet()

        print(f"Packet #{seq_num}: {packet}")