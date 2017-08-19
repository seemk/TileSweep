# Exports:
# MAPNIK_LIBRARY
# MAPNIK_INCLUDE_DIR

find_library(MAPNIK_LIBRARY
  NAMES mapnik
)

set(MAPNIK_LIBRARY_NEEDED MAPNIK_LIBRARY)

find_path(MAPNIK_INCLUDE_DIR
  NAMES map.hpp
  PATH_SUFFIXES mapnik
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args("mapnik" DEFAULT_MSG
  ${MAPNIK_LIBRARY_NEEDED}
  MAPNIK_INCLUDE_DIR
)
