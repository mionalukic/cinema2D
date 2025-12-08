# 🎬 Cinema2D – OpenGL 2D Cinema Simulation

**Cinema2D** is an interactive 2D cinema simulation built in C++ using **OpenGL 3.3**, **GLFW**, **GLEW**, and **stb_image**.  
The project visualizes a full cinema experience with animated seating, character movement, dynamic lighting, and event-driven behavior.

---

## ✨ Key Features

### 🎟️ Seat Interaction
- **Click**: `FREE → RESERVED → FREE`
- **Keys 1–9**: buy *n* adjacent available seats
- Seat states:
  - **FREE** – blue texture  
  - **RESERVED** – yellow texture  
  - **BOUGHT** – red texture  

---

### 🧍 Dynamic People Animation
Press **Enter** to open the door and trigger audience entry:

- variable walking speeds  
- start delays  
- horizontal randomness for natural movement  
- multi-stage movement:
  1. enter through the door  
  2. move vertically to the row  
  3. slide horizontally to the assigned seat  

---

### 🎥 Movie Playback
- screen color changes every 20 frames  
- random color effects  
- total duration: **20 seconds**  
- after the movie:
  - screen turns white  
  - people exit the cinema  
  - door opens automatically  

---

### 🚪 Exit Animation
Once the movie ends:

1. audience moves horizontally toward the aisle  
2. descends vertically to the door level  
3. exits through the door  

When all people exit, the simulation resets.

---

### 🖱️ Custom PNG Cursor
Cinema2D uses a PNG camera icon as a custom cursor via `glfwCreateCursor`.

---

### 🖼️ Textures
Loaded using **stb_image**, with:

- vertical flip correction  
- format detection (RGB/RGBA)  
- mipmap generation  
- linear filtering  

---

## 🧩 Project Structure
Cinema2D/
│── Bioskop1/
│ ├── Main.cpp
│ ├── Util.cpp
│ ├── Util.h
│ ├── basic.vert
│ ├── basic.frag
│ ├── res/ → PNG textures
│── .gitignore
│── Bioskop1.sln
└── README.md

---

## 🔧 Technologies Used

- **C++17**
- **OpenGL 3.3 Core**
- **GLFW**
- **GLEW**
- **stb_image**
- **GLSL shaders**

---

## 🛠️ Shader Overview

### 🎛️ Vertex Shader
Handles:

- door rotation around a pivot point  
- overlay vertical sliding (UI animation)  
- seat and character positioning using offsets  
- UV coordinate output for texture sampling  

### 🎨 Fragment Shader
Handles:

- textured rendering using `texture1`  
- pure color rendering using `chCol`  
- seat state texture logic controlled by `isSeat`  

