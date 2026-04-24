// ======================================================================
// \title  Main.cpp
// \brief main program for the F' application. Intended for CLI-based systems (Linux, macOS)
//
// ======================================================================
// Used to access topology functions
#include <FPrimeDeployment/Top/FPrimeDeploymentTopology.hpp>
#include <FPrimeDeployment/Top/FPrimeDeploymentTopologyAc.hpp>
// OSAL initialization
#include <Os/Os.hpp>
// Used for command line argument processing
#include <getopt.h>
// Used for printf functions
#include <cstdlib>


extern "C" {
    #include "cfe.h"
    #include "cfe_config.h"
    #include "fprime_app_version.h"
    #include "fprime_app_internal_cfg.h"
    void FPRIME_APP_Main(void);
}

CFE_SB_PipeId_t CommandPipe;
CFE_Status_t FPRIME_APP_Init(FPrimeApp::TopologyState& inputs);

// Topology state stored at file scope so the delete callback can always reach it.
static FPrimeApp::TopologyState g_topologyState;

static void FPRIME_APP_StopAndTeardown(FPrimeApp::TopologyState& inputs)
{
    FPrimeApp::timer.stop();
    FPrimeApp::teardownTopology(inputs);
}

static void FPRIME_APP_Shutdown(FPrimeApp::TopologyState& inputs, uint32 status)
{
    printf("F Prime App: exiting run loop, tearing down topology\n");
    FPRIME_APP_StopAndTeardown(inputs);
    CFE_ES_ExitApp(status);
}

void FPRIME_APP_delete_callback(void)
{
    printf("F Prime App: delete callback -- stopping timer and tearing down topology\n");
    FPRIME_APP_StopAndTeardown(g_topologyState);
    printf("F Prime App: delete callback -- exiting application\n");
}


void FPRIME_APP_Main(void) {
    printf("FPrime ALL YOUR BASE ARE BELONG TO US\n");
    Os::init();

    printf("FPrime ALL YOUR BASE ARE BELONG TO US\n");
    FPrimeApp::TopologyState& inputs = g_topologyState;
    uint32 run_status = CFE_ES_RunStatus_APP_RUN;
    
    CFE_Status_t status = FPRIME_APP_Init(inputs);
    if (status != CFE_SUCCESS) {
        CFE_EVS_SendEvent(2, CFE_EVS_EventType_ERROR, "F Prime App: Error creating SB Command Pipe, RC = 0x%08lX", (unsigned long)status);
        FPRIME_APP_Shutdown(inputs, status);
    }
  
    FPrimeApp::timer.startTimer(Fw::TimeInterval(1, 0));
    while (CFE_ES_RunLoop(&run_status) == true)
    {
        FPrimeApp::timer.cycle();
        FPrimeApp::cfsBridge.process();
    }
    FPRIME_APP_Shutdown(inputs, run_status);
}

CFE_Status_t FPRIME_APP_Init(FPrimeApp::TopologyState& inputs)
{
    printf("Initializing FPrime App...\n");
    CFE_Status_t status;
    char         VersionString[FPRIME_APP_CFG_MAX_VERSION_STR_LEN];

    /*
    ** Register the events
    */
    status = CFE_EVS_Register(NULL, 0, CFE_EVS_EventFilter_BINARY);
    if (status != CFE_SUCCESS)
    {
        printf("F Prime App: Error Registering Events, RC = 0x%08lX\n", (unsigned long)status);
        CFE_ES_WriteToSysLog("F Prime App: Error Registering Events, RC = 0x%08lX\n", (unsigned long)status);
    }
    else
    {
        status = FPrimeApp::cfsBridge.configure(FPRIME_APP_PLATFORM_PIPE_DEPTH, FPRIME_APP_PLATFORM_PIPE_NAME);
        if (status != CFE_SUCCESS)
        {
            printf("F Prime App: Error creating SB Command Pipe, RC = 0x%08lX\n", (unsigned long)status);
            CFE_EVS_SendEvent(2, CFE_EVS_EventType_ERROR,
                              "F Prime App: Error creating SB Command Pipe, RC = 0x%08lX", (unsigned long)status);
        }
    }
    if (status == CFE_SUCCESS)
    {
        printf("Subscribing to cFS messages...\n");
        FPrimeApp::cfsBridge.subscribe(ComCfg::Apid::FW_PACKET_COMMAND);
    }
    printf("Setting up the topology, yo!\n");
    OS_TaskInstallDeleteHandler(&FPRIME_APP_delete_callback);
    FPrimeApp::setupTopology(inputs);

    if (status == CFE_SUCCESS)
    {
        CFE_Config_GetVersionString(VersionString, FPRIME_APP_CFG_MAX_VERSION_STR_LEN, "F Prime App", FPRIME_APP_VERSION,
                                    FPRIME_APP_BUILD_CODENAME, FPRIME_APP_LAST_OFFICIAL);

        printf("F Prime App Initialized. Version: %s\n", VersionString);
        CFE_EVS_SendEvent(1, CFE_EVS_EventType_INFORMATION, "F Prime App Initialized.%s",
                          VersionString);
    }

    return status;
}
