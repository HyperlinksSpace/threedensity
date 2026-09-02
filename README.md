# Three Density

**Three Density** is a cinematic third-person combat action game built with **Unreal Engine 5.7**. Fight through enemy waves, master combo attacks, and survive hazardous arenas in a polished UE5 experience.

## Download

**[Download the latest Windows build](https://github.com/HyperlinksSpace/threedensity/releases/latest/download/ThreeDensity-Win64.zip)**

Or visit the **[game page](https://hyperlinksspace.github.io/threedensity/)** for screenshots and install instructions.

## System Requirements

| | Minimum | Recommended |
|---|---|---|
| **OS** | Windows 10 64-bit | Windows 11 64-bit |
| **CPU** | Quad-core 2.5 GHz | 6-core 3.0 GHz |
| **RAM** | 8 GB | 16 GB |
| **GPU** | DirectX 12, 4 GB VRAM | DirectX 12, 8 GB VRAM |
| **Storage** | 4 GB | 4 GB SSD |

## Install & Play

1. Download `ThreeDensity-Win64.zip` from [Releases](https://github.com/HyperlinksSpace/threedensity/releases/latest).
2. Extract the ZIP to any folder.
3. Run `ThreeDensity/Binaries/Win64/threedensity.exe`.

## Controls

| Action | Keyboard | Gamepad |
|---|---|---|
| Move | W A S D | Left Stick |
| Look | Mouse | Right Stick |
| Jump | Space | A / Cross |
| Sprint | Left Shift | L3 |
| Attack | Left Mouse | RT |
| Combo / Heavy | Right Mouse | RB |
| Dodge | Left Ctrl | B / Circle |

## Features

- Third-person melee combat with combos and dodge mechanics
- AI enemies with State Tree behavior
- Lumen global illumination and Nanite-ready environment
- Checkpoint system and damageable interactables
- Gamepad and keyboard/mouse support

## Build from Source

Requires Unreal Engine 5.7.

```bash
# Generate project files (right-click threedensity.uproject → Generate VS project files)
# Or open directly in Unreal Editor 5.7

# Package for Windows (Shipping)
Engine/Build/BatchFiles/RunUAT.bat BuildCookRun \
  -project="path/to/threedensity.uproject" \
  -platform=Win64 -clientconfig=Shipping \
  -cook -build -stage -pak -archive
```

## License

Game code follows the Unreal Engine EULA. Template content © Epic Games, Inc.
