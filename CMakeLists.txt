cmake_minimum_required(VERSION 3.22)
project(CFE_FPRIME_APP C CXX)

add_compile_options(-fPIC)
set(FPRIME_INCLUDE_FRAMEWORK_CODE ON)
find_package(FPrime REQUIRED PATHS "${CMAKE_CURRENT_LIST_DIR}/lib/fprime")


set(APP_SRC_FILES
  fsw/src/fprime_app.c
  fsw/src/fprime_app_cmds.c
  fsw/src/fprime_app_utils.c
)

if (CFE_EDS_ENABLED)
  list(APPEND APP_SRC_FILES
    fsw/src/fprime_app_eds_dispatch.c
  )
else()
  list(APPEND APP_SRC_FILES
    fsw/src/fprime_app_dispatch.c
  )
endif()


# Add table
add_cfe_tables(fprime_app fsw/tables/fprime_app_tbl.c)

# If UT is enabled, then add the tests from the subdirectory
# Note that this is an app, and therefore does not provide
# stub functions, as other entities would not typically make
# direct function calls into this application.
if (ENABLE_UNIT_TESTS)
  add_subdirectory(unit-test)
endif (ENABLE_UNIT_TESTS)
