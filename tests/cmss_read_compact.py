import glob
import os
import sys

cmss_root, directory_root, compact_root = sys.argv[1:]
sys.path.insert(0, cmss_root)
sys.path.insert(
    0, os.path.join(cmss_root, "plugins", "mapproxy-terrain")
)

from mapproxy_terrain.storage import CompactV2TerrainStore

store = CompactV2TerrainStore(compact_root)
tiles = glob.glob(os.path.join(directory_root, "*", "*", "*.terrain"))
if not tiles:
    raise AssertionError("directory output did not contain terrain tiles")

for tile_path in tiles:
    y = int(os.path.splitext(os.path.basename(tile_path))[0])
    x = int(os.path.basename(os.path.dirname(tile_path)))
    z = int(os.path.basename(os.path.dirname(os.path.dirname(tile_path))))
    with open(tile_path, "rb") as stream:
        expected = stream.read()
    payload = store.get_tile((x, y, z))
    if payload is None:
        raise AssertionError(f"CMSS did not find compact tile {z}/{x}/{y}")
    if payload.data != expected:
        raise AssertionError(f"CMSS payload differs for compact tile {z}/{x}/{y}")

print(f"verified {len(tiles)} compact terrain tile(s) with CMSS")
