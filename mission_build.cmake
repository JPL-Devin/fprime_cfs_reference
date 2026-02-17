###########################################################
#
# FPRIME_APP mission build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

# The list of header files that control the FPRIME_APP configuration
set(FPRIME_APP_MISSION_CONFIG_FILE_LIST
  fprime_app_fcncode_values.h
  fprime_app_interface_cfg_values.h
  fprime_app_mission_cfg.h
  fprime_app_perfids.h
  fprime_app_msg.h
  fprime_app_msgdefs.h
  fprime_app_msgstruct.h
  fprime_app_tbl.h
  fprime_app_tbldefs.h
  fprime_app_tblstruct.h
  fprime_app_topicid_values.h
)

generate_configfile_set(${FPRIME_APP_MISSION_CONFIG_FILE_LIST})

