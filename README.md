# ClanBomber — Nintendo Switch Homebrew Port

![Clanbomber Icon](icon.jpg)

A homebrew port of [**ClanBomber2**](https://github.com/viti95/ClanBomber2) — the SDL2-based fork of the classic [Bomberman-style multiplayer game](https://www.nongnu.org/clanbomber/) by Andreas Hundt and Denis Oliver Kropp — to the Nintendo Switch via the [devkitPro](https://devkitpro.org/) toolchain.

Builds a `clanbomber.nro` runnable from the **Homebrew Menu** on a Switch with custom firmware (Atmosphère) or in an emulator such as Ryujinx / suyu. An optional NSP forwarder makes the game launchable from the Switch's home menu like a regular title.

## Status

Playable. Works in emulator and on real hardware.

- ✅ Boot to main menu, intro animation, audio
- ✅ Local multiplayer match playable end-to-end
- ✅ JoyCon / Pro Controller input (D-Pad, left analog stick, A/B/X/Y, Plus, Minus)
- ✅ NRO icon, custom title metadata
- ✅ NSP forwarder support (built externally — see below)
- ✅ Quit-to-menu (Home Button)
- ✅ On-screen keyboard for player name entry
- ✅ Map Editor Support
- ❌ No online multiplayer (already removed in upstream)

## Prerequisites

1. **devkitPro** — install from <https://devkitpro.org/wiki/Getting_Started>. Default Windows path expected: `C:\devkitPro`.

2. **Switch SDL2 stack** via devkitPro's pacman:
   ```
   dkp-pacman -S switch-sdl2 switch-sdl2_image switch-sdl2_mixer switch-sdl2_ttf
   ```
   Pulls in transitive deps: `freetype`, `harfbuzz`, `libpng`, `libjpeg-turbo`, `libwebp`, `bzip2`, `zlib`, `libogg`, `libvorbisidec`, `libmodplug`, `mpg123`, `libopus`, `opusfile`, `flac`, `libdrm_nouveau`, `mesa`.

## Build

From a regular Windows `cmd` prompt at the repo root:

```
.\build.cmd
```

This sets `DEVKITPRO` / `DEVKITA64` / `PORTLIBS_PREFIX`, prepends the toolchain to `PATH`, and runs `make`. First invocation also stages the `romfs/` directory by copying assets (fonts, maps, pictures, sounds) from `upstream/src/` into it.

Output: **`clanbomber.nro`** (~13.5 MB with bundled assets).

To clean intermediate state:
```
.\build.cmd clean          REM removes build/ and *.nro
.\build.cmd clean-assets   REM removes romfs/
```

### Custom NRO icon

The Makefile auto-detects `icon.jpg` (256×256 JPEG) at the repo root and embeds it into the NRO's NACP. To rebrand: drop your own 256×256 JPEG named `icon.jpg` over the existing file and rebuild — no other change needed. The shipped `icon.jpg` is generated with AI.

### Asset overrides

Anything placed under `src/{fonts,maps,pics,wavs}/` is copied into the `romfs/` staging area **after** the upstream assets, so it wins. The Makefile's asset rule does two passes:

```make
# 1) upstream → romfs (only if newer, preserves originals)
cp -u upstream/src/fonts/*.ttf  romfs/fonts/
cp -u upstream/src/maps/*.map   romfs/maps/
cp -u upstream/src/pics/*.png   romfs/pics/
cp -u upstream/src/wavs/*.wav   romfs/wavs/
cp -u upstream/src/wavs/*.mod   romfs/wavs/

# 2) src/ overlay (force-overwrite, switch-port customisations)
cp -f src/fonts/*  romfs/fonts/
cp -f src/maps/*   romfs/maps/
cp -f src/pics/*   romfs/pics/
cp -f src/wavs/*   romfs/wavs/
```

Use cases:
- **Re-skin a sprite** — drop e.g. `src/pics/ps_controls.png` to replace the controller icon in Player Setup with a Switch Pro Controller graphic. The currently bundled override is a copy of the upstream PNG; replace it with your own artwork and rebuild.
- **Custom font** — `src/fonts/DejaVuSans-Bold.ttf` to swap the in-game font.
- **Bonus maps** — `src/maps/*.map` to ship custom maps inside the NRO's RomFS (alongside any user maps the player drops into `sdmc:/switch/clanbomber/maps/` at runtime).
- **New sound effects** — `src/wavs/*.wav` or `*.mod` to override SFX or background music.

Override files are tracked in git (so they're reproducible), unlike the generated `romfs/` directory which is regenerated each build and `.gitignore`'d.

## Deploy

### Emulator (Ryujinx / suyu)

File → Load Application From File → pick `clanbomber.nro`. Configure Player 1 input mapping to your keyboard or controller in the emulator settings.

### Real hardware (Switch with Atmosphère CFW)

Copy the NRO to the SD card:

```
sdmc:/switch/clanbomber/clanbomber.nro
```

Boot into Homebrew Menu (album icon → press R) and select ClanBomber.

### Optional: NSP forwarder (home-menu tile)

For a real home-menu tile that looks like an installed game, build an NSP forwarder externally. The simplest path is the web-based generator at <https://nsp-forwarder.n8.io/> — point it at `/switch/clanbomber/clanbomber.nro` and supply your console's `prod.keys`. Install the resulting NSP via Tinfoil / DBI / Awoo Installer.

The NSP is per-console (signed with your personal keys), so it isn't included in this repository.

## Controls

### Menu navigation
| Action | Input |
|---|---|
| Move selection | D-Pad or left analog stick |
| Confirm | A or Plus |
| Back | B |
| Toggle player on/off (Player Setup) | X |
| Toggle highlighting (Player Setup) | Y |
| Edit player name (Player Setup) | A → opens on-screen keyboard |

### In-game (per player using a JoyCon / Pro Controller)
| Action | Input |
|---|---|
| Move | D-Pad or left analog stick |
| Drop bomb | A or B |
| Quit to main menu | Minus |
| Continue at round-end | A |

### Map Selector (Options → Map Selection)
| Action | Input |
|---|---|
| Navigate list | D-Pad ↑ / ↓ |
| Switch theme | D-Pad ← / → |
| Toggle current map | Y |
| Select all maps | L |
| Select current map only | R |
| Back | B or Minus |

### Map Editor
The editor uses a palette model — pick a tile with ZL/ZR, then place it with A. Same for bomber spawns (cycle with X, place with Y).

**Selection screen:**
| Action | Input |
|---|---|
| Navigate list | D-Pad ↑ / ↓ |
| Edit selected map | A |
| Create new map (opens on-screen keyboard) | ZL |
| Delete current map | ZR |
| Back | B or Minus |

**Edit mode:**
| Action | Input |
|---|---|
| Move cursor | D-Pad or left analog stick |
| Place current tile | A |
| Erase (set hole) | B |
| Cycle bomber slot 1..8 | X |
| Place spawn at cursor (current slot) | Y |
| Previous tile in palette | ZL |
| Next tile in palette | ZR |
| Change author (on-screen keyboard) | L |
| Toggle Text / Normal mode | R |
| Help overlay | Plus |
| Save and exit | Minus |
| Player count −1 | L-Stick click |
| Player count +1 | R-Stick click |

`Plus` and `Minus` correspond to SDL `START` / `BACK` in the libnx HID mapping. Physical "A" is on the right (Nintendo convention) — confirm — and physical "B" is below — back. The translation lives in `src/switch_main.cpp` via an `SDL_AddEventWatch` that synthesises keyboard events for the existing menu loops; the Map Editor edit mode bypasses this and listens to raw `SDL_CONTROLLERBUTTONDOWN` events directly so the palette workflow doesn't collide with the global mapping.

The first page of the in-game Help screen (Main Menu → Help Screen) lists this entire mapping for reference at runtime.

## SD-card layout

Once launched, ClanBomber creates and uses:

```
sdmc:/switch/clanbomber/
├── clanbomber.nro            (the game binary you copied)
├── clanbomber.cfg            (player roster, settings)
├── clanbomber_net.cfg        (network config, mostly unused on Switch)
├── maps.disabled             (which maps are hidden, if any)
└── maps/                     (your own user maps, .map files)
```

Drop additional `.map` files into `maps/` — they show up in the Map Selector alongside the 33 maps bundled inside the NRO's RomFS.

## Repository layout

```
.
├── Makefile             Top-level Switch build rules (devkitPro)
├── build.cmd            Windows helper: sets DEVKITPRO and runs make
├── icon.ico, icon.jpg   App icon source + 256x256 build asset
├── LICENSE              GPLv2 license text
├── README.md            This file
├── src/
│   ├── config.h         Minimal autotools config.h replacement
│   ├── switch_main.cpp  Switch entry point: romfsInit, controller→key
│   │                    event watch, swkbd helper, hand-off to ClanBomber
│   └── pics/            Optional asset overrides — files placed here
│       └── ps_controls.png    overwrite the same-named upstream asset
│                              at build time (see "Asset overrides" below)
└── upstream/            ClanBomber2 sources (GPLv2+) with the patches
                         listed in "Upstream patches" applied directly
```

## Upstream patches

The Switch port modifies the following files inside `upstream/src/`:

| File | What changed |
|---|---|
| `ClanBomber.cpp` | Wrap `int main()` in `#ifndef __SWITCH__` so our own entry point provides `main`. Skip explicit `SDL_Quit()` / `TTF_Quit()` / `Mix_CloseAudio()` in `~ClanBomberApplication()` on Switch (kernel reclaims resources cleanly; explicit teardown crashes audren). Hide the "Quit Game" main-menu entry on Switch — exit happens via the Home button. Prepend a Switch-specific first page to the `show_tutorial()` Help screen listing the full gamepad mapping. |
| `UtilsGetHome.cpp` | Add `__SWITCH__` branch returning `sdmc:/switch` so the upstream `GetXxxHome() / "clanbomber"` concatenation lands at `sdmc:/switch/clanbomber/`. |
| `Map.cpp` | Switch-specific path for `maps.disabled` (drops the hidden-dot prefix that doesn't make sense on FAT32). |
| `PlayerSetup.cpp` | On entry, coerce inherited keyboard/mouse/out-of-range-joystick controller selections to `AI`. In the controller-cycling step, skip `KEYMAP_1..3`, `RCMOUSE`, and `JOYSTICK_N` slots that have no physically-attached pad (probed via `SDL_IsGameController`). Route `enter_name()` through the libnx software keyboard (`swkbd`) instead of `CB_EnterText()`. Switch-specific helper text ("Y ENABLES OR DISABLES A PLAYER, X TOGGLES HIGHLIGHTING"). |
| `Menu.cpp` | Accept `SDL_SCANCODE_BACKSPACE` in addition to `ESCAPE` for "back" in the main menu, so the synthesized B-button event leaves menus without also exiting matches. |
| `Credits.cpp` | Same BACKSPACE alongside ESCAPE handling for the credits screen. Add a "Switch Port" section listing **Mayo0x0** as port author. |
| `Controller_Joystick.{cpp,h}` | Re-implemented on top of `SDL_GameController` (named buttons / standard mapping) instead of raw `SDL_Joystick`. Movement reads left analog stick **or** D-Pad; bomb fires on physical **A or B**. Falls back to raw `SDL_Joystick` for devices SDL doesn't recognise. |
| `MapSelector.cpp` | Switch-specific helper text using the Y / L / R button labels. Existing `SDL_SCANCODE_SPACE`/`A`/`S` handlers stay — they're driven from the Switch buttons via the global event watch. |
| `MapEditor.{cpp,h}` | Add `current_tile_idx` and `current_bomber_slot` palette state. Switch-only edit-mode branch listens to raw `SDL_CONTROLLERBUTTONDOWN` + `SDL_CONTROLLERAXISMOTION` events for the palette workflow. `new_map()` and `get_new_author()` use the libnx `swkbd` instead of `CB_EnterText()`. Switch HUD shows the current tile + bomber slot (14pt font) directly above the existing bottom strip. Switch-specific `show_help()` page lists every editor binding. |
| `GameStatus.cpp` / `GameStatus_Team.cpp` | Accept `SDL_SCANCODE_RETURN` (Switch A / Plus) alongside `SPACE` for the round-end "continue" prompt, and display "PRESS A TO CONTINUE" on Switch. |

All modifications are guarded by `#ifdef __SWITCH__` where they would otherwise change behaviour for upstream Linux builds.

### Switch-specific text-entry helper

A single non-static helper in `src/switch_main.cpp` is shared across PlayerSetup and MapEditor:

```cpp
bool switch_swkbd_input(std::string &value, const char *guide_text, int max_len);
```

It opens the libnx system software keyboard (preset: default, with the current value pre-seeded), blocks until the user confirms or cancels, and writes the result back to `value`. Upstream files declare it via an `extern` inside an `#ifdef __SWITCH__` block — no extra header dependency.

## Known limitations

- Localization disabled (`-DENABLE_NLS=0`) — gettext isn't wired up against RomFS yet. UI is English.

## License

This port is a derivative work of **ClanBomber2** (GPLv2-or-later) by Andreas Hundt, Denis Oliver Kropp, René López, and the [`viti95/ClanBomber2`](https://github.com/viti95/ClanBomber2) maintainers. Per the GPL, the entire combined work — including the Switch-specific code in `src/` and the patches inside `upstream/` — is also distributed under **GPLv2-or-later**.

- Full license text: [`LICENSE`](LICENSE) at repo root (copy of upstream's `COPYING`).
- The bundled `DejaVuSans-Bold.ttf` font is licensed under the [DejaVu Fonts License](upstream/LICENSE.DEJAVU).

You are free to redistribute, modify, and republish under GPL terms. If you publish a derivative version, you must:

- Preserve the original copyright notices in modified source files
- Mark your changes (Git history is sufficient)
- Provide source code to recipients (a public Git repository satisfies this)
- License the combined work under GPLv2 (or any later GPL version)

## Credits

- **ClanBomber** original game: Andreas Hundt, Denis Oliver Kropp
- **ClanBomber2** SDL2 maintenance: René López and contributors
- **SDL2 fork** [`viti95/ClanBomber2`](https://github.com/viti95/ClanBomber2)
- **Switch port**: this repository
- **devkitPro** & **libnx** team — Switch homebrew toolchain
- **switchbrew** community — homebrew documentation
