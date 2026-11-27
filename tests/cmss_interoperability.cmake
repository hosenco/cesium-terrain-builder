file(REMOVE_RECURSE "${TEST_WORK}")
file(MAKE_DIRECTORY "${TEST_WORK}/directory")
file(MAKE_DIRECTORY "${TEST_WORK}/compact")
file(WRITE "${TEST_WORK}/pixel.raw" "A")
file(WRITE "${TEST_WORK}/input.vrt" [=[
<VRTDataset rasterXSize="1" rasterYSize="1">
  <SRS>EPSG:4326</SRS>
  <GeoTransform>3, 1, 0, 6, 0, -1</GeoTransform>
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

foreach(storage directory compact)
  set(command_args
    --thread-count 1 --start-zoom 8 --end-zoom 8
    --output-storage "${storage}"
    --output-dir "${TEST_WORK}/${storage}"
    "${TEST_WORK}/input.vrt")
  execute_process(
    COMMAND "${CTB_TILE}" ${command_args}
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
  )
  if(NOT result EQUAL 0)
    message(FATAL_ERROR "${storage} output failed\n${stdout}\n${stderr}")
  endif()
endforeach()

execute_process(
  COMMAND "${PYTHON}"
    "${CMAKE_CURRENT_LIST_DIR}/cmss_read_compact.py"
    "${CMSS_ROOT}" "${TEST_WORK}/directory" "${TEST_WORK}/compact"
  RESULT_VARIABLE python_result
  OUTPUT_VARIABLE python_stdout
  ERROR_VARIABLE python_stderr
)
if(NOT python_result EQUAL 0)
  message(FATAL_ERROR "CMSS failed to read CTB compact output\n${python_stdout}\n${python_stderr}")
endif()
