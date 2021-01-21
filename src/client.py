import argparse
import socket
import os
import struct

def parse_arguments():
    parser = argparse.ArgumentParser(description='Transfer files between devices using sockets.')
    parser.add_argument('server_ip',
                        help='A valid IP address for the server')
    parser.add_argument('port_number',
                        type=int,
                        help='The port the server is listening on')
    parser.add_argument('filename',
                        help='Path of the file to send')
    args = parser.parse_args()

    if not 1 <= args.port_number <= 65535:
        parser.error('port_number must be in range 1-65535')

    return args

class NetworkFileTransferer:
    ''' It is able to transfer a file over a socket with a predefined protocol.

        The protocol sends data as specified:
        1. A 32 bit integer in network byte-order, specifying the length of the file's name
        2. The filename as a UTF-8 encoded string, followed by a null terminating character (\x00)
        3. A 32 bit integer in network byte-order, specifying the file content's length
        4. The file's content as binary data
        '''

    def __init__(self, connected_socket, file_object):
        self._socket = connected_socket
        self._source_file = file_object
        self._buffer = b''

    def _send_buffer(self):
        ''' Send the content of own buffer.

            Calls socket.send multiple times if necessary.
            '''
        buffer_length_sent = 0
        buffer_size = len(self._buffer)
        while buffer_length_sent < buffer_size:
            buffer_length_sent += self._socket.send(self._buffer[buffer_length_sent:buffer_size])


    def transfer(self):
        '''Send the file by filling own buffer with correct data and calling _send_buffer.'''
        # extract base file name
        file_name = os.path.basename(self._source_file.name)
        # convert name into bytes and append \0
        file_name = bytes(file_name, 'UTF-8') + b'\0'
        file_name_length = len(file_name)
        # pack length of file name into big endian 4 bytes and prepare for shipment
        self._buffer = struct.pack('>i', file_name_length)
        self._send_buffer()
        self._buffer = file_name
        self._send_buffer()
        # pack length of file into big endian 4 bytes
        self._buffer = struct.pack('>i', os.stat(self._source_file.fileno()).st_size)
        self._send_buffer()
        while piece := self._source_file.read(4096):
            self._buffer = piece
            self._send_buffer()

def main():
    arguments = parse_arguments()

    with open(arguments.filename, mode='rb') as source:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as client_socket:
            client_socket.connect((arguments.server_ip, arguments.port_number))

            transferer = NetworkFileTransferer(client_socket, source)
            transferer.transfer()

main()
