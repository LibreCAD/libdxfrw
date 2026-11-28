#!/usr/bin/env python3
"""Independently decode the captured AC1024 APPID and APPID_CONTROL bodies.

The inputs are raw object bodies emitted by the same-import capture selector.
This script implements the small R2010 bit/handle primitives it needs rather
than invoking libdxfrw or another DWG parser.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path


class Bits:
    def __init__(self, data: bytes, bit: int = 0):
        self.data = data
        self.bit = bit

    def take(self, count: int) -> int:
        if count < 0 or self.bit + count > len(self.data) * 8:
            raise ValueError("field exceeds captured body")
        value = 0
        for _ in range(count):
            value = (value << 1) | (
                (self.data[self.bit // 8] >> (7 - self.bit % 8)) & 1
            )
            self.bit += 1
        return value

    def rc(self) -> int:
        return self.take(8)

    def rs(self) -> int:
        return self.rc() | (self.rc() << 8)

    def bs(self) -> tuple[int, int, int, int]:
        start = self.bit
        selector = self.take(2)
        value = (self.rs() if selector == 0 else self.rc()
                 if selector == 1 else 0 if selector == 2 else 256)
        return value, start, self.bit, selector

    def bl(self) -> tuple[int, int, int, int]:
        start = self.bit
        selector = self.take(2)
        if selector == 0:
            value = (self.rc() | (self.rc() << 8) | (self.rc() << 16)
                     | (self.rc() << 24))
        elif selector == 1:
            value = self.rc()
        else:
            value = 0
        return value, start, self.bit, selector

    def handle(self) -> dict[str, int]:
        start = self.bit
        header = self.rc()
        code, size = header >> 4, header & 0x0F
        if size > 8:
            raise ValueError("handle payload exceeds 64 bits")
        reference = 0
        for _ in range(size):
            reference = (reference << 8) | self.rc()
        return {"startBit": start, "endBit": self.bit, "code": code,
                "sizeBytes": size, "reference": reference}

    def bits_to(self, end: int) -> str:
        if end < self.bit or end > len(self.data) * 8:
            raise ValueError("invalid residual bit span")
        probe = Bits(self.data, self.bit)
        return "".join(str(probe.take(1)) for _ in range(end - self.bit))


def read_type(data: bytes, expected: int) -> tuple[Bits, dict[str, int]]:
    bits = Bits(data)
    start = bits.bit
    selector = bits.take(2)
    if selector != 0:
        raise ValueError("AC1024 fixed object type did not use raw-byte form")
    value = bits.rc()
    if value != expected:
        raise ValueError(f"expected object type {expected}, got {value}")
    return bits, {"startBit": start, "endBit": bits.bit,
                  "type": value, "selector": selector}


def read_common(data: bytes, expected_type: int) -> tuple[Bits, dict]:
    bits, type_span = read_type(data, expected_type)
    own_handle = bits.handle()
    eed, eed_start, eed_end, eed_selector = bits.bs()
    if eed != 0:
        raise ValueError("capture has EED; this proof expects empty EED")
    reactors, reactor_start, reactor_end, reactor_selector = bits.bl()
    if reactors != 0:
        raise ValueError("capture has reactors; this proof expects none")
    xdict_missing = bits.take(1)
    common = {
        "type": type_span,
        "ownHandle": own_handle,
        "eedSizeBS": {"startBit": eed_start, "endBit": eed_end,
                      "selector": eed_selector, "value": eed},
        "reactorCountBL": {"startBit": reactor_start, "endBit": reactor_end,
                            "selector": reactor_selector, "value": reactors},
        "xdictMissing": {"startBit": bits.bit - 1, "endBit": bits.bit,
                         "value": xdict_missing},
    }
    return bits, common


def decode_appid(data: bytes, object_data_bits: int, handle_bits: int) -> dict:
    bits, common = read_common(data, 67)
    xref_flags, flags_start, flags_end, flags_selector = bits.bs()
    unknown = bits.rc()

    body_end = object_data_bits
    footer = Bits(data, body_end - 17)
    encoded_string_bits = footer.rs()
    has_strings = footer.take(1)
    if not has_strings:
        raise ValueError("APPID capture has no detached string stream")
    string_end = body_end - 17
    string_start = string_end - encoded_string_bits
    if string_start != bits.bit:
        raise ValueError("detached APPID string span does not follow body fields")
    strings = Bits(data, string_start)
    units, units_start, _, units_selector = strings.bs()
    code_units = [strings.rs() for _ in range(units)]
    name = bytes(byte for unit in code_units
                 for byte in (unit & 0xFF, unit >> 8)).decode("utf-16le")
    if strings.bit != string_end:
        raise ValueError("APPID string decoder did not consume its exact span")

    handles = Bits(data, object_data_bits)
    parent = handles.handle()
    external_xref = handles.handle()
    expected_end = object_data_bits + handle_bits
    residual = handles.bits_to(expected_end)
    if handles.bit + len(residual) != expected_end:
        raise ValueError("APPID handle stream size does not match frame prefix")

    return {
        "bodyBytes": len(data),
        "bodySha256": hashlib.sha256(data).hexdigest(),
        "objectDataBits": object_data_bits,
        "handleStreamBits": handle_bits,
        "common": common,
        "xrefMetadataBS": {"startBit": flags_start, "endBit": flags_end,
                           "selector": flags_selector, "value": xref_flags},
        "unknown71RC": {"startBit": flags_end, "endBit": bits.bit,
                         "value": unknown},
        "detachedString": {
            "presenceBit": {"startBit": body_end - 1, "endBit": body_end,
                            "value": has_strings},
            "encodedSpanBits": encoded_string_bits,
            "startBit": string_start,
            "endBit": string_end,
            "lengthBS": {"startBit": units_start, "endBit": units_start + 10,
                         "selector": units_selector, "value": units},
            "utf16CodeUnits": code_units,
            "name": name,
        },
        "handles": {
            "parentAppIdControlSoftPointer": parent,
            "externalReferenceBlockHardPointer": external_xref,
            "residualBitsAfterKnownHandles": residual,
            "residualSpan": {"startBit": handles.bit,
                             "endBit": expected_end},
        },
    }


def decode_control(data: bytes, object_data_bits: int, handle_bits: int) -> dict:
    bits, common = read_common(data, 66)
    count, count_start, count_end, count_selector = bits.bl()
    string_flag = bits.take(1)
    if string_flag != 0:
        raise ValueError("APPID_CONTROL declares a detached string stream")
    if bits.bit != object_data_bits:
        raise ValueError("APPID_CONTROL body fields do not end at frame boundary")

    handles = Bits(data, object_data_bits)
    owner = handles.handle()
    entries = [handles.handle() for _ in range(count)]
    expected_end = object_data_bits + handle_bits
    residual = handles.bits_to(expected_end)
    if handles.bit + len(residual) != expected_end:
        raise ValueError("APPID_CONTROL handle stream size mismatch")
    if any(item["code"] != 2 for item in entries):
        raise ValueError("APPID_CONTROL member is not a DWG soft owner (code 2)")

    return {
        "bodyBytes": len(data),
        "bodySha256": hashlib.sha256(data).hexdigest(),
        "objectDataBits": object_data_bits,
        "handleStreamBits": handle_bits,
        "common": common,
        "entryCountBL": {"startBit": count_start, "endBit": count_end,
                         "selector": count_selector, "value": count},
        "stringStreamAbsentBit": {"startBit": object_data_bits - 1,
                                  "endBit": object_data_bits,
                                  "value": string_flag},
        "handles": {
            "nullOwnerSoftPointer": owner,
            "entries": entries,
            "residualBitsAfterKnownHandles": residual,
            "residualSpan": {"startBit": handles.bit,
                             "endBit": expected_end},
        },
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("appid_body", type=Path)
    parser.add_argument("appid_control_body", type=Path)
    args = parser.parse_args()
    appid = decode_appid(args.appid_body.read_bytes(), 132, 28)
    control = decode_control(args.appid_control_body.read_bytes(), 42, 214)
    appid_members = [entry["reference"] for entry in control["handles"]["entries"]]
    if (appid["detachedString"]["name"] != "ACAD"
            or appid["handles"]["parentAppIdControlSoftPointer"]["reference"] != 9
            or control["common"]["ownHandle"]["reference"] != 9
            or appid["common"]["ownHandle"]["reference"] != 18
            or appid_members.count(18) != 1):
        raise ValueError("captured APPID graph does not match the typed publication")
    print(json.dumps({"appid": appid, "appidControl": control}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
