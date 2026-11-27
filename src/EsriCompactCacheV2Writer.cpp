#include "EsriCompactCacheV2Writer.hpp"

#include <cstdio>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>

#include "cpl_vsi.h"
#include "CTBException.hpp"

namespace {
  static const unsigned int kBundleSize = 128;
  static const unsigned int kTileCount = kBundleSize * kBundleSize;
  static const unsigned int kHeaderSize = 64;
  static const unsigned int kIndexSize = kTileCount * 8;
  static const unsigned long long kDataOffset = kHeaderSize + kIndexSize;
  static std::mutex gBundleMapMutex;
  static std::map<std::string, std::shared_ptr<std::mutex> > gBundleMutexes;

  static std::shared_ptr<std::mutex> mutexForBundle(const std::string &path) {
    std::lock_guard<std::mutex> lock(gBundleMapMutex);
    std::map<std::string, std::shared_ptr<std::mutex> >::iterator found =
      gBundleMutexes.find(path);
    if (found != gBundleMutexes.end())
      return found->second;
    std::shared_ptr<std::mutex> mutex(new std::mutex());
    gBundleMutexes[path] = mutex;
    return mutex;
  }

  static std::string bundlePath(const std::string &root, unsigned int zoom,
                                unsigned int x, unsigned int y) {
    std::ostringstream path;
    path << root << "/L" << (zoom < 10 ? "0" : "") << zoom
         << "/R";
    path.fill('0');
    path.width(4);
    path << std::hex << (y / kBundleSize) * kBundleSize << "C";
    path.width(4);
    path << (x / kBundleSize) * kBundleSize << ".bundle";
    return path.str();
  }

  static void seek(VSILFILE *file, vsi_l_offset offset) {
    if (VSIFSeekL(file, offset, SEEK_SET) != 0)
      throw ctb::CTBException("Could not seek Compact Cache bundle");
  }

  static void writeBytes(VSILFILE *file, const void *data, size_t size) {
    if (VSIFWriteL(data, 1, size, file) != size)
      throw ctb::CTBException("Could not write Compact Cache bundle");
  }

  static void writeLE32(VSILFILE *file, unsigned int value) {
    unsigned char bytes[4] = {
      static_cast<unsigned char>(value),
      static_cast<unsigned char>(value >> 8),
      static_cast<unsigned char>(value >> 16),
      static_cast<unsigned char>(value >> 24)
    };
    writeBytes(file, bytes, sizeof(bytes));
  }

  static void writeLE64(VSILFILE *file, unsigned long long value) {
    unsigned char bytes[8];
    for (size_t i = 0; i < sizeof(bytes); ++i)
      bytes[i] = static_cast<unsigned char>(value >> (i * 8));
    writeBytes(file, bytes, sizeof(bytes));
  }

  static unsigned long long readLE64(VSILFILE *file) {
    unsigned char bytes[8];
    if (VSIFReadL(bytes, 1, sizeof(bytes), file) != sizeof(bytes))
      throw ctb::CTBException("Could not read Compact Cache index");
    unsigned long long value = 0;
    for (size_t i = 0; i < sizeof(bytes); ++i)
      value |= static_cast<unsigned long long>(bytes[i]) << (i * 8);
    return value;
  }

  static void initializeBundle(VSILFILE *file) {
    seek(file, 0);
    writeLE32(file, 3);
    writeLE32(file, kTileCount);
    writeLE32(file, 0);
    writeLE32(file, 5);
    writeLE64(file, 0);
    writeLE64(file, kDataOffset);
    writeLE64(file, 40);
    writeLE32(file, 20 + kIndexSize);
    writeLE32(file, 3);
    writeLE32(file, 16);
    writeLE32(file, kTileCount);
    writeLE32(file, 5);
    writeLE32(file, kIndexSize);

    unsigned char emptyIndex[4096] = {0};
    for (size_t remaining = kIndexSize; remaining > 0;) {
      size_t count = remaining < sizeof(emptyIndex) ? remaining : sizeof(emptyIndex);
      writeBytes(file, emptyIndex, count);
      remaining -= count;
    }
  }

  static std::string indexDirectory(const std::string &path) {
    const std::string::size_type slash = path.find_last_of("/\\");
    return slash == std::string::npos ? std::string() : path.substr(0, slash);
  }

  static unsigned long long indexOffset(unsigned int x, unsigned int y) {
    return kHeaderSize +
      static_cast<unsigned long long>((y % kBundleSize) * kBundleSize +
                                      (x % kBundleSize)) * 8;
  }
}

ctb::EsriCompactCacheV2Writer::EsriCompactCacheV2Writer(
  const std::string &outputDir): moutputDir(outputDir) {}

bool ctb::EsriCompactCacheV2Writer::hasTile(
  i_zoom zoom, i_tile x, i_tile y) const {
  const std::string path = bundlePath(moutputDir, zoom, x, y);
  std::lock_guard<std::mutex> lock(*mutexForBundle(path));
  VSILFILE *file = VSIFOpenL(path.c_str(), "rb");
  if (file == NULL)
    return false;

  try {
    seek(file, indexOffset(x, y));
    const unsigned long long entry = readLE64(file);
    VSIFCloseL(file);
    return (entry >> 40) != 0;
  } catch (...) {
    VSIFCloseL(file);
    throw;
  }
}

std::vector<unsigned char> ctb::EsriCompactCacheV2Writer::readTile(
  i_zoom zoom, i_tile x, i_tile y) const {
  const std::string path = bundlePath(moutputDir, zoom, x, y);
  std::lock_guard<std::mutex> lock(*mutexForBundle(path));
  VSILFILE *file = VSIFOpenL(path.c_str(), "rb");
  if (file == NULL)
    return std::vector<unsigned char>();

  try {
    seek(file, indexOffset(x, y));
    const unsigned long long entry = readLE64(file);
    const unsigned int size = static_cast<unsigned int>(entry >> 40);
    if (size == 0) {
      VSIFCloseL(file);
      return std::vector<unsigned char>();
    }

    const unsigned long long offsetMask = (1ULL << 40) - 1;
    seek(file, entry & offsetMask);
    std::vector<unsigned char> payload(size);
    if (VSIFReadL(payload.data(), 1, payload.size(), file) != payload.size())
      throw ctb::CTBException("Could not read Compact Cache payload");
    VSIFCloseL(file);
    return payload;
  } catch (...) {
    VSIFCloseL(file);
    throw;
  }
}

bool ctb::EsriCompactCacheV2Writer::writeTile(
  i_zoom zoom, i_tile x, i_tile y,
  const std::vector<unsigned char> &payload) {
  if (payload.empty())
    throw ctb::CTBException("Cannot store an empty Compact Cache tile");
  if (payload.size() > 0xffffff)
    throw ctb::CTBException("Compact Cache tile exceeds the 24-bit size limit");

  const std::string path = bundlePath(moutputDir, zoom, x, y);
  std::lock_guard<std::mutex> lock(*mutexForBundle(path));
  const std::string directory = indexDirectory(path);
  if (VSIMkdirRecursive(directory.c_str(), 0755) != 0) {
    VSIStatBufL stat;
    if (VSIStatExL(directory.c_str(), &stat, VSI_STAT_EXISTS_FLAG) != 0 ||
        !VSI_ISDIR(stat.st_mode))
      throw ctb::CTBException("Could not create Compact Cache directory");
  }

  VSIStatBufL stat;
  const bool exists = VSIStatExL(path.c_str(), &stat, VSI_STAT_EXISTS_FLAG) == 0;
  VSILFILE *file = VSIFOpenL(path.c_str(), exists ? "r+b" : "w+b");
  if (file == NULL)
    throw ctb::CTBException("Could not open Compact Cache bundle");

  try {
    unsigned long long end;
    if (!exists) {
      initializeBundle(file);
      end = kDataOffset;
    } else {
      if (VSIFSeekL(file, 0, SEEK_END) != 0)
        throw ctb::CTBException("Could not seek Compact Cache bundle");
      end = VSIFTellL(file);
    }
    if (end >= (1ULL << 40) - 1)
      throw ctb::CTBException("Compact Cache bundle exceeds the 40-bit offset limit");

    seek(file, end);
    writeLE32(file, static_cast<unsigned int>(payload.size()));
    const unsigned long long payloadOffset = VSIFTellL(file);
    writeBytes(file, payload.data(), payload.size());
    const unsigned long long newEnd = payloadOffset + payload.size();

    seek(file, indexOffset(x, y));
    writeLE64(file, payloadOffset |
      (static_cast<unsigned long long>(payload.size()) << 40));

    seek(file, 8);
    unsigned char oldMaxBytes[4];
    if (VSIFReadL(oldMaxBytes, 1, sizeof(oldMaxBytes), file) != sizeof(oldMaxBytes))
      throw ctb::CTBException("Could not read Compact Cache header");
    const unsigned int oldMax = oldMaxBytes[0] |
      (static_cast<unsigned int>(oldMaxBytes[1]) << 8) |
      (static_cast<unsigned int>(oldMaxBytes[2]) << 16) |
      (static_cast<unsigned int>(oldMaxBytes[3]) << 24);
    if (payload.size() > oldMax) {
      seek(file, 8);
      writeLE32(file, static_cast<unsigned int>(payload.size()));
    }
    seek(file, 24);
    writeLE64(file, newEnd);

    if (VSIFCloseL(file) != 0)
      throw ctb::CTBException("Could not close Compact Cache bundle");
    return true;
  } catch (...) {
    VSIFCloseL(file);
    throw;
  }
}
