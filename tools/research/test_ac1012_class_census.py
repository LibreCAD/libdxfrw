#!/usr/bin/env python3
"""Synthetic bounds and corruption controls for the AC1012 map census."""
import unittest

from audit_ac1012_class_census import census
from legacy_dwg_dictionary_wire import crc16


def frame(kind, handle):
    # Bit-short raw 16-bit little-endian type, then self-handle H(0, 1).
    bits = '00' + format(kind & 255, '08b') + format(kind >> 8, '08b')
    bits += '00000001' + format(handle, '08b')
    bits += '0' * (-len(bits) % 8)
    body = int(bits, 2).to_bytes(len(bits) // 8, 'big')
    result = len(body).to_bytes(2, 'little') + body
    return result + crc16(result).to_bytes(2, 'little')


def sample():
    data = bytearray(140)
    data[:6] = b'AC1012'
    data[21:25] = (1).to_bytes(4, 'little')
    data[25:34] = b'\x02' + (128).to_bytes(4, 'little') + (12).to_bytes(4, 'little')
    first, second = frame(531, 1), frame(42, 2)
    data[48:48 + len(first)] = first
    data[64:64 + len(second)] = second
    # One map page: handle +1/location +48, handle +1/location +16.
    page = b'\x00\x06\x01\x30\x01\x10'
    data[128:140] = page + crc16(page).to_bytes(2, 'big') + b'\x00\x02\x01\xd0'
    return bytes(data)


class ObjectMapControls(unittest.TestCase):
    def test_identity_crc_bounds_and_retry(self):
        original = sample()
        expected = census(original)
        self.assertEqual([(r['handle'], r['offset'], r['type']) for r in expected['frames']],
                         [(1, 48, 531), (2, 64, 42)])
        mutations = []
        bad = bytearray(original); bad[131] ^= 1; mutations.append(bad)  # map CRC
        bad = bytearray(original); bad[50] ^= 1; mutations.append(bad)  # frame CRC
        bad = bytearray(original); bad[138] ^= 1; mutations.append(bad)  # terminator CRC
        bad = bytearray(original); bad[133] = 0  # duplicate physical location
        bad[134:136] = crc16(bad[128:134]).to_bytes(2, 'big'); mutations.append(bad)
        bad = bytearray(original); bad[132] = 0  # duplicate handle, valid page CRC
        bad[134:136] = crc16(bad[128:134]).to_bytes(2, 'big'); mutations.append(bad)
        bad = bytearray(original); bad[54] ^= 0x40  # changed frame identity with valid CRC
        end = 50 + int.from_bytes(bad[48:50], 'little')
        bad[end:end + 2] = crc16(bad[48:end]).to_bytes(2, 'little'); mutations.append(bad)
        for damaged in mutations:
            with self.subTest(mutation=mutations.index(damaged)):
                with self.assertRaises(ValueError):
                    census(bytes(damaged))
                self.assertEqual(census(original), expected)
        # A self-consistent type change is counted, not silently treated as
        # corruption; the caller compares the physical census with its peer.
        changed_type = bytearray(original)
        replacement = frame(530, 1)
        changed_type[48:48 + len(replacement)] = replacement
        self.assertEqual(census(bytes(changed_type))['frames'][0]['type'], 530)
        with self.assertRaises(ValueError):
            census(original[:-1])
        self.assertEqual(census(original), expected)


if __name__ == '__main__':
    unittest.main()
