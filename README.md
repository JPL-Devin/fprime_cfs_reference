# F Prime cFS Reference

A reference project showing how [F Prime (F')](https://github.com/nasa/fprime) can be used as the implementation architecture for [core Flight System (cFS)](https://github.com/nasa/cFE) applications. Each cFS app in this project is implemented using F Prime's component-based architecture, with F Prime providing the topology wiring, autocoded commanding/telemetry, and ground data system (GDS) while cFS provides the runtime executive, process management, and software bus.

> [!WARNING]
> This code is experimental and is not fit for use in any project. Check back shortly!

## Architecture

This project demonstrates F Prime as the implementation architecture for cFS applications. Each cFS app is built using an F Prime topology — components, ports, and autocoded commanding/telemetry — while the cFS runtime (cFE) provides process management, the software bus (SB), and system services.

The **CfsBridge** component (`libs/fprime_cfs/`) translates between the cFS software bus and F Prime's internal data flow, allowing F Prime topologies to send and receive messages on the SB. Two F Prime–implemented cFS apps are included:

### System Diagram

```
 cFS Runtime (core-cpu1)
+-------------------------------------------------------------------+
|  cFE Executive (ES, EVS, SB, TIME, TBL)                          |
|                                                                   |
|        cFS Software Bus (SB)                                      |
|        ════════════╦══════════════════════╗                        |
|                    ║                      ║                        |
|  +-----------------╨---------+  +---------╨-----------------+     |
|  |       fprime_app          |  |       fprime_gds          |     |
|  |  .......................  |  |  .......................   |     |
|  |  :   F Prime Topology :  |  |  :   F Prime Topology :   |     |
|  |  :                    :  |  |  :                    :   |     |
|  |  :  +------+  +-----+:  |  |  :  +-----------+    :   |     |
|  |  :  |CdhCor|  |Comp |:  |  |  :  |ComCcsdsNo |    :   |     |
|  |  :  |e Sub |  |Queue|:  |  |  :  |Router Sub |    :   |     |
|  |  :  +--+---+  +--+--+:  |  |  :  +-----+-----+    :   |     |
|  |  :     |          |   :  |  |  :        |          :   |     |
|  |  :  +--+----------+--+:  |  |  :  +-----+-----+    :   |     |
|  |  :  |   CfsBridge    |:  |  |  :  |  CfsBridge |    :   |     |
|  |  :  +-------+--------+:  |  |  :  +-----+------+    :   |     |
|  |  :..........│..........:  |  |  :........│...........:   |     |
|  |             │             |  |           │               |     |
|  +-------------│-------------+  +-----------│---------------+     |
|                │                            │                     |
|        ════════╩════════════════════════════╩═══════               |
|        cFS Software Bus (SB)                                      |
+-------------------------------------------------------------------+
                                                    │
                                              TCP :15010
                                                    │
                                              ┌─────┴─────┐
                                              │ F Prime   │
                                              │ GDS (Web) │
                                              └───────────┘
```

**fprime_app** is the primary F Prime application. It contains:

- A **CdhCore** subtopology (command dispatcher, event manager, health checker, telemetry)
- A **CfsBridge** that receives commands from the cFS SB and sends telemetry/events back
- A **ComQueue** for prioritized downlink of events and telemetry
- An **FprimeRouter** for routing deframed commands to the command dispatcher
- Rate groups driven by a **PollingTimer**

**fprime_gds** is a cFS app for testing with the F Prime GDS. It bridges cFS SB messages to the F Prime GDS over TCP:

- A **CfsBridge** that forwards F Prime telemetry/events from the cFS SB to the GDS
- A **ComCcsdsNoRouter** subtopology for CCSDS framing/deframing
- A **TcpServer** (Drv.TcpServer) listening on port 15010 for the F Prime GDS client

### Inside the F Prime App (fprime_app topology)

```
         ┌─────────────────────────────────────────────────────┐
         │                  FPrimeDeployment                   │
         │                                                     │
         │   ┌──────────┐    ┌──────────────┐                  │
         │   │  timer   │───>│rateGroupDrvr │                  │
         │   └──────────┘    └──────┬───────┘                  │
         │                         │                           │
         │                   ┌─────┴──────┐                    │
         │                   │ rateGroup1 │                    │
         │                   └──┬──┬──┬───┘                    │
         │                      │  │  │                        │
         │      ┌───────────────┘  │  └──────────────┐         │
         │      v                  v                  v         │
         │  ┌────────┐      ┌──────────┐       ┌────────┐      │
         │  │cmdDisp │      │ tlmSend  │       │ health │      │
         │  └───┬────┘      └────┬─────┘       └────────┘      │
         │      │                │                             │
         │      │          ┌─────┴──────┐                      │
         │      │          │  comQueue   │                      │
         │      │          └─────┬──────┘                      │
         │      │                │                             │
         │  ┌───┴────┐    ┌─────┴──────┐                      │
         │  │ router │    │  cfsBridge  │<── cFS SB (commands) │
         │  └───┬────┘    └─────┬──────┘──> cFS SB (telemetry)│
         │      │               │                              │
         │      └───────────────┘                              │
         │          (commands in, telemetry out)                │
         └─────────────────────────────────────────────────────┘
```

## Repository Structure

```
fprime_cfs_reference/
├── apps/                          # cFS applications (submodules)
│   ├── fprime_app/                # F Prime flight application (cFS app)
│   │   ├── FPrimeDeployment/      # F Prime deployment (topology + Main.cpp)
│   │   │   └── Top/               # Topology FPP files and C++ setup
│   │   ├── fprime_config/         # ComCfg.fpp (APID definitions, frame config)
│   │   └── fsw/                   # cFS-standard flight software source
│   ├── fprime_gds/                # F Prime GDS bridge application (cFS app)
│   │   └── GdsBridge/             # GDS bridge deployment (topology + Main.cpp)
│   │       └── Subtopologies/     # ComCcsdsNoRouter subtopology
│   ├── ci_lab/                    # cFS Command Ingest lab app
│   ├── to_lab/                    # cFS Telemetry Output lab app
│   ├── sch_lab/                   # cFS Scheduler lab app
│   └── sample_app/                # cFS Sample Application
├── cfe/                           # core Flight Executive (submodule)
├── libs/                          # Libraries
│   ├── fprime/                    # F Prime framework (submodule)
│   ├── fprime_cfs/                # F Prime ↔ cFS bridge library
│   │   └── FPrimeCfs/
│   │       ├── CfsBridge/         # CfsBridge component (SB ↔ F Prime)
│   │       └── PollingTimer/      # Polling-based rate group timer
│   └── sample_lib/                # cFS Sample Library
├── osal/                          # OS Abstraction Layer (submodule)
├── psp/                           # Platform Support Package (submodule)
├── tools/                         # cFS ground tools (submodules)
│   ├── cFS-GroundSystem/
│   ├── elf2cfetbl/
│   └── tblCRCTool/
├── fprime_cfs_reference_defs/     # Mission configuration
│   ├── targets.cmake              # CPU targets and app lists
│   ├── cpu1_cfe_es_startup.scr    # cFS startup script (which apps to load)
│   └── ...                        # Toolchain files, platform configs
├── Makefile                       # Top-level GNU make wrapper for CMake
├── do                             # Convenience: activate venv, build, install
├── do-clean                       # Convenience: clean rebuild from scratch
├── run                            # Convenience: build and run core-cpu1
└── gds                            # Convenience: launch F Prime GDS client
```

## Prerequisites

- **Linux** (tested on Ubuntu 22.04)
- **GCC** (with `g++` for C++ support)
- **CMake** >= 3.22
- **Make**
- **Python** >= 3.9
- **pip**

### Install System Dependencies (Ubuntu/Debian)

```bash
sudo apt-get update
sudo apt-get install -y cmake make gcc g++
```

## Building

### 1. Clone with Submodules

```bash
git clone --recurse-submodules https://github.com/fprime-community/fprime_cfs_reference.git
cd fprime_cfs_reference
```

### 2. Create a Python Virtual Environment

```bash
python3 -m venv fprime-venv
source fprime-venv/bin/activate
```

### 3. Install F Prime Tools

```bash
pip install -r libs/fprime/requirements.txt
```

This installs `fprime-tools`, `fprime-gds`, the FPP compiler toolchain, and all Python dependencies.

### 4. Configure the Build (Prep)

```bash
make SIMULATION=native prep
```

This runs CMake via the cFS build system to configure a native Linux build. The build artifacts are placed in `build-artifacts/`.

> [!NOTE]
> The `SIMULATION=native` flag tells cFS to build for the native Linux platform. This is required for running on your development machine.

### 5. Build

```bash
make
```

### 6. Install

```bash
make install
```

This copies the executables, shared libraries (`.so`), tables, and startup scripts into `build-artifacts/exe/`.

### Clean Rebuild

To perform a complete clean rebuild:

```bash
make distclean
make SIMULATION=native prep
make
make install
```

## Running

### Start the cFS Application

```bash
cd build-artifacts/exe/cpu1
./core-cpu1
```

This starts the cFS executive which loads the F Prime apps (`fprime_app` and `fprime_gds`) as dynamic shared libraries according to `cfe_es_startup.scr`.

You should see output indicating:
- cFE core services initializing (ES, EVS, SB, TIME, TBL)
- F Prime GDS App initializing and listening on port 15010
- F Prime App initializing with topology setup and rate group started

### Connect the F Prime GDS

In a separate terminal (with the virtual environment activated):

```bash
source fprime-venv/bin/activate
fprime-gds \
    --ip-port 15010 \
    --dictionary ./build-artifacts/exe/Linux/fprime_app/dict/FPrimeDeploymentTopologyDictionary.json \
    -n \
    --ip-client
```

Then open the GDS web interface at `http://localhost:5000` in your browser.

**Flags explained:**

| Flag | Purpose |
|------|---------|
| `--ip-port 15010` | Connect to the GDS bridge's TCP port |
| `--dictionary ...` | Path to the F Prime topology dictionary |
| `-n` | No launch of a flight binary (cFS is already running) |
| `--ip-client` | Connect as a TCP client to the GDS bridge server |

## Developer Commands

The cFS build system uses the top-level `Makefile`. All build, install, and test operations go through `make`:

| Command | Description |
|---------|-------------|
| `make SIMULATION=native prep` | Configure the CMake build for native Linux |
| `make` | Build all apps and libraries |
| `make install` | Install executables, `.so` files, tables, and startup scripts to `build-artifacts/exe/` |
| `make distclean` | Remove all build artifacts for a clean rebuild |

When developing F Prime components, `fprime-util` commands are available from within an F Prime app directory (e.g., `apps/fprime_app/FPrimeDeployment/`):

| Command | Description |
|---------|-------------|
| `fprime-util impl` | Generate implementation template files (`.cpp`/`.hpp`) from FPP models |
| `fprime-util impl --ut` | Generate unit test implementation templates |

> [!NOTE]
> Use the top-level `make` commands for building the full cFS + F Prime system. The `fprime-util` commands are useful for component development tasks like generating implementation stubs.

## Configuration

### cFS Startup Script

The file `fprime_cfs_reference_defs/cpu1_cfe_es_startup.scr` controls which apps are loaded at startup:

```
CFE_LIB, cfe_assert,  CFE_Assert_LibInit, ASSERT_LIB,    0,   0,     0x0, 0;
CFE_LIB, sample_lib,  SAMPLE_LIB_Init,    SAMPLE_LIB,    0,   0,     0x0, 0;
CFE_APP, fprime_gds,  FPRIME_GDS_Main,    FPRIME_GDS,   25,   16384, 0x0, 0;
CFE_APP, fprime_app,  FPRIME_APP_Main,    FPRIME_APP,   25,   65536, 0x0, 0;
```

Each line specifies: object type, filename, entry point, cFS name, priority, stack size, load address, exception action.

### Mission Targets

The file `fprime_cfs_reference_defs/targets.cmake` defines the mission configuration:

- `MISSION_GLOBAL_APPLIST`: All F Prime-related apps built for every target
- `MISSION_CPUNAMES`: The CPUs in the mission (default: `cpu1`)
- Per-CPU app lists and system toolchain selection

## License

See [LICENSE](LICENSE) for details.
