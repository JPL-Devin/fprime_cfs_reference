####
# default/config/CMakeLists.txt:
#
# Sets a list of source files for cmake to process as part of autocoding.
####
register_fprime_config(
    CONFIGURATION_OVERRIDES
        "${CMAKE_CURRENT_LIST_DIR}/ComCfg.fpp"
        "${CMAKE_CURRENT_LIST_DIR}/ComCcsdsRouterConfig.fpp"
        "${CMAKE_CURRENT_LIST_DIR}/CfsRouterCfg.fpp"
    INTERFACE
    DEPENDS
        FPrimeCfs_CfsBridge_Types
        FPrimeCfs_CfsRouter
)
