# Develop an F Prime Application for cFS

This guide walks you through developing a new F Prime application that runs as a cFS (core Flight System) app. You will create a cFS application module, define an F Prime topology inside it, bridge F Prime's data flow to the cFS software bus, build the system, and connect the F Prime GDS for commanding and telemetry.

---

## Prerequisites

Before starting, you should have:

* Completed the [F Prime Hello World](https://fprime.jpl.nasa.gov/latest/tutorials-hello-world/docs/hello-world/) tutorial (basic familiarity with F Prime components, FPP, and `fprime-util`).
* A general understanding of [cFS application development](https://github.com/nasa/cFS) (entry points, software bus, startup scripts).
* A working understanding of [FPP component modeling](https://nasa.github.io/fpp/fpp-users-guide.html) (ports, commands, events, telemetry, topologies).
* A working clone of the [fprime_cfs_reference](https://github.com/fprime-community/fprime_cfs_reference) repository with all submodules and prerequisites installed per the [README](../../README.md).

---

## Background

### How It Works

In a standard F Prime deployment, the framework provides its own process management, communication driver, and ground system interface. In the cFS integration, these responsibilities are split:

| Responsibility | Standard F Prime | cFS + F Prime |
|----------------|-----------------|---------------|
| Process management | F Prime `Main.cpp` (standalone binary) | cFS Executive Service (ES) loads the app as a `.so` |
| Inter-app messaging | Direct F Prime port connections | cFS Software Bus (SB) via `CfsBridge` |
| Ground system comms | F Prime `TcpServer`/`TcpClient` in the app topology | Separate `fprime_gds` cFS app handles TCP to/from GDS |
| Rate group driver | OS-level timer or threaded driver | `PollingTimer` (cooperative polling within the cFS run loop) |

The key integration component is `CfsBridge` from the `fprime_cfs` library. It:

1. **Deframes** incoming cFS SB messages into F Prime `Fw::Buffer` data for routing to the command dispatcher
2. **Frames** outgoing F Prime telemetry/events into cFS SB messages for transmission

### Application Structure

An F Prime cFS application consists of:

```
my_app/
├── CMakeLists.txt                    # cFS app + F Prime project registration
├── mission_build.cmake               # cFS mission build integration
├── arch_build.cmake                  # cFS architecture build hooks
├── fsw/                              # cFS-standard flight software
│   ├── src/                          # C source (entry point, dispatch, commands)
│   │   └── my_app.c                  # Contains MY_APP_Main() entry point
│   └── inc/                          # C headers (message IDs, event IDs, config)
├── MyDeployment/                     # F Prime deployment
│   ├── CMakeLists.txt                # Deployment build (calls add_cfe_app)
│   ├── Main.cpp                      # F Prime topology setup inside cFS entry point
│   └── Top/                          # Topology definition
│       ├── topology.fpp              # FPP wiring (instances + connections)
│       ├── instances.fpp             # FPP component instance declarations
│       ├── MyDeploymentTopology.cpp   # Topology configuration C++
│       └── CMakeLists.txt            # Topology build targets
├── my_config/config/                 # F Prime configuration overrides
│   ├── CMakeLists.txt
│   └── ComCfg.fpp                    # APID and frame context definitions
├── config/                           # cFS default config headers
├── eds/                              # EDS definitions (optional)
└── unit-test/                        # Unit tests
```

---

## Step 1 - Create the cFS Application Skeleton

Create a new directory under `apps/` for your application. This follows the cFS convention where each app is a self-contained module with its own build files and source.

```bash
mkdir -p apps/my_app/fsw/src apps/my_app/fsw/inc apps/my_app/config
```

### CMakeLists.txt (top-level)

Create `apps/my_app/CMakeLists.txt`. This file registers the app with both the F Prime build system and the cFS build system:

```cmake
cmake_minimum_required(VERSION 3.22)
project(CFE_MY_APP C CXX)
register_fprime_project()

add_compile_options(-fPIC)

set(APP_SRC_FILES
  fsw/src/my_app.c
  fsw/src/my_app_cmds.c
  fsw/src/my_app_dispatch.c
)

add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/my_config/config")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/MyDeployment")

if (ENABLE_UNIT_TESTS)
  add_subdirectory(unit-test)
endif (ENABLE_UNIT_TESTS)
```

Key points:
- `register_fprime_project()` initializes F Prime's build system within the cFS build
- `add_compile_options(-fPIC)` is required because cFS loads apps as shared libraries
- The cFS C source files and the F Prime deployment are both included

### mission_build.cmake and arch_build.cmake

These files are required by the cFS build system for each application. Create minimal versions:

**`apps/my_app/mission_build.cmake`:**
```cmake
# Mission build configuration for my_app
```

**`apps/my_app/arch_build.cmake`:**
```cmake
# Architecture build configuration for my_app
```

---

## Step 2 - Implement the cFS Entry Point

Every cFS application needs a C entry point function. For an F Prime cFS app, this function initializes cFS services (event registration, SB pipe creation) and then sets up and runs the F Prime topology.

Create `apps/my_app/fsw/src/my_app.c`:

```c
#include "my_app.h"
#include "my_app_eventids.h"
#include "my_app_dispatch.h"

MY_APP_Data_t MY_APP_Data;

void MY_APP_Main(void)
{
    CFE_Status_t     status;
    CFE_SB_Buffer_t *SBBufPtr;

    CFE_ES_PerfLogEntry(MY_APP_PERF_ID);

    status = MY_APP_Init();
    if (status != CFE_SUCCESS)
    {
        MY_APP_Data.RunStatus = CFE_ES_RunStatus_APP_ERROR;
    }

    while (CFE_ES_RunLoop(&MY_APP_Data.RunStatus) == true)
    {
        CFE_ES_PerfLogExit(MY_APP_PERF_ID);
        status = CFE_SB_ReceiveBuffer(&SBBufPtr, MY_APP_Data.CommandPipe, CFE_SB_PEND_FOREVER);
        CFE_ES_PerfLogEntry(MY_APP_PERF_ID);

        if (status == CFE_SUCCESS)
        {
            MY_APP_TaskPipe(SBBufPtr);
        }
        else
        {
            MY_APP_Data.RunStatus = CFE_ES_RunStatus_APP_ERROR;
        }
    }

    CFE_ES_PerfLogExit(MY_APP_PERF_ID);
    CFE_ES_ExitApp(MY_APP_Data.RunStatus);
}
```

> [!NOTE]
> This is the standard cFS application pattern. The F Prime topology setup will be wired in through `MY_APP_Init()` as shown in the next steps.

---

## Step 3 - Define the F Prime Topology

The F Prime topology defines the component instances, their wiring, and how they interact. This is the core of the F Prime application logic.

### Create the Deployment Directory

```bash
mkdir -p apps/my_app/MyDeployment/Top
```

### instances.fpp

Create `apps/my_app/MyDeployment/Top/instances.fpp` to declare component instances:

```
module MyApp {

  module Default {
    constant QUEUE_SIZE = 10
    constant STACK_SIZE = 64 * 1024
  }

  # Active component instances
  instance rateGroup1: Svc.ActiveRateGroup base id 0x10001000 \
    queue size Default.QUEUE_SIZE \
    stack size Default.STACK_SIZE \
    priority 43

  instance cfsBridge: FPrimeCfs.CfsBridge base id 0x10002000 \
    queue size Default.QUEUE_SIZE

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
      configurationTable.entries[Ports_ComPacketQueue::EVENTS].depth = 100;
      configurationTable.entries[Ports_ComPacketQueue::EVENTS].priority = 0;
      configurationTable.entries[Ports_ComPacketQueue::TELEMETRY].depth = 100;
      configurationTable.entries[Ports_ComPacketQueue::TELEMETRY].priority = 1;
      configurationTable.entries[Ports_ComPacketQueue::NUM_CONSTANTS + Ports_ComBufferQueue::FILE].depth = 100;
      configurationTable.entries[Ports_ComPacketQueue::NUM_CONSTANTS + Ports_ComBufferQueue::FILE].priority = 2;
      comQueue.configure(configurationTable, 0, ConfigObjects::MyApp_comQueue::mallocator);
      """
      phase Fpp.ToCpp.Phases.tearDownComponents """
      comQueue.cleanup();
      """
  }

  # Passive component instances
  instance chronoTime: Svc.ChronoTime base id 0x10010000

  instance rateGroupDriver: Svc.RateGroupDriver base id 0x10011000

  instance timer: Svc.PollingTimer base id 0x10012000

  instance fprimeRouter: Svc.FprimeRouter base id 0x10013000

}
```

Key points:
- `CfsBridge` is the crucial integration component from the `fprime_cfs` library
- `PollingTimer` drives rate groups cooperatively (no OS-level timer thread)
- `CdhCore` subtopology (imported in the topology) provides command dispatch, events, telemetry, and health
- Base IDs follow the convention `0xDSSCCxxx` (D=deployment, SS=subtopology, CC=component)

### topology.fpp

Create `apps/my_app/MyDeployment/Top/topology.fpp` to wire the instances together:

```
module MyApp {

  enum Ports_RateGroups {
    rateGroup1
  }

  enum Ports_ComPacketQueue : U8 {
      EVENTS,
      TELEMETRY
  }

  enum Ports_ComBufferQueue : U8 {
      FILE
  }

  topology MyDeployment {

    # Import the CdhCore subtopology for command dispatch, events, telemetry
    import CdhCore.Subtopology

    # Declare instances used in the topology
    instance chronoTime
    instance timer
    instance rateGroupDriver
    instance rateGroup1
    instance cfsBridge
    instance fprimeRouter
    instance comQueue

    # Pattern graph specifiers
    command connections instance CdhCore.cmdDisp
    event connections instance CdhCore.events
    telemetry connections instance CdhCore.tlmSend
    text event connections instance CdhCore.textLogger
    health connections instance CdhCore.$health
    time connections instance chronoTime

    # Rate group connections
    connections RateGroups {
      timer.CycleOut -> rateGroupDriver.CycleIn
      rateGroupDriver.CycleOut[Ports_RateGroups.rateGroup1] -> rateGroup1.CycleIn
      rateGroup1.RateGroupMemberOut[0] -> CdhCore.cmdDisp.run
      rateGroup1.RateGroupMemberOut[1] -> CdhCore.tlmSend.Run
      rateGroup1.RateGroupMemberOut[2] -> CdhCore.$health.Run
    }

    # CfsBridge connections (SB <-> F Prime)
    connections CfsBridge {
      cfsBridge.dataOut -> fprimeRouter.dataIn
      fprimeRouter.dataReturnOut -> cfsBridge.dataReturnIn
    }

    # Command routing
    connections Routing {
      fprimeRouter.commandOut      -> CdhCore.cmdDisp.seqCmdBuff
      CdhCore.cmdDisp.seqCmdStatus -> fprimeRouter.cmdResponseIn
    }

    # Telemetry/event queuing and downlink
    connections Queueing {
      CdhCore.events.PktSend  -> comQueue.comPacketQueueIn[Ports_ComPacketQueue.EVENTS]
      CdhCore.tlmSend.PktSend -> comQueue.comPacketQueueIn[Ports_ComPacketQueue.TELEMETRY]
      comQueue.dataOut -> cfsBridge.dataIn
      cfsBridge.dataReturnOut -> comQueue.dataReturnIn
      cfsBridge.comStatusOut -> comQueue.comStatusIn
    }

  }

}
```

---

## Step 4 - Create the Deployment Main.cpp

The deployment `Main.cpp` bridges the cFS entry point with the F Prime topology lifecycle. It replaces the standard cFS run loop with the F Prime polling pattern.

Create `apps/my_app/MyDeployment/Main.cpp`:

```cpp
#include <MyDeployment/Top/MyDeploymentTopology.hpp>
#include <MyDeployment/Top/MyDeploymentTopologyAc.hpp>
#include <Os/Os.hpp>
#include <getopt.h>
#include <cstdlib>

extern "C" {
    #include "cfe.h"
    #include "cfe_config.h"
    #include "my_app_version.h"
    #include "my_app_internal_cfg.h"
    void MY_APP_Main(void);
}

CFE_SB_PipeId_t CommandPipe;
CFE_Status_t MY_APP_Init(MyApp::TopologyState& inputs);

static MyApp::TopologyState g_topologyState;

static void MY_APP_StopAndTeardown(MyApp::TopologyState& inputs)
{
    MyApp::timer.stop();
    MyApp::teardownTopology(inputs);
}

static void MY_APP_Shutdown(MyApp::TopologyState& inputs, uint32 status)
{
    MY_APP_StopAndTeardown(inputs);
    CFE_ES_ExitApp(status);
}

void MY_APP_delete_callback(void)
{
    MY_APP_StopAndTeardown(g_topologyState);
}

void MY_APP_Main(void) {
    Os::init();

    MyApp::TopologyState& inputs = g_topologyState;
    uint32 run_status = CFE_ES_RunStatus_APP_RUN;

    CFE_Status_t status = MY_APP_Init(inputs);
    if (status != CFE_SUCCESS) {
        MY_APP_Shutdown(inputs, status);
    }

    // Start the polling timer at 1 Hz
    MyApp::timer.startTimer(Fw::TimeInterval(1, 0));

    // Main loop: cooperative polling (no blocking SB receive)
    while (CFE_ES_RunLoop(&run_status) == true)
    {
        MyApp::timer.cycle();
        MyApp::cfsBridge.process();
    }
    MY_APP_Shutdown(inputs, run_status);
}

CFE_Status_t MY_APP_Init(MyApp::TopologyState& inputs)
{
    CFE_Status_t status;
    char VersionString[MY_APP_CFG_MAX_VERSION_STR_LEN];

    status = CFE_EVS_Register(NULL, 0, CFE_EVS_EventFilter_BINARY);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("My App: Error Registering Events, RC = 0x%08lX\n", (unsigned long)status);
    }
    else
    {
        // Configure the CfsBridge with a cFS SB pipe
        status = MyApp::cfsBridge.configure(MY_APP_PLATFORM_PIPE_DEPTH, MY_APP_PLATFORM_PIPE_NAME);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(2, CFE_EVS_EventType_ERROR,
                "My App: Error creating SB Command Pipe, RC = 0x%08lX", (unsigned long)status);
        }
    }
    if (status == CFE_SUCCESS)
    {
        // Subscribe to the F Prime command APID on the cFS SB
        MyApp::cfsBridge.subscribe(ComCfg::Apid::FW_PACKET_COMMAND);
    }

    // Install the cFS delete handler for clean shutdown
    OS_TaskInstallDeleteHandler(&MY_APP_delete_callback);

    // Set up the F Prime topology (init, wire, configure, start tasks)
    MyApp::setupTopology(inputs);

    if (status == CFE_SUCCESS)
    {
        CFE_Config_GetVersionString(VersionString, MY_APP_CFG_MAX_VERSION_STR_LEN,
            "My App", MY_APP_VERSION, MY_APP_BUILD_CODENAME, MY_APP_LAST_OFFICIAL);
        CFE_EVS_SendEvent(1, CFE_EVS_EventType_INFORMATION, "My App Initialized.%s", VersionString);
    }

    return status;
}
```

Key patterns to note:

1. **`MY_APP_Main()`** uses `extern "C"` linkage so cFS can call it as a standard C entry point
2. The main loop calls `timer.cycle()` and `cfsBridge.process()` cooperatively instead of blocking on a cFS SB pipe
3. **`cfsBridge.configure()`** creates a cFS SB pipe for receiving messages
4. **`cfsBridge.subscribe()`** subscribes to the command APID on the cFS SB
5. **`OS_TaskInstallDeleteHandler()`** ensures clean topology teardown when cFS shuts down the app

---

## Step 5 - Create the Topology Configuration

Create `apps/my_app/MyDeployment/Top/MyDeploymentTopology.cpp`:

```cpp
#include <MyDeployment/Top/MyDeploymentTopologyAc.hpp>
#include <Fw/Types/MallocAllocator.hpp>

namespace MyApp {

Fw::MallocAllocator mallocator;

const Svc::RateGroupDriver::DividerSet rateGroupDivisorsSet{{{1, 0}, {2, 0}, {4, 0}}};

U32 rateGroup1Context[Svc::ActiveRateGroup::CONNECTION_COUNT_MAX] = {};

void configureTopology() {
    rateGroupDriver.configure(rateGroupDivisorsSet);
    rateGroup1.configure(rateGroup1Context, FW_NUM_ARRAY_ELEMENTS(rateGroup1Context));
}

void setupTopology(const TopologyState& state) {
    initComponents(state);
    setBaseIds();
    connectComponents();
    regCommands();
    configComponents(state);
    configureTopology();
    loadParameters();
    startTasks(state);
}

void teardownTopology(const TopologyState& state) {
    stopTasks(state);
    freeThreads(state);
    tearDownComponents(state);
}

};  // namespace MyApp
```

Create `apps/my_app/MyDeployment/Top/CMakeLists.txt`:

```cmake
set(SOURCE_FILES
    "${CMAKE_CURRENT_LIST_DIR}/MyDeploymentTopology.cpp"
)

register_fprime_deployment_topology(
    "${SOURCE_FILES}"
)
```

### Deployment CMakeLists.txt

Create `apps/my_app/MyDeployment/CMakeLists.txt`:

```cmake
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Top/")

# Create the cFS application from the F Prime Main.cpp deployment
add_cfe_app(my_app "${CMAKE_CURRENT_LIST_DIR}/Main.cpp")

target_include_directories(my_app PUBLIC ../fsw/inc)

target_compile_options(my_app PRIVATE -Wno-pedantic)

target_link_libraries(my_app ${FPRIME_CURRENT_MODULE}_Top)
add_dependencies(my_app ${FPRIME_CURRENT_MODULE}_Top)

set_target_properties(my_app PROPERTIES FPRIME_TYPE Deployment)
fprime_attach_custom_targets(my_app)
set(FPRIME_USE_PLAIN_LINK_SIGNATURE ON)
fprime_target_implementations(my_app)
```

Key points:
- `add_cfe_app()` creates the cFS shared library (`.so`) from the F Prime deployment
- `target_link_libraries()` links the F Prime topology to the cFS app
- `set_target_properties(... FPRIME_TYPE Deployment)` marks this as an F Prime deployment for the build system

---

## Step 6 - Configure the Communication Stack

Create the F Prime configuration module that defines the APID mappings and frame context. This tells F Prime how to map its packet types to cFS message IDs.

Create `apps/my_app/my_config/config/ComCfg.fpp`:

```
dictionary type FwPacketDescriptorType = U16

module ComCfg {
    dictionary constant SpacecraftId = 0x0044
    dictionary constant TmFrameFixedSize = 1024

    constant AosMaxFrameFixedSize = 1536
    constant AggregationSize = TmFrameFixedSize - 6 - 6 - 1 - 2

    dictionary enum Pvn : U8 {
        SPACE_PACKET_PROTOCOL         = 0x0
        ENCAPSULATION_PACKET_PROTOCOL = 0x7
        INVALID_UNINITIALIZED         = 0x8
    } default INVALID_UNINITIALIZED

    enum TransmissionType : U8 {
        COMMAND,
        TELEMETRY
    }

    dictionary enum Apid : FwPacketDescriptorType {
        FW_PACKET_COMMAND        = 0x0000
        FW_PACKET_TELEM          = 0x0001
        FW_PACKET_LOG            = 0x0002
        FW_PACKET_FILE           = 0x0003
        FW_PACKET_PACKETIZED_TLM = 0x0004
        FW_PACKET_DP             = 0x0005
        FW_PACKET_IDLE           = 0x0006
        FW_PACKET_HAND           = 0x00FE
        FW_PACKET_UNKNOWN        = 0x00FF
        SPP_IDLE_PACKET          = 0x07FF
        INVALID_UNINITIALIZED    = 0x0800
    } default INVALID_UNINITIALIZED

    struct FrameContext {
        comQueueIndex: FwIndexType
        apid: Apid
        transmissionType: TransmissionType
        messageId: U16
        sequenceCount: U16
        vcId: U8
        pvn: Pvn
        sendNow: bool
        hasSecHdr: bool
    } default {
        comQueueIndex = 0
        apid = Apid.FW_PACKET_UNKNOWN
        sequenceCount = 0
        vcId = 1
        transmissionType = TransmissionType.TELEMETRY
        messageId = 0xFFFF
        pvn = Pvn.INVALID_UNINITIALIZED
        sendNow = false
        hasSecHdr = false
    }
}
```

Create `apps/my_app/my_config/config/CMakeLists.txt`:

```cmake
register_fprime_config(
    CONFIGURATION_OVERRIDES
        "${CMAKE_CURRENT_LIST_DIR}/ComCfg.fpp"
    INTERFACE
    DEPENDS
        FPrimeCfs_CfsBridge_Types
)
```

---

## Step 7 - Register the App in the Mission

### Add to targets.cmake

Edit `fprime_cfs_reference_defs/targets.cmake` and add your app to the global app list:

```cmake
list(APPEND MISSION_GLOBAL_APPLIST sample_app sample_lib fprime fprime_cfs fprime_app fprime_gds my_app)
```

### Add to the Startup Script

Edit `fprime_cfs_reference_defs/cpu1_cfe_es_startup.scr` to load your app at startup:

```
CFE_APP, my_app, MY_APP_Main, MY_APP, 25, 65536, 0x0, 0;
```

The fields are: object type, filename, entry point, cFS name, priority, stack size, load address, exception action.

### Add the Submodule (if separate repo)

If your app is a separate repository, add it as a submodule:

```bash
git submodule add https://github.com/your-org/my_app.git apps/my_app
```

Otherwise, develop it directly under `apps/`.

---

## Step 8 - Build and Test

### Build the Full System

```bash
source fprime-venv/bin/activate
make distclean
make SIMULATION=native prep
make
make install
```

### Run

```bash
cd build-artifacts/exe/cpu1
./core-cpu1
```

Watch the console output for your app's initialization messages.

### Connect the GDS

In a separate terminal:

```bash
source fprime-venv/bin/activate
fprime-gds \
    --ip-port 15010 \
    --dictionary ./build-artifacts/exe/Linux/my_app/dict/MyDeploymentTopologyDictionary.json \
    -n \
    --ip-client
```

> [!NOTE]
> The dictionary path depends on your deployment name. The build system generates it under `build-artifacts/exe/Linux/<app_name>/dict/`.

---

## Key Concepts

### CfsBridge Data Flow

The `CfsBridge` component handles the translation between cFS and F Prime:

**Uplink (commands from GDS):**
```
GDS -> TCP -> fprime_gds CfsBridge -> cFS SB -> fprime_app CfsBridge -> FprimeRouter -> CmdDisp
```

**Downlink (telemetry/events to GDS):**
```
Components -> TlmSend/Events -> ComQueue -> fprime_app CfsBridge -> cFS SB -> fprime_gds CfsBridge -> TCP -> GDS
```

### PollingTimer vs. Standard Timer

In a standard F Prime deployment, rate groups are typically driven by an OS-level timer (e.g., `LinuxTimer`). In the cFS integration, `PollingTimer` is used instead. It is a passive component that checks `std::chrono` timestamps when polled from the main loop (`timer.cycle()`). This cooperative approach avoids conflicts with cFS's own task scheduling.

### The Main Loop Pattern

The cFS + F Prime main loop pattern is:

```cpp
while (CFE_ES_RunLoop(&run_status) == true)
{
    timer.cycle();           // Drive rate groups if interval has elapsed
    cfsBridge.process();     // Drain F Prime queue + poll cFS SB for new messages
}
```

This differs from the standard cFS pattern (blocking `CFE_SB_ReceiveBuffer`) because F Prime needs to actively poll both its internal message queue and the cFS SB pipe.

### Adding Custom Components

Once the topology skeleton is in place, adding custom components follows standard F Prime practices:

1. Create the component FPP model
2. Generate implementation stubs with `fprime-util impl`
3. Implement the handlers
4. Add instances to `instances.fpp`
5. Wire them in `topology.fpp`
6. Rebuild with `make`

---

## Troubleshooting

### Build Errors: "fpp tools not found"

Install the F Prime Python dependencies:
```bash
pip install -r libs/fprime/requirements.txt
```

### Build Errors: Missing 32-bit Libraries

Install multilib support:
```bash
sudo apt-get install gcc-multilib g++-multilib
```

### Runtime: "No subscribers for MsgId"

This cFS message is informational and indicates that a message was published on the SB but no app has subscribed to it. It is normal during startup before all apps have initialized.

### Runtime: "Task Permissions" Warning

The F Prime OSAL cannot set task priorities without elevated permissions. This is cosmetic on development machines. To resolve:
```bash
sudo setcap 'cap_sys_nice=eip' build-artifacts/exe/cpu1/core-cpu1
```

### GDS Cannot Connect

Ensure:
1. The `fprime_gds` app is loaded in the startup script and running (check console output for "Listening for single client at 0.0.0.0:15010")
2. You are using `--ip-client` (the GDS connects to the server, not vice versa)
3. The port matches (default: 15010)

---

## Reference

- [F Prime Documentation](https://fprime.jpl.nasa.gov/)
- [cFS Documentation](https://github.com/nasa/cFS)
- [FPP Users Guide](https://nasa.github.io/fpp/fpp-users-guide.html)
- [fprime_cfs_reference Repository](https://github.com/fprime-community/fprime_cfs_reference)
- [fprime_cfs Library](https://github.com/fprime-community/fprime_cfs) - CfsBridge and PollingTimer components
