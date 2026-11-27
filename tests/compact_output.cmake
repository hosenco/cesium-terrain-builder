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
    --output-storage compact
    --output-dir "${TEST_WORK}/output"
    "${TEST_WORK}/input.vrt"
  RESULT_VARIABLE result
  OUTPUT_VARIABLE stdout
  ERROR_VARIABLE stderr
)

if(NOT result EQUAL 0)
  message(FATAL_ERROR "ctb-tile failed (${result})\nstdout:\n${stdout}\nstderr:\n${stderr}")
endif()

if(NOT EXISTS "${TEST_WORK}/output/L00/R0000C0000.bundle")
  message(FATAL_ERROR "Expected a Compact Cache V2 bundle")
endif()
if(EXISTS "${TEST_WORK}/output/0/0/0.terrain")
  message(FATAL_ERROR "Compact output must not create loose terrain files")
endif()
if(NOT EXISTS "${TEST_WORK}/output/layer.json")
  message(FATAL_ERROR "Compact output should create layer.json")
endif()

set(bundle "${TEST_WORK}/output/L00/R0000C0000.bundle")
file(SIZE "${bundle}" initial_size)
file(WRITE "${TEST_WORK}/pixel.raw" "B")

execute_process(
  COMMAND "${CTB_TILE}"
    --thread-count 1 --start-zoom 0 --end-zoom 0 --resume
    --output-storage compact --output-dir "${TEST_WORK}/output"
    "${TEST_WORK}/input.vrt"
  RESULT_VARIABLE resume_result
)
if(NOT resume_result EQUAL 0)
  message(FATAL_ERROR "Compact resume run failed")
endif()
file(SIZE "${bundle}" resumed_size)
if(NOT resumed_size EQUAL initial_size)
  message(FATAL_ERROR "Compact resume should skip an indexed tile")
endif()

execute_process(
  COMMAND "${CTB_TILE}"
    --thread-count 1 --start-zoom 0 --end-zoom 0
    --output-storage compact --output-dir "${TEST_WORK}/output"
    "${TEST_WORK}/input.vrt"
  RESULT_VARIABLE replace_result
)
if(NOT replace_result EQUAL 0)
  message(FATAL_ERROR "Compact replacement run failed")
endif()
file(SIZE "${bundle}" replacement_size)
if(NOT replacement_size GREATER initial_size)
  message(FATAL_ERROR "Compact replacement should append a new tile record")
endif()

file(MAKE_DIRECTORY "${TEST_WORK}/mesh")
execute_process(
  COMMAND "${CTB_TILE}"
    --thread-count 1 --start-zoom 0 --end-zoom 0
    --output-format Mesh --output-storage compact
    --output-dir "${TEST_WORK}/mesh" "${TEST_WORK}/input.vrt"
  RESULT_VARIABLE mesh_result
  OUTPUT_VARIABLE mesh_stdout
  ERROR_VARIABLE mesh_stderr
)
if(NOT mesh_result EQUAL 0)
  message(FATAL_ERROR "Compact Mesh output failed\n${mesh_stdout}\n${mesh_stderr}")
endif()
if(NOT EXISTS "${TEST_WORK}/mesh/L00/R0000C0000.bundle")
  message(FATAL_ERROR "Compact Mesh output should create a bundle")
endif()

execute_process(
  COMMAND "${CTB_TILE}"
    --output-format GTiff --output-storage compact
    --output-dir "${TEST_WORK}/invalid" "${TEST_WORK}/input.vrt"
  RESULT_VARIABLE invalid_result
  ERROR_VARIABLE invalid_stderr
)
if(invalid_result EQUAL 0)
  message(FATAL_ERROR "Compact storage must reject GDAL output formats")
endif()

file(MAKE_DIRECTORY "${TEST_WORK}/friendly")
execute_process(
  COMMAND "${CTB_TILE}"
    --thread-count 1 --start-zoom 0 --end-zoom 0
    --cesium-friendly --output-storage compact
    --output-dir "${TEST_WORK}/friendly" "${TEST_WORK}/input.vrt"
  RESULT_VARIABLE friendly_result
  OUTPUT_VARIABLE friendly_stdout
  ERROR_VARIABLE friendly_stderr
)
if(NOT friendly_result EQUAL 0)
  message(FATAL_ERROR "Compact Cesium-friendly run failed\n${friendly_stdout}\n${friendly_stderr}")
endif()
foreach(root_x 0 1)
  execute_process(
    COMMAND "${COMPACT_PROBE}" "${TEST_WORK}/friendly" --has 0 "${root_x}" 0
    RESULT_VARIABLE root_result
  )
  if(NOT root_result EQUAL 0)
    message(FATAL_ERROR "Compact Cesium-friendly root tile x=${root_x} is missing")
  endif()
endforeach()
