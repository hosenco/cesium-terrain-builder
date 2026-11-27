#ifndef ESRICOMPACTCACHEV2WRITER_HPP
#define ESRICOMPACTCACHEV2WRITER_HPP

#include <string>
#include <vector>

#include "config.hpp"
#include "types.hpp"

namespace ctb {
  class EsriCompactCacheV2Writer;
}

class CTB_DLL ctb::EsriCompactCacheV2Writer {
public:
  explicit EsriCompactCacheV2Writer(const std::string &outputDir);

  bool hasTile(i_zoom zoom, i_tile x, i_tile y) const;
  std::vector<unsigned char> readTile(i_zoom zoom, i_tile x, i_tile y) const;
  bool writeTile(i_zoom zoom, i_tile x, i_tile y,
                 const std::vector<unsigned char> &payload);

private:
  std::string moutputDir;
};

#endif
