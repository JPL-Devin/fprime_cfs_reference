# ======================================================================
# CfsRouterCfg.fpp
# Project override of the CfsRouter routing table: routes F Prime commands,
# file packets, and the cFS scheduler (SCH) tick message that drives the
# rate groups via the FPrimeCfs.SchAppDriver.
# ======================================================================
module FPrimeCfs {

    @ Number of F Prime command output ports on the CfsRouter
    constant CFS_ROUTER_FPRIME_COMMAND_PORTS = 10

    @ Number of cFS command output ports on the CfsRouter
    constant CFS_ROUTER_CFS_COMMAND_PORTS = 10

    @ Number of cFS telemetry output ports on the CfsRouter
    constant CFS_ROUTER_CFS_TELEMETRY_PORTS = 10

    @ Maximum number of buffers simultaneously outstanding on the pass-through
    @ routes (cFS command, cFS telemetry, unknown) awaiting return
    constant CFS_ROUTER_MAX_PENDING_BUFFERS = 10

    @ Number of entries in the APID routing table
    constant CFS_ROUTER_ROUTE_TABLE_SIZE = 3

    @ The APID routing table: APID -> (route type, output port index)
    constant CFS_ROUTER_ROUTE_TABLE = [
        { apid = ComCfg.Apid.FW_PACKET_COMMAND, routeType = CfsRouteType.FPRIME_COMMAND, portIndex = 0 },
        { apid = ComCfg.Apid.FW_PACKET_FILE,    routeType = CfsRouteType.FILE,           portIndex = 0 },
        { apid = ComCfg.Apid.CFS_SCH_TICK,      routeType = CfsRouteType.CFS_COMMAND,    portIndex = 0 },
    ]
}
