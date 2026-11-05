# 3D Software Render Engine (Pure C++ / MiniFB)

A high-performance, single-threaded 3D Software Graphics Engine built from scratch in pure C++17, bypassing modern graphics APIs like OpenGL, Vulkan, or DirectX. 

The project features dynamic 3D rotations, perspective projection, poly-filling optimization, rasterization techniques, and real-time *Blinn-Phong lighting* (ambient, diffuse, and specular reflections).

---

### ✨ Features / Возможности
1. Pure Software Rasterization: Custom implementation of **Bresenham's line algorithm** for wireframes and polygon edge profiling.
2. Advanced Blinn-Phong Shading: Real-time computation of vertex and face normals to calculate realistic specular highlights (`shiny = 128.0f`) and diffuse light attenuation.
3. Custom Linear Algebra Pipeline: Built-in 3D vector maths (`Vector3D` cross products, normalization, dot products) and a 4x4 Matrix operations system (`Matrix_4x4`) for scalable PVM (Projection-View-Model) transformations.
4. Dynamic Camera & Light Source: Interactive viewport control. Move the camera orbitally, manipulate light power (`U`/`O` keys), and watch the dynamic shadow mapping scale across the custom grid horizon.
5. Lightweight Framebuffer: Utilizes `MiniFB` only as a fast, cross-platform bridge to display the image.

---

### 🛠️ Architecture & Math Engine / Техническое устройство

The rendering pipeline processes vectors mathematically in a strict linear sequence for every frame:
`[Local Space Setup] -> [Model-View Translation via Matrix] -> [Perspective Projection Correction] -> [Normal Mapping / Shading] -> [Polygon Edge Analysis & Fill] -> [Buffer Flush]`

✏️ Polygon Filling Algorithm: Uses structured multi-line edge tracking buffers (`left_x`, `right_x`, `left_light`, `right_light`) to scanline-fill triangles/quads while interpolating light gradients linearly.
🤔 W-Clipping Mitigation: Implements an implicit division block checking against zero/identity coordinates (`invW = 1 / w_out`) to guarantee seamless projection matrix calculation.
⚡ Performance: Highly optimized memory usage and cache locality. Runs entirely on **a single CPU core**, utilizing maximum `~5-16% CPU` overhead across Windows and Linux system schedules.

---

### ⌨️ Interactive Controls / Управление

> `▲ / ▼` — Rotate scene around the X-axis (Pitch / Ограничение по горизонту).
> `◀ / ▶` — Rotate scene around the Y-axis (Yaw).
> `J / L` — Orbital move of the light source around the object.
> `I / K` — Change light source vertical altitude angle.
> `U / O` — Increase / Decrease light power intensity.
> `+ / -` — Zoom camera closer or further away from the object center.

---

### 🚀 Compilation & Launch / Компиляция и запуск

Ensure you have `CMake` and a modern C++ compiler (`GCC`/`Clang`/`MinGW`) installed.

#### Linux
```bash
rm -rf build
cmake -S . -B build
cmake --build build
./3D_renderer
```

#### Windows (PowerShell / MinGW)
```powershell
Remove-Item -Recurse -Force build
cmake -G "MinGW Makefiles" -B build
cmake --build build
.\3D_renderer.exe
```

---

### 🌋 Future Roadmap
❓ Integration of a software **Z-Buffer** (Depth Buffer) to handle complex overlapping geometry or multi-object interactions.
❓ Expanding geometry formats to read and parse external `.obj` files natively.
❓ Implementing an optimized multi-threaded rasterizer split via openMP threads.

---

### 📄 License

This project is licensed under the **GNU GPLv3** — free to fork, hack, break, and explore.

> *Note from the author:* Suffer my genius code! Designed to be deeply analyzed by anyone wishing to build their own custom shapes or fall down the rabbit hole of 3D computer graphics algorithms. 📐✨

### ⚡ Performance (for those who stayed)
Только 1 ядро! / One core only!

Arch Linux (i5-12450H)
<img width="1920" height="1080" alt="output" src="https://github.com/user-attachments/assets/edc7a0a6-331b-4a41-a447-a0a2033276f8" />
(Max: ~16% CPU)

Windows (Ryzen 5 5000U) (GIF heavily compresses the quality; it's actually much smoother, trust.)
<img width="960" height="541" alt="win11_output" src="https://github.com/user-attachments/assets/0c481ad6-947d-43ba-a4db-1b9436607b53" />
(Max ~5% CPU)


