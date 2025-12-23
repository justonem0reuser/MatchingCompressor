# Building MatchingCompressor

There are two ways to build the MatchingCompressor VST/AU plugin from source: by using Cmake or Projucer (included in JUCE).
This document describes both of them.

## Building using Cmake

**Warning:** The CMake workflow fetches external dependencies outside the project directory (to `../third-party/`) to make them reusable for several projects. This makes the project not fully isolated. Changes to shared external dependencies may affect the build.

### Prerequisites

#### All Platforms
- **CMake** 3.22 or higher
- **C++17** compatible compiler
- **Git** (for fetching JUCE)

#### macOS
- **Xcode Command Line Tools** or full Xcode
- macOS 10.13 or later recommended

```bash
xcode-select --install
brew install cmake  # if not already installed
```

#### Linux (Ubuntu/Debian)
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

#### Linux (Fedora/RHEL)
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

#### Windows
- **Visual Studio 2019** or later with C++ workload
- **CMake** (can be installed via Visual Studio Installer)

### Building

#### Quick Start (All Platforms)

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

#### Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_STANDALONE` | `ON` (local), `OFF` (CI) | Build standalone application |
| `VERSION` | `0.1.0` | Plugin version string |
| `CMAKE_BUILD_TYPE` | - | `Debug` or `Release` |

Example with options:
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_STANDALONE=ON -DVERSION=1.0.0
```

#### Platform-Specific Notes

##### macOS
Builds produce:
- `MatchingCompressor.vst3` - VST3 plugin
- `MatchingCompressor.component` - Audio Unit plugin
- `MatchingCompressor.app` - Standalone application (if enabled)

```bash
# Parallel build using all cores
cmake --build . --config Release -- -j$(sysctl -n hw.ncpu)
```

##### Linux
Builds produce:
- `MatchingCompressor.vst3` - VST3 plugin
- `MatchingCompressor` - Standalone application (if enabled)

```bash
# Parallel build using all cores
cmake --build . --config Release -- -j$(nproc)
```

##### Windows
Builds produce:
- `MatchingCompressor.vst3` - VST3 plugin
- `MatchingCompressor.exe` - Standalone application (if enabled)

```powershell
# Using Visual Studio generator
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

## Building using Projucer (Windows/MacOS)

### Prerequisites
- **JUCE** with **Projucer** 8.0.0 or higher
- **AlgLib** 4.00.0
- **Visual Studio 2019** or later with C++ workload (for Windows)
- **XCode** (for MacOS)
- **Make** (for Linux)

### Building

- Clone the repository or download the source code.
- Open the `MatchingCompressor.jucer` file using the Projucer (the IDE tool for creating and managing JUCE projects).
- In `File → Global paths` window choose your OS (Windows or MacOS) and correct `Path to JUCE` and `JUCE modules` according to your JUCE directories.
- In `Settings → Header Search Path` enter the path name for your **Alglib** source files.
- Select the exporter that your need, press `Save and open in IDE` button (on the right of the `Selected exporter` combo-box).
- On Windows and MacOS: in your IDE, choose the required target: VST3, AU or Standalone.
- Build the solution using Visual Studio on Windows, XCode on MacOS or Make on Linux.


## Build Artifacts

After a successful build, artifacts location depends on the option you use.

### CMake

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

### Projucer + Visual Studio (Windows)

```
Builds/VisualStudio2022/x64/Release/ # or another Visual Studio version
├── VST3/
│   └── MatchingCompressor.vst3
├── Standalone Plugin/
│   └── MatchingCompressor.exe
└── ...
```

### Projucer + XCode (MacOS)

```
Builds/MacOSX/build/Release/ 
├── VST3/
│   └── MatchingCompressor.vst3
├── AU/
│   └── MatchingCompressor.component
├── MatchingCompressor.app
└── ...
```


## Installation

### Automatic Installation (CMake)

After building, install the plugins to standard system locations:

```bash
# Modern CMake style
cmake --install build/Release # or build/Debug

# Or traditional Unix style (from build/Release directory)
cd build/Release && make install # or build/Debug
```

This installs to the standard plugin directories:

| Platform | Plugin | Location |
|----------|--------|----------|
| macOS | AU | `~/Library/Audio/Plug-Ins/Components/` |
| macOS | VST3 | `~/Library/Audio/Plug-Ins/VST3/` |
| Linux | VST3 | `~/.vst3/` |
| Windows | VST3 | `C:\Program Files\Common Files\VST3\` |

### Manual Installation

Alternatively, you can copy the plugins manually.
The commands below are for the paths generated by CMake.
Use the correct paths depending on your build option. 

#### macOS
```bash
cp -r build/MatchingCompressor_artefacts/Release/AU/MatchingCompressor.component ~/Library/Audio/Plug-Ins/Components/
cp -r build/MatchingCompressor_artefacts/Release/VST3/MatchingCompressor.vst3 ~/Library/Audio/Plug-Ins/VST3/
```

#### Linux
```bash
mkdir -p ~/.vst3
cp -r build/MatchingCompressor_artefacts/Release/VST3/MatchingCompressor.vst3 ~/.vst3/
```

#### Windows
Copy `build/MatchingCompressor_artefacts/Release/VST3/MatchingCompressor.vst3` to `C:\Program Files\Common Files\VST3\`


## Troubleshooting

### CMake cannot find dependencies
Dependencies (JUCE, AlgLib) are fetched automatically. Ensure you have internet access during the first configure step.

### Projucer: The path to your JUCE folder is incorrect
In `File → Global paths` correct `Path to JUCE` and `JUCE modules` directory, then press `Re-scan JUCE Modules` button.
`Path to JUCE` should point the main JUCE directory (in which Projucer executed file is located) and `JUCE modules` should point to its `\modules` subfolder.
Use only absolute paths.

### Linux: Missing development libraries
If you see errors about missing headers, install the development packages listed in Prerequisites.

### macOS: Code signing issues
For local development, the plugin is ad-hoc signed. For distribution, you'll need an Apple Developer certificate.

### CMake build is slow
The first build downloads JUCE (~200MB) and compiles it. Subsequent builds are faster. Use parallel builds:
```bash
cmake --build . --config Release -j8
```

### Windows: Fails to install the built plugin
Ensure that you have the right to write to `C:\Program Files\Common Files\VST3\` directory (i.e., have Admin rights).
Alternatively, you can write to the per-user directory `C:\Users\<User>\Documents\VST3\`. Just ensure that your DAW reads this directory while scanning for VST plugins.


## Dependencies

To build using Projucer, you should download the following dependencies manually.
When building using CMake, these dependencies are automatically fetched during configuration.

| Dependency | Version | License |
|------------|---------|---------|
| [JUCE](https://juce.com/) | 8.0.12 | AGPLv3 / Commercial |
| [AlgLib](https://www.alglib.net/) | 4.00.0 | GPLv2+ |
