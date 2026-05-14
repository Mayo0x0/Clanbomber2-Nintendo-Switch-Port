/*
 * ClanBomber for Nintendo Switch — homebrew port entry point.
 *
 * Copyright (C) 2026 ClanBomber Switch port contributors.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <https://www.gnu.org/licenses/>.
 */

#include <switch.h>
#include <SDL2/SDL.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "ClanBomber.h"
#include "GameConfig.h"

extern ClanBomberApplication *app;

// ---------------------------------------------------------------------------
// libnx software keyboard helper. Called from PlayerSetup::enter_name on
// Switch (declared there via `extern`). The current value is seeded into
// the keyboard; on confirm the result is written back into `name`. Returns
// true if the user confirmed a non-empty string.
// ---------------------------------------------------------------------------
bool switch_swkbd_input(std::string &name, const char *guide_text, int max_len) {
    SwkbdConfig kbd;
    Result rc = swkbdCreate(&kbd, 0);
    if (R_FAILED(rc)) {
        return false;
    }

    swkbdConfigMakePresetDefault(&kbd);
    if (guide_text && guide_text[0]) {
        swkbdConfigSetGuideText(&kbd, guide_text);
    }
    if (!name.empty()) {
        swkbdConfigSetInitialText(&kbd, name.c_str());
    }
    if (max_len > 0) {
        swkbdConfigSetStringLenMax(&kbd, (u32)max_len);
    }

    char out[256] = {0};
    rc = swkbdShow(&kbd, out, sizeof(out));
    swkbdClose(&kbd);

    if (R_SUCCEEDED(rc) && out[0]) {
        name.assign(out);
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Synthetic key event helpers
// ---------------------------------------------------------------------------

static void push_synthetic_key(SDL_Scancode sc, bool down) {
    if (sc == SDL_SCANCODE_UNKNOWN) return;
    SDL_Event e;
    SDL_zero(e);
    e.type = down ? SDL_KEYDOWN : SDL_KEYUP;
    e.key.state = down ? SDL_PRESSED : SDL_RELEASED;
    e.key.repeat = 0;
    e.key.keysym.scancode = sc;
    e.key.keysym.sym = SDL_GetKeyFromScancode(sc);
    SDL_PushEvent(&e);
}

// SDL2 reports Switch face buttons by *position* (Xbox-style), not by Nintendo
// label. So the physical button labelled "A" on a Switch controller (the right
// face button) is reported as SDL_CONTROLLER_BUTTON_B, and the physical "B"
// (bottom) is reported as SDL_CONTROLLER_BUTTON_A. We swap the mapping
// accordingly so users get Nintendo-conventional behaviour: physical A confirms,
// physical B goes back.
//
// We deliberately do NOT translate the back-button to SDL_SCANCODE_ESCAPE here,
// because the in-game loop in ClanBomber.cpp also uses ESCAPE to leave the
// match. Instead we use SDL_SCANCODE_BACKSPACE for "menu back" — most upstream
// menus already accept both, and the ones that don't get patched alongside this
// file. Only the actual Minus button maps to ESCAPE, so it alone exits matches.
static SDL_Scancode controller_button_to_scancode(int button) {
    switch (button) {
        case SDL_CONTROLLER_BUTTON_DPAD_UP:    return SDL_SCANCODE_UP;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:  return SDL_SCANCODE_DOWN;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:  return SDL_SCANCODE_LEFT;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return SDL_SCANCODE_RIGHT;
        case SDL_CONTROLLER_BUTTON_B:          return SDL_SCANCODE_RETURN;     // physical A (Nintendo confirm)
        case SDL_CONTROLLER_BUTTON_START:      return SDL_SCANCODE_RETURN;     // Plus
        case SDL_CONTROLLER_BUTTON_A:          return SDL_SCANCODE_BACKSPACE;  // physical B (Nintendo back, no in-game effect)
        case SDL_CONTROLLER_BUTTON_BACK:       return SDL_SCANCODE_ESCAPE;     // Minus — exits in-game and menus
        case SDL_CONTROLLER_BUTTON_X:          return SDL_SCANCODE_SPACE;      // physical Switch Y — toggle in Player/Map Setup
        case SDL_CONTROLLER_BUTTON_Y:          return SDL_SCANCODE_H;          // physical Switch X — toggle highlighting in PlayerSetup
        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:  return SDL_SCANCODE_A;       // L — MapSelector: select all maps
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return SDL_SCANCODE_S;       // R — MapSelector: select current map only
        default: return SDL_SCANCODE_UNKNOWN;
    }
}

// ---------------------------------------------------------------------------
// Analog stick → arrow key translation
//
// SDL fires CONTROLLERAXISMOTION continuously while the stick is displaced.
// We track per-axis "direction state" (-1/0/+1) and only emit synthetic
// KEYDOWN/KEYUP events on transitions, which is what menu loops expect.
// ---------------------------------------------------------------------------

static const Sint16 AXIS_DEADZONE = 16384;
static const Sint16 TRIGGER_THRESHOLD = 16384;  // ZL/ZR press point
static int axis_x_state = 0;          // -1 = left, 0 = neutral, +1 = right
static int axis_y_state = 0;          // -1 = up,   0 = neutral, +1 = down
static int trigger_l_state = 0;       // 0 = released, 1 = pressed (ZL)
static int trigger_r_state = 0;       // 0 = released, 1 = pressed (ZR)

static void handle_axis_motion(int axis, Sint16 value) {
    if (axis == SDL_CONTROLLER_AXIS_LEFTX) {
        int new_state = 0;
        if (value >  AXIS_DEADZONE) new_state =  1;
        else if (value < -AXIS_DEADZONE) new_state = -1;
        if (new_state == axis_x_state) return;
        if (axis_x_state == -1) push_synthetic_key(SDL_SCANCODE_LEFT,  false);
        if (axis_x_state ==  1) push_synthetic_key(SDL_SCANCODE_RIGHT, false);
        if (new_state    == -1) push_synthetic_key(SDL_SCANCODE_LEFT,  true);
        if (new_state    ==  1) push_synthetic_key(SDL_SCANCODE_RIGHT, true);
        axis_x_state = new_state;
    } else if (axis == SDL_CONTROLLER_AXIS_LEFTY) {
        int new_state = 0;
        if (value >  AXIS_DEADZONE) new_state =  1;
        else if (value < -AXIS_DEADZONE) new_state = -1;
        if (new_state == axis_y_state) return;
        if (axis_y_state == -1) push_synthetic_key(SDL_SCANCODE_UP,   false);
        if (axis_y_state ==  1) push_synthetic_key(SDL_SCANCODE_DOWN, false);
        if (new_state    == -1) push_synthetic_key(SDL_SCANCODE_UP,   true);
        if (new_state    ==  1) push_synthetic_key(SDL_SCANCODE_DOWN, true);
        axis_y_state = new_state;
    } else if (axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT) {
        // ZL: synthesizes the N key — Map Editor "new map"
        int new_state = (value > TRIGGER_THRESHOLD) ? 1 : 0;
        if (new_state != trigger_l_state) {
            push_synthetic_key(SDL_SCANCODE_N, new_state == 1);
            trigger_l_state = new_state;
        }
    } else if (axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT) {
        // ZR: synthesizes the D key — Map Editor "delete map"
        int new_state = (value > TRIGGER_THRESHOLD) ? 1 : 0;
        if (new_state != trigger_r_state) {
            push_synthetic_key(SDL_SCANCODE_D, new_state == 1);
            trigger_r_state = new_state;
        }
    }
}

// ---------------------------------------------------------------------------
// SDL event watch: gamepad → keyboard
// ---------------------------------------------------------------------------

static int switch_event_watch(void* /*userdata*/, SDL_Event* event) {
    switch (event->type) {
        case SDL_CONTROLLERDEVICEADDED:
            SDL_GameControllerOpen(event->cdevice.which);
            break;
        case SDL_CONTROLLERBUTTONDOWN:
            push_synthetic_key(controller_button_to_scancode(event->cbutton.button), true);
            break;
        case SDL_CONTROLLERBUTTONUP:
            push_synthetic_key(controller_button_to_scancode(event->cbutton.button), false);
            break;
        case SDL_CONTROLLERAXISMOTION:
            handle_axis_motion(event->caxis.axis, event->caxis.value);
            break;
        default:
            break;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// Switch boot/teardown
// ---------------------------------------------------------------------------

static void switch_init() {
    Result rc = romfsInit();
    if (R_FAILED(rc)) {
        fprintf(stderr, "romfsInit failed: 0x%x\n", rc);
        std::exit(1);
    }

    // Bring up the GameController subsystem before ClanBomberApplication does
    // its own SDL_Init(VIDEO | AUDIO | JOYSTICK). SDL_Init is additive, so the
    // later call simply ORs in the missing subsystems.
    if (SDL_Init(SDL_INIT_GAMECONTROLLER) == 0) {
        SDL_AddEventWatch(switch_event_watch, nullptr);

        // Open every controller already connected at startup. The event watch
        // handles hot-plug additions afterwards.
        for (int i = 0; i < SDL_NumJoysticks(); i++) {
            if (SDL_IsGameController(i)) {
                SDL_GameControllerOpen(i);
            }
        }
    }

    Config::set_fullscreen(true);
    Config::set_renderDriver("opengles2");
}

static void switch_shutdown() {
    romfsExit();
}

int main(int argc, char **argv) {
    switch_init();

    app = new ClanBomberApplication();
    app->main();

    // Drop the event watch before any further teardown so a stray controller
    // event arriving while we wind down doesn't fire it against torn-down
    // state.
    SDL_DelEventWatch(switch_event_watch, nullptr);

    // We intentionally do NOT `delete app` on Switch. The
    // ClanBomberApplication destructor's explicit SDL_Quit() during applet
    // runtime tear-down crashes the OS (audren/HID ordering), and the kernel
    // reclaims all resources cleanly at process exit anyway.

    switch_shutdown();
    return 0;
}
