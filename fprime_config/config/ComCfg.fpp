# ======================================================================
# FPP file for configuration of the communications stack
# ======================================================================

module ComCfg {

    @ Spacecraft ID (10 bits) for CCSDS Data Link layer
    dictionary constant SpacecraftId = 0x0044

    @ Fixed size of CCSDS TM frames
    dictionary constant TmFrameFixedSize = 1024  # Needs to be at least COM_BUFFER_MAX_SIZE + (2 * SpacePacketHeaderSize) + 1

    @ Upper Bound on Fixed size of CCSDS AOS frames
    constant AosMaxFrameFixedSize = 1536

    @ Aggregation buffer for ComAggregator component
    constant AggregationSize = TmFrameFixedSize - 6 - 6 - 1 - 2  # 2 header (6) + 1 idle byte + 2 trailer bytes

    @ Transmission type for the CfsBridge component
    enum TransmissionType : U8 {
        COMMAND,
        TELEMETRY
    }

    @ APIDs are 11 bits in the Space Packet protocol, so we use U16. Max value 7FF
    dictionary enum Apid : FwPacketDescriptorType {
        # APIDs prefixed with FW are reserved for F Prime and need to be present
        # in the enumeration. Their values can be changed
        FW_PACKET_COMMAND        = 0x0000  @< Command packet type - incoming
        FW_PACKET_TELEM          = 0x0001  @< Telemetry packet type - outgoing
        FW_PACKET_LOG            = 0x0002  @< Log type - outgoing
        FW_PACKET_FILE           = 0x0003  @< File type - incoming and outgoing
        FW_PACKET_PACKETIZED_TLM = 0x0004  @< Packetized telemetry packet type
        FW_PACKET_DP             = 0x0005  @< Data Product packet type
        FW_PACKET_IDLE           = 0x0006  @< F Prime idle
        FW_PACKET_HAND           = 0x00FE  @< F Prime handshake
        FW_PACKET_UNKNOWN        = 0x00FF  @< F Prime unknown packet
        SPP_IDLE_PACKET          = 0x07FF  @< Per Space Packet Standard, all 1s (11bits) is reserved for Idle Packets
        INVALID_UNINITIALIZED    = 0x0800  @< Anything equal or higher value is invalid and should not be used
    } default INVALID_UNINITIALIZED

    @ Context required for the CfsBridge component. This is a delta on the existing configuration from the default
    @ ComCfg. It adds in the fields necessary to influence the CfsBridge behavior when framing and deframing messages.
    @

    struct FrameContext {
        comQueueIndex: FwIndexType  @< REQUIRED: Queue Index used by the ComQueue, other components shall not modify
        apid: Apid            @< REQUIRED: 11 bits APID in CCSDS
        transmissionType: TransmissionType @< REQUIRED: Type of transmission (command or telemetry). Default: telemetry
        messageId: U16              @< REQUIRED: CFS Message ID. Default: 0xFFFF (read runtime configuration)
        sequenceCount: U16  @< REQUIRED to appease F Prime's build
        vcId: U8           @< REQUIRED to appease F Prime's build
        sendNow: bool               @< Flag to AOS Framer that the Frame this packet goes into should be sent ASAP
    } default {
        vcId = 1
        comQueueIndex = 0
        apid = Apid.FW_PACKET_UNKNOWN
        transmissionType = TransmissionType.TELEMETRY
        messageId = 0xFFFF # Invalid message ID, will force component to read from the runtime configuration
    }
}
