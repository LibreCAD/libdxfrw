"""Bounded R13/R14/R2000 object-frame and plain DICTIONARY observers.

These research checks validate selected physical frames; they do not establish
native duplicate-merge semantics or qualify whole-drawing output.
"""
import hashlib
import re

from audit_ac1024_sortents_boundary import Bits as SharedBits, crc16, require


VERSIONS = {"r13": b"AC1012", "r14": b"AC1014", "r2000": b"AC1015"}


class Bits(SharedBits):
    @property
    def position(self):
        return self.bit

    def little(self, n):
        return sum(self.take(8) << (8 * i) for i in range(n))

    def handle(self, role, base=0, own=False):
        actual, length, value = super().handle()
        if own:
            require(actual == 0 and value != 0, "invalid self handle")
        elif actual == 0 and length == 0:
            pass
        elif actual == role:
            pass
        elif role == 4 and actual in (6, 8, 10, 12):
            require(actual not in (6, 8) or length == 0, "invalid relative handle width")
            delta = 1 if actual in (6, 8) else value
            value = base + delta if actual in (6, 10) else base - delta
            require(0 <= value <= 0xFFFFFFFF, "relative handle overflow")
        else:
            raise ValueError(f"invalid handle role {actual} expected {role}")
        return {"role": actual, "length": length, "value": value}


def locate_object_frame(data, trace, version, handle, object_type):
    """Resolve one traced legacy frame and verify its physical prefix and CRC."""
    require(version in VERSIONS and data.startswith(VERSIONS[version]), "DWG version mismatch")
    require(isinstance(handle, int) and 0 < handle <= 0xFFFFFFFF, "invalid handle")
    require(isinstance(object_type, int) and object_type >= 0, "invalid object type")
    blocks = re.split(r"(?=Object number:)", trace)
    pattern = re.compile(r"\bhandle: 0\.\d+\." + format(handle, "X") + r"\s+\[H 5\]", re.IGNORECASE)
    selected = [block for block in blocks if pattern.search(block)]
    require(len(selected) == 1, "trace does not identify one selected object frame")
    match = re.search(r"Size: (\d+) \[MS\], Type: " + str(object_type)
                      + r" \[BS\], Address: (\d+)", selected[0])
    require(match is not None, "object frame size/address or type missing")
    size, address = map(int, match.groups())
    require(size < 0x8000 and address >= 2 and address + size + 2 <= len(data),
            "object frame is outside the source")
    require(data[address - 2:address] == size.to_bytes(2, "little"), "unexpected frame-size prefix")
    require(int.from_bytes(data[address + size:address + size + 2], "little")
            == crc16(data[address - 2:address + size]), "object frame CRC mismatch")
    return address, data[address:address + size]


def decode_dictionary_frame(frame, version, expected_self, *, expected_entries=None,
                            allow_empty_name=False, with_default=False):
    """Walk a selected legacy DICTIONARY or DICTIONARYWDFLT handle tail."""
    require(version in VERSIONS, "unsupported dictionary version")
    require(not with_default or version in ("r13", "r14"), "unqualified default-dictionary version")
    bits = Bits(frame)
    require(bits.bs() == (500 if with_default else 42), "wrong object type")
    boundary = bits.little(4) if version == "r2000" else None
    own = bits.handle(0, own=True)
    require(own["value"] == expected_self, "self handle mismatch")
    eed = []
    while True:
        length = bits.bs()
        if not length:
            break
        require(length <= 1024 and len(eed) < 32, "EED limit")
        appid = bits.handle(5)
        eed.append({"appid": appid, "hex": bytes(bits.take(8) for _ in range(length)).hex()})
    if boundary is None:
        boundary = bits.little(4)
    reactors, count = bits.bl(), bits.bl()
    require(reactors <= 8 and count <= 64, "dictionary count exceeds bounded profile")
    if expected_entries is not None:
        require(count == len(expected_entries), "dictionary entry count differs from the bounded model")
    if version == "r14" and not with_default:
        require(bits.take(8) == 0, "unexpected R14-only byte")
    cloning_start = bits.position
    cloning = bits.bs() if version == "r2000" or with_default else None
    hard_owner = bits.take(8) if version == "r2000" or with_default else None
    names = []
    for _ in range(count):
        length = bits.bs()
        require((length > 0 or allow_empty_name) and length <= 256, "dictionary TV name length is invalid")
        raw = bytes(bits.take(8) for _ in range(length))
        require(raw.count(b"\0") <= 1 and (b"\0" not in raw or raw.endswith(b"\0")),
                "dictionary TV name has an invalid terminator")
        names.append({"name": (raw[:-1] if raw.endswith(b"\0") else raw).decode("ascii"),
                      "nameHasTerminator": raw.endswith(b"\0")})
    require(len({name["name"] for name in names}) == len(names), "duplicate dictionary name")
    require(bits.position == boundary, "dictionary data/handle boundary mismatch")
    owner = bits.handle(4, expected_self)
    reactor_handles = [bits.handle(4, expected_self) for _ in range(reactors)]
    xdict_offset = bits.position
    xdict = bits.handle(3, expected_self)
    entries = [dict(name=name["name"], nameHasTerminator=name["nameHasTerminator"],
                    handle=bits.handle(2, expected_self)) for name in names]
    if expected_entries is not None:
        require({row["name"]: row["handle"]["value"] for row in entries} == expected_entries,
                "physical dictionary entries differ from the model")
    default_start = bits.position
    default = bits.handle(5, expected_self) if with_default else None
    require(len(frame) * 8 - bits.position < 8, "unexpected trailing data")
    return {"self": own, "dataEndBit": boundary, "cloning": cloning, "hardOwner": hard_owner,
            "owner": owner, "reactors": reactor_handles, "xdict": xdict,
            "xdictBitOffset": xdict_offset, "entries": entries, "eed": eed,
            "cloningStartBit": cloning_start, "defaultStartBit": default_start, "default": default,
            "paddingBits": len(frame) * 8 - bits.position,
            "frameSha256": hashlib.sha256(frame).hexdigest()}
