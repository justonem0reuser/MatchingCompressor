# Building MatchingCompressor

This document describes how to build the MatchingCompressor VST plugin from source.

## Prerequisites

### All Platforms
- **CMake** 3.22 or higher
- **C++17** compatible compiler
- **Git** (for fetching JUCE)

### macOS
- **Xcode Command Line Tools** or full Xcode
- macOS 10.13 or later recommended

```bash
xcode-select --install
brew install cmake  # if not already installed
```

### Linux (Ubuntu/Debian)
```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    git \
    libasound2-dev \
    libx11-dev \
    libxrandr-dev \
    libxinerama-dev \
    libxcursor-dev \
    libfreetype6-dev \
    libfontconfig1-dev \
    libgl1-mesa-dev \
    libwebkit2gtk-4.0-dev
```

### Linux (Fedora/RHEL)
```bash
sudo dnf install -y \
    gcc-c++ \
    cmake \
    git \
    alsa-lib-devel \
    libX11-devel \
    libXrandr-devel \
    libXinerama-devel \
    libXcursor-devel \
    freetype-devel \
    fontconfig-devel \
    mesa-libGL-devel \
    webkit2gtk3-devel
```

### Windows
- **Visual Studio 2019** or later with C++ workload
- **CMake** (can be installed via Visual Studio Installer)

## Building

### Quick Start (All Platforms)

```bash
# Clone the repository
git clone https://github.com/justonem0reuser/MatchingCompressor.git
cd MatchingCompressor

# Create build directory
mkdir build && cd build

# Configure (dependencies are fetched automatically)
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . --config Release
```

### Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_STANDALONE` | `ON` (local), `OFF` (CI) | Build standalone application |
| `VERSION` | `0.1.0` | Plugin version string |
| `CMAKE_BUILD_TYPE` | - | `Debug` or `Release` |

Example with options:
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_STANDALONE=ON -DVERSION=1.0.0
```

### Platform-Specific Notes

#### macOS
Builds produce:
- `MatchingCompressor.vst3` - VST3 plugin
- `MatchingCompressor.component` - Audio Unit plugin
- `MatchingCompressor.app` - Standalone application (if enabled)

```bash
# Parallel build using all cores
cmake --build . --config Release -- -j$(sysctl -n hw.ncpu)
```

#### Linux
Builds produce:
- `MatchingCompressor.vst3` - VST3 plugin
- `MatchingCompressor` - Standalone application (if enabled)

```bash
# Parallel build using all cores
cmake --build . --config Release -- -j$(nproc)
```

#### Windows
Builds produce:
- `MatchingCompressor.vst3` - VST3 plugin
- `MatchingCompressor.exe` - Standalone application (if enabled)

```powershell
# Using Visual Studio generator
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

## Build Artifacts

After a successful build, artifacts are located in:
```
build/MatchingCompressor_artefacts/Release/
├── VST3/
│   └── MatchingCompressor.vst3
├── AU/                          # macOS only
│   └── MatchingCompressor.component
├── Standalone/
│   └── MatchingCompressor.app   # or .exe on Windows
└── ...
```

## Installation

### macOS

```bash
# VST3
cp -r build/MatchingCompressor_artefacts/Release/VST3/MatchingCompressor.vst3 ~/Library/Audio/Plug-Ins/VST3/

# Audio Unit
cp -r build/MatchingCompressor_artefacts/Release/AU/MatchingCompressor.component ~/Library/Audio/Plug-Ins/Components/
```

### Linux

```bash
# VST3 (user install)
mkdir -p ~/.vst3
cp -r build/MatchingCompressor_artefacts/Release/VST3/MatchingCompressor.vst3 ~/.vst3/

# VST3 (system-wide)
sudo cp -r build/MatchingCompressor_artefacts/Release/VST3/MatchingCompressor.vst3 /usr/lib/vst3/
```

### Windows

Copy `MatchingCompressor.vst3` to one of:
- `C:\Program Files\Common Files\VST3\` (system-wide)
- `%LOCALAPPDATA%\Programs\Common\VST3\` (user install)

Or run the installer from the releases page.

## Troubleshooting

### CMake cannot find dependencies
Dependencies (JUCE, AlgLib) are fetched automatically via CMake's FetchContent. Ensure you have internet access during the first configure step.

### Linux: Missing development libraries
If you see errors about missing headers, install the development packages listed in Prerequisites.

### macOS: Code signing issues
For local development, the plugin is ad-hoc signed. For distribution, you'll need an Apple Developer certificate.

### Build is slow
The first build downloads JUCE (~200MB) and compiles it. Subsequent builds are faster. Use parallel builds:
```bash
cmake --build . --config Release -j8
```

## Dependencies

The following dependencies are automatically fetched during configuration:

| Dependency | Version | License |
|------------|---------|---------|
| [JUCE](https://juce.com/) | 8.0.1 | GPLv3 / Commercial |
| [AlgLib](https://www.alglib.net/) | 4.00.0 | GPLv2+ |
