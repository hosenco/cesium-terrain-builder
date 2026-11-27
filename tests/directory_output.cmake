file(REMOVE_RECURSE "${TEST_WORK}")
file(MAKE_DIRECTORY "${TEST_WORK}/output")
file(WRITE "${TEST_WORK}/pixel.raw" "A")
file(WRITE "${TEST_WORK}/input.vrt" [=[
<VRTDataset rasterXSize="1" rasterYSize="1">
  <SRS>EPSG:4326</SRS>
  <GeoTransform>-100, 1, 0, -9, 0, -1</GeoTransform>
  <VRTRasterBand dataType="Byte" band="1" subClass="VRTRawRasterBand">
    <SourceFilename relativeToVRT="1">pixel.raw</SourceFilename>
    <ImageOffset>0</ImageOffset>
    <PixelOffset>1</PixelOffset>
    <LineOffset>1</LineOffset>
    <ByteOrder>LSB</ByteOrder>
  </VRTRasterBand>
</VRTDataset>
]=])

set(ENV{PATH} "${CTB_LIBRARY_DIR};${GDAL_BIN_DIR};$ENV{PATH}")
set(ENV{GDAL_DATA} "${GDAL_DATA_DIR}")
set(ENV{PROJ_LIB} "${PROJ_DATA_DIR}")

execute_process(
  COMMAND "${CTB_TILE}"
    --thread-count 1
    --start-zoom 0
    --end-zoom 0
    --output-dir "${TEST_WORK}/output"
    "${TEST_WORK}/input.vrt"
  RESULT_VARIABLE result
  OUTPUT_VARIABLE stdout
  ERROR_VARIABLE stderr
)

if(NOT result EQUAL 0)
  message(FATAL_ERROR "ctb-tile failed (${result})\nstdout:\n${stdout}\nstderr:\n${stderr}")
endif()

set(tile_path "${TEST_WORK}/output/0/0/0.terrain")
if(NOT EXISTS "${tile_path}")
  message(FATAL_ERROR "Expected default Terrain output at ${tile_path}")
endif()

file(READ "${tile_path}" tile_hex HEX LIMIT 2)
if(NOT tile_hex STREQUAL "1f8b")
  message(FATAL_ERROR "Expected gzip-compressed terrain payload, got prefix ${tile_hex}")
endif()
