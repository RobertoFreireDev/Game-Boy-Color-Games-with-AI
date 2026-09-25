# Setup, build scripts and scaffolding

Read when: installing tools, recreating build/VS Code files, or scaffolding a missing engine (12.1). Section numbers match the index in CLAUDE.md. Section 1.3 (project structure) stays in CLAUDE.md.

## 1. Setup and tutorial (for the human)

### 1.1 Install (Windows)

1. **GBDK-2020**: download the latest `gbdk-win64.zip` from https://github.com/gbdk-2020/gbdk-2020/releases and extract it to `C:\gbdk` (so `C:\gbdk\bin\lcc.exe` exists). Then run once in a terminal:
   `setx GBDK_HOME C:\gbdk`
2. **Emulicious** (accurate emulator with tile/palette/memory viewers): download from https://emulicious.net, extract to `C:\Emulicious`, then:
   `setx EMULICIOUS C:\Emulicious\Emulicious.exe`
   (Emulicious needs Java; the site explains which download includes it.)
3. **Visual Studio Code** extensions: *C/C++* (`ms-vscode.cpptools`). Optional: *Emulicious Debugger* for stepping through C code (see its README; build with `-debug`).
4. Restart VS Code so it sees the new environment variables.

Linux/macOS: extract GBDK anywhere, `export GBDK_HOME=/path/to/gbdk`, and use `build.sh`.

### 1.2 Build and run

- **Build**: `Ctrl+Shift+B` in VS Code, or run `build.bat` in the project folder. Output: `build/game.gb`.
- **Run**: `Terminal → Run Task → Run` (builds, then opens Emulicious), or run `run.bat`.
- You can also drag `build/game.gb` onto any GBC emulator or flash it to a cartridge.

The ROM keeps the `.gb` extension; the header marks it as Color-only, which is what emulators check.

### 1.4 Build scripts (create exactly these)

**build.bat**
```bat
@echo off
setlocal enabledelayedexpansion
if "%GBDK_HOME%"=="" set "GBDK_HOME=C:\gbdk"
set "LCC=%GBDK_HOME%\bin\lcc.exe"
set "CFLAGS=-Isrc -Iassets"
set "LFLAGS=-Wm-yC -Wm-ynGAME -Wl-yt0x1B -Wl-ya1 -Wl-yoA -autobank"
if exist build rmdir /s /q build
mkdir build\obj
set "OBJS="
for /r src %%f in (*.c) do (
  "%LCC%" %CFLAGS% -c -o "build\obj\%%~nf.o" "%%f" || goto :fail
  set "OBJS=!OBJS! build\obj\%%~nf.o"
)
for /r assets %%f in (*.c) do (
  "%LCC%" %CFLAGS% -c -o "build\obj\%%~nf.o" "%%f" || goto :fail
  set "OBJS=!OBJS! build\obj\%%~nf.o"
)
"%LCC%" %LFLAGS% -o build\game.gb !OBJS! || goto :fail
echo BUILD OK: build\game.gb
exit /b 0
:fail
echo BUILD FAILED
exit /b 1
```

**build.sh**
```sh
#!/bin/sh
set -e
LCC="${GBDK_HOME:-$HOME/gbdk}/bin/lcc"
rm -rf build && mkdir -p build/obj
OBJS=""
for f in $(find src assets -name '*.c'); do
  o="build/obj/$(basename "${f%.c}").o"
  "$LCC" -Isrc -Iassets -c -o "$o" "$f"
  OBJS="$OBJS $o"
done
"$LCC" -Wm-yC -Wm-ynGAME -Wl-yt0x1B -Wl-ya1 -Wl-yoA -autobank -o build/game.gb $OBJS
echo "BUILD OK: build/game.gb"
```

**run.bat**
```bat
@echo off
if "%EMULICIOUS%"=="" set "EMULICIOUS=C:\Emulicious\Emulicious.exe"
start "" "%EMULICIOUS%" "%~dp0build\game.gb"
```

Flags: `-Wm-yC` Color-only · `-Wm-ynGAME` header title (max 11 chars, A–Z) · `-Wl-yt0x1B` MBC5+RAM+battery · `-Wl-ya1` one 8 KB save-RAM bank · `-Wl-yoA` automatic ROM size · `-autobank` place `#pragma bank 255` files automatically.

**.vscode/tasks.json**
```json
{
  "version": "2.0.0",
  "tasks": [
    { "label": "Build", "type": "shell",
      "command": "${workspaceFolder}/build.bat",
      "linux": { "command": "./build.sh" }, "osx": { "command": "./build.sh" },
      "group": { "kind": "build", "isDefault": true }, "problemMatcher": [] },
    { "label": "Run", "type": "shell",
      "command": "${workspaceFolder}/run.bat",
      "linux": { "command": "emulicious build/game.gb" },
      "osx": { "command": "emulicious build/game.gb" },
      "dependsOn": "Build", "problemMatcher": [] }
  ]
}
```

**.vscode/c_cpp_properties.json** (IntelliSense only; the real compiler is `lcc`)
```json
{
  "configurations": [{
    "name": "GBDK",
    "includePath": ["${env:GBDK_HOME}/include", "${workspaceFolder}/src", "${workspaceFolder}/assets"],
    "defines": ["__PORT_sm83", "__TARGET_gb", "__SDCC", "NONBANKED=", "BANKED=", "CRITICAL=",
                "__critical=", "__banked=", "__nonbanked=", "__at(x)=", "__sfr=", "__naked=",
                "__interrupt=", "__reentrant="],
    "cStandard": "c99", "intelliSenseMode": "gcc-x86"
  }],
  "version": 4
}
```

Add `build/` to `.gitignore`.

## 12. Checklists (scaffolding)

### 12.1 Scaffold order (only if the engine is missing)
1. `build.bat`, `build.sh`, `run.bat`, `.vscode/*`, `.gitignore`, `GAME.md`.
2. Engine in this order: `tiles.h`, `core`, `input`, `fade`, `gfx`, `sprites`, `text` (+ `font_main.c` with all 96 glyphs, box tiles and `sfx_menu`), `scene`, `audio`, `anim`, `map`, `collide`, `tween`, `particles`, `engine.h`.
3. `assets.h`, a title map, `scenes/title.c`, `main.c`. Build and fix.
4. Then the actual game.
