import json

from serial_packet_handler import SerialPacketHandler

# Determines whether debug information is logged
DEBUG = False

class SerialCommandHandlerHost():
    # Layered class with support for handling commands
    def __init__(self, serial_port) -> None:
        # Initialize lower layer
        self.packet_handler = SerialPacketHandler(serial_port)


    def _send_command_and_args(self, command: str, *args, **kwargs) -> bool:
        # Send command ID, args and kwards to the payload
        if DEBUG: print(f"Sending {command} with args {args} and kwargs {kwargs}...")
        command_json = json.dumps([
            command,
            args,
            kwargs
        ])

        return self.packet_handler.write_packet(command_json.encode())


    def run_command(self, command: str, *args, **kwargs) -> tuple[bool, object]:
        # Send a command to the device (formatted in JSON as [cmd, args, kwargs])
        # Wait to receive the result from the payload
        # Returns a boolean (whether the command was successul) and the actual result
        if not self._send_command_and_args(command, *args, **kwargs):
            if DEBUG: print("Payload did not respond!")
            return False, None

        # Receive result
        if DEBUG: print("Awaiting response...")
        response = self.packet_handler.read_packet()

        if response is None:
            if DEBUG: print("Error receiving response!")
            return False, None
        
        packet = response[0]
        successful, result = json.loads(packet)

        if DEBUG: print(f"Recevied {result} ({'successful' if successful else 'failed'})")

        return successful, result