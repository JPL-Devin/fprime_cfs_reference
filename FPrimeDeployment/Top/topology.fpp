module FPrimeApp {

  # ----------------------------------------------------------------------
  # Symbolic constants for port numbers
  # ----------------------------------------------------------------------

  enum Ports_RateGroups {
    rateGroup1
  }


  # ComPacket Queue enum for queue types
  enum Ports_ComPacketQueue : U8 {
      EVENTS,
      TELEMETRY 
  }

  # ComPacket Queue enum for queue types
  enum Ports_ComBufferQueue : U8 {
      FILE
  }

  topology FPrimeDeployment {

  # ----------------------------------------------------------------------
  # Subtopology imports
  # ----------------------------------------------------------------------
    import CdhCore.Subtopology
    
  # ----------------------------------------------------------------------
  # Instances used in the topology
  # ----------------------------------------------------------------------
    instance chronoTime
    instance timer
    instance rateGroupDriver
    instance rateGroup1
    instance cfsBridge
    instance fprimeRouter
    instance comQueue

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
      # timer to drive rate group
      timer.CycleOut -> rateGroupDriver.CycleIn

      # Rate group 1
      rateGroupDriver.CycleOut[Ports_RateGroups.rateGroup1] -> rateGroup1.CycleIn
      rateGroup1.RateGroupMemberOut[0] -> CdhCore.tlmSend.Run
      rateGroup1.RateGroupMemberOut[1] -> CdhCore.$health.Run
    }

    connections CfsBridge {
      cfsBridge.dataOut -> fprimeRouter.dataIn
      fprimeRouter.dataReturnOut -> cfsBridge.dataReturnIn
    }

    connections Routing {
      fprimeRouter.commandOut      -> CdhCore.cmdDisp.seqCmdBuff
      CdhCore.cmdDisp.seqCmdStatus -> fprimeRouter.cmdResponseIn
    }
    connections Queueing {
      CdhCore.events.PktSend  -> comQueue.comPacketQueueIn[Ports_ComPacketQueue.EVENTS]
      CdhCore.tlmSend.PktSend -> comQueue.comPacketQueueIn[Ports_ComPacketQueue.TELEMETRY]
      comQueue.dataOut -> cfsBridge.dataIn
      cfsBridge.dataReturnOut -> comQueue.dataReturnIn
      cfsBridge.comStatusOut -> comQueue.comStatusIn
    }

  }

}
