#!/usr/bin/env python3
"""Fast malformed-frame and version controls for the shared legacy walker."""
import unittest

from legacy_dwg_dictionary_wire import crc16, decode_dictionary_frame, locate_object_frame


# Empty controls and a populated R13 dictionary were locally generated from
# scratch with LibreDWG 0.14. Only the object bodies are embedded here.
EMPTY = {
    "r13": "4a804be48000000406c123410c30",
    "r14": "4a804be5000000040600c123410c30",
    "r2000": "4a96800000004be40640403048d0430c00",
}
POPULATED = bytes.fromhex(
    "4a8045e36010000405034214dd185b99185c9910853414d455f4b455942511154d517d3d393167"
    "042d0430c0846084b884bc")
ENTRIES = {"Standard": 24, "SAME_KEY": 46, "DEST_ONLY": 47}


def flip(data, bit):
    damaged = bytearray(data)
    damaged[bit // 8] ^= 0x80 >> (bit % 8)
    return bytes(damaged)


class LegacyDictionaryWireTests(unittest.TestCase):
    def test_empty_versions_and_version_mismatch(self):
        for version, encoded in EMPTY.items():
            with self.subTest(version=version):
                frame = bytes.fromhex(encoded)
                row = decode_dictionary_frame(frame, version, 0x2F, expected_entries={})
                self.assertEqual(row["owner"]["value"], 0xC)
                self.assertEqual(row["cloning"], 1 if version == "r2000" else None)
                for bad in (frame[:-1], flip(frame, row["xdictBitOffset"] + 3)):
                    with self.assertRaises(ValueError):
                        decode_dictionary_frame(bad, version, 0x2F, expected_entries={})
                self.assertEqual(decode_dictionary_frame(frame, version, 0x2F, expected_entries={}), row)
        with self.assertRaises(ValueError):
            decode_dictionary_frame(bytes.fromhex(EMPTY["r13"]), "r14", 0x2F, expected_entries={})

    def test_populated_roles_and_retry(self):
        row = decode_dictionary_frame(POPULATED, "r13", 23, expected_entries=ENTRIES)
        self.assertEqual(row["dataEndBit"], 310)
        self.assertEqual([entry["handle"]["value"] for entry in row["entries"]], [24, 46, 47])
        # Parent role, xdictionary role, and first entry role are distinct.
        for bit in (row["dataEndBit"] + 3, row["xdictBitOffset"] + 3,
                    row["xdictBitOffset"] + 8 + 3):
            with self.subTest(bit=bit), self.assertRaises(ValueError):
                decode_dictionary_frame(flip(POPULATED, bit), "r13", 23, expected_entries=ENTRIES)
        with self.assertRaises(ValueError):
            decode_dictionary_frame(POPULATED, "r13", 23, expected_entries={**ENTRIES, "SAME_KEY": 45})
        self.assertEqual(decode_dictionary_frame(POPULATED, "r13", 23, expected_entries=ENTRIES), row)

    def test_frame_locator_refuses_corruption_and_ambiguous_trace(self):
        prefix = len(POPULATED).to_bytes(2, "little")
        payload = prefix + POPULATED
        data = b"AC1012" + payload + crc16(payload).to_bytes(2, "little")
        trace = (f"Object number: 0/0, Size: {len(POPULATED)} [MS], Type: 42 [BS], Address: 8\n"
                 "handle: 0.1.17 [H 5]\n")
        self.assertEqual(locate_object_frame(data, trace, "r13", 23, 42), (8, POPULATED))
        for invalid_data, invalid_trace, version in (
            (data[:-1], trace, "r13"), (data[:-1] + bytes([data[-1] ^ 1]), trace, "r13"),
            (data[:6] + b"\x00" + data[7:], trace, "r13"), (data, trace + trace, "r13"),
            (data, trace, "r14"), (data, trace.replace("Type: 42", "Type: 43"), "r13"),
        ):
            with self.subTest(version=version), self.assertRaises(ValueError):
                locate_object_frame(invalid_data, invalid_trace, version, 23, 42)
        self.assertEqual(locate_object_frame(data, trace, "r13", 23, 42), (8, POPULATED))


if __name__ == "__main__":
    unittest.main()
