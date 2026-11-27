#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

#include "cpl_vsi.h"
#include "CTBCompactTileSerializer.hpp"
#include "CTBOutputStream.hpp"
#include "EsriCompactCacheV2Writer.hpp"
#include "TerrainTile.hpp"
#include "zlib.h"

static void require(bool condition, const char *message) {
  if (!condition) {
    std::cerr << message << std::endl;
    std::exit(1);
  }
}

class VectorOutputStream : public ctb::CTBOutputStream {
public:
  virtual uint32_t write(const void *ptr, uint32_t size) {
    const unsigned char *bytes = static_cast<const unsigned char *>(ptr);
    data.insert(data.end(), bytes, bytes + size);
    return size;
  }
  std::vector<unsigned char> data;
};

static std::vector<unsigned char> inflatePayload(
  const std::vector<unsigned char> &compressed) {
  z_stream stream = {};
  require(inflateInit2(&stream, 15 + 16) == Z_OK, "inflateInit2 failed");
  std::vector<unsigned char> output(10000);
  stream.next_in = const_cast<Bytef *>(compressed.data());
  stream.avail_in = static_cast<uInt>(compressed.size());
  stream.next_out = output.data();
  stream.avail_out = static_cast<uInt>(output.size());
  const int result = inflate(&stream, Z_FINISH);
  const size_t written = output.size() - stream.avail_out;
  inflateEnd(&stream);
  require(result == Z_STREAM_END, "Payload is not a complete gzip stream");
  output.resize(written);
  return output;
}

int main(int argc, char **argv) {
  require(argc == 2, "Expected a temporary output directory");
  VSIRmdirRecursive(argv[1]);

  ctb::TerrainTile tile(ctb::TileCoordinate(3, 4, 5));
  ctb::CTBCompactTileSerializer serializer(argv[1], false);
  serializer.startSerialization();
  serializer.serializeTile(&tile);
  serializer.endSerialization();

  ctb::EsriCompactCacheV2Writer writer(argv[1]);
  require(writer.hasTile(3, 4, 5), "Serializer should persist the terrain tile");
  const std::vector<unsigned char> compressed = writer.readTile(3, 4, 5);
  require(compressed.size() >= 2 && compressed[0] == 0x1f && compressed[1] == 0x8b,
          "Serializer payload should be gzip encoded");

  VectorOutputStream raw;
  tile.writeFile(raw);
  require(inflatePayload(compressed) == raw.data,
          "Decompressed compact payload should equal the Terrain bytes");
  return 0;
}
