#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "minifb::minifb" for configuration ""
set_property(TARGET minifb::minifb APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(minifb::minifb PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "C;CXX"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libminifb.a"
  )

list(APPEND _cmake_import_check_targets minifb::minifb )
list(APPEND _cmake_import_check_files_for_minifb::minifb "${_IMPORT_PREFIX}/lib/libminifb.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
