# 🏎️ FreeRTOS Prototype - Autonomous Car Simulation

Welcome to the **FreeRTOS Autonomous Car Prototype**. This project is a state-of-the-art simulation of an autonomous vehicle control system, integrating **FreeRTOS** for real-time task management and **OpenCV** with **YOLOv3-tiny** for real-time computer vision and object detection.

This prototype was developed to demonstrate the integration of RTOS deterministic behavior with heavy AI workloads in a POSIX (Linux) environment.

---

## 🚀 Getting Started

### 1. Prerequisites

Before running the project, ensure you have the following installed on your system (tested on Ubuntu/Debian):

#### **System Packages**
```bash
sudo apt update
sudo apt install -y build-essential gcc g++ gdb make pkg-config libopencv-dev
```

#### **Python (for AWS & Support Tools)**
```bash
sudo apt install -y python3 python3-pip
```

### 2. Cloning the Repository
Since this project uses multiple submodules for the FreeRTOS kernel and libraries, you **must** clone recursively:

```bash
git clone --recurse-submodules https://github.com/Ajeremias/FreeRTOS-prototipo.git
```

If you have already cloned it without submodules, run:
```bash
git submodule update --init --recursive
```

### 3. Building the Project
The main prototype is located in the `Posix_GCC` demo directory.

```bash
cd FreeRTOS/Demo/Posix_GCC
make clean
make
```

### 4. Running the Simulation
Execute the compiled binary from the same directory:

```bash
./build/rtos_car
```

> [!TIP]
> **Vision Requirement:** The simulation will attempt to open `/dev/video0` (Webcam). If no camera is found, it will automatically fallback to a simulated obstacle detection mode.

---

## 🛠️ Project Structure

- **`FreeRTOS/Demo/Posix_GCC`**: 📍 **Core Prototype**. Contains the main logic, OpenCV integration, and YOLO models.
- **`FreeRTOS/Source`**: The FreeRTOS Kernel.
- **`FreeRTOS-Plus`**: Supplementary libraries including TCP/IP and Trace suites.
- **`tools/`**: Automation scripts for AWS IoT setup and CMock testing.

---

## 🔌 VS Code Recommended Extensions

To have the best development experience, we recommend installing the following extensions:

1.  **C/C++ Extension Pack** (Microsoft) - For IntelliSense and debugging.
2.  **CMake Tools** - For advanced build management.
3.  **Python** - To run the scripts in the `tools` directory.
4.  **Cortex-Debug** - If you plan to port this to bare-metal ARM hardware.

---

## 🧠 Core Features

- **Real-Time Task Scheduling**: Separate tasks for Vision (High Priority), Control, and Monitoring.
- **Object Detection**: Uses YOLOv3-tiny to detect people and cars in real-time.
- **Safety Interlocks**: Automatic emergency braking when obstacles are detected.
- **POSIX Simulator**: Allows development and testing directly on Linux without dedicated hardware.

---

## 🤝 Forking & Contributing

1. **Fork** the project.
2. Create your **Feature Branch** (`git checkout -b feature/AmazingFeature`).
3. **Commit** your changes (`git commit -m 'Add some AmazingFeature'`).
4. **Push** to the branch (`git push origin feature/AmazingFeature`).
5. Open a **Pull Request**.

---

## 📜 License
This project is based on the FreeRTOS distribution and is licensed under the **MIT License**. See the [LICENSE.md](LICENSE.md) file for details.

---
*Developed by Ajeremias as part of the FreeRTOS Autonomous Systems Prototype series.*
