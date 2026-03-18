// ======================================================================
// \title  CfsBridge.cpp
// \author mstarch
// \brief  cpp file for CfsBridge component implementation class
// ======================================================================

#include "FPrimeCfs/CfsBridge/CfsBridge.hpp"
#include "Fw/Logger/Logger.hpp"
extern "C" {
    #include "cfe_sb.h"   // for CFE_SB_TransmitMsg
}

namespace FPrimeCfs
{

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

CfsBridge ::CfsBridge(const char *const compName) : CfsBridgeComponentBase(compName) {}

CfsBridge ::~CfsBridge() {}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

// TODO: need a separate thread to handle the data coming from CFS...or possibly stroke from the main app

void CfsBridge ::dataIn_handler(FwIndexType portNum, Fw::Buffer &data, const ComCfg::FrameContext &context)
{
    Fw::Logger::log("[INFO] Received data on dataIn port, size %zu\n", data.getSize());
    CFE_Status_t status = CFE_SB_TransmitMsg(reinterpret_cast<CFE_MSG_Message_t *>(data.getData()), false);

    CFE_MSG_Message_t *msg = reinterpret_cast<CFE_MSG_Message_t *>(data.getData());
    Fw::Logger::log("[DEBUG] Message: ");
    for (FwSizeType i = 0; i < data.getSize(); i++) {
        Fw::Logger::log("%02x", data.getData()[i]);
        if ((i % 2) == 1) {
            Fw::Logger::log(" ");
        }
    }
    Fw::Logger::log("\n");

    Fw::Logger::log("[DEBUG] (AS CFS) Message: ");
    for (FwSizeType i = 0; i < data.getSize(); i++) {
        Fw::Logger::log("%02x",reinterpret_cast<uint8_t *>(msg)[i]);
        if ((i % 2) == 1) {
            Fw::Logger::log(" ");
        }
    }
    Fw::Logger::log("\n");

    Fw::Logger::log("[DEBUG] (Length): ");
    for (FwSizeType i = 0; i < 2; i++) {
        Fw::Logger::log("%02x",reinterpret_cast<uint8_t *>(&msg->CCSDS.Pri.Length)[i]);
        if ((i % 2) == 1) {
            Fw::Logger::log(" ");
        }
    }
    Fw::Logger::log("\n");

    Fw::Logger::log("[INFO] CCSDS Primary Header Length: %u\n", msg->CCSDS.Pri.Length);
    if (status != CFE_SUCCESS)
    {
        Fw::Logger::log("[ERROR] Failed to send CFS message: %x\n", status);
    } else {
        Fw::Logger::log("[INFO] Successfully sent CFS message");
    }
    // Always deallocate the data
    this->dataReturnOut_out(portNum, data, context);
}

void CfsBridge ::dataReturnIn_handler(FwIndexType portNum, Fw::Buffer &data, const ComCfg::FrameContext &context)
{
    // TODO: this would be deallocation of the message sent from cFS to us.
}

} // namespace FPrimeCfs
