# Project Setup Guide

## Prerequisites

### Windows

**1. Install Clang (LLVM)**
- Download the latest LLVM installer from https://github.com/llvm/llvm-project/releases
- Look for `LLVM-x.x.x-win64.exe`
- During install, select **"Add LLVM to the system PATH for all users"**
- Verify:
```powershell
clang --version
```

**2. Install CMake**
- Download from https://cmake.org/download — grab the `.msi` installer
- During install, select **"Add CMake to the system PATH for all users"**
- Verify:
```powershell
cmake --version
```

**3. Install Ninja**
- Download the latest `ninja-win.zip` from https://github.com/ninja-build/ninja/releases
- Extract `ninja.exe` and place it somewhere on your PATH, e.g. `C:\Tools\`
- Add `C:\Tools\` to your system PATH via System Properties → Environment Variables
- Verify:
```powershell
ninja --version
```

**4. Install Git**
- Download from https://git-scm.com/download/win and install with defaults
- Verify:
```powershell
git --version
```

---

### Linux (Ubuntu/Debian)

Install everything in one command:

```bash
sudo apt update
sudo apt install clang cmake ninja-build git
```

Verify:
```bash
clang --version
cmake --version
ninja --version
git --version
```

---
## Cloning the Project

```bash
git clone https://github.com/you/my-app
cd my-app
```

---

## Running the Project

### Install dependencies
```bash
make install
```

### Build and run
```bash
make run
```

### Clean build
```bash
make clean
```

> Note: After creating new cpp/or header files, must run `make install` to compile and link the files.

### Debugging
1. Install `lldb` via:
```bash
sudo apt insall lldb
```

2. Install VSCode Extension: `CodeLLDB` and `C/C++ extension`
> Hit `F5` to start debugging.

> **Note:** The first run takes a few minutes because vcpkg is downloading and compiling dependencies. Every run after that is fast — only changed files get recompiled.

---