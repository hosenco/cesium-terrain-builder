#ifndef CTBCOMPACTTILESERIALIZER_HPP
#define CTBCOMPACTTILESERIALIZER_HPP

#include <string>

#include "EsriCompactCacheV2Writer.hpp"
#include "MeshSerializer.hpp"
#include "TerrainSerializer.hpp"

namespace ctb {
  class CTBCompactTileSerializer;
}

class CTB_DLL ctb::CTBCompactTileSerializer :
  public ctb::TerrainSerializer,
  public ctb::MeshSerializer {
public:
  CTBCompactTileSerializer(const std::string &outputDir, bool resume);

  virtual void startSerialization();
  virtual bool mustSerializeCoordinate(const ctb::TileCoordinate *coordinate);
  virtual bool serializeTile(const ctb::TerrainTile *tile);
  virtual bool serializeTile(const ctb::MeshTile *tile,
                             bool writeVertexNormals = false);
  virtual void endSerialization();
  bool hasTile(const ctb::TileCoordinate *coordinate) const;

private:
  EsriCompactCacheV2Writer mwriter;
  bool mresume;
};

#endif
