#include "CTBCompactTileSerializer.hpp"

#include <cstring>
#include <vector>

#include "CTBException.hpp"
#include "CTBOutputStream.hpp"
#include "zlib.h"

namespace {
  class GzipMemoryOutputStream : public ctb::CTBOutputStream {
  public:
    GzipMemoryOutputStream() : mstream() {
      std::memset(&mstream, 0, sizeof(mstream));
      if (deflateInit2(&mstream, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
                       15 + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK)
        throw ctb::CTBException("Could not initialize gzip output");
    }

    ~GzipMemoryOutputStream() {
      deflateEnd(&mstream);
    }

    virtual uint32_t write(const void *ptr, uint32_t size) {
      mstream.next_in = static_cast<Bytef *>(const_cast<void *>(ptr));
      mstream.avail_in = size;
      drain(Z_NO_FLUSH);
      if (mstream.avail_in != 0)
        throw ctb::CTBException("Could not compress terrain payload");
      return size;
    }

    std::vector<unsigned char> finish() {
      int result;
      do {
        result = drain(Z_FINISH);
      } while (result == Z_OK);
      if (result != Z_STREAM_END)
        throw ctb::CTBException("Could not finish terrain gzip payload");
      return mdata;
    }

  private:
    int drain(int flush) {
      unsigned char buffer[16384];
      int result;
      do {
        mstream.next_out = buffer;
        mstream.avail_out = sizeof(buffer);
        result = deflate(&mstream, flush);
        if (result != Z_OK && result != Z_STREAM_END &&
            result != Z_BUF_ERROR)
          throw ctb::CTBException("Could not compress terrain payload");
        const size_t written = sizeof(buffer) - mstream.avail_out;
        mdata.insert(mdata.end(), buffer, buffer + written);
      } while (mstream.avail_out == 0);
      return result;
    }

    z_stream mstream;
    std::vector<unsigned char> mdata;
  };
}

ctb::CTBCompactTileSerializer::CTBCompactTileSerializer(
  const std::string &outputDir, bool resume) :
  mwriter(outputDir), mresume(resume) {}

void ctb::CTBCompactTileSerializer::startSerialization() {}

bool ctb::CTBCompactTileSerializer::mustSerializeCoordinate(
  const ctb::TileCoordinate *coordinate) {
  return !mresume ||
    !mwriter.hasTile(coordinate->zoom, coordinate->x, coordinate->y);
}

bool ctb::CTBCompactTileSerializer::serializeTile(
  const ctb::TerrainTile *tile) {
  GzipMemoryOutputStream output;
  tile->writeFile(output);
  return mwriter.writeTile(tile->zoom, tile->x, tile->y, output.finish());
}

bool ctb::CTBCompactTileSerializer::serializeTile(
  const ctb::MeshTile *tile, bool writeVertexNormals) {
  GzipMemoryOutputStream output;
  tile->writeFile(output, writeVertexNormals);
  return mwriter.writeTile(tile->zoom, tile->x, tile->y, output.finish());
}

void ctb::CTBCompactTileSerializer::endSerialization() {}

bool ctb::CTBCompactTileSerializer::hasTile(
  const ctb::TileCoordinate *coordinate) const {
  return mwriter.hasTile(coordinate->zoom, coordinate->x, coordinate->y);
}
