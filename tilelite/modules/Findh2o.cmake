find_library(H2O_LIBRARY h2o-evloop)
set(H2O_LIBRARY_NEEDED H2O_LIBRARY)

find_path(H2O_INCLUDE_DIR
    NAMES h2o.h
    PATH_SUFFIXES include)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args("H2O" DEFAULT_MSG
  ${H2O_LIBRARY_NEEDED}
  H2O_INCLUDE_DIR
)
