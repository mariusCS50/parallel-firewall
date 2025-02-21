# Parallel Firewall

This project implements a parallel firewall designed as a university assignment. It features a multithreaded design where a single producer thread generates packets and multiple consumer threads process them. The consumers use a shared circular buffer (ring buffer) to retrieve packets without busy waiting. The result of the processing is logged in a structured log file with PASS/DROP decisions.

## Features

- **Multithreading & Synchronization**
  - A single **producer** thread (implemented in [src/producer.c](src/producer.c)) inserts packets into a shared ring buffer.
  - Multiple **consumer** threads (implemented in [src/consumer.c](src/consumer.c)) process packets concurrently using proper notification mechanisms for synchronization.

- **Shared Data Structure**
  - The circular buffer is implemented in [src/ring_buffer.c](src/ring_buffer.c) and its header in [src/ring_buffer.h](src/ring_buffer.h), ensuring efficient packet distribution.

- **Serial & Parallel Implementations**
  - Besides the parallel firewall version, a serial implementation is provided in [src/serial.c](src/serial.c) for baseline testing and comparison.

- **Logging**
  - The results (including packet hash, processing decision, timestamps, etc.) are recorded via functions in the utilities ([utils/log/utils.h](utils/log/utils.h) and corresponding source files), ensuring a verifiable audit trail of the firewall operations.

- **Automated Building and Testing**
  - Build the project with a Makefile in the [src](src) directory.
  - Automated tests are provided in the [tests](tests) directory that generate test packets and compare outputs against a reference execution.
  - A checker script ([checker/checker.sh](checker/checker.sh)) and a Docker-based local testing environment (managed with [local.sh](local.sh)) ensure a consistent grading process.

## Building the Project

There are several ways to compile and run the firewall:

### Building the Firewall and Serial Implementations

1. Change to the `src` directory:

```bash
cd src
make
```

This will compile both the parallel firewall executable (typically named `firewall`) and the serial version of the application.

### Running Automated Tests

From the [`tests`](tests) directory you can run the grading and testing scripts:

```bash
cd tests
./grade.sh
```

This process runs test cases using different numbers of threads and validates both correctness and logging output.

### Docker Integration and the Checker

A Docker-based testing environment is provided to ensure a consistent build and test setup:

- **Build the Docker Image:**

```bash
./local.sh docker build
```

- **Run the Checker:**

```bash
./local.sh checker
```

The scripts in [`local.sh`](local.sh) and the Docker configuration (Dockerfile) automatically package the project and run the necessary tests and linters in a container.

## Implementation Details

- **Directory Structure:**
  - **[`src`](src):** Core implementation including:
    - producer.c and producer.h: Producer thread logic.
    - consumer.c and consumer.h: Consumer thread functions.
    - packet.c and packet.h: Packet structure and related utilities.
    - ring_buffer.c and ring_buffer.h: Circular buffer implementation.
    - serial.c: Serial firewall version for baseline comparison.
    - firewall.c: Contains components specific to the firewall execution.
  - **[`tests`](tests):** Automated tests and scripts like checker.py and grade.sh.
  - **[`checker`](checker):** High-level checker script (checker.sh) used by Docker and for local runs.
  - **local.sh:** Shell script managing Docker builds, testing, and image pushing.
  - **[`utils`](utils):** Utility functions for logging and debugging (e.g., utils/log/utils.h).

- **Synchronization & Efficiency:**
  Consumer threads are required not to use busy waiting. Instead, they must be notified of new packets arriving in the ring buffer. This design is a key part of the assignment that encourages proper synchronization primitives usage.

- **Testing & Validation:**
  The automated testing suite generates multiple test cases with varying packet sizes and thread configurations. The comparison between the serial and parallel implementations ensures that the firewall’s behavior remains consistent regardless of concurrency.

## Example Workflow

1. **Build the Project:**

	```bash
	cd src
	make
	```

2. **Run the Automated Tests:**

	```bash
	cd tests
	./grade.sh
	```

3. **Build and Run the Docker Checker:**

	```bash
	./local.sh docker build
	./local.sh checker
	```

4. **Fast Run Automated Tests and Grade (no linter, no prebuilds)**:
   ```bash
   cd tests
   make check
   ```