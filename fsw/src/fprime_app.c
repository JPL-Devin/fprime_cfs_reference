/************************************************************************
 * NASA Docket No. GSC-19,200-1, and identified as "cFS Draco"
 *
 * Copyright (c) 2023 United States Government as represented by the
 * Administrator of the National Aeronautics and Space Administration.
 * All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ************************************************************************/

/**
 * \file
 *   This file contains the source code for the Sample App.
 */

/*
** Include Files:
*/
#include "fprime_app.h"
#include "fprime_app_cmds.h"
#include "fprime_app_utils.h"
#include "fprime_app_eventids.h"
#include "fprime_app_dispatch.h"
#include "fprime_app_tbl.h"
#include "fprime_app_version.h"

/*
** global data
*/
FPRIME_APP_Data_t FPRIME_APP_Data;

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * *  * * * * **/
/*                                                                            */
/* Application entry point and main process loop                              */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * *  * * * * **/
void FPRIME_APP_Main(void)
{
    CFE_Status_t     status;
    CFE_SB_Buffer_t *SBBufPtr;

    /*
    ** Create the first Performance Log entry
    */
    CFE_ES_PerfLogEntry(FPRIME_APP_PERF_ID);

    /*
    ** Perform application-specific initialization
    ** If the Initialization fails, set the RunStatus to
    ** CFE_ES_RunStatus_APP_ERROR and the App will not enter the RunLoop
    */
    status = FPRIME_APP_Init();
    if (status != CFE_SUCCESS)
    {
        FPRIME_APP_Data.RunStatus = CFE_ES_RunStatus_APP_ERROR;
    }

    /*
    ** Sample App Runloop
    */
    while (CFE_ES_RunLoop(&FPRIME_APP_Data.RunStatus) == true)
    {
        /*
        ** Performance Log Exit Stamp
        */
        CFE_ES_PerfLogExit(FPRIME_APP_PERF_ID);

        /* Pend on receipt of command packet */
        status = CFE_SB_ReceiveBuffer(&SBBufPtr, FPRIME_APP_Data.CommandPipe, CFE_SB_PEND_FOREVER);

        /*
        ** Performance Log Entry Stamp
        */
        CFE_ES_PerfLogEntry(FPRIME_APP_PERF_ID);

        if (status == CFE_SUCCESS)
        {
            FPRIME_APP_TaskPipe(SBBufPtr);
        }
        else
        {
            CFE_EVS_SendEvent(FPRIME_APP_PIPE_ERR_EID, CFE_EVS_EventType_ERROR,
                              "FPRIME APP: SB Pipe Read Error, App Will Exit");

            FPRIME_APP_Data.RunStatus = CFE_ES_RunStatus_APP_ERROR;
        }
    }

    /*
    ** Performance Log Exit Stamp
    */
    CFE_ES_PerfLogExit(FPRIME_APP_PERF_ID);

    CFE_ES_ExitApp(FPRIME_APP_Data.RunStatus);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  */
/*                                                                            */
/* Initialization                                                             */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
CFE_Status_t FPRIME_APP_Init(void)
{
    CFE_Status_t status;
    char         VersionString[FPRIME_APP_CFG_MAX_VERSION_STR_LEN];

    /* Zero out the global data structure */
    memset(&FPRIME_APP_Data, 0, sizeof(FPRIME_APP_Data));

    FPRIME_APP_Data.RunStatus = CFE_ES_RunStatus_APP_RUN;

    /*
    ** Register the events
    */
    status = CFE_EVS_Register(NULL, 0, CFE_EVS_EventFilter_BINARY);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("F Prime App: Error Registering Events, RC = 0x%08lX\n", (unsigned long)status);
    }
    else
    {
        /*
         ** Initialize housekeeping packet (clear user data area).
         */
        CFE_MSG_Init(CFE_MSG_PTR(FPRIME_APP_Data.HkTlm.TelemetryHeader), CFE_SB_ValueToMsgId(FPRIME_APP_HK_TLM_MID),
                     sizeof(FPRIME_APP_Data.HkTlm));

        /*
         ** Create Software Bus message pipe.
         */
        status = CFE_SB_CreatePipe(&FPRIME_APP_Data.CommandPipe, FPRIME_APP_PLATFORM_PIPE_DEPTH,
                                   FPRIME_APP_PLATFORM_PIPE_NAME);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(FPRIME_APP_CR_PIPE_ERR_EID, CFE_EVS_EventType_ERROR,
                              "F Prime App: Error creating SB Command Pipe, RC = 0x%08lX", (unsigned long)status);
        }
    }

    if (status == CFE_SUCCESS)
    {
        /*
        ** Subscribe to Housekeeping request commands
        */
        status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(FPRIME_APP_SEND_HK_MID), FPRIME_APP_Data.CommandPipe);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(FPRIME_APP_SUB_HK_ERR_EID, CFE_EVS_EventType_ERROR,
                              "F Prime App: Error Subscribing to HK request, RC = 0x%08lX", (unsigned long)status);
        }
    }

    if (status == CFE_SUCCESS)
    {
        /*
        ** Subscribe to ground command packets
        */
        status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(FPRIME_APP_CMD_MID), FPRIME_APP_Data.CommandPipe);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(FPRIME_APP_SUB_CMD_ERR_EID, CFE_EVS_EventType_ERROR,
                              "F Prime App: Error Subscribing to Commands, RC = 0x%08lX", (unsigned long)status);
        }
    }

    if (status == CFE_SUCCESS)
    {
        /*
        ** Register Example Table(s)
        */
        status = CFE_TBL_Register(&FPRIME_APP_Data.TblHandles[0], "ExampleTable", sizeof(FPRIME_APP_ExampleTable_t),
                                  CFE_TBL_OPT_DEFAULT, FPRIME_APP_TblValidationFunc);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(FPRIME_APP_TABLE_REG_ERR_EID, CFE_EVS_EventType_ERROR,
                              "F Prime App: Error Registering Example Table, RC = 0x%08lX", (unsigned long)status);
        }
        else
        {
            status = CFE_TBL_Load(FPRIME_APP_Data.TblHandles[0], CFE_TBL_SRC_FILE, FPRIME_APP_PLATFORM_TABLE_FILE);
        }

        CFE_Config_GetVersionString(VersionString, FPRIME_APP_CFG_MAX_VERSION_STR_LEN, "F Prime App", FPRIME_APP_VERSION,
                                    FPRIME_APP_BUILD_CODENAME, FPRIME_APP_LAST_OFFICIAL);

        CFE_EVS_SendEvent(FPRIME_APP_INIT_INF_EID, CFE_EVS_EventType_INFORMATION, "F Prime App Initialized.%s",
                          VersionString);
    }

    return status;
}
