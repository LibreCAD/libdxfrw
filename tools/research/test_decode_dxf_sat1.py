"""In-memory regression checks for the ASCII DXF SAT-v1 research decoder."""

from __future__ import annotations

import unittest

from tools.research import decode_dxf_sat1


def _encrypted_line(value: bytes) -> bytes:
    encoded = bytearray()
    for byte in value:
        encrypted = 0x9F - byte if byte > 0x20 else byte
        if encrypted == ord("^"):
            encoded.extend(b"^ ")
        else:
            encoded.append(encrypted)
    return bytes(encoded)


def _carrier(marker: bytes) -> tuple[bytes, bytes]:
    sat_lines = [
        b"400 0 1 0",
        b"body " + b"x" * 260 + b"^",
        marker,
    ]
    payload = b"\n".join(sat_lines) + b"\n"
    groups: list[tuple[int, bytes]] = [
        (0, b"SECTION"),
        (2, b"ENTITIES"),
        (0, b"3DSOLID"),
        (5, b"A"),
        (70, b"1"),
    ]
    for line in sat_lines:
        encrypted = _encrypted_line(line)
        for offset in range(0, len(encrypted), 255):
            groups.append((1 if offset == 0 else 3, encrypted[offset:offset + 255]))
    groups.extend(((0, b"ENDSEC"), (0, b"EOF")))
    drawing = b"".join(
        str(code).encode("ascii") + b"\n" + value + b"\n"
        for code, value in groups
    )
    return drawing, payload


class DecodeDxfSat1Tests(unittest.TestCase):
    def test_prefixed_and_unprefixed_acis_asm_markers_and_chunking(self) -> None:
        markers = (
            b"End-of-ACIS-data",
            b"#End-of-ACIS-data",
            b"End-of-ASM-data",
            b"#End-of-ASM-data",
        )
        for marker in markers:
            with self.subTest(marker=marker):
                drawing, expected = _carrier(marker)
                self.assertEqual(
                    decode_dxf_sat1.extract_sat1_payload(drawing, "0xA"),
                    expected,
                )

    def test_unknown_terminal_marker_is_refused(self) -> None:
        drawing, _ = _carrier(b"End-of-unknown-data")
        with self.assertRaises(decode_dxf_sat1.DecodeError):
            decode_dxf_sat1.extract_sat1_payload(drawing, "A")

    def test_terminal_marker_with_trailing_dxf_padding_is_accepted(self) -> None:
        drawing, expected = _carrier(b"End-of-ACIS-data \t")
        self.assertEqual(
            decode_dxf_sat1.extract_sat1_payload(drawing, "A"),
            expected,
        )


if __name__ == "__main__":
    unittest.main()
