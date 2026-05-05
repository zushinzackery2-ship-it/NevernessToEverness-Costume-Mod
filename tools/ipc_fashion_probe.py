import argparse
import os
import struct
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT_DIR = os.path.abspath(os.path.join(SCRIPT_DIR, "..", ".."))
IPC_SCRIPTS = os.path.join(ROOT_DIR, "WinHttpRedirectProxy-main", "scripts")
sys.path.insert(0, IPC_SCRIPTS)

from ipc_client import IpcClient
OFF_GOBJECTS = 0x0E6AEF00
OFF_GNAMES = 0x0E5CAD80
OFF_GWORLD = 0x0E2CFDB0
OFF_UOBJECT_CLASS = 0x10
OFF_UOBJECT_NAME = 0x18
OFF_UOBJECT_OUTER = 0x20
OFF_WORLD_GAME_INSTANCE = 0x230
OFF_GAME_INSTANCE_LOCAL_PLAYERS = 0x38
OFF_PLAYER_CONTROLLER = 0x30
OFF_CONTROLLER_ACK_PAWN = 0x370
OFF_CHARACTER_CUR_APPEARANCE = 0x24A0
OFF_CHARACTER_SOFT_APPEARANCE = 0x24A8
OFF_CHARACTER_DEFAULT_ID = 0x24D0
OFF_CHARACTER_FASHION_ID = 0x24D8
OFF_CHARACTER_DATA_ASSET_DEFAULT_TABLE = 0x7E8
OFF_CHARACTER_DATA_ASSET_APPEARANCE_TABLE = 0x7F0
OFF_DATA_TABLE_ROW_MAP = 0x30
OFF_STATIC_APPEARANCE_TYPE = 0x8
OFF_STATIC_APPEARANCE_CHARACTER_ID = 0x0C
OFF_STATIC_APPEARANCE_DATA = 0x190
OFF_STATIC_APPEARANCE_IS_DEFAULT = 0x1D8
APPEARANCE_TYPES = {
    0: "Appearance_None",
    1: "Fashion",
    2: "Glide",
    3: "HeadIcon",
    4: "EAppearanceType_MAX",
}


class NamePoolResolver:
    def __init__(self, client, gnames_addr):
        self.client = client
        self.gnames_addr = gnames_addr
        self.cache = {}
        self.blocks = {}

    def resolve(self, fname_addr):
        ci = self.client.read_i32(fname_addr)
        if ci is None or ci <= 0:
            return None
        if ci in self.cache:
            return self.cache[ci]

        block = ci >> 16
        offset = (ci & 0xFFFF) * 2
        block_ptr = self.blocks.get(block)
        if block_ptr is None:
            block_ptr = self.client.read_ptr(self.gnames_addr + 0x10 + block * 8)
            if not block_ptr or block_ptr < 0x10000:
                return None
            self.blocks[block] = block_ptr

        header_addr = block_ptr + offset
        string_addr = header_addr + 2
        header = self.client.read_u16(header_addr)
        if header is None:
            return None
        length = header >> 6
        if length <= 0 or length > 1024:
            return None
        wide = (header & 0x1) != 0
        raw = self.client.read(string_addr, length * (2 if wide else 1))
        if not raw:
            return None
        try:
            name = raw.decode("utf-16-le" if wide else "ascii", errors="replace").rstrip("\x00")
        except Exception:
            return None
        if not name:
            return None
        self.cache[ci] = name
        return name


def hx(value):
    return f"0x{value:016X}" if value else "0x0000000000000000"


class Reader:
    def __init__(self, ipc):
        self.ipc = ipc

    def read(self, address, size):
        if not self.valid_ptr(address):
            return None
        data = self.ipc.read(address, size)
        if not data or len(data) < size:
            return None
        return data

    def u8(self, address):
        data = self.read(address, 1)
        return data[0] if data else None

    def i32(self, address):
        data = self.read(address, 4)
        return struct.unpack_from("<i", data, 0)[0] if data else None

    def u32(self, address):
        data = self.read(address, 4)
        return struct.unpack_from("<I", data, 0)[0] if data else None

    def u64(self, address):
        data = self.read(address, 8)
        return struct.unpack_from("<Q", data, 0)[0] if data else None

    def ptr(self, address):
        return self.u64(address)

    def valid_ptr(self, value):
        return value is not None and 0x10000 < value < 0x800000000000


class ObjectArray:
    def __init__(self, reader, address):
        self.reader = reader
        self.address = address
        self.objects_ptr = reader.ptr(address) or 0
        self.max_elements = reader.u32(address + 0x10) or 0
        self.num_elements = reader.u32(address + 0x14) or 0
        self.max_chunks = reader.u32(address + 0x18) or 0
        self.num_chunks = reader.u32(address + 0x1C) or 0
        self.chunks = []
        self.load_chunks()

    def load_chunks(self):
        if not self.reader.valid_ptr(self.objects_ptr) or self.num_chunks <= 0 or self.num_chunks > 0x1000:
            return
        data = self.reader.read(self.objects_ptr, self.num_chunks * 8)
        if not data:
            return
        for index in range(self.num_chunks):
            self.chunks.append(struct.unpack_from("<Q", data, index * 8)[0])

    def get_ptr(self, index):
        if index < 0 or index >= self.num_elements:
            return None
        chunk_index = index >> 16
        chunk_offset = index & 0xFFFF
        if chunk_index >= len(self.chunks):
            return None
        chunk = self.chunks[chunk_index]
        if not self.reader.valid_ptr(chunk):
            return None
        item = chunk + chunk_offset * 0x18
        obj = self.reader.ptr(item)
        return obj if self.reader.valid_ptr(obj) else None

    def iter_ptrs(self):
        for index in range(self.num_elements):
            obj = self.get_ptr(index)
            if obj:
                yield index, obj


def read_fname_raw(reader, address):
    data = reader.read(address, 8)
    if not data:
        return 0, 0, 0
    ci, num = struct.unpack_from("<II", data, 0)
    return ci, num, struct.unpack_from("<Q", data, 0)[0]


def resolve_fname(reader, resolver, address):
    ci, num, raw = read_fname_raw(reader, address)
    if ci == 0:
        name = "None"
    else:
        name = resolver.resolve(address) or f"ci_{ci:X}"
    if num:
        name = f"{name}_{num}"
    return name, raw, ci, num


def object_short_name(reader, resolver, obj):
    if not reader.valid_ptr(obj):
        return "<null>"
    name, _, _, _ = resolve_fname(reader, resolver, obj + OFF_UOBJECT_NAME)
    return name


def object_class_name(reader, resolver, obj):
    if not reader.valid_ptr(obj):
        return "<null>"
    klass = reader.ptr(obj + OFF_UOBJECT_CLASS)
    return object_short_name(reader, resolver, klass)


def object_path(reader, resolver, obj, max_depth=8):
    names = []
    cur = obj
    for _ in range(max_depth):
        if not reader.valid_ptr(cur):
            break
        names.append(object_short_name(reader, resolver, cur))
        cur = reader.ptr(cur + OFF_UOBJECT_OUTER)
    return ".".join(reversed(names)) if names else "<null>"


def tarray_ptr_num(reader, address):
    data = reader.ptr(address)
    num = reader.i32(address + 8) or 0
    max_count = reader.i32(address + 12) or 0
    return data, num, max_count


def bit_array_has(reader, bit_array_addr, index):
    inline_addr = bit_array_addr
    secondary = reader.ptr(bit_array_addr + 0x10)
    num_bits = reader.i32(bit_array_addr + 0x18) or 0
    if index < 0 or index >= num_bits:
        return False
    data_addr = secondary if reader.valid_ptr(secondary) else inline_addr
    word = reader.u32(data_addr + (index // 32) * 4)
    if word is None:
        return False
    return (word & (1 << (index & 31))) != 0


def iter_row_map(reader, table_addr):
    row_map = table_addr + OFF_DATA_TABLE_ROW_MAP
    data_ptr, num_allocated, max_allocated = tarray_ptr_num(reader, row_map)
    first_free = reader.i32(row_map + 0x30)
    num_free = reader.i32(row_map + 0x34) or 0
    live_count = max(0, num_allocated - num_free)
    if not reader.valid_ptr(data_ptr) or num_allocated <= 0 or num_allocated > 200000:
        return [], {"num_allocated": num_allocated, "max_allocated": max_allocated, "num_free": num_free, "live_count": live_count, "first_free": first_free}
    rows = []
    for index in range(num_allocated):
        if not bit_array_has(reader, row_map + 0x10, index):
            continue
        element = data_ptr + index * 0x18
        key_ci, key_num, key_raw = read_fname_raw(reader, element)
        value = reader.ptr(element + 8)
        if reader.valid_ptr(value):
            rows.append((index, element, key_raw, key_ci, key_num, value))
    return rows, {"num_allocated": num_allocated, "max_allocated": max_allocated, "num_free": num_free, "live_count": live_count, "first_free": first_free}


def find_game_module(ipc):
    modules = ipc.enum_modules()
    if not modules:
        return None, []
    for module in modules:
        if module["name"].lower() == "htgame.exe":
            return module, modules
    for module in modules:
        if module["name"].lower().endswith(".exe"):
            return module, modules
    return modules[0], modules


def inspect_character(reader, resolver, base):
    gworld_slot = base + OFF_GWORLD
    world = reader.ptr(gworld_slot)
    game_instance = reader.ptr(world + OFF_WORLD_GAME_INSTANCE) if reader.valid_ptr(world) else None
    local_players_data, local_players_num, local_players_max = tarray_ptr_num(reader, game_instance + OFF_GAME_INSTANCE_LOCAL_PLAYERS) if reader.valid_ptr(game_instance) else (None, 0, 0)
    local_player = reader.ptr(local_players_data) if reader.valid_ptr(local_players_data) and local_players_num > 0 else None
    controller = reader.ptr(local_player + OFF_PLAYER_CONTROLLER) if reader.valid_ptr(local_player) else None
    pawn = reader.ptr(controller + OFF_CONTROLLER_ACK_PAWN) if reader.valid_ptr(controller) else None
    default_name, default_raw, default_ci, default_num = resolve_fname(reader, resolver, pawn + OFF_CHARACTER_DEFAULT_ID) if reader.valid_ptr(pawn) else ("<invalid>", 0, 0, 0)
    fashion_name, fashion_raw, fashion_ci, fashion_num = resolve_fname(reader, resolver, pawn + OFF_CHARACTER_FASHION_ID) if reader.valid_ptr(pawn) else ("<invalid>", 0, 0, 0)
    cur_appearance = reader.ptr(pawn + OFF_CHARACTER_CUR_APPEARANCE) if reader.valid_ptr(pawn) else None
    soft_appearance_data = reader.read(pawn + OFF_CHARACTER_SOFT_APPEARANCE, 0x28) if reader.valid_ptr(pawn) else None
    return {
        "gworld_slot": gworld_slot,
        "world": world,
        "game_instance": game_instance,
        "local_players_data": local_players_data,
        "local_players_num": local_players_num,
        "local_players_max": local_players_max,
        "local_player": local_player,
        "controller": controller,
        "pawn": pawn,
        "pawn_class": object_class_name(reader, resolver, pawn),
        "default_name": default_name,
        "default_raw": default_raw,
        "default_ci": default_ci,
        "default_num": default_num,
        "fashion_name": fashion_name,
        "fashion_raw": fashion_raw,
        "fashion_ci": fashion_ci,
        "fashion_num": fashion_num,
        "cur_appearance": cur_appearance,
        "cur_appearance_name": object_path(reader, resolver, cur_appearance, 6),
        "cur_appearance_class": object_class_name(reader, resolver, cur_appearance),
        "soft_appearance_hex": soft_appearance_data.hex(" ") if soft_appearance_data else "<unreadable>",
    }


def inspect_appearance_table(reader, resolver, table, character_raw, limit=16):
    rows, meta = iter_row_map(reader, table)
    counts = {}
    matching = []
    for index, element, key_raw, key_ci, key_num, value in rows:
        appearance_type = reader.u8(value + OFF_STATIC_APPEARANCE_TYPE)
        character_id_raw = reader.u64(value + OFF_STATIC_APPEARANCE_CHARACTER_ID) or 0
        if character_id_raw != character_raw:
            continue
        counts[appearance_type] = counts.get(appearance_type, 0) + 1
        if len(matching) < limit:
            key_name, _, _, _ = resolve_fname(reader, resolver, element)
            char_name, _, _, _ = resolve_fname(reader, resolver, value + OFF_STATIC_APPEARANCE_CHARACTER_ID)
            script_struct = reader.ptr(value + OFF_STATIC_APPEARANCE_DATA)
            instanced_data = reader.ptr(value + OFF_STATIC_APPEARANCE_DATA + 8)
            is_default = reader.u8(value + OFF_STATIC_APPEARANCE_IS_DEFAULT)
            matching.append({
                "index": index,
                "row": key_name,
                "type": appearance_type,
                "type_name": APPEARANCE_TYPES.get(appearance_type, f"Unknown_{appearance_type}"),
                "character": char_name,
                "value": value,
                "script": object_path(reader, resolver, script_struct, 4),
                "instanced_data": instanced_data,
                "is_default": is_default,
            })
    return rows, meta, counts, matching


def inspect_default_table(reader, resolver, table, character_raw, limit=16):
    rows, meta = iter_row_map(reader, table)
    matching = []
    for index, element, key_raw, key_ci, key_num, value in rows:
        if key_raw != character_raw:
            continue
        if len(matching) < limit:
            key_name, _, _, _ = resolve_fname(reader, resolver, element)
            soft_obj_hex = reader.read(value + 8, 0x28)
            matching.append({
                "index": index,
                "row": key_name,
                "value": value,
                "soft_object_hex": soft_obj_hex.hex(" ") if soft_obj_hex else "<unreadable>",
            })
    return rows, meta, matching


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("pid", type=int)
    parser.add_argument("--asset-limit", type=int, default=128)
    parser.add_argument("--character-only", action="store_true")
    args = parser.parse_args()

    ipc = IpcClient(args.pid)
    if not ipc.is_connected():
        print("connect failed")
        return 1

    module, modules = find_game_module(ipc)
    if not module:
        print("module enumeration failed")
        return 1

    base = module["base"]
    reader = Reader(ipc)
    resolver = NamePoolResolver(ipc, base + OFF_GNAMES)
    objects = ObjectArray(reader, base + OFF_GOBJECTS)
    character = inspect_character(reader, resolver, base)

    print(f"module={module['name']} base={hx(base)} size=0x{module['size']:X}")
    print(f"GObjects={hx(base + OFF_GOBJECTS)} num={objects.num_elements} max={objects.max_elements} chunks={objects.num_chunks}/{objects.max_chunks}")
    print(f"GNames={hx(base + OFF_GNAMES)}")
    print(f"GWorldSlot={hx(character['gworld_slot'])} GWorld={hx(character['world'])}")
    print(f"GameInstance={hx(character['game_instance'])}")
    print(f"LocalPlayers data={hx(character['local_players_data'])} num={character['local_players_num']} max={character['local_players_max']}")
    print(f"LocalPlayer={hx(character['local_player'])} Controller={hx(character['controller'])} Pawn={hx(character['pawn'])} PawnClass={character['pawn_class']}")
    print(f"DefaultCharacterID={character['default_name']} raw=0x{character['default_raw']:016X} ci={character['default_ci']} num={character['default_num']}")
    print(f"FashionID={character['fashion_name']} raw=0x{character['fashion_raw']:016X} ci={character['fashion_ci']} num={character['fashion_num']}")
    print(f"CurPlayerAppearance={hx(character['cur_appearance'])} class={character['cur_appearance_class']} name={character['cur_appearance_name']}")
    print(f"SoftCurPlayerAppearanceRaw={character['soft_appearance_hex']}")

    if args.character_only:
        ipc.close()
        return 0

    if character["default_raw"] == 0:
        print("no valid DefaultCharacterID; stop")
        ipc.close()
        return 1

    scanned_assets = 0
    hit_assets = 0
    for index, obj in objects.iter_ptrs():
        class_name = object_class_name(reader, resolver, obj)
        if class_name != "CharacterDataAsset":
            continue
        scanned_assets += 1
        if scanned_assets > args.asset_limit:
            break
        appearance_table = reader.ptr(obj + OFF_CHARACTER_DATA_ASSET_APPEARANCE_TABLE)
        default_table = reader.ptr(obj + OFF_CHARACTER_DATA_ASSET_DEFAULT_TABLE)
        app_rows, app_meta, app_counts, app_matching = inspect_appearance_table(reader, resolver, appearance_table, character["default_raw"])
        default_rows, default_meta, default_matching = inspect_default_table(reader, resolver, default_table, character["default_raw"])
        if not app_matching and not default_matching:
            continue
        hit_assets += 1
        print("")
        print(f"CharacterDataAsset index=0x{index:X} addr={hx(obj)} name={object_path(reader, resolver, obj, 6)}")
        print(f"  AppearanceDataTable={hx(appearance_table)} rows={len(app_rows)} live={app_meta.get('live_count')} alloc={app_meta.get('num_allocated')} counts=" + ",".join(f"{APPEARANCE_TYPES.get(k, k)}:{v}" for k, v in sorted(app_counts.items(), key=lambda item: str(item[0]))))
        for row in app_matching:
            print(f"    app[{row['index']}] row={row['row']} type={row['type_name']} isDefault={row['is_default']} value={hx(row['value'])} script={row['script']} data={hx(row['instanced_data'])}")
        print(f"  DefaultAppearanceDataTable={hx(default_table)} rows={len(default_rows)} live={default_meta.get('live_count')} alloc={default_meta.get('num_allocated')} matches={len(default_matching)}")
        for row in default_matching:
            print(f"    default[{row['index']}] row={row['row']} value={hx(row['value'])} soft={row['soft_object_hex']}")

    print("")
    print(f"scanned CharacterDataAsset={scanned_assets} hit_assets={hit_assets}")
    ipc.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
