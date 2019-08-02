# Adapted from https://github.com/ladislav-zezula/CascLib/blob/master/src/CascDecrypt.cpp
import sys, struct

from fixedint import MutableUInt32 as uint32

ROUNDS = 20
CONSTANT = b"expand 16-byte k"

def rol32(value, count):
    return (value << count) | (value >> (32 - count))

def initialize(key, iv):
    state = [None,] * 16

    state[0] = uint32.from_bytes(CONSTANT, byteorder = 'little')
    state[1] = uint32.from_bytes(key, byteorder = 'little')
    state[2] = uint32.from_bytes(key[4:], byteorder = 'little')
    state[3] = uint32.from_bytes(key[8:], byteorder = 'little')
    state[4] = uint32.from_bytes(key[12:], byteorder = 'little')
    state[5] = uint32.from_bytes(CONSTANT[4:], byteorder = 'little')
    state[6] = uint32.from_bytes(iv, byteorder = 'little')
    if len(iv) > 4:
        state[7] = uint32.from_bytes(iv[4:], byteorder = 'little')
    else:
        state[7] = uint32(0)
    state[8] = uint32(0)
    state[9] = uint32(0)
    state[10] = uint32.from_bytes(CONSTANT[8:], byteorder = 'little')
    state[11] = uint32.from_bytes(key, byteorder = 'little')
    state[12] = uint32.from_bytes(key[4:], byteorder = 'little')
    state[13] = uint32.from_bytes(key[8:], byteorder = 'little')
    state[14] = uint32.from_bytes(key[12:], byteorder = 'little')
    state[15] = uint32.from_bytes(CONSTANT[12:], byteorder = 'little')

    return state

def decrypt(state, data):
    remaining = len(data)
    data_out = [None,] * len(data)
    decrypted = 0

    while remaining > 0:
        state_copy = [uint32(v) for v in state]
        sz = min(remaining, 64)

        for i in range(0, ROUNDS, 2):
            state_copy[0x04] ^= rol32(state_copy[0x00] + state_copy[0x0c], 0x07)
            state_copy[0x08] ^= rol32(state_copy[0x04] + state_copy[0x00], 0x09)
            state_copy[0x0c] ^= rol32(state_copy[0x08] + state_copy[0x04], 0x0d)
            state_copy[0x00] ^= rol32(state_copy[0x0c] + state_copy[0x08], 0x12)

            state_copy[0x09] ^= rol32(state_copy[0x05] + state_copy[0x01], 0x07)
            state_copy[0x0d] ^= rol32(state_copy[0x09] + state_copy[0x05], 0x09)
            state_copy[0x01] ^= rol32(state_copy[0x0d] + state_copy[0x09], 0x0d)
            state_copy[0x05] ^= rol32(state_copy[0x01] + state_copy[0x0d], 0x12)

            state_copy[0x0e] ^= rol32(state_copy[0x0a] + state_copy[0x06], 0x07)
            state_copy[0x02] ^= rol32(state_copy[0x0e] + state_copy[0x0a], 0x09)
            state_copy[0x06] ^= rol32(state_copy[0x02] + state_copy[0x0e], 0x0d)
            state_copy[0x0a] ^= rol32(state_copy[0x06] + state_copy[0x02], 0x12)

            state_copy[0x03] ^= rol32(state_copy[0x0f] + state_copy[0x0b], 0x07)
            state_copy[0x07] ^= rol32(state_copy[0x03] + state_copy[0x0f], 0x09)
            state_copy[0x0b] ^= rol32(state_copy[0x07] + state_copy[0x03], 0x0d)
            state_copy[0x0f] ^= rol32(state_copy[0x0b] + state_copy[0x07], 0x12)

            state_copy[0x01] ^= rol32(state_copy[0x00] + state_copy[0x03], 0x07)
            state_copy[0x02] ^= rol32(state_copy[0x01] + state_copy[0x00], 0x09)
            state_copy[0x03] ^= rol32(state_copy[0x02] + state_copy[0x01], 0x0d)
            state_copy[0x00] ^= rol32(state_copy[0x03] + state_copy[0x02], 0x12)

            state_copy[0x06] ^= rol32(state_copy[0x05] + state_copy[0x04], 0x07)
            state_copy[0x07] ^= rol32(state_copy[0x06] + state_copy[0x05], 0x09)
            state_copy[0x04] ^= rol32(state_copy[0x07] + state_copy[0x06], 0x0d)
            state_copy[0x05] ^= rol32(state_copy[0x04] + state_copy[0x07], 0x12)

            state_copy[0x0b] ^= rol32(state_copy[0x0a] + state_copy[0x09], 0x07)
            state_copy[0x08] ^= rol32(state_copy[0x0b] + state_copy[0x0a], 0x09)
            state_copy[0x09] ^= rol32(state_copy[0x08] + state_copy[0x0b], 0x0d)
            state_copy[0x0a] ^= rol32(state_copy[0x09] + state_copy[0x08], 0x12)

            state_copy[0x0c] ^= rol32(state_copy[0x0f] + state_copy[0x0e], 0x07)
            state_copy[0x0d] ^= rol32(state_copy[0x0c] + state_copy[0x0f], 0x09)
            state_copy[0x0e] ^= rol32(state_copy[0x0d] + state_copy[0x0c], 0x0d)
            state_copy[0x0f] ^= rol32(state_copy[0x0e] + state_copy[0x0d], 0x12)

        xor_values = [state_copy[i] + state[i] for i in range(0, len(state))]
        xor_bytes = b''.join([v.to_bytes(4, byteorder = 'little') for v in xor_values])

        for i in range(decrypted, decrypted + sz):
            data_out[i] = (data[i] ^ xor_bytes[i - decrypted]).to_bytes(1, byteorder = 'little')

        state[8] += 1
        if state[8] == 0:
            state[9] += 1

        remaining -= sz
        decrypted += sz

    return b''.join(data_out)

