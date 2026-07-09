###########################################################
#
# FPRIME_APP platform build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

# The list of header files that control the FPRIME_APP configuration
set(FPRIME_APP_PLATFORM_CONFIG_FILE_LIST
  fprime_app_internal_cfg_values.h
  fprime_app_platform_cfg.h
  fprime_app_perfids.h
  fprime_app_msgids.h
  fprime_app_msgid_values.h
)

# Compatibility shims for older cFE versions
include(cfs_compat)

generate_configfile_set(${FPRIME_APP_PLATFORM_CONFIG_FILE_LIST})

