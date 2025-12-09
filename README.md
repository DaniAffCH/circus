# circus

<div align="center">
  <img src="assets/logo.png" alt="circus Logo" width="250"/>
</div>

**circus** is a multi-platform humanoid robot soccer simulator built on top of MuJoCo. It supports mixed teams of robots, enabling realistic and flexible RoboCup simulations. Designed for research and development, circus provides accurate physics, advanced robot models, and seamless integration across operating systems.

## Install

This project uses pixi to declare and reproduce the build/runtime dependency environment. See [pixi.toml](pixi.toml) for the dependency list and build configuration, and see [CMakeLists.txt](CMakeLists.txt) for the CMake build setup.

#### Overview
- pixi manages the environment and dependencies defined in [pixi.toml](pixi.toml).
- CMake is used to configure and build the native C++ targets defined in [CMakeLists.txt](CMakeLists.txt).
- The main executable target is `main` (entrypoint: [src/main.cpp](src/main.cpp)) and is placed in the build directory.

#### Prerequisites
- pixi installed (install following pixi project documentation).

### Build
From the project root run:
```
pixi build [-v]
```
This instructs pixi to create the environment specified in [pixi.toml](pixi.toml) and install declared host/development dependencies.

### Run
The pixi manifest declares a task named `main` that runs the python entrypoint (see [pixi.toml](pixi.toml) tasks). Run
```
pixi run main
```

### Links
- pixi configuration: [pixi.toml](pixi.toml)
- Build configuration: [CMakeLists.txt](CMakeLists.txt)
- C++ entrypoint: [src/main.cpp](src/main.cpp)
