# Voxel Engine

A high-performance, GPU-driven voxle engine written in C++23 and using OpenGL 4.6, GLM, GLAD.

<p align="center">
  <img src="https://img.shields.io/badge/License-MIT-yellow.svg" alt="License">
  <img src="https://img.shields.io/badge/C%2B%2B-23-blue.svg" alt="C++23">
  <img src="https://img.shields.io/badge/OpenGL-4.6-8A2BE2.svg" alt="OpenGL 4.6">
  <img src="https://img.shields.io/badge/GLAD-Loader-orange.svg" alt="GLAD">
  <img src="https://img.shields.io/badge/GLM-Math%20Library-3DDC84.svg" alt="GLM">
  <img src="https://img.shields.io/badge/CMake-Build%20System-064F8C.svg" alt="CMake">
  <img src="https://img.shields.io/badge/Platform-Windows-blue.svg" alt="Windows">
</p>

<!-- TABLE OF CONTENTS -->
<details>
  <summary>Table of Contents</summary>
  <ol>
    <li>
      <a href="#overview">Overview</a>
    </li>
    <li>
      <a href="#getting-started">Getting Started</a>
      <ul>
        <li><a href="#prerequisites">Prerequisites</a></li>
        <li><a href="#installation">Installation</a></li>
        <li><a href="#build-options">Build Options</a></li>
      </ul>
    </li>
    <li><a href="#controls">Controls</a></li>
    <li><a href="#features">Features</a></li>
        <ul>
        <li><a href="#optimization-features">Optimization Features</a></li>
        <li><a href="#utility-features">Utility Features</a></li>
      </ul>
    <li><a href="#high-level-rendering-architecture">High-Level Rendering Architecture</a></li>
    <li><a href="#engine-configuration">Engine Configuration</a>
          <ul>
        <li><a href="#application-configuration">Application Configuration</a></li>
        <li><a href="#logging-configuration">Logging Configuration</a></li>
      </ul>
    </li>
    <li><a href="#usage">Usage</a></li>
      <ul>
        <li><a href="#world-generation-strategy">World Generation Strategy</a></li>
        <li><a href="#gizmos">Gizmos</a></li>
        <li><a href="#profiling">Profiling</a></li>
      </ul>
    <li><a href="#performancce-and-benchmarks">Performance and Benchmarks</a></li>
    <li><a href="#contributing">Contributing</a></li>
    <li><a href="#license">License</a></li>
    <li><a href="#contact">Contact</a></li>
    <li><a href="#acknowledgments">Acknowledgments</a></li>
  </ol>
</details>

## Overview

TODO

## Getting Started

### Prerequisites

- Compiler: C++23 capable compiler (GCC 11+, Clang 14+, MSVC 2019/2022 with C++23 support).

- Graphics: GPU and drivers supporting OpenGL 4.6.

- Libraries: GLM, GLAD, spdlog, stb (or use the bundled loader)

- Tools: CMake 3.20+, a build tool (make, ninja, or MSBuild).

### Installations

Clone the repository and set up a build directory:

```bash
git clone https://github.com/omar-owis/VoxelEngine.git
cd VoxelEngine && mkdir build && cd build
```

Configure the project with CMake:

```bash
cmake .. -DCMAKE_BUILD_TYPE=Release [BUILD OPTIONS]
```

### Build Options

The engine exposes several CMake options to customize logging, profiling, and chunk configuration. These can be passed via -D{option}={value} during configuration.
| Option | Type | Default | Allowed Values | Description |
| --- | --- | --- | --- | --- |
| **VE_BUILD_TEST** | `BOOL` | `OFF` | `ON`, `OFF` | Builds the Voxel Engine test suite. |
| **VE_LOGGING_ENABLED** | `BOOL` | `ON` | `ON`, `OFF` | Enables engine logging (spdlog). See usage section to configure logging |
| **VE_PROFILING_ENABLED** | `BOOL` | `ON` | `ON`, `OFF` | Enables profiling and Google Trace export. see usage section to use profiling |
| **VE_CHUNK_TYPE** | `STRING` | `CHUNK8` | `CHUNK8`, `CHUNK16`, `CHUNK32` | Selects the chunk size used by the engine. Adds one of: 8x8x8 Chunk Size, 16x16x16 Chunk Size, 32x32x32 Chunk Size. Invalid values produce a fatal CMake error. |

**Note:** CHUNK32 is not supported yet. The current compressed‑quad format used in chunk meshes cannot fit the required xyzwh data for each quad into the existing 2‑byte encoding. Work is underway to extend the compression scheme so that CHUNK32 can be fully supported in a future update.

## Controls

| Input                 | Action                                                                  |
| --------------------- | ----------------------------------------------------------------------- |
| **W / A / S / D**     | Move forward / left / backward / right                                  |
| **Space**             | Move up                                                                 |
| **Left Shift**        | Move down                                                               |
| **Mouse Movement**    | Look around (update camera direction)                                   |
| **Mouse Scroll**      | Adjust camera movement speed                                            |
| **Left Mouse Button** | Remove targeted voxel                                                   |
| **Escape**            | Close the application                                                   |
| **F1**                | Toggle profiling (Google Trace)                                         |
| **F2**                | Toggle draw mode (Fill ↔ Wireframe)                                     |
| **F3**                | Toggle chunk bounds visualization                                       |
| **Tab**               | Switch between Main Camera and isometric Camera; toggle frustum outline |

## Features

### Optimization Features

- **Binary Greedy Meshing**: Bitmask-based greedy quad generation that merges contiguous visible voxels per face into large binary-packed quads, drastically reducing triangle count and while preserving exact chunk silhouettes.

![Greedy Meshing Wireframe](../readme-assets/Demos/GreedyMeshWireframe.png)

- **VRAM Chunk Mesh Cache**: Lock-free, gpu lookups, FIFO Page replacement
- **AZDO rendering techinques**: Persistent Mapped Buffers, Multi-Draw Indirect, Direct State Access
- **Triple-buffering**: indirect command buffer and other rendering buffers split into three rotating regions so the CPU updates one segment while the GPU consumes the other two, eliminating write–read conflicts.
- **Lock-free mutlithreading**: Custom thread pool enabling concurrent chunk meshing and lock‑free uploads into the mesh cache,
- **GPU Frustum Culling pipeline**: Compute shader evaluates chunk visibility each frame, pushes visible cached meshes into the indirect draw buffer, and issues GPU-side meshing requests for uncached chunks to render on the next frame.

  ![GPU Frustum Culling Demo](../readme-assets/Demos/GPUFrustumCullingDemo.gif?raw=true)

- **Quad compression & Instancing**: Quads are packed into a 2‑byte format encoding their local XYZ position, width, height, face direction, and RGBA color
- **Face culling**: OpenGL face culling
- **3D ring buffer**: 3D ring‑buffer world streaming, that continuously loads/unloads chunk data around the camera, keeping nearby regions resident in RAM

![3D Ring Buffer](../readme-assets/Demos/3DRingBufferDemo.gif?raw=true)

### Utility Features

- **Gizmos**:
- **Logging**:
- **Profiling**:
- **Ray-casting**:
- **Voxel editing**:

## High-Level Rendering Architecture

![High-Level Rendering Pipeline](../readme-assets/EngineRenderingPipeline.png)

TODO text

## Engine Configuration

### Application Configuration

### Logging Configuration

## Usage

### World Generation Strategy

### Gizmos

### Profiling

## Performance and Benchmarks

TODO my pc specs (gpu, ram, processor) + os env

### Meshing

TODO speeds with different chunksize builds, quads per chunk, chunk generation strategy used, etc...

### Chunk Mesh Cache

TODO evicitions, uploads, page allocations, etc...

## License
