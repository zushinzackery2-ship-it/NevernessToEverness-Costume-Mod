"""
Debug script for CurrentFashionSetter mod.
Reads game memory via IPC to inspect FashionID and AppearanceDataTable.
"""
import sys
import os
import struct
import time

# Add the scripts directory to path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'WinHttpRedirectProxy-main', 'scripts'))
from ipc_client import IpcClient


class MemReader:
    def __init__(self, ipc: IpcClient):
        self.ipc = ipc

    def u8(self, addr):
        d = self.ipc.read(addr, 1)
        return d[0] if d else None

    def u16(self, addr):
        d = self.ipc.read(addr, 2)
        return struct.unpack('<H', d)[0] if d else None

    def u32(self, addr):
        d = self.ipc.read(addr, 4)
        return struct.unpack('<I', d)[0] if d else None

    def i32(self, addr):
        d = self.ipc.read(addr, 4)
        return struct.unpack('<i', d)[0] if d else None

    def u64(self, addr):
        d = self.ipc.read(addr, 8)
        return struct.unpack('<Q', d)[0] if d else None

    def ptr(self, addr):
        return self.u64(addr)

    def bytes(self, addr, size):
        return self.ipc.read(addr, size)

    def valid_ptr(self, v):
        return v is not None and 0x100000000 < v < 0x800000000000


def read_fname(mem: MemReader, fname_addr: int) -> str:
    """Read FName at given address."""
    # FName is typically 8 bytes: ComparisonIndex (4) + Number (4)
    d = mem.bytes(fname_addr, 8)
    if not d or len(d) < 8:
        return "<invalid>"
    comparison_index = struct.unpack_from('<I', d, 0)[0]
    number = struct.unpack_from('<I', d, 4)[0]
    # We don't have name resolution here, just return the index
    return f"FName(CI={comparison_index:#x}, Num={number})"


def main():
    pid = 11872  # HTGame PID from earlier
    print(f"[*] Connecting to PID {pid}...")
    ipc = IpcClient(pid)
    if not ipc.is_connected():
        print("[!] Failed to connect to game process")
        return

    mem = MemReader(ipc)
    print("[+] Connected")

    # Known offsets from SDK analysis
    # AHTPlayerCharacter::FashionID = 0x24D8
    # AHTPlayerCharacter::DefaultCharacterID = 0x24D0
    # AHTPlayerCharacter::CurPlayerAppearance = 0x24A0
    # AHTPlayerCharacter::SoftCurPlayerAppearance = 0x24A8
    # AHTPlayerCharacter::CurMeshData = 0x2490
    
    # We need to find the character object first
    # For now, let's try to read GWorld and follow the chain
    # GWorld address needs to be known or scanned
    
    # Let's try to scan for GWorld by looking for known patterns
    # For now, we'll just dump some memory at known offsets if we had the character address
    
    print("[*] This script needs the character object address to continue.")
    print("[*] Please provide the character address in hex (e.g., 0x12345678):")
    print("[*] Or press Enter to exit.")
    
    # For testing, let's try to find the character by scanning GObjects
    # This is a simplified version - in reality we'd need to parse GObjects properly
    
    # Let's try to read some memory to see if we can find patterns
    print("[*] Attempting to find character objects...")
    
    # We'll use a simple heuristic: look for objects with specific patterns
    # This is a placeholder - real implementation would need proper GObjects parsing
    
    print("[*] Script finished. For full analysis, we need to implement GObjects parsing.")
    ipc.close()


if __name__ == "__main__":
    main()