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

## Setting Up vcpkg

### Windows

```powershell
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
C:\vcpkg\bootstrap-vcpkg.bat
```

### Linux

```bash
git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
~/vcpkg/bootstrap-vcpkg.sh
```

---

## Setting the VCPKG_ROOT Environment Variable

This tells CMake where to find vcpkg. You only do this **once per machine**.

### Windows

Open PowerShell **as Administrator** and run:

```powershell
[System.Environment]::SetEnvironmentVariable("VCPKG_ROOT", "C:\vcpkg", "Machine")
```

Then **restart your terminal** and verify:

```powershell
echo $env:VCPKG_ROOT
# Should print: C:\vcpkg
```

### Linux

```bash
echo 'export VCPKG_ROOT="$HOME/vcpkg"' >> ~/.bashrc
source ~/.bashrc
```

Verify:

```bash
echo $VCPKG_ROOT
# Should print: /home/yourname/vcpkg
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

Both scripts will:
1. Configure the project (first time only, downloads and builds dependencies)
2. Compile the code
3. Run the executable

> **Note:** The first run takes a few minutes because vcpkg is downloading and compiling dependencies. Every run after that is fast — only changed files get recompiled.

---

## Day-to-Day Workflow

| Task | Windows | Linux |
|---|---|---|
| Build and run | `.\run.bat` | `./run.sh` |
| Just build | `cmake --build --preset default` | `cmake --build --preset default` |
| Clean build folder | `cmake -E remove_directory build` | `cmake -E remove_directory build` |
| Reconfigure (new deps) | `cmake --preset default` | `cmake --preset default` |

---

## Adding a New Package

1. Find the package name at https://vcpkg.io/en/packages
2. Add it to `vcpkg.json`:
```json
{
  "dependencies": [
    "existing-package",
    "new-package-name"
  ]
}
```
3. Add to `CMakeLists.txt`:
```cmake
find_package(new-package CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE new-package::new-package)
```
4. Run configure once to download it:
```bash
cmake --preset default
```
5. Back to normal: `./run.sh` or `.\run.bat`
