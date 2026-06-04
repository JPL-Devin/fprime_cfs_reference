cmake_minimum_required(VERSION 3.22)
project(CFE_FPRIME_APP C CXX)
register_fprime_project()

add_compile_options(-fPIC)


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
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/fprime_config/config")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/FPrimeDeployment")

# Add table
add_cfe_tables(fprime_app fsw/tables/fprime_app_tbl.c)

# If UT is enabled, then add the tests from the subdirectory
# Note that this is an app, and therefore does not provide
# stub functions, as other entities would not typically make
# direct function calls into this application.
if (ENABLE_UNIT_TESTS)
  add_subdirectory(unit-test)
endif (ENABLE_UNIT_TESTS)
