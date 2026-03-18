// ======================================================================
// \title  CfsBridge.hpp
// \author mstarch
// \brief  hpp file for CfsBridge component implementation class
// ======================================================================

#ifndef FPrimeCfs_CfsBridge_HPP
#define FPrimeCfs_CfsBridge_HPP

#include "FPrimeCfs/CfsBridge/CfsBridgeComponentAc.hpp"

namespace FPrimeCfs
{

class CfsBridge final : public CfsBridgeComponentBase
{

public:
    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct CfsBridge object
    CfsBridge(const char *const compName //!< The component name
    );

    //! Destroy CfsBridge object
    ~CfsBridge();

private:
    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for dataIn
    //!
    //! Data to be sent on the wire (coming in to the component)
    void dataIn_handler(FwIndexType portNum, //!< The port number
                        Fw::Buffer &data, const ComCfg::FrameContext &context) override;

    //! Handler implementation for dataReturnIn
    //!
    //! Port receiving back ownership of buffer sent out on dataOut
    void dataReturnIn_handler(FwIndexType portNum, //!< The port number
                              Fw::Buffer &data, const ComCfg::FrameContext &context) override;
};

} // namespace FPrimeCfs

#endif
