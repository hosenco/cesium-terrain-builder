get_filename_component(GDAL_BIN_DIR "${GDAL_LIBRARY}" DIRECTORY)
get_filename_component(OSGEO_ROOT "${GDAL_BIN_DIR}" DIRECTORY)

add_test(
  NAME ctb-tile-default-directory-output
  COMMAND "${CMAKE_COMMAND}"
    "-DCTB_TILE=$<TARGET_FILE:ctb-tile>"
    "-DCOMPACT_PROBE=$<TARGET_FILE:compact_writer_test>"
    "-DCTB_LIBRARY_DIR=$<TARGET_FILE_DIR:ctb>"
    "-DGDAL_BIN_DIR=${OSGEO_ROOT}/bin"
    "-DGDAL_DATA_DIR=${OSGEO_ROOT}/share/gdal"
    "-DPROJ_DATA_DIR=${OSGEO_ROOT}/share/proj"
    "-DTEST_WORK=${CMAKE_CURRENT_BINARY_DIR}/default-directory-output"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/directory_output.cmake"
)

add_executable(compact_writer_test compact_writer_test.cpp)
target_link_libraries(compact_writer_test ctb ${ZLIB_LIBRARIES})
add_test(NAME esri-compact-writer-single-tile COMMAND compact_writer_test "${CMAKE_CURRENT_BINARY_DIR}/compact-writer")
set_tests_properties(esri-compact-writer-single-tile PROPERTIES
  ENVIRONMENT "PATH=$<TARGET_FILE_DIR:ctb>\\;${OSGEO_ROOT}/bin\\;$ENV{PATH}")

add_executable(compact_serializer_test compact_serializer_test.cpp)
target_link_libraries(compact_serializer_test ctb ${ZLIB_LIBRARIES})
add_test(NAME compact-terrain-serializer COMMAND compact_serializer_test "${CMAKE_CURRENT_BINARY_DIR}/compact-serializer")
set_tests_properties(compact-terrain-serializer PROPERTIES
  ENVIRONMENT "PATH=$<TARGET_FILE_DIR:ctb>\\;${OSGEO_ROOT}/bin\\;$ENV{PATH}")

add_test(
  NAME ctb-tile-compact-output
  COMMAND "${CMAKE_COMMAND}"
    "-DCTB_TILE=$<TARGET_FILE:ctb-tile>"
    "-DCOMPACT_PROBE=$<TARGET_FILE:compact_writer_test>"
    "-DCTB_LIBRARY_DIR=$<TARGET_FILE_DIR:ctb>"
    "-DGDAL_BIN_DIR=${OSGEO_ROOT}/bin"
    "-DGDAL_DATA_DIR=${OSGEO_ROOT}/share/gdal"
    "-DPROJ_DATA_DIR=${OSGEO_ROOT}/share/proj"
    "-DTEST_WORK=${CMAKE_CURRENT_BINARY_DIR}/compact-output"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/compact_output.cmake"
)

set(CMSS_ROOT "" CACHE PATH "Path to the CMSS checkout for Compact Cache interoperability tests")
if(EXISTS "${CMSS_ROOT}/mapproxy/cache/compact.py" AND
   EXISTS "${CMSS_ROOT}/plugins/mapproxy-terrain/mapproxy_terrain/storage.py")
  add_test(
    NAME ctb-compact-cmss-interoperability
    COMMAND "${CMAKE_COMMAND}"
      "-DCTB_TILE=$<TARGET_FILE:ctb-tile>"
      "-DCTB_LIBRARY_DIR=$<TARGET_FILE_DIR:ctb>"
      "-DGDAL_BIN_DIR=${OSGEO_ROOT}/bin"
      "-DGDAL_DATA_DIR=${OSGEO_ROOT}/share/gdal"
      "-DPROJ_DATA_DIR=${OSGEO_ROOT}/share/proj"
      "-DCMSS_ROOT=${CMSS_ROOT}"
      "-DPYTHON=${CMSS_ROOT}/.venv/Scripts/python.exe"
      "-DTEST_WORK=${CMAKE_CURRENT_BINARY_DIR}/cmss-interoperability"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/cmss_interoperability.cmake"
  )
endif()
