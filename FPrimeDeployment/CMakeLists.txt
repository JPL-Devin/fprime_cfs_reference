#####
# 'FPrimeDeployment' Deployment:
#
# This registers the 'FPrimeDeployment' deployment to the build system. 
# Custom components that have not been added at the project-level should be added to 
# the list below.
#
#####

###
# Topology and Components
###
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Top/")

# Create the app module. This will create the CFS application and build it around the F Prime Main.cpp deployment.
add_cfe_app(fprime_app "${CMAKE_CURRENT_LIST_DIR}/Main.cpp")

# TODO: should the fsw/src and fsw/inc directories be somewhere else?
target_include_directories(fprime_app PUBLIC ../fsw/inc)

target_compile_options(fprime_app PRIVATE
    -Wno-pedantic
)

target_link_libraries(fprime_app ${FPRIME_CURRENT_MODULE}_Top)
add_dependencies(fprime_app ${FPRIME_CURRENT_MODULE}_Top)
# Adds 
#add_cfe_app_dependency(fprime_app  ${FPRIME_CURRENT_MODULE}_Top)
#add_cfe_app_dependency(fprime_app  Fw_Types)

set_target_properties(fprime_app PROPERTIES FPRIME_TYPE Deployment)
fprime_attach_custom_targets(fprime_app)
set(FPRIME_USE_PLAIN_LINK_SIGNATURE ON)
fprime_target_implementations(fprime_app)

