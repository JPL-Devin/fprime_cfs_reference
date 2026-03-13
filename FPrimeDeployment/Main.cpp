// ======================================================================
// \title  Main.cpp
// \brief main program for the F' application. Intended for CLI-based systems (Linux, macOS)
//
// ======================================================================
// Used to access topology functions
#include <FPrimeDeployment/Top/FPrimeDeploymentTopology.hpp>
// OSAL initialization
#include <Os/Os.hpp>
// Used for signal handling shutdown
#include <signal.h>
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
CFE_Status_t FPRIME_APP_Init();

void FPRIME_APP_TaskPipe(const CFE_SB_Buffer_t *SBBufPtr)
{
    //static CFE_SB_MsgId_t CMD_MID     = CFE_SB_MSGID_RESERVED;
    //static CFE_SB_MsgId_t SEND_HK_MID = CFE_SB_MSGID_RESERVED;

    CFE_SB_MsgId_t MsgId = CFE_SB_INVALID_MSG_ID;

    /* cache the local MID Values here, this avoids repeat lookups */
    //if (!CFE_SB_IsValidMsgId(CMD_MID))
    //{
    //    CMD_MID     = CFE_SB_ValueToMsgId(FPRIME_APP_CMD_MID);
    //    SEND_HK_MID = CFE_SB_ValueToMsgId(FPRIME_APP_SEND_HK_MID);
    //}

    CFE_MSG_GetMsgId(&SBBufPtr->Msg, &MsgId);

    /* Process all SB messages */
    // if (CFE_SB_MsgId_Equal(MsgId, SEND_HK_MID))
    // {
    //     /* Housekeeping request */
    //     FPRIME_APP_SendHkCmd((const FPRIME_APP_SendHkCmd_t *)SBBufPtr);
    // }
    // else if (CFE_SB_MsgId_Equal(MsgId, CMD_MID))
    // {
    //     /* Ground command */
    //     FPRIME_APP_ProcessGroundCommand(SBBufPtr);
    // }
    //else
    {
        /* Unknown command */
        CFE_EVS_SendEvent(4, CFE_EVS_EventType_ERROR, "SAMPLE: invalid command packet,MID = 0x%x",
                          (unsigned int)CFE_SB_MsgIdToValue(MsgId));
    }
}

void FPRIME_APP_Main(void) {
    printf("FPrime ALL YOUR BASE ARE BELONG TO US\n");
    uint32 run_status = CFE_ES_RunStatus_APP_RUN;
    CFE_SB_Buffer_t *SBBufPtr;
    
    // 
    CFE_Status_t status = FPRIME_APP_Init();
    if (status != CFE_SUCCESS) {
        CFE_EVS_SendEvent(2, CFE_EVS_EventType_ERROR, "F Prime App: Error creating SB Command Pipe, RC = 0x%08lX", (unsigned long)status);
    }
    

    while (CFE_ES_RunLoop(&run_status) == true)
    {
        /*
        ** Performance Log Exit Stamp
        */
        //CFE_ES_PerfLogExit(FPRIME_APP_PERF_ID);

        /* Pend on receipt of command packet */
        printf("[FPRIME] Waiting for command...\n");
        status = CFE_SB_ReceiveBuffer(&SBBufPtr, CommandPipe, CFE_SB_PEND_FOREVER);
        printf("[FPRIME] Command received!\n");
        /*
        ** Performance Log Entry Stamp
        */
        //CFE_ES_PerfLogEntry(FPRIME_APP_PERF_ID);

        if (status == CFE_SUCCESS)
        {
            FPRIME_APP_TaskPipe(SBBufPtr);
        }
        else
        {
            CFE_EVS_SendEvent(5, CFE_EVS_EventType_ERROR,
                              "FPRIME APP: SB Pipe Read Error, App Will Exit");

            run_status = CFE_ES_RunStatus_APP_ERROR;
        }
    }

    CFE_ES_ExitApp(run_status);
}

CFE_Status_t FPRIME_APP_Init(void)
{
    printf("Initializing FPrime App...\n");
    CFE_Status_t status;
    char         VersionString[FPRIME_APP_CFG_MAX_VERSION_STR_LEN];

    /* Zero out the global data structure */
    //memset(&FPRIME_APP_Data, 0, sizeof(FPRIME_APP_Data));

    //FPRIME_APP_Data.RunStatus = CFE_ES_RunStatus_APP_RUN;

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
        /*
         ** Initialize housekeeping packet (clear user data area).
         */
        //CFE_MSG_Init(CFE_MSG_PTR(FPRIME_APP_Data.HkTlm.TelemetryHeader), CFE_SB_ValueToMsgId(FPRIME_APP_HK_TLM_MID),
        //             sizeof(FPRIME_APP_Data.HkTlm));

        /*
         ** Create Software Bus message pipe.
         */
        status = CFE_SB_CreatePipe(&CommandPipe, FPRIME_APP_PLATFORM_PIPE_DEPTH,
                                   FPRIME_APP_PLATFORM_PIPE_NAME);
        if (status != CFE_SUCCESS)
        {
            printf("F Prime App: Error creating SB Command Pipe, RC = 0x%08lX\n", (unsigned long)status);
            CFE_EVS_SendEvent(2, CFE_EVS_EventType_ERROR,
                              "F Prime App: Error creating SB Command Pipe, RC = 0x%08lX", (unsigned long)status);
        }
    }

    // if (status == CFE_SUCCESS)
    // {
    //     /*
    //     ** Subscribe to Housekeeping request commands
    //     */
    //     status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(FPRIME_APP_SEND_HK_MID), FPRIME_APP_Data.CommandPipe);
    //     if (status != CFE_SUCCESS)
    //     {
    //         CFE_EVS_SendEvent(FPRIME_APP_SUB_HK_ERR_EID, CFE_EVS_EventType_ERROR,
    //                           "F Prime App: Error Subscribing to HK request, RC = 0x%08lX", (unsigned long)status);
    //     }
    // }

    // if (status == CFE_SUCCESS)
    // {
    //     /*
    //     ** Subscribe to ground command packets
    //     */
    //     status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(FPRIME_APP_CMD_MID), FPRIME_APP_Data.CommandPipe);
    //     if (status != CFE_SUCCESS)
    //     {
    //         CFE_EVS_SendEvent(FPRIME_APP_SUB_CMD_ERR_EID, CFE_EVS_EventType_ERROR,
    //                           "F Prime App: Error Subscribing to Commands, RC = 0x%08lX", (unsigned long)status);
    //     }
    // }

    if (status == CFE_SUCCESS)
    {
        /*
        ** Register Example Table(s)
        */
        /*status = CFE_TBL_Register(&FPRIME_APP_Data.TblHandles[0], "ExampleTable", sizeof(FPRIME_APP_ExampleTable_t),
                                  CFE_TBL_OPT_DEFAULT, FPRIME_APP_TblValidationFunc);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(FPRIME_APP_TABLE_REG_ERR_EID, CFE_EVS_EventType_ERROR,
                              "F Prime App: Error Registering Example Table, RC = 0x%08lX", (unsigned long)status);
        }
        else
        {
            status = CFE_TBL_Load(FPRIME_APP_Data.TblHandles[0], CFE_TBL_SRC_FILE, FPRIME_APP_PLATFORM_TABLE_FILE);
        }*/

        CFE_Config_GetVersionString(VersionString, FPRIME_APP_CFG_MAX_VERSION_STR_LEN, "F Prime App", FPRIME_APP_VERSION,
                                    FPRIME_APP_BUILD_CODENAME, FPRIME_APP_LAST_OFFICIAL);

        printf("F Prime App Initialized. Version: %s\n", VersionString);
        CFE_EVS_SendEvent(1, CFE_EVS_EventType_INFORMATION, "F Prime App Initialized.%s",
                          VersionString);
    }

    return status;
}

/**
 * \brief print command line help message
 *
 * This will print a command line help message including the available command line arguments.
 *
 * @param app: name of application
 */
void print_usage(const char* app) {
    (void)printf("Usage: ./%s [options]\n-a\thostname/IP address\n-p\tport_number\n", app);
}

/**
 * \brief shutdown topology cycling on signal
 *
 * The reference topology allows for a simulated cycling of the rate groups. This simulated cycling needs to be stopped
 * in order for the program to shutdown. This is done via handling signals such that it is performed via Ctrl-C
 *
 * @param signum
 */
static void signalHandler(int signum) {
    FPrimeApp::stopRateGroups();
}

/**
 * \brief execute the program
 *
 * This F´ program is designed to run in standard environments (e.g. Linux/macOs running on a laptop). Thus it uses
 * command line inputs to specify how to connect.
 *
 * @param argc: argument count supplied to program
 * @param argv: argument values supplied to program
 * @return: 0 on success, something else on failure
 */
int main(int argc, char* argv[]) {
    I32 option = 0;
    CHAR* hostname = nullptr;
    U16 port_number = 0;

    Os::init();

    // Loop while reading the getopt supplied options
    while ((option = getopt(argc, argv, "hp:a:")) != -1) {
        switch (option) {
            // Handle the -a argument for address/hostname
            case 'a':
                hostname = optarg;
                break;
            // Handle the -p port number argument
            case 'p':
                port_number = static_cast<U16>(atoi(optarg));
                break;
            // Cascade intended: help output
            case 'h':
            // Cascade intended: help output
            case '?':
            // Default case: output help and exit
            default:
                print_usage(argv[0]);
                return (option == 'h') ? 0 : 1;
        }
    }
    // Object for communicating state to the topology
    FPrimeApp::TopologyState inputs;
    inputs.hostname = hostname;
    inputs.port = port_number;

    // Setup program shutdown via Ctrl-C
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    (void)printf("Hit Ctrl-C to quit\n");

    // Setup, cycle, and teardown topology
    FPrimeApp::setupTopology(inputs);
    FPrimeApp::startRateGroups(Fw::TimeInterval(1,0));  // Program loop cycling rate groups at 1Hz
    FPrimeApp::teardownTopology(inputs);
    (void)printf("Exiting...\n");
    return 0;
}
