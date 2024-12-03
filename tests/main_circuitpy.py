
# Circuit python imports
import board
import busio
import binascii

from command_handler_host import SerialCommandHandlerHost
from serial_file_transfer import SerialFileTransfer

# Initialise uart
serial = busio.UART(board.TX, board.RX, baudrate=9600, timeout=5)

command_handler = SerialCommandHandlerHost(serial)
file_transfer = SerialFileTransfer(serial)

#successful, result = command_handler.run_command("take_photo", "picture2", w=960, h=480, quality=100, cells_x=4, cells_y=4)
#successful, result = command_handler.run_command("exec_python", "for i in range(100): print('hello!')")

successful, result = command_handler.run_command("list_dir", "/home/pi/images")

#result = command_handler._send_command_and_args("send_file", "/home/pi/images/picture1.jpg")

#if result:
#    file_transfer.receive_file("RECEIVED.jpg")

print(successful, result)

#packet = binascii.a2b_base64(result)
#print(packet)
#print("LENGTH: ", len(packet))