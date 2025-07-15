# System Monitor

A cross-platform system monitoring application built with C++, SDL2, and Dear ImGui. This application provides real-time information about your system's hardware and network activity.

## Features

*   **System Information:**
    *   Operating System
    *   Logged-in User
    *   Hostname
    *   Total number of running processes
    *   CPU Type
*   **Performance Monitoring (with graphical plots):**
    *   **CPU:** Real-time usage percentage.
    *   **Fan:** Status and current speed in RPM.
    *   **Thermal:** CPU temperature in Celsius.
    *   Interactive controls to pause plots, adjust FPS, and change the Y-axis scale.
*   **Memory and Process Management:**
    *   **Memory:** Visual progress bars for Physical (RAM), Virtual (Swap), and Disk usage.
    *   **Process Table:** A filterable and sortable table displaying running processes with details like:
        *   PID (Process ID)
        *   Name
        *   State
        *   CPU Usage (%)
        *   Memory Usage (KB)
        *   Multi-row selection for processes.
*   **Network Monitoring:**
    *   **Interfaces:** Lists all network interfaces with their corresponding IPv4 addresses.
    *   **Statistics:** Detailed tables for Receive (RX) and Transmit (TX) statistics, including bytes, packets, errors, drops, and more.
    *   **Usage Graphs:** Real-time graphs for RX and TX rates (in MB/s).
    *   Byte values are automatically formatted to be human-readable (B, KB, MB, GB).

## Dependencies

To build and run this project, you will need the SDL2 development library.

*   **Ubuntu/Debian:**
    ```bash
    sudo apt-get install libsdl2-dev
    ```
*   **macOS (using Homebrew):**
    ```bash
    brew install sdl2
    ```
*   **Windows (using MSYS2/MinGW):**
    ```bash
    pacman -S mingw-w64-x86_64-SDL2
    ```

## Building and Running

The project includes a `Makefile` that simplifies the build process across different platforms (Linux, macOS, and MinGW for Windows).

1.  **Clone the repository:**
    ```bash
    git clone <repository-url>
    cd system-monitor
    ```

2.  **Build the application:**
    Open a terminal in the project's root directory and run the `make` command.
    ```bash
    make
    ```
    This will compile the source files and create an executable named `monitor`.

3.  **Run the application:**
    ```bash
    ./monitor
    ```

4.  **Clean the build files:**
    To remove the compiled object files and the executable, run:
    ```bash
    make clean
    ```

## File Structure

*   `main.cpp`: The main entry point of the application. Handles window creation, the main loop, and rendering with Dear ImGui.
*   `system.cpp`: Contains the logic for fetching system-level information (OS, CPU, Fan, Thermal).
*   `mem.cpp`: Implements the functionality for monitoring memory usage and listing processes.
*   `network.cpp`: Handles the collection of network interface data and traffic statistics.
*   `header.h`: The main header file containing all necessary includes, struct definitions, and function prototypes.
*   `Makefile`: The build script for compiling the project.
*   `imgui/`: Contains the Dear ImGui library source code and backends for SDL2/OpenGL3.
