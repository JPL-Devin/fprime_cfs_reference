###########################################################
#
# FPRIME_REFERENCE_APP mission build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

# The list of header files that control the FPRIME_REFERENCE_APP configuration
set(FPRIME_REFERENCE_APP_MISSION_CONFIG_FILE_LIST
  fprime_reference_app_fcncode_values.h
  fprime_reference_app_interface_cfg_values.h
  fprime_reference_app_mission_cfg.h
  fprime_reference_app_perfids.h
  fprime_reference_app_msg.h
  fprime_reference_app_msgdefs.h
  fprime_reference_app_msgstruct.h
  fprime_reference_app_tbl.h
  fprime_reference_app_tbldefs.h
  fprime_reference_app_tblstruct.h
  fprime_reference_app_topicid_values.h
)

generate_configfile_set(${FPRIME_REFERENCE_APP_MISSION_CONFIG_FILE_LIST})

