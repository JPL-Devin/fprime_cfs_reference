module FPrimeApp {

  # ----------------------------------------------------------------------
  # Base ID Convention
  # ----------------------------------------------------------------------
  #
  # All Base IDs follow the 8-digit hex format: 0xDSSCCxxx
  #
  # Where:
  #   D   = Deployment digit (1 for this deployment)
  #   SS  = Subtopology digits (00 for main topology, 01-05 for subtopologies)
  #   CC  = Component digits (00, 01, 02, etc.)
  #   xxx = Reserved for internal component items (events, commands, telemetry)
  #

  # ----------------------------------------------------------------------
  # Defaults
  # ----------------------------------------------------------------------

  module Default {
    constant QUEUE_SIZE = 10
    constant STACK_SIZE = 64 * 1024
  }

  # ----------------------------------------------------------------------
  # Active component instances
  # ----------------------------------------------------------------------

  instance rateGroup1: Svc.ActiveRateGroup base id 0x10001000 \
    queue size Default.QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 43

  @ Instance to bridge F Prime communication to the CFS bus
  instance cfsBridge: FPrimeCfs.CfsBridge base id 0x10002000 \
    queue size Default.QUEUE_SIZE

  # ----------------------------------------------------------------------
  # Queued component instances
  # ----------------------------------------------------------------------

  instance comQueue: Svc.ComQueue base id 0x10003000 \
      queue size ComCcsdsConfig.QueueSizes.comQueue \
      stack size ComCcsdsConfig.StackSizes.comQueue \
      priority ComCcsdsConfig.Priorities.comQueue \
  {
      phase Fpp.ToCpp.Phases.configObjects """
      Fw::MallocAllocator mallocator;
      """
      phase Fpp.ToCpp.Phases.configComponents """
      Svc::ComQueue::QueueConfigurationTable configurationTable;

      // Events (highest-priority)
      configurationTable.entries[Ports_ComPacketQueue::EVENTS].depth = 100;
      configurationTable.entries[Ports_ComPacketQueue::EVENTS].priority = 0;

      // Telemetry
      configurationTable.entries[Ports_ComPacketQueue::TELEMETRY].depth = 100;
      configurationTable.entries[Ports_ComPacketQueue::TELEMETRY].priority = 1;

      // File Downlink Queue (buffer queue using NUM_CONSTANTS offset)
      configurationTable.entries[Ports_ComPacketQueue::NUM_CONSTANTS + Ports_ComBufferQueue::FILE].depth = 100;
      configurationTable.entries[Ports_ComPacketQueue::NUM_CONSTANTS + Ports_ComBufferQueue::FILE].priority = 2;

      // Allocation identifier is 0 as the MallocAllocator discards it
      comQueue.configure(configurationTable, 0, ConfigObjects::FPrimeApp_comQueue::mallocator);
      """
      phase Fpp.ToCpp.Phases.tearDownComponents """
      comQueue.cleanup();
      """
  }

  # ----------------------------------------------------------------------
  # Passive component instances
  # ----------------------------------------------------------------------

  instance chronoTime: Svc.ChronoTime base id 0x10010000

  instance rateGroupDriver: Svc.RateGroupDriver base id 0x10011000

  instance timer: Svc.LinuxTimer base id 0x10012000

  instance fprimeRouter: Svc.FprimeRouter base id 0x10013000


}
