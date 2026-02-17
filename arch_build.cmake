###########################################################
#
# FPRIME_REFERENCE_APP platform build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

# The list of header files that control the FPRIME_REFERENCE_APP configuration
set(FPRIME_REFERENCE_APP_PLATFORM_CONFIG_FILE_LIST
  fprime_reference_app_internal_cfg_values.h
  fprime_reference_app_platform_cfg.h
  fprime_reference_app_perfids.h
  fprime_reference_app_msgids.h
  fprime_reference_app_msgid_values.h
)

generate_configfile_set(${FPRIME_REFERENCE_APP_PLATFORM_CONFIG_FILE_LIST})

