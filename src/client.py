import argparse
import socket
import os

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
    
    if args.port_number < 1 or args.port_number > 65535:
        parser.error('port_number must be in range 1-65535')
    
    return args

class NetworkFileTransferer:
    ''' It is able to transfer a file over a socket with a predefined protocol.
    
        The protocol sends data as specified:
        1. A 32 bit integer in network byte-order, specifying the length of the file's name + 1
        2. The filename as a Unicode string, followed by a null terminating character (\0)
        3. A 32 bit integer specifying the file content's length
        4. The file's content as binary data
        '''

    def __init__(self, connected_socket, file_object):
        self.socket = connected_socket
        self.source_file = file_object

    def _actually_send_buffer(self):


    def transfer(self):
        
        while piece := self.source_file.read(4096):
            piece_length_sent = 0
            while piece_length_sent < len(piece):
                piece_length_sent += self.socket.send(piece[piece_length_sent:len(piece)])

def main():
    arguments = parse_arguments()

    with open(arguments.filename, mode='rb') as source:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as client_socket:
            client_socket.connect((arguments.server_ip, arguments.port_number))

            print(os.stat(source.fileno()).st_size)
            
            while piece := source.read(5):
                print(piece)
                
            print(total)

main()
