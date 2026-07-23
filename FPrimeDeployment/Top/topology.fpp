module FPrimeApp {

  # ----------------------------------------------------------------------
  # Symbolic constants for port numbers
  # ----------------------------------------------------------------------

  enum Ports_RateGroups {
    rateGroup1
  }


  topology FPrimeDeployment {

  # ----------------------------------------------------------------------
  # Subtopology imports
  # ----------------------------------------------------------------------
    import CdhCore.Subtopology
    import ComCcsds.SpacePacketFraming

  # ----------------------------------------------------------------------
  # Instances used in the topology
  # ----------------------------------------------------------------------
    instance chronoTime
    instance schAppDriver
    instance rateGroupDriver
    instance rateGroup1
    instance cfsBridge

  # ----------------------------------------------------------------------
  # Pattern graph specifiers
  # ----------------------------------------------------------------------

    command connections instance CdhCore.cmdDisp
    event connections instance CdhCore.events
    telemetry connections instance CdhCore.tlmSend
    text event connections instance CdhCore.textLogger
    health connections instance CdhCore.$health
    time connections instance chronoTime

  # ----------------------------------------------------------------------
  # Telemetry packets (only used when TlmPacketizer is used)
  # ----------------------------------------------------------------------

    # include "FPrimeDeploymentPackets.fppi"

  # ----------------------------------------------------------------------
  # Direct graph specifiers
  # ----------------------------------------------------------------------


    connections RateGroups {
      # cFS scheduler (SCH) tick messages, routed to the SchAppDriver, drive the rate group
      ComCcsds.fprimeRouter.cfsCommandOut[0] -> schAppDriver.cfsCommandIn
      schAppDriver.bufferReturnOut           -> ComCcsds.fprimeRouter.bufferReturnIn
      schAppDriver.CycleOut                  -> rateGroupDriver.CycleIn

      # Rate group 1
      rateGroupDriver.CycleOut[Ports_RateGroups.rateGroup1] -> rateGroup1.CycleIn
      rateGroup1.RateGroupMemberOut[0] -> CdhCore.cmdDisp.run
      rateGroup1.RateGroupMemberOut[1] -> CdhCore.tlmSend.Run
      rateGroup1.RateGroupMemberOut[2] -> CdhCore.$health.Run
      rateGroup1.RateGroupMemberOut[3] -> ComCcsds.comQueue.run
      rateGroup1.RateGroupMemberOut[4] -> ComCcsds.aggregator.timeout
      rateGroup1.RateGroupMemberOut[5] -> ComCcsds.commsBufferManager.schedIn
    }

    connections CfsBridge {
      # Downlink: framing layer -> bridge -> cFS software bus
      ComCcsds.SpacePacketFraming.dataOut -> cfsBridge.dataIn
      cfsBridge.dataReturnOut             -> ComCcsds.SpacePacketFraming.dataReturnIn
      cfsBridge.comStatusOut              -> ComCcsds.SpacePacketFraming.comStatusIn

      # Uplink: cFS software bus -> bridge -> framing layer
      cfsBridge.dataOut                         -> ComCcsds.SpacePacketFraming.dataIn
      ComCcsds.SpacePacketFraming.dataReturnOut -> cfsBridge.dataReturnIn
    }

    connections Routing {
      # The router instance is FPrimeCfs.CfsRouter, selected via the ComCcsdsRouterConfig
      # configuration override (fprime_config/config/ComCcsdsRouterConfig.fpp)
      ComCcsds.fprimeRouter.commandOut[0] -> CdhCore.cmdDisp.seqCmdBuff
      CdhCore.cmdDisp.seqCmdStatus   -> ComCcsds.fprimeRouter.cmdResponseIn
    }
    connections Queueing {
      CdhCore.events.PktSend  -> ComCcsds.comQueue.comPacketQueueIn[ComCcsds.Ports_ComPacketQueue.EVENTS]
      CdhCore.tlmSend.PktSend -> ComCcsds.comQueue.comPacketQueueIn[ComCcsds.Ports_ComPacketQueue.TELEMETRY]
    }

  }

}
