# CSCAN — CS2 Economy Advisor

A Windows desktop app that watches your Counter-Strike 2 screen during a match, reads the HUD with OCR, and recommends what to buy each round (full buy, force, eco, save) based on your money, side, and score.

> **Status:** work in progress. Screen capture, the economy engine, and Tesseract OCR all run, but the HUD regions are hand-calibrated pixel coordinates and don't yet line up reliably across resolutions, so live in-game reading is still being tuned. Without Tesseract installed, the app falls back to a mock OCR mode for testing the rest of the pipeline.

## How it works

```
Screen capture (Qt, ~3 FPS)
   → OCR on HUD regions (Tesseract): money, score, buy timer
   → Game-state detection: buy phase, alive vs. spectating, CT/T side
   → Economy engine: buy decision + weapon/utility/armor picks
   → Qt6 UI with live preview and debug panel
```

- **Side detection** uses the shape of the CT/T emblem rather than HUD color, since players can recolor their HUD.
- **Spectator detection** (overlay bar + chat backup) stops the app from reading a teammate's money after you die.
- **Read-only:** CSCAN only looks at pixels on screen. It never reads or writes game memory.

## Background

CSCAN started as a browser-based economy helper ([CS2EconTool](https://github.com/CS2EconTool/CS2EconTool.github.io)) where players typed in their money and round results each round. Data entry ate roughly half of the ~20-second buy phase, so I rebuilt it as a desktop app that reads the screen automatically.

## Tech stack

C++17 · Qt6 (Widgets) · Tesseract 5 / Leptonica · CMake · vcpkg · Windows

## Building (Windows)

1. Install **Qt 6** (MSVC 2022 64-bit kit) and **Visual Studio 2022** with the C++ workload.
2. Optional, for real OCR: install Tesseract with vcpkg:
   ```
   vcpkg install tesseract:x64-windows
   ```
   and make sure `eng.traineddata` is in a `tessdata` folder (see `OCRProcessor.cpp` for the search paths).
3. Open the folder in Visual Studio (File → Open → Folder) and build the **x64-release** configuration. Qt and vcpkg paths are set in `CSCAN/CMakeLists.txt` and `CMakePresets.json`.
4. Copy the Qt and Tesseract DLLs next to `CSCAN.exe` (`windeployqt` handles the Qt ones).

## Project layout

```
CSCAN/
  CMakeLists.txt      build config
  main.cpp            entry point
  MainWindow.*        UI, debug panel, region adjustment
  ScreenCapture.*     timed screen grabs
  OCRProcessor.*      HUD region OCR + game-state detection
  EconomyEngine.*     weapon database and buy-decision logic
```

## Known issues / next steps

- HUD regions are fixed pixel rectangles; they should be stored as fractions of the frame size so they work at any resolution.
- Spectator detection thresholds need tuning against real gameplay.
- Planned: switch from OCR to CS2's Game State Integration (GSI) for exact data, and add an LLM coaching layer on top of the economy engine.
