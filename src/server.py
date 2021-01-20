import argparse
import os
import struct

def parse_arguments():
    parser = argparse.ArgumentParser(description='Transfer files between devices using sockets')
    parser.add_argument('port_number',
                        type=int,
                        help='The port number to listen on')
    parser.add_argument('destination_directory',
                        help='The directory where incoming files will be placed')
    args = parser.parse_args()

    if args.port_number < 1 or args.port_number > 65535:
        parser.error('port_number must be in range 1-65535')

    return args

class NetworkFileReceiver:
    def __init__(self, connected_socket, directory_fd):
        self._socket = connected_socket
        self._directory_fd = directory_fd
        self._buffer = bytearray()

    def _get_from_socket(self, num_bytes):
        '''Reads data from own socket into buffer until num_bytes bytes have been read.'''


    def receive(self):

        self._get_from_socket(4)
        file_name_length = struct.unpack('=i', self._buffer[0:4])
        self._get_from_socket(file_name_length)
        # also strips the ending \x00
        file_name = str(self._buffer[0:file_name_length-1], 'UTF-8')
        open_flags = os.O_WRONLY
        if hasattr(os, 'O_BINARY'):
            open_flags = open_flags | getattr(os,'O_BINARY')
        os.open(file_name, open_flags, dir_fd=self._directory_fd)
