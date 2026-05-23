# DDEditor

[日本語](README_ja.md)

## Overview

DDEditor is a beatmap editor for the VR rhythm game "Dance Dash".

In addition to editing metadata such as song information and difficulty information, it allows users to place and adjust notes, long notes, obstacles, stream notes, and other elements in 2D and 3D views.  
It also supports creating beatmaps for songs with variable BPM.

> This tool is an unofficial personal project.  
> Dance Dash and its related rights belong to their respective owners.

## Features

- Music file playback
- Beatmap data loading
- Beatmap data editing
- Difficulty-based beatmap management
- Timing settings such as BPM and offset
- Beatmap creation for songs with variable BPM
- Beatmap preview in 2D / 3D views
- JSON-based data loading and saving

## Requirements

### To run the application

- Windows 10 / 11
- Environment supporting OpenGL 3.3 or later

### To build from source

- Windows 10 / 11
- Visual Studio 2022
- CMake 3.20 or later
- C++17 compatible compiler
- Environment supporting OpenGL 3.3 or later

## Download

The pre-built Windows version is available from GitHub Releases.

1. Open the Releases page of this repository.
2. Select the latest release.
3. Download the following zip file from Assets.

```text
DDEditor-v1.0.0-win64.zip
```

4. Extract the zip file to any folder.
5. Run `DDEditor.exe` inside the extracted folder.

## Build from Source

This repository includes the external libraries required for building under the `external/` directory.

In an environment where Visual Studio 2022 is installed, run the following commands.

```bash
cmake -S . -B out/build -A x64
cmake --build out/build --config Release
```

If you use Ninja, run the following commands in Visual Studio 2022 Developer PowerShell or Developer Command Prompt.

```bash
cmake -S . -B out/build/x64-Release -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build out/build/x64-Release
```

If you are using Visual Studio's CMake integration, you can also open this folder directly in Visual Studio 2022 and build the project from there.

## Tutorial

This section will be added later.

### 1. Open a project

<!-- TODO -->

### 2. Load a music file

<!-- TODO -->

### 3. Set BPM and offset

<!-- TODO -->

### 4. Set variable BPM timing

<!-- TODO -->

### 5. Edit beatmap data

<!-- TODO -->

### 6. Switch difficulty

<!-- TODO -->

### 7. Save beatmap data

<!-- TODO -->

## Notes

- This tool is an unofficial personal project.
- This repository does not include any non-redistributable music, images, or game data.
- Any included sample data is limited to redistributable materials.
- A Windows security warning may appear when launching the application for the first time.
- If the application does not work correctly, please check that the required resource folders exist in the same directory as the executable.

## License

Please see the `LICENSE.txt` file for the license of this project.

For external libraries, please refer to `THIRD_PARTY_NOTICES.txt` or the license information included with each library.
