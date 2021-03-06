import argparse
import os
import struct
import socket
import logging
import threading

def parse_arguments():
    parser = argparse.ArgumentParser(description='Transfer files between devices using sockets')
    parser.add_argument('port_number',
                        type=int,
                        help='The port number to listen on')
    parser.add_argument('destination_directory',
                        help='The directory where incoming files will be placed')
    parser.add_argument('--logfile',
                        default=None,
                        help='An optional file to redirect informational messages to'
                        )
    args = parser.parse_args()

    if not 1 <= args.port_number <= 65535:
        parser.error('port_number must be in range 1-65535')

    if not os.access(args.destination_directory, os.W_OK | os.X_OK):
        parser.error('cannot open directory {0}'.format(args.destination_directory))

    return args

class NetworkFileReceiver:
    def __init__(self, connected_socket : socket.socket, directory_name : str, job_number):
        self._socket = connected_socket
        self._directory_name = directory_name
        self._job_number = job_number
        self._buffer = bytearray()
        self._logger = logging.getLogger('server')

    def _get_from_socket(self, num_bytes):
        ''' Reads data from own socket into own buffer.

            It attempts to read num_bytes bytes, only reading less
            if the socket is closed before it can read the full amount.
            Returns the amount read: either num_bytes or less than num_bytes
            if the socket is closed prematurely.
        '''
        total_read = 0
        self._buffer = bytearray()
        while total_read < num_bytes:
            data = self._socket.recv(num_bytes)
            total_read += len(data)
            if not data:
                break
            self._buffer += data
        return total_read

    def receive(self):
        # read and interpret name length
        self._get_from_socket(4)
        file_name_length, = struct.unpack('>i', self._buffer)
        # read name
        self._get_from_socket(file_name_length)
        # decode name, also strips the ending \x00
        file_name = str(self._buffer[:-1], 'UTF-8')
        file_name = os.path.join(self._directory_name, file_name)
        with open(file_name, 'wb') as out_file:
            # read and interpret file length
            self._get_from_socket(4)
            file_length, = struct.unpack('>i', self._buffer)
            length_read = 0
            # read and write into file until there is no more left
            while length_read < file_length:
                read_this_cycle = self._get_from_socket(4096)
                length_read += read_this_cycle
                out_file.write(self._buffer)
        self._logger.info('[%d] file name: "%s", file size: "%dB". DONE',
                          self._job_number, file_name, file_length
                         )

def initialize_logger(logfile):
    ''' Given an open file-like object, create a new logger named "server".

        The new logger has a logging level of info, an attached handler which just
        passes logs to a formatter that adds date and time and then outputs to the logfile
    '''
    logger = logging.getLogger('server')
    logger.setLevel(logging.INFO)
    formatter = logging.Formatter(fmt='%(asctime)s: %(message)s', datefmt='%d %b %Y %H:%M:%S')
    handler = logging.StreamHandler(logfile)
    handler.setFormatter(formatter)
    logger.addHandler(handler)
    return logger

def start_new_receiver(client_socket, mailbox_directory, job_number):
    receiver = NetworkFileReceiver(client_socket, mailbox_directory, job_number)
    receiver.receive()

def start_job(client_socket, mailbox_directory, job_number):
    threading.Thread(target=start_new_receiver,
                     args=(client_socket, mailbox_directory, job_number)
                    ).start()

def main():
    args = parse_arguments()
    with open(1, 'w') if args.logfile is None else open(args.logfile, 'a') as logfile:
        logger = initialize_logger(logfile)
        logger.info('Server started')
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server_socket:
            logger.info('Listening on port %(port)d', {'port': args.port_number})
            server_socket.bind(('', args.port_number))
            server_socket.listen()
            received_count = 0
            while True:
                client_socket, remote_address = server_socket.accept()
                logger.info('Connection from %s dispatching worker %d',
                            remote_address, received_count)
                start_job(client_socket, args.destination_directory, received_count)
                received_count += 1

if __name__ == '__main__':
    main()
