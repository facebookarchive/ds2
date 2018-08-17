#!/usr/bin/env python

import datetime
import socket
import sys

one_char_packets = ['+', '-', '\x03']

def checksum(message):
    """
    Calculate the GDB server protocol checksum of the message.

    The GDB server protocol uses a simple modulo 256 sum.
    """
    check = 0
    for c in message:
        check += ord(c)
    return check % 256


def frame_packet(message):
    """
    Create a framed packet that's ready to send over the GDB connection
    channel.

    Framing includes surrounding the message between $ and #, and appending
    a two character hex checksum.
    """
    if message in one_char_packets:
        return message
    return "$%s#%02x" % (message, checksum(message))


def escape_binary(message):
    """
    Escape the binary message using the process described in the GDB server
    protocol documentation.

    Most bytes are sent through as-is, but $, #, and { are escaped by writing
    a { followed by the original byte mod 0x20.
    """
    out = ""
    for c in message:
        d = ord(c)
        if d in (0x23, 0x24, 0x7d):
            out += chr(0x7d)
            out += chr(d ^ 0x20)
        else:
            out += c
    return out


def hex_encode_bytes(message):
    """
    Encode the binary message by converting each byte into a two-character
    hex string.
    """
    out = ""
    for c in message:
        out += "%02x" % ord(c)
    return out


def hex_decode_bytes(hex_bytes):
    """
    Decode the hex string into a binary message by converting each
    two-character hex string into a single output byte.
    """
    out = ""
    hex_len = len(hex_bytes)
    i = 0
    while i < hex_len - 1:
        out += chr(int(hex_bytes[i:i + 2]), 16)
        i += 2
    return out


def response_is_ok(response):
    return response == 'OK'


class client:
    """
    A simple TCP-based GDB remote client.
    """

    def __init__(self):
        self._socket = None
        self._receivedData = ""
        self._receivedDataOffset = 0
        self._shouldSendAck = True
        self.trace = False

    def connect_to_host(self, port, host='127.0.0.1'):
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((host, port))
        s.settimeout(2)
        self._socket = s
        self.send_ack()

    def stop(self):
        self._socket.close()

    def send_ack(self):
        self.send_packet('+', get_response=False)

    def send_nack(self):
        self.send_packet('+', get_response=False)

    def send_QStartNoAckMode(self):
        response = self.send_packet('QStartNoAckMode')
        if response_is_ok(response):
            self._shouldSendAck = False

    def send_qSpeedTest(self, send_size, recv_size):
        payload = "qSpeedTest:response_size:%i;data:%s" % (recv_size,
                                                           's' * send_size)
        response = self.send_packet(payload)
        return response

    def send_packet(self, packet, get_response=True):
        wire_packet = frame_packet(packet)
        if self.trace:
            print("<%4u> send packet: %s" % (len(wire_packet), wire_packet))
        self._socket.send(wire_packet)
        if get_response:
            if self._shouldSendAck:
                ack = self.receive_response()
                if ack != '+':
                    raise ValueError("didn't get ack")
            return self.receive_response()
        return None

    def receive_response(self):
        try:
            while True:
                packet = self._parsePacket()
                if packet is not None:
                    if self.trace:
                        print("<%4u> read packet: %s" % (len(packet), packet))
                    if self._shouldSendAck and packet not in one_char_packets:
                        self.send_ack()
                    return packet
                data = self._socket.recv(1024)
                if data is None or len(data) == 0:
                    return None
                # In Python 2, sockets return byte strings. In Python 3,
                # sockets return bytes. If we got bytes (and not a byte
                # string), decode them to a string for later handling.
                if isinstance(data, bytes) and not isinstance(data, str):
                    data = data.decode()
                self._receivedData += data
        except self.InvalidPacketException:
            type, value, traceback = sys.exc_info()
            self._socket.close()
            print('error: self.InvalidPacketException(%s), closing.' % (value))
            print('self._receivedData = %s' % (self._receivedData))
            print('self.self._receivedDataOffset = %u' % (
                    self._receivedDataOffset))
        return None

    def _parsePacket(self):
        """
        Reads bytes from self._receivedData, returning:
        - a packet's contents if a valid packet is found
        - the PACKET_ACK unique object if we got an ack
        - None if we only have a partial packet

        Raises an InvalidPacketException if unexpected data is received
        or if checksums fail.

        Once a complete packet is found at the front of self._receivedData,
        its data is removed form self._receivedData.
        """
        data = self._receivedData
        i = self._receivedDataOffset
        data_len = len(data)
        if data_len == 0:
            return None
        if i == 0:
            # If we're looking at the start of the received data, that means
            # we're looking for the start of a new packet, denoted by a $.
            # It's also possible we'll see an ACK here, denoted by a +
            if data[0] == '+':
                self._receivedData = data[1:]
                return '+'
            if data[0] == '-':
                self._receivedData = data[1:]
                return '-'
            if ord(data[0]) == 3:
                self._receivedData = data[1:]
                return '\x03'
            if data[0] == '$':
                i += 1
            else:
                raise self.InvalidPacketException(
                        "Unexpected leading byte: %s" % data[0])

        # If we're looking beyond the start of the received data, then we're
        # looking for the end of the packet content, denoted by a #.
        # Note that we pick up searching from where we left off last time
        while i < data_len and data[i] != '#':
            i += 1

        # If there isn't enough data left for a checksum, just remember where
        # we left off so we can pick up there the next time around
        if i > data_len - 3:
            self._receivedDataOffset = i
            return None

        # If we have enough data remaining for the checksum, extract it and
        # compare to the packet contents
        packet = data[1:i]
        i += 1
        try:
            check = int(data[i:i + 2], 16)
        except ValueError:
            raise self.InvalidPacketException("Checksum is not valid hex")
        i += 2
        # Only check the checksum if ACK mode is enabled
        if self._shouldSendAck and check != checksum(packet):
            raise self.InvalidPacketException(
                    "Checksum %02x does not match content %02x" %
                    (check, checksum(packet)))
        # remove parsed bytes from _receivedData and reset offset so parsing
        # can start on the next packet the next time around
        self._receivedData = data[i:]
        self._receivedDataOffset = 0
        return packet

    class InvalidPacketException(Exception):
        pass


def main():
    args = sys.argv[1:]
    port = int(args[0])
    gdbremote = client()
    gdbremote.connect_to_host(port=port)
    gdbremote.send_QStartNoAckMode()
    num_packets = 1000
    send_sizes = [0, 32, 512, 1024]
    recv_sizes = [0, 32, 512, 1024]
    for send_size in send_sizes:
        for recv_size in recv_sizes:
            total_us = 0
            for _i in range(num_packets):
                start = datetime.datetime.now()
                gdbremote.send_qSpeedTest(send_size, recv_size)
                end = datetime.datetime.now()
                delta = end - start
                us = delta.seconds * 1000000 + delta.microseconds
                total_us += us
            avg_us = float(total_us) / float(num_packets)
            print('send=%5u, recv=%5u: avg=%8u us (%5.2f packets per second)' % (
                    send_size, recv_size, avg_us,
                    float(1000000) / float(avg_us)))


if __name__ == '__main__':
    main()
