# Runtime-Adaptive Cache-Aware Merge Sort (RACAMS)

**RACAMS** is an adaptive sorting framework designed for real-time wireless network packet analysis. It dynamically adjusts its sorting and merging strategies based on runtime system conditions, with the goal of improving memory locality, reducing cache-related performance bottlenecks, and efficiently processing high-frequency data streams.

## Overview

Traditional sorting algorithms such as standard Merge Sort can experience performance degradation when processing large datasets that exceed CPU cache capacity. Frequent memory accesses and poor cache utilization can increase cache misses, memory stalls, and processing latency.

RACAMS addresses this limitation through a **runtime-adaptive and cache-aware approach**. The framework monitors system-level conditions and dynamically modifies its sorting behavior based on available memory and observed hardware performance characteristics.

The framework is integrated into a real-time wireless network packet analysis testbed, allowing captured packets to be continuously processed and sorted based on attributes such as timestamps, source/destination information, and packet size.

## Key Features

- Runtime-adaptive sorting
- Cache-aware processing
- Real-time packet analysis
- Scapy/Npcap integration
- C/C++ algorithm engine
- Python integration through ctypes
- Runtime resource monitoring

## Tech Stack (Significant Tools Used)

C/C++ • Python • Scapy • Npcap • MSYS2 • Ubuntu Linux •
ctypes • psutil • tracemalloc • Streamlit

## System Architecture

```text
Wireless Network Traffic
          │
          ▼
     Npcap / Scapy
          │
          ▼
   Packet Capture Layer
          │
          ▼
    Packet Processing
          │
          ▼
      RACAMS Engine
          │
    ┌─────┴─────┐
    │ Runtime   │
    │ Monitoring│
    └─────┬─────┘
          │
          ▼
 Cache / Memory-Aware
 Sorting & Merging
          │
          ▼
   Processed Packets
          │
          ▼
 Performance Analysis
```

## Technologies & Tools

| Technology / Tool   | Purpose                                                   |
| ------------------- | --------------------------------------------------------- |
| **C / C++**         | Implementation of the RACAMS sorting engine               |
| **Python**          | System integration, packet processing, and analysis       |
| **Scapy**           | Network packet sniffing and processing                    |
| **Npcap**           | Windows packet capture support                            |
| **MSYS2 x86**       | C/C++ development and compilation environment             |
| **Ubuntu Linux VM** | Linux-based testing and system-level performance analysis |
| **ctypes**          | Python-to-C/C++ library integration                       |
| **psutil**          | CPU, memory, and system resource monitoring               |
| **tracemalloc**     | Python memory allocation tracking                         |
| **Streamlit**       | Real-time analysis interface / testbed dashboard          |

## How It Works

RACAMS follows a runtime-adaptive approach rather than relying on a fixed sorting configuration.

1. **Packet Capture**
   Network packets are captured using Npcap and Scapy.

2. **Data Processing**
   Captured packets are converted into sortable data structures for processing.

3. **Runtime Monitoring**
   System information such as CPU and memory conditions is monitored during execution.

4. **Adaptive Sorting**
   RACAMS evaluates runtime conditions and adjusts its partitioning and merging behavior accordingly.

5. **Memory-Aware Processing**
   The algorithm prioritizes improved data locality to reduce unnecessary memory access and cache-related overhead.

6. **Performance Evaluation**
   RACAMS is benchmarked against conventional sorting approaches using execution, memory, throughput, and system-level performance measurements.

## Performance Analysis

The system was evaluated using multiple levels of testing:

* **Unit Testing** — Verification of individual algorithm components
* **Integration Testing** — Validation of interaction between the sorting engine, packet processing layer, and monitoring components
* **System Testing** — Evaluation of the complete real-time packet analysis workflow
* **Comparative Benchmarking** — Comparison between RACAMS and traditional sorting implementations

Performance monitoring utilized tools such as `psutil` and `tracemalloc`, while Linux-based testing provided a controlled environment for system-level performance analysis.

## Project Structure

```text
RACAMS/
├── src/                  # RACAMS algorithm implementation
├── python/               # Python integration and packet processing
├── tests/                # Unit and system tests
├── benchmarks/           # Performance benchmarking scripts/results
├── dashboard/            # Streamlit interface
├── docs/                 # Research documentation
├── requirements.txt      # Python dependencies
└── README.md
```

> The actual directory structure may vary depending on the implementation.

## Requirements

### Software

* Python 3.x
* C/C++ compiler
* MSYS2 x86 (Windows development)
* Ubuntu Linux environment / VM
* Npcap
* Git

### Python Libraries

```bash
pip install scapy psutil streamlit
```

`ctypes` and `tracemalloc` are included in the Python standard library.

## Running the Project

### 1. Clone the Repository

```bash
git clone https://github.com/your-username/RACAMS.git
cd RACAMS
```

### 2. Install Dependencies

```bash
pip install -r requirements.txt
```

### 3. Build the RACAMS Engine

Compile the C/C++ RACAMS implementation using the appropriate compiler environment.

For Windows development, MSYS2 x86 can be used to build the native library.

### 4. Run the Packet Analysis System

Launch the Python application or Streamlit dashboard according to the project's entry point.

Example:

```bash
streamlit run app.py
```

## Research Context

RACAMS was developed as part of an experimental-comparative research project investigating the effects of cache-aware and runtime-adaptive sorting on real-time data processing.

The primary research focus is not simply reducing sorting execution time, but improving the interaction between the sorting algorithm and the underlying memory hierarchy when processing large and continuously arriving datasets.

The framework was tested using a real-time wireless network packet analysis testbed to simulate data-intensive processing conditions.

## Limitations

* Performance characteristics depend on the underlying CPU architecture and cache hierarchy.
* Network packet analysis is performed within a controlled testbed environment.
* Hardware-level measurements may vary across systems.
* Results may differ depending on dataset size, system workload, and runtime conditions.

## Demo Video
[▶ Watch the Demo](https://youtu.be/Vd9Ayo23hPs)

## Future Improvements

* Support for additional CPU architectures
* More advanced hardware performance counter integration
* Automated hardware/cache profiling
* Multi-threaded sorting
* GPU-assisted sorting
* Expanded network traffic datasets
* Deployment on dedicated Linux hardware for further benchmarking

## Academic Project

**Runtime-Adaptive Cache-Aware Merge Sort Integrated in a Real-Time Wireless Network Packet Analysis**

Developed as an academic thesis project in **Computer Science**.

The project explores the intersection of:

* Algorithms and Data Structures
* Computer Architecture
* Cache and Memory Optimization
* Network Packet Processing
* Real-Time Systems
* System Performance Analysis

## Attribution

RACAMS was developed by Russell Mallari and 2 others as an academic research project
during A.Y. 2025–2026.

The public repository represents a portfolio-oriented version of the
research implementation. Certain research materials, source code, and implementation
details are intentionally excluded for intellectual property precautions.
