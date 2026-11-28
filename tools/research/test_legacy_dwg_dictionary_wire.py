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
POPULATED = {
    "r13": bytes.fromhex("4a8045e36010000405034214dd185b99185c9910853414d455f4b455942511154d517d3d393167"
                         "042d0430c0846084b884bc"),
    "r14": bytes.fromhex("4a8045e5601000040503004254dd185b99185c990010953414d455f4b45590042911154d517d3d393164"
                         "03042d0430c0846084b884bc"),
    "r2000": bytes.fromhex("4a980040000045e4050340401095374616e64617264004254d0535157d2d1564010a444553545f4f4e4c"
                           "5900c10b410c302118212e212f"),
}
ENTRIES = {"Standard": 24, "SAME_KEY": 46, "DEST_ONLY": 47}
# Locally authored seed saved by ODA 27.1.0; these selected type-500 bodies
# exercise the observed legacy DICTIONARYWDFLT subclass fields and EED.
WITH_DEFAULT = {
    "r13": bytes.fromhex("3d004044914512a000d002d54524541545f41535f484152444600009580400024050100419393d493505310428c6ec844d444c"),
    "r14": bytes.fromhex("3d004044a9c00000090140401064e4f524d414c410a31bd211351130"),
}


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
        for version, frame in POPULATED.items():
            with self.subTest(version=version):
                row = decode_dictionary_frame(frame, version, 23, expected_entries=ENTRIES)
                self.assertEqual(row["dataEndBit"], {"r13": 310, "r14": 342, "r2000": 352}[version])
                self.assertEqual(row["cloning"], 1 if version == "r2000" else None)
                self.assertEqual([entry["handle"]["value"] for entry in row["entries"]], [24, 46, 47])
                # Parent role, xdictionary role, and first entry role are distinct.
                for bit in (row["dataEndBit"] + 3, row["xdictBitOffset"] + 3,
                            row["xdictBitOffset"] + 8 + 3):
                    with self.subTest(bit=bit), self.assertRaises(ValueError):
                        decode_dictionary_frame(flip(frame, bit), version, 23, expected_entries=ENTRIES)
                with self.assertRaises(ValueError):
                    decode_dictionary_frame(frame, version, 23, expected_entries={**ENTRIES, "SAME_KEY": 45})
                self.assertEqual(decode_dictionary_frame(frame, version, 23, expected_entries=ENTRIES), row)

    def test_frame_locator_refuses_corruption_and_ambiguous_trace(self):
        frame = POPULATED["r13"]
        prefix = len(frame).to_bytes(2, "little")
        payload = prefix + frame
        data = b"AC1012" + payload + crc16(payload).to_bytes(2, "little")
        trace = (f"Object number: 0/0, Size: {len(frame)} [MS], Type: 42 [BS], Address: 8\n"
                 "handle: 0.1.17 [H 5]\n")
        self.assertEqual(locate_object_frame(data, trace, "r13", 23, 42), (8, frame))
        for invalid_data, invalid_trace, version in (
            (data[:-1], trace, "r13"), (data[:-1] + bytes([data[-1] ^ 1]), trace, "r13"),
            (data[:6] + b"\x00" + data[7:], trace, "r13"), (data, trace + trace, "r13"),
            (data, trace, "r14"), (data, trace.replace("Type: 42", "Type: 43"), "r13"),
        ):
            with self.subTest(version=version), self.assertRaises(ValueError):
                locate_object_frame(invalid_data, invalid_trace, version, 23, 42)
        self.assertEqual(locate_object_frame(data, trace, "r13", 23, 42), (8, frame))

    def test_dictionary_with_default_selected_legacy_frames(self):
        for version, frame in WITH_DEFAULT.items():
            with self.subTest(version=version):
                row = decode_dictionary_frame(frame, version, 0x12,
                                              expected_entries={"NORMAL": 0x13}, with_default=True)
                self.assertEqual((row["cloning"], row["hardOwner"]), (1, 0))
                self.assertEqual(row["owner"]["value"], 0xA)
                self.assertEqual(row["entries"][0]["handle"]["value"], 0x13)
                self.assertEqual(row["default"]["value"], 0x13)
                self.assertEqual(len(row["eed"]), 1 if version == "r13" else 0)
                for bit in (row["dataEndBit"] + 3, row["xdictBitOffset"] + 3,
                            row["xdictBitOffset"] + 8 * (1 + row["xdict"]["length"]) + 3,
                            row["defaultStartBit"] + 3):
                    with self.subTest(bit=bit), self.assertRaises(ValueError):
                        decode_dictionary_frame(flip(frame, bit), version, 0x12,
                                                expected_entries={"NORMAL": 0x13}, with_default=True)
                with self.assertRaises(ValueError):
                    decode_dictionary_frame(frame[:-1], version, 0x12,
                                            expected_entries={"NORMAL": 0x13}, with_default=True)
                self.assertEqual(decode_dictionary_frame(frame, version, 0x12,
                                 expected_entries={"NORMAL": 0x13}, with_default=True), row)
        with self.assertRaises(ValueError):
            decode_dictionary_frame(WITH_DEFAULT["r14"], "r2000", 0x12,
                                    expected_entries={"NORMAL": 0x13}, with_default=True)
if __name__ == "__main__":
    unittest.main()
