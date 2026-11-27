#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "cpl_vsi.h"
#include "CTBException.hpp"
#include "EsriCompactCacheV2Writer.hpp"

static void require(bool condition, const char *message) {
  if (!condition) {
    std::cerr << message << std::endl;
    std::exit(1);
  }
}

int main(int argc, char **argv) {
  if (argc == 6 && std::string(argv[2]) == "--has") {
    ctb::EsriCompactCacheV2Writer writer(argv[1]);
    return writer.hasTile(
      static_cast<unsigned short>(std::atoi(argv[3])),
      static_cast<unsigned int>(std::atoi(argv[4])),
      static_cast<unsigned int>(std::atoi(argv[5]))) ? 0 : 1;
  }
  require(argc == 2, "Expected a temporary output directory");
  VSIRmdirRecursive(argv[1]);

  ctb::EsriCompactCacheV2Writer writer(argv[1]);
  const std::vector<unsigned char> expected = {'t', 'e', 'r', 'r', 'a', 'i', 'n'};

  writer.writeTile(10, 130, 200, expected);

  require(writer.hasTile(10, 130, 200), "Written coordinate should be present");
  require(!writer.hasTile(10, 131, 200), "Missing coordinate should be absent");
  require(writer.readTile(10, 130, 200) == expected, "Read payload should match written bytes");

  const std::vector<unsigned char> replacement = {'n', 'e', 'w'};
  require(writer.writeTile(10, 130, 200, replacement), "Replacing a tile should succeed");
  require(writer.readTile(10, 130, 200) == replacement,
          "Read should return the newest tile payload");

  ctb::EsriCompactCacheV2Writer reopened(argv[1]);
  require(reopened.hasTile(10, 130, 200),
          "A reopened writer should observe the persisted coordinate");
  require(reopened.readTile(10, 130, 200) == replacement,
          "A reopened writer should read the replacement payload");

  const std::vector<unsigned char> boundary = {'b'};
  writer.writeTile(9, 127, 127, boundary);
  writer.writeTile(9, 128, 128, std::vector<unsigned char>(1, 'c'));
  writer.writeTile(9, 256, 128, std::vector<unsigned char>(1, 'd'));
  require(writer.readTile(9, 127, 127) == boundary,
          "Coordinate 127/127 should remain distinct");
  require(writer.readTile(9, 128, 128) == std::vector<unsigned char>(1, 'c'),
          "Coordinate 128/128 should remain distinct");
  require(writer.readTile(9, 256, 128) == std::vector<unsigned char>(1, 'd'),
          "Non-zero bundle columns should remain distinct");

  std::vector<std::thread> workers;
  for (unsigned int i = 0; i < 16; ++i) {
    workers.push_back(std::thread([&, i]() {
      const std::vector<unsigned char> value(4, static_cast<unsigned char>(i));
      writer.writeTile(12, 300 + i, 400, value);
      writer.writeTile(13, 20 + i, 20, value);
    }));
  }
  for (size_t i = 0; i < workers.size(); ++i)
    workers[i].join();
  for (unsigned int i = 0; i < 16; ++i) {
    const std::vector<unsigned char> value(4, static_cast<unsigned char>(i));
    require(writer.readTile(12, 300 + i, 400) == value,
            "Concurrent same-bundle write was lost or crossed");
    require(writer.readTile(13, 20 + i, 20) == value,
            "Concurrent cross-bundle write was lost");
  }

  bool rejectedEmpty = false;
  try {
    writer.writeTile(1, 0, 0, std::vector<unsigned char>());
  } catch (const ctb::CTBException &) {
    rejectedEmpty = true;
  }
  require(rejectedEmpty, "Empty Compact Cache payload should be rejected");

  bool rejectedOversized = false;
  try {
    writer.writeTile(1, 0, 0, std::vector<unsigned char>(0x1000000));
  } catch (const ctb::CTBException &) {
    rejectedOversized = true;
  }
  require(rejectedOversized,
          "Payloads beyond the 24-bit Compact Cache size field should be rejected");

  return 0;
}
