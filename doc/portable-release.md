# Windows portable release follow-up

## Goal

Produce a Windows x64 release directory that can be copied to another machine
and used by invoking the executable directly, without an OSGeo4W installation
and without user-configured environment variables.

## Proposed package layout

```text
ctb/
  bin/
    ctb-tile.exe
    ctb-info.exe
    ctb-export.exe
    ctb-extents.exe
    ctb.dll
    <required GDAL, PROJ, and transitive runtime DLLs>
    <GDAL driver plugins, if required>
  data/
    gdal/
      <GDAL data files>
    proj/
      proj.db
      <other PROJ data files>
```

## Required work

1. Before `GDALAllRegister()`, derive the executable directory and set
   process-local `PATH`, `GDAL_DATA`, and `PROJ_LIB` to packaged locations.
2. Collect DLL dependencies recursively, not only direct imports. Include GDAL
   driver plugins and the required MSVC runtime.
3. Add a CMake packaging target that stages the executables, DLLs, and data
   directories into the portable layout.
4. Validate the package on a clean Windows x64 machine without OSGeo4W:
   executable startup/help, coordinate transformation, Terrain output,
   Compact output, and representative GDAL drivers.

## Constraints

The current Release build links to `gdal301.dll` from OSGeo4W64. The package
must not mix this runtime with the separate GDAL 3.13 installation under
`C:\OSGeo4W`.

Full static linking is not the preferred solution: GDAL and PROJ still have
numerous transitive dependencies, plugins, and resource files. A compatible
dynamic-runtime bundle is the smallest reliable approach.
