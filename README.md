# <img src="icon.png" width="36" alt=""> CSCAN - CS2 Economy Advisor

CSCAN tells you what to buy each round in Counter-Strike 2. It reads your money, side, score and gear through CS2's official Game State Integration and shows a buy recommendation that always fits your budget, with the reason behind it.

<img src="screenshot.png" width="420" alt="CSCAN showing $5,000 on T side in the buy phase and recommending a full buy: AK-47, Kevlar + Helmet and four grenades for $4,900">

## Download

**[Download CSCAN.exe](https://github.com/freddypaniagua/cscan-releases/releases/latest/download/CSCAN.exe)** (Windows 10/11, 64-bit)

That one file is the whole app. Put it anywhere and run it. There is nothing to unzip or install.

One thing to expect: the app is not code-signed yet, so Windows may show "Windows protected your PC". Click **More info**, then **Run anyway**.

Older versions are on the [releases page](https://github.com/freddypaniagua/cscan-releases/releases). Up to version 0.10 the download was a zip with a folder of files; from 0.11 it is the single exe. Each release also lists a "relink kit" zip. You do not need it to use CSCAN; it is there because of the license of Qt, the toolkit CSCAN is built with.

## Setup

1. Start CSCAN.
2. Click **INSTALL CONFIG**. This writes one small file into CS2's `cfg` folder that tells the game to send its data to CSCAN on your PC.
3. Restart CS2 and join a match. The status in the top bar turns green and says "Connected to CS2".

## What it does

- Shows your money, side, score and round state as they change.
- Recommends the kind of round (full buy, force buy, eco, anti-eco, pistol round) and an exact shopping list with prices.
- Explains each call with the numbers, for example "Save: $2,050 now + $1,900 loss bonus = $3,950 next round, enough for a full buy."
- Keeps your own numbers on screen while you are dead and watching a teammate.
- Shows the result when the match ends.
- Can draw the advice on top of the game, for one-monitor setups, anywhere on the screen you drag it (new in 0.10, see below).
- Tells you when a newer version is available.

## In-game overlay (new in 0.10)

If you play on one monitor, CSCAN can show the advice on top of the game, so you don't have to alt-tab in the buy phase.

1. Tick **Show overlay in game** at the bottom of the CSCAN window. It is off until you do.
2. Set CS2 to **Fullscreen Windowed** (CS2 settings > Video > Display Mode). In plain Fullscreen the game covers the overlay.
3. Put it where you want it with **MOVE** (see below).

A small see-through panel then appears in the buy phase with the decision, the shopping list and what you keep. It stays 10 seconds into the round, then fades out. At the end of a match it shows the result for a few seconds.

### Put it where you want: the MOVE button

<img src="overlay-move.png" width="244" align="right" alt="The overlay in move mode: a small dark panel tagged DRAG ME with sample advice, FULL BUY and four items with prices">

The overlay starts at the middle of the right edge. That spot can cover teammates or parts of the HUD, so move it once to a place that suits your screen, for example the gap between the radar and the player pictures at the top.

1. With CS2 running, alt-tab to CSCAN and click **ADVANCED**.
2. Under **IN-GAME OVERLAY**, press **MOVE**. The overlay appears, tagged "DRAG ME".
3. Drag it with the mouse to where you want it.
4. Press **DONE** (the same button). CSCAN remembers the spot.

Press DONE before you go back into the game: while MOVE is on, the overlay catches mouse clicks instead of letting them through. If the overlay is too big for the gap, set **Text size** to Small in the same place.

### Other settings

- **F8** shows or hides the overlay at any time, also in the middle of a round. While the overlay is on, CSCAN holds that key, so it no longer reaches the game. Pick another key, or **None**, under **ADVANCED > IN-GAME OVERLAY**.
- Opacity, text size and six fixed positions are in the same place. **PREVIEW** shows the panel for a few seconds.
- The overlay is a separate window on top of the game. It injects nothing into CS2, and it is built to let mouse clicks pass through to the game.
- Keep it off on FACEIT, ESEA and in tournaments until you have checked their rules on overlays.
- This is the overlay's first release. If it does not show over your game or gets in the way, untick the box and CSCAN works as before.

## Good to know

- **How it gets data:** Game State Integration, a feature Valve built for outside tools. CSCAN does not read or change the game's memory and injects nothing into the game.
- **Other platforms:** FACEIT, ESEA and tournaments have their own rules about helper tools. Check them before using CSCAN there.
- **Privacy:** nothing about your matches leaves your PC. On start, CSCAN asks GitHub once whether a newer version exists; you can turn that off under **ADVANCED > UPDATES AND ABOUT**.
- **Removing it:** delete `CSCAN.exe`, and delete `gamestate_integration_cscan.cfg` from CS2's `game/csgo/cfg` folder.
- **Early version:** this is version 0.11. Prices are checked by hand against the in-game buy menu and can lag behind a Valve balance update.

## About this repository

This repository holds the downloads only. CSCAN's source code is not public.

CSCAN is a fan-made tool. It is not affiliated with or endorsed by Valve. Counter-Strike is a trademark of Valve Corporation. Licenses for the software and fonts CSCAN is built with are inside the app, under **ADVANCED > ABOUT**.
