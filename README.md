# PULSE (Predictive Urban Logistics & Simulation Engine)

PULSE is a high-performance, GPU-accelerated urban traffic simulation framework. It enables detailed lane-level modeling of vehicles, with macro and microscopic views. It is integrated with a 3D visualization engine for interactive rendering and analysis.. The  main simulation core is in CUDA

---

## 📂 Repository Structure

```
📁 PULSE/
│
├── main.cpp                       # Entry point for the visualization client
├── imgui.ini                      # UI state from last run 
│
├── assets/                        # All static resources
│   ├── audios/                    # Reserved for future audio integration. added a sample file
│   ├── media/                     # UI/media files (e.g., logos, splash screens). have Added a sample file 
│   ├── models/                    # Blender models used in rendering. Not using For now
│   ├── shaders/                   # GLSL shader programs (OpenGL). All shaders are present here.
│   ├── textures/                  # Texture maps for objects/roads. Not using for now.
│   └── vendors/                   # Third-party files like ImGui
│
├── headers/                       # Project header files
│   ├── CUDA_SimulationHeaders/   # IDM, lane, and traffic modeling (CUDA) will be here. Currenlty only `idm.hpp` is implemented
│   ├── ServerHeaders/            # server/client logic headers are present here
│   └── Visualisation_Headers/    # Rendering pipeline components (camera, redering, OSM). All the essential custom openGL header files are present here and store the logic/code corrosponding to there name
│
├── server/                        # Networking and distributed simulation
│   ├── linux-server/src/         # Linux-side GPU server for simulation -> Implemented a stanAlone file which can be placed in server and run using the command: ` `
│   └── windows-client/src/       # Windows-side visualization client -> The function whihc are called to setup connections and send and recieve data from the client window
```
Other `.cpp` files are in the `src` directory

---

## 🔧 Setup Instructions
1. First of all, it will download aall the `vcpkg` packages and build them sequentially. This can take a lot of time (about 30-45min) and internet resource(1GB).
2. Change the location of Shaders and osm file path in the `main.cpp` file according to your device.
3. Use the `x64` Architecture option in the VS22.
4. Use the windows 10 Toolkit. (although it can handle win7 also).

### 📌 Requirements

- **C++17** or higher
- **CUDA Toolkit 11.x or higher** 
- **Visual Studio 2022** (for Windows dev)
- **Assimp**, **GLFW3**, **GLAD**, **ImGui**, **stb_image**, **Boost/Asio**, **libosmium**,

---

### 🚀 Build Instructions (Windows)

1. **Clone the repo:**: Use the Visual Studio's Repo clone feature and clone this repo: `https://github.com/vaurkhorov/pulse`
3. **Open `simulation.sln` in Visual Studio**
4. Check the setup instruction above.
5. **Build and Run** using `x64-Debug` or `x64-Release` mode

---

## 💡 Core Modules

### 🧠 CUDA Simulation Engine
- Intelligent Driver Model (IDM)
- Road parsing from `.osm` files

### 🛰️ Networking Layer
- client-server architecture
- Windows client sends visualization input
- Linux server runs CUDA simulation and sends back traffic state

### 🎮 3D Visualization Engine
- Built with OpenGL
- Draws roads, lanes, buildings from OSM
- ImGui-based interactive debugging UI
- Camera control, real-time rendering

---

## 🛠️ To-Do / Backlog

- [ ] Make the new Map editor and optimize it.
- [ ] Macro view implementation.
- [ ] Increase the number of simulating vehicles.
- [ ] Add a setup screen at the application initialisation

---

## 🤝 Contributing

We welcome contributions to make PULSE better!

### Guidelines:
- Follow C++17 conventions
- Use `camelCase` for variable names
- Avoid pushing DLLs or build artifacts
- Document new features in `README.md` and code comments
- Avoid pushing to main branch directly.
