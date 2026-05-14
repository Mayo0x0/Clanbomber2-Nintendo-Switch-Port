/*
 * This file is part of ClanBomber;
 * you can get it at "http://www.nongnu.org/clanbomber".
 *
 * Copyright (C) 1999-2004, 2007 Andreas Hundt, Denis Oliver Kropp
 * Copyright (C) 2009, 2010 Rene Lopez <rsl@members.fsf.org>
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
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include <unistd.h>

#include "ClanBomber.h"
#include "MapEditor.h"
#include "Utils.h"

#include "GameConfig.h"
#include "Map.h"

#ifdef __SWITCH__
#include "FontSDL.h"   // 14pt FontSDL instance for the compact editor HUD

// Tile palette cycled with ZL/ZR in the gamepad-driven Switch editor.
// `tile_char` is what MapEntry::set_data expects; `label` is the HUD text.
namespace {
    struct EditorTile {
        char tile_char;
        const char *label;
    };
    static const EditorTile EDITOR_TILE_PALETTE[] = {
        { ' ', "Ground"      },
        { '*', "Wall"        },
        { '+', "Box"         },
        { 'R', "Random Box"  },
        { 'S', "Ice"         },
        { 'o', "Bomb Trap"   },
        { '<', "Arrow Left"  },
        { '>', "Arrow Right" },
        { '^', "Arrow Up"    },
        { 'v', "Arrow Down"  },
    };
    static const int EDITOR_TILE_PALETTE_SIZE =
        sizeof(EDITOR_TILE_PALETTE) / sizeof(EditorTile);
}
#endif

MapEditor::MapEditor( ClanBomberApplication *_app )
{
    app = _app;

    map = new Map(app);

    current_map = 0;
    map_at_top = std::min( current_map-8, map->get_map_count()-16 );
    if (map_at_top < 0)
    {
        map_at_top = 0;
    }

    list_width = 230;
    for (int i=0; i<map->get_map_count(); i++)
    {
        std::string s = map->map_list[i]->get_name();

        int width;
        Resources::Font_small()->getSize(s, &width, NULL);

        if (width+10 > list_width)
        {
            list_width = width+40;
        }
    }

    cur_x = 0;
    cur_y = 0;
    text_editor_mode = false;
    current_tile_idx = 0;
    current_bomber_slot = 0;
}

MapEditor::~MapEditor()
{
    delete map;
}

void MapEditor::exec()
{
    draw_select_screen();

    while (1)
    {
        SDL_Event event;

        while(SDL_PollEvent(&event))
        {
            if (event.type == SDL_KEYDOWN)
            {
                switch(event.key.keysym.scancode)
                {
                case SDL_SCANCODE_BACKSPACE:
                case SDL_SCANCODE_ESCAPE:
                    return;
                    break;
                case SDL_SCANCODE_RETURN:
                    if (map->map_list[current_map]->is_writable())
                    {
                        Resources::Menu_clear()->play();
                        edit_map( current_map );
                    }
                    break;
                case SDL_SCANCODE_N:
                    Resources::Menu_clear()->play();
                    if ( new_map() )
                    {
                        edit_map( current_map );
                    }
                    break;
                case SDL_SCANCODE_D:
                    if (map->map_list[current_map]->is_writable())
                    {
                        Resources::Menu_clear()->play();
                        current_map = map->delete_entry( current_map );
                        map_at_top = std::min( current_map-8, map->get_map_count()-16 );
                    }
                    break;
                case SDL_SCANCODE_UP:
                    if (current_map > 0)
                    {
                        current_map--;
                        map_at_top = std::min( current_map-8, map->get_map_count()-16 );
                        if (map_at_top < 0)
                        {
                            map_at_top = 0;
                        }
                        Resources::Menu_break()->play();
                    }
                    break;
                case SDL_SCANCODE_DOWN:
                    if (current_map < map->get_map_count()-1)
                    {
                        current_map++;
                        map_at_top = std::min( current_map-8, map->get_map_count()-16 );
                        if (map_at_top < 0)
                        {
                            map_at_top = 0;
                        }
                        Resources::Menu_break()->play();
                    }
                    break;
                }
            }
            draw_select_screen();
        }
    }
}

void MapEditor::draw_select_screen(bool flip)
{
    Resources::MapEditor_background()->blit(0, 0);

    const int ml_height = 116;
    // map list background
    CB_FillRect(45, ml_height, list_width, 400, 75, 75, 75, 128);

    // highlight selected map name
    CB_FillRect(45, ml_height + (current_map - map_at_top) * 25, list_width, 25,
                25, 100, 200, 128);

    // show up to twenty map names
    for (int i=0; i < std::min(16, map->get_map_count()); i++)
    {
        // writable ?
        if (!map->map_list[i+map_at_top]->is_writable())
        {
            CB_FillRect(45, ml_height + i * 25, list_width, 25, 255, 25, 25, 100);
        }

        // show name
        std::string s(map->map_list[i+map_at_top]->get_name());
        Resources::Font_small()->render(s, 60, 118 + i * 25,
                                        cbe::FontAlignment_0topleft);
    }

    // show scroll indicators
    if (map_at_top > 0)
    {
        Resources::Font_big()->render("+", 165, 85, cbe::FontAlignment_0topcenter);
    }
    if (map_at_top < map->get_map_count()-16)
    {
        Resources::Font_big()->render("-", 165, 516, cbe::FontAlignment_0topcenter);
    }

#ifdef __SWITCH__
    Resources::Font_big()->render(_("Select a map and press A"), 520,
                                  150, cbe::FontAlignment_0topcenter);
    Resources::Font_big()->render(_("Maps marked red are readonly"), 520, 190,
                                  cbe::FontAlignment_0topcenter);
    Resources::Font_big()->render(_("Press ZL to create a new map"), 520, 250,
                                  cbe::FontAlignment_0topcenter);
    Resources::Font_big()->render(_("Press ZR to delete a map"), 520, 290,
                                  cbe::FontAlignment_0topcenter);
#else
    Resources::Font_big()->render(_("Select a map to edit and press Enter"), 520,
                                  150, cbe::FontAlignment_0topcenter);
    Resources::Font_big()->render(_("Maps marked red are readonly"), 520, 190,
                                  cbe::FontAlignment_0topcenter);
    Resources::Font_big()->render(_("Press N to create a new map"), 520, 250,
                                  cbe::FontAlignment_0topcenter);
    Resources::Font_big()->render(_("Press D to delete a map"), 520, 290,
                                  cbe::FontAlignment_0topcenter);
#endif
    if(flip)
    {
        CB_Flip();
    }
}

bool MapEditor::new_map()
{
#ifdef __SWITCH__
    // Use the libnx software keyboard — Switch has no physical keyboard so
    // CB_EnterText() can't accept letters. swkbdShow is a blocking system
    // overlay, so we skip the animated entry-field render loop entirely.
    extern bool switch_swkbd_input(std::string &, const char *, int);

    std::string new_string;
    if (switch_swkbd_input(new_string, "Enter map name", 20))
    {
        if (new_string.length())
        {
            current_map = map->new_entry(new_string);
            map_at_top = std::min(current_map - 8, map->get_map_count() - 16);
            return true;
        }
    }
    return false;
#else
    std::string new_string;

    while (1)
    {
        draw_select_screen(false);

        CB_FillRect(200, 300, 400, 100, 0, 0, 0, 128);

        std::string name = _("Name: ") + new_string;
        Resources::Font_big()->render(name, 230, 330, cbe::FontAlignment_0topleft);

        Resources::Font_small()->render(_("PRESS ESC TO ABORT"), 400, 380,
                                        cbe::FontAlignment_0topcenter);

        CB_Flip();

        switch(CB_EnterText( new_string ))
        {
        case 1:
            if (new_string.length())
            {
                current_map = map->new_entry( new_string );
                map_at_top = std::min( current_map-8, map->get_map_count()-16 );
                return true;
            }
            return false;
        case -1:
            return false;
        }
    }
    // impossible to reach this
#endif
}

void MapEditor::edit_map( int number )
{
    MapEntry *entry = map->map_list[number];
    map->load( number );

    draw_editor();
    CB_Flip();

#ifdef __SWITCH__
    // Switch port: gamepad-driven editor. We listen to raw
    // SDL_CONTROLLERBUTTONDOWN and SDL_CONTROLLERAXISMOTION events directly
    // and ignore the synthetic SDL_KEYDOWN events from switch_main.cpp's
    // global event watch — except for the arrow scancodes, which conveniently
    // unify D-Pad and left-analog-stick cursor moves.
    while (1)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            bool maptile_placed = false;

            if (event.type == SDL_KEYDOWN)
            {
                switch (event.key.keysym.scancode)
                {
                case SDL_SCANCODE_LEFT:
                    Resources::Menu_break()->play();
                    cur_x--;
                    break;
                case SDL_SCANCODE_RIGHT:
                    Resources::Menu_break()->play();
                    cur_x++;
                    break;
                case SDL_SCANCODE_UP:
                    Resources::Menu_break()->play();
                    cur_y--;
                    break;
                case SDL_SCANCODE_DOWN:
                    Resources::Menu_break()->play();
                    cur_y++;
                    break;
                default:
                    break;
                }
                clip_cursor();
            }
            else if (event.type == SDL_CONTROLLERBUTTONDOWN)
            {
                switch (event.cbutton.button)
                {
                case SDL_CONTROLLER_BUTTON_B:
                    // physical Switch A — place current palette tile
                    Resources::Menu_clear()->play();
                    entry->set_data(cur_x, cur_y,
                                    EDITOR_TILE_PALETTE[current_tile_idx].tile_char);
                    map->reload();
                    maptile_placed = true;
                    break;
                case SDL_CONTROLLER_BUTTON_A:
                    // physical Switch B — erase (set hole)
                    Resources::Menu_clear()->play();
                    entry->set_data(cur_x, cur_y, '-');
                    map->reload();
                    break;
                case SDL_CONTROLLER_BUTTON_Y:
                    // physical Switch X — cycle bomber slot 1..8
                    Resources::Menu_break()->play();
                    current_bomber_slot = (current_bomber_slot + 1) % 8;
                    break;
                case SDL_CONTROLLER_BUTTON_X:
                    // physical Switch Y — place current bomber spawn at cursor
                    Resources::Menu_clear()->play();
                    entry->set_bomber_pos(cur_x, cur_y, current_bomber_slot);
                    map->reload();
                    break;
                case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
                    // L — change author (swkbd)
                    Resources::Menu_break()->play();
                    entry->set_author(get_new_author());
                    break;
                case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
                    // R — toggle Text / Normal editor mode
                    Resources::Menu_clear()->play();
                    text_editor_mode = !text_editor_mode;
                    break;
                case SDL_CONTROLLER_BUTTON_START:
                    // Plus — show help screen
                    show_help();
                    break;
                case SDL_CONTROLLER_BUTTON_BACK:
                    // Minus — save and exit
                    entry->write_back();
                    return;
                case SDL_CONTROLLER_BUTTON_LEFTSTICK:
                    // L-Stick click — decrement player count
                    Resources::Menu_clear()->play();
                    entry->set_max_players(entry->get_max_players() - 1);
                    map->reload();
                    break;
                case SDL_CONTROLLER_BUTTON_RIGHTSTICK:
                    // R-Stick click — increment player count
                    Resources::Menu_clear()->play();
                    entry->set_max_players(entry->get_max_players() + 1);
                    map->reload();
                    break;
                default:
                    break;
                }

                if (maptile_placed && text_editor_mode)
                {
                    cur_x++;
                    clip_cursor();
                }
            }
            else if (event.type == SDL_CONTROLLERAXISMOTION)
            {
                // ZL / ZR cycle the tile palette. Edge-detect via static
                // state so the press only fires once per pull.
                static int zl_state = 0;
                static int zr_state = 0;
                const Sint16 threshold = 16384;

                if (event.caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT)
                {
                    int new_state = (event.caxis.value > threshold) ? 1 : 0;
                    if (new_state == 1 && zl_state == 0)
                    {
                        current_tile_idx = (current_tile_idx
                                            + EDITOR_TILE_PALETTE_SIZE - 1)
                                           % EDITOR_TILE_PALETTE_SIZE;
                        Resources::Menu_break()->play();
                    }
                    zl_state = new_state;
                }
                else if (event.caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT)
                {
                    int new_state = (event.caxis.value > threshold) ? 1 : 0;
                    if (new_state == 1 && zr_state == 0)
                    {
                        current_tile_idx = (current_tile_idx + 1)
                                           % EDITOR_TILE_PALETTE_SIZE;
                        Resources::Menu_break()->play();
                    }
                    zr_state = new_state;
                }
            }

            draw_editor();
            CB_Flip();
        }
    }
#else
    while (1)
    {
        SDL_Event event;

        while(SDL_PollEvent(&event))
        {
            if(event.type == SDL_KEYUP)
            {
                switch (event.key.keysym.scancode)
                {
                case SDL_SCANCODE_ESCAPE:
                    entry->write_back();
                    return;
                    break;
                case SDL_SCANCODE_F1: // help screen
                    show_help();
                    break;
                case SDL_SCANCODE_F2: // editor_mode
                    text_editor_mode = !text_editor_mode;
                    Resources::Menu_clear()->play();
                    break;
                default:
                    break;
                }
            }
            else if (event.type == SDL_KEYDOWN)
            {
                bool maptile_set = true;

                switch (event.key.keysym.scancode)
                {
                case SDL_SCANCODE_BACKSPACE:
                    maptile_set = false;
                    Resources::Menu_clear()->play();
                    cur_x--;
                    clip_cursor();
                    entry->set_data(cur_x,cur_y,'-');
                    map->reload();
                    break;
                case SDL_SCANCODE_PAGEUP: // increase number of players
                    maptile_set = false;
                    Resources::Menu_clear()->play();
                    entry->set_max_players(entry->get_max_players()+1);
                    map->reload();
                    break;
                case SDL_SCANCODE_PAGEDOWN: // decrease number of players
                    maptile_set = false;
                    Resources::Menu_clear()->play();
                    entry->set_max_players(entry->get_max_players()-1);
                    map->reload();
                    break;
                case SDL_SCANCODE_SPACE: // none
                    Resources::Menu_clear()->play();
                    entry->set_data(cur_x,cur_y,'-');
                    map->reload();
                    break;
                case SDL_SCANCODE_A: // author
                    maptile_set = false;
                    Resources::Menu_break()->play();
                    entry->set_author( get_new_author() );
                    break;
                case SDL_SCANCODE_G: // ground
                    Resources::Menu_clear()->play();
                    entry->set_data(cur_x,cur_y,' ');
                    map->reload();
                    break;
                case SDL_SCANCODE_W: // wall
                    Resources::Menu_clear()->play();
                    entry->set_data(cur_x,cur_y,'*');
                    map->reload();
                    break;
                case SDL_SCANCODE_B: // box
                    Resources::Menu_clear()->play();
                    entry->set_data(cur_x,cur_y,'+');
                    map->reload();
                    break;
                case SDL_SCANCODE_R: // random box
                    Resources::Menu_clear()->play();
                    entry->set_data(cur_x,cur_y,'R');
                    map->reload();
                    break;
                case SDL_SCANCODE_H: // arrow left
                    Resources::Menu_clear()->play();
                    entry->set_data(cur_x,cur_y,'<');
                    map->reload();
                    break;
                case SDL_SCANCODE_K: // arrow right
                    Resources::Menu_clear()->play();
                    entry->set_data(cur_x,cur_y,'>');
                    map->reload();
                    break;
                case SDL_SCANCODE_U: // arrow up
                    Resources::Menu_clear()->play();
                    entry->set_data(cur_x,cur_y,'^');
                    map->reload();
                    break;
                case SDL_SCANCODE_J: // arrow down
                    Resources::Menu_clear()->play();
                    entry->set_data(cur_x,cur_y,'v');
                    map->reload();
                    break;
                case SDL_SCANCODE_I: // ice
                    Resources::Menu_clear()->play();
                    entry->set_data(cur_x,cur_y,'S');
                    map->reload();
                    break;
                case SDL_SCANCODE_O: // bomb trap
                    Resources::Menu_clear()->play();
                    entry->set_data(cur_x,cur_y,'o');
                    map->reload();
                    break;
                case SDL_SCANCODE_1: // place first player
                    Resources::Menu_clear()->play();
                    entry->set_bomber_pos(cur_x,cur_y, 0);
                    map->reload();
                    break;
                case SDL_SCANCODE_2:
                    Resources::Menu_clear()->play();
                    entry->set_bomber_pos(cur_x,cur_y, 1);
                    map->reload();
                    break;
                case SDL_SCANCODE_3:
                    Resources::Menu_clear()->play();
                    entry->set_bomber_pos(cur_x,cur_y, 2);
                    map->reload();
                    break;
                case SDL_SCANCODE_4:
                    Resources::Menu_clear()->play();
                    entry->set_bomber_pos(cur_x,cur_y, 3);
                    map->reload();
                    break;
                case SDL_SCANCODE_5:
                    Resources::Menu_clear()->play();
                    entry->set_bomber_pos(cur_x,cur_y, 4);
                    map->reload();
                    break;
                case SDL_SCANCODE_6:
                    Resources::Menu_clear()->play();
                    entry->set_bomber_pos(cur_x,cur_y, 5);
                    map->reload();
                    break;
                case SDL_SCANCODE_7:
                    Resources::Menu_clear()->play();
                    entry->set_bomber_pos(cur_x,cur_y, 6);
                    map->reload();
                    break;
                case SDL_SCANCODE_8:
                    Resources::Menu_clear()->play();
                    entry->set_bomber_pos(cur_x,cur_y, 7);
                    map->reload();
                    break;
                case SDL_SCANCODE_LEFT:
                    maptile_set = false;
                    Resources::Menu_break()->play();
                    cur_x--;
                    break;
                case SDL_SCANCODE_RIGHT:
                    maptile_set = false;
                    Resources::Menu_break()->play();
                    cur_x++;
                    break;
                case SDL_SCANCODE_UP:
                    maptile_set = false;
                    Resources::Menu_break()->play();
                    cur_y--;
                    break;
                case SDL_SCANCODE_DOWN:
                    maptile_set = false;
                    Resources::Menu_break()->play();
                    cur_y++;
                    break;
                default:
                    maptile_set = false;
                }

                if (maptile_set && text_editor_mode)
                {
                    cur_x++;
                }

                clip_cursor();
            }

            draw_editor();

            CB_Flip();
        }
    }
#endif
}

void MapEditor::draw_editor()
{
    Resources::MapEditor_background()->blit(0, 0);

    // show map
    map->show();
    map->show_random_boxes();
    map->show_start_positions();

    // show cursor
    CB_FillRect(60 + cur_x * 40, 40 + cur_y * 40, 40, 40, 150, 125, 25, 150);

    // show map name
    Resources::Font_big()->render(map->map_list[current_map]->get_name(), 780, 3,
                                  cbe::FontAlignment_0topright);

    // huh, what's this? ;)
#ifdef __SWITCH__
    Resources::Font_small()->render(_("PRESS + FOR HELP"), 20, 3,
                                    cbe::FontAlignment_0topleft);

    // Palette HUD: render in a smaller 14pt font directly above the existing
    // bottom strip (y=580). Tile label on the left above "EDITOR MODE",
    // Bomber label on the right above "NUMBER OF PLAYERS".
    static cbe::FontSDL *fnt_editor_hud = nullptr;
    if (!fnt_editor_hud) {
        fnt_editor_hud = new cbe::FontSDL(
            CB_DATADIR "/fonts/DejaVuSans-Bold.ttf", 14);
    }

    std::string tile_label =
        std::string(_("Tile: ")) + EDITOR_TILE_PALETTE[current_tile_idx].label;
    fnt_editor_hud->render(tile_label, 20, 562,
                           cbe::FontAlignment_0topleft);

    std::string bomber_label =
        std::string(_("Bomber: ")) + std::to_string(current_bomber_slot + 1);
    fnt_editor_hud->render(bomber_label, 780, 562,
                           cbe::FontAlignment_0topright);
#else
    Resources::Font_small()->render(_("PRESS F1 FOR HELP"), 20, 3,
                                    cbe::FontAlignment_0topleft);
#endif

    if (text_editor_mode)
    {
        Resources::Font_small()->render(_("TEXT EDITOR MODE"), 20, 580,
                                        cbe::FontAlignment_0topleft);
    }
    else
    {
        Resources::Font_small()->render(_("NORMAL EDITOR MODE"), 20, 580,
                                        cbe::FontAlignment_0topleft);
    }

    std::string numplayers = _("NUMBER OF PLAYERS   ") + std::to_string(map->map_list[current_map]->get_max_players());
    Resources::Font_small()->render(numplayers, 780, 580,
                                    cbe::FontAlignment_0topright);
}

std::string MapEditor::get_new_author()
{
#ifdef __SWITCH__
    // Use the libnx software keyboard — CB_EnterText() needs letter scancodes
    // that don't exist on the Switch.
    extern bool switch_swkbd_input(std::string &, const char *, int);

    std::string author = map->map_list[current_map]->get_author();
    if (switch_swkbd_input(author, "Enter author name", 30) && author.length())
    {
        Resources::Menu_clear()->play();
        return author;
    }
    Resources::Menu_break()->play();
    return map->map_list[current_map]->get_author();
#else
    std::string author(map->map_list[current_map]->get_author());

    while (1)
    {
        draw_editor();

        CB_FillRect(150, 300, 400, 100, 0, 0, 0, 200);

        std::string pauthor = _("Author: ") + map->map_list[current_map]->get_author();
        Resources::Font_big()->render(pauthor, 180, 300,
                                      cbe::FontAlignment_0topleft);

        Resources::Font_small()->render(_("PRESS ESC TO ABORT"), 400, 380,
                                        cbe::FontAlignment_0topcenter);

        CB_Flip();

        switch(CB_EnterText(author))
        {
        case 1:
            if (author.length())
            {
                Resources::Menu_clear()->play();
                return author;
            }
        case -1:
            Resources::Menu_break()->play();
            return map->map_list[current_map]->get_author();
        }
    }
    // cannot reach this!
#endif
}

void MapEditor::show_help()
{
#ifdef __SWITCH__
    Resources::MapEditor_background()->blit(0, 0);

    Resources::Font_big()->render(_("Map Editor Controls"), 400, 30,
                                  cbe::FontAlignment_0topcenter);

    const int LX = 60;
    const int RX = 320;
    int y = 100;

    Resources::Font_small()->render(_("D-Pad / Left Stick"), LX, y,
                                    cbe::FontAlignment_0topleft);
    Resources::Font_small()->render(_("Move cursor"), RX, y,
                                    cbe::FontAlignment_0topleft);
    y += 30;
    Resources::Font_small()->render(_("A"), LX, y,
                                    cbe::FontAlignment_0topleft);
    Resources::Font_small()->render(_("Place current tile"), RX, y,
                                    cbe::FontAlignment_0topleft);
    y += 30;
    Resources::Font_small()->render(_("B"), LX, y,
                                    cbe::FontAlignment_0topleft);
    Resources::Font_small()->render(_("Erase (set hole)"), RX, y,
                                    cbe::FontAlignment_0topleft);
    y += 30;
    Resources::Font_small()->render(_("X"), LX, y,
                                    cbe::FontAlignment_0topleft);
    Resources::Font_small()->render(_("Cycle bomber slot (1-8)"), RX, y,
                                    cbe::FontAlignment_0topleft);
    y += 30;
    Resources::Font_small()->render(_("Y"), LX, y,
                                    cbe::FontAlignment_0topleft);
    Resources::Font_small()->render(_("Place bomber spawn at cursor"), RX, y,
                                    cbe::FontAlignment_0topleft);
    y += 30;
    Resources::Font_small()->render(_("ZL"), LX, y,
                                    cbe::FontAlignment_0topleft);
    Resources::Font_small()->render(_("Previous tile in palette"), RX, y,
                                    cbe::FontAlignment_0topleft);
    y += 30;
    Resources::Font_small()->render(_("ZR"), LX, y,
                                    cbe::FontAlignment_0topleft);
    Resources::Font_small()->render(_("Next tile in palette"), RX, y,
                                    cbe::FontAlignment_0topleft);
    y += 30;
    Resources::Font_small()->render(_("L"), LX, y,
                                    cbe::FontAlignment_0topleft);
    Resources::Font_small()->render(_("Change author"), RX, y,
                                    cbe::FontAlignment_0topleft);
    y += 30;
    Resources::Font_small()->render(_("R"), LX, y,
                                    cbe::FontAlignment_0topleft);
    Resources::Font_small()->render(_("Toggle Text / Normal mode"), RX, y,
                                    cbe::FontAlignment_0topleft);
    y += 30;
    Resources::Font_small()->render(_("Plus"), LX, y,
                                    cbe::FontAlignment_0topleft);
    Resources::Font_small()->render(_("This help screen"), RX, y,
                                    cbe::FontAlignment_0topleft);
    y += 30;
    Resources::Font_small()->render(_("Minus"), LX, y,
                                    cbe::FontAlignment_0topleft);
    Resources::Font_small()->render(_("Save and exit"), RX, y,
                                    cbe::FontAlignment_0topleft);
    y += 30;
    Resources::Font_small()->render(_("L-Stick click"), LX, y,
                                    cbe::FontAlignment_0topleft);
    Resources::Font_small()->render(_("-1 player"), RX, y,
                                    cbe::FontAlignment_0topleft);
    y += 30;
    Resources::Font_small()->render(_("R-Stick click"), LX, y,
                                    cbe::FontAlignment_0topleft);
    Resources::Font_small()->render(_("+1 player"), RX, y,
                                    cbe::FontAlignment_0topleft);

    Resources::Font_big()->render(_("Press any key"), 400, 545,
                                  cbe::FontAlignment_0topcenter);

    CB_Flip();
    CB_WaitForKeypress();
#else
    Resources::MapEditor_background()->blit(0, 0);

    Resources::Game_maptiles()->put_screen( 40, 70, Config::get_theme()*4 + 0 );
    Resources::Font_small()->render(_("PRESS G FOR A GROUND TILE"), 110, 78,
                                    cbe::FontAlignment_0topleft);

    Resources::Game_maptiles()->put_screen( 40, 110, Config::get_theme()*4 + 1 );
    Resources::Font_small()->render(_("PRESS W FOR A WALL"), 110, 118,
                                    cbe::FontAlignment_0topleft);

    Resources::Game_maptiles()->put_screen( 40, 150, Config::get_theme()*4 + 2 );
    Resources::Font_small()->render(_("PRESS B FOR A BOX"), 110, 158,
                                    cbe::FontAlignment_0topleft);

    Resources::Game_maptiles()->put_screen( 40, 190, Config::get_theme()*4 + 2 );
    Resources::Game_maptile_addons()->put_screen( 40, 190, 5 );
    Resources::Font_small()->render(_("PRESS R FOR A RANDOM TILE"), 110, 198,
                                    cbe::FontAlignment_0topleft);

    Resources::Game_maptiles()->put_screen( 40, 230, Config::get_theme()*4 + 0 );
    Resources::Game_maptile_addons()->put_screen( 40, 230, 7 );
    Resources::Font_small()->render(_("PRESS O FOR A BOMB TRAP"),	110, 238,
                                    cbe::FontAlignment_0topleft);

    Resources::Game_maptiles()->put_screen( 40, 270, Config::get_theme()*4 + 0 );
    Resources::Game_maptile_addons()->put_screen( 40, 270, 0 );
    Resources::Font_small()->render(_("PRESS I FOR AN ICE TILE"),	110, 278,
                                    cbe::FontAlignment_0topleft);

    Resources::Game_maptiles()->put_screen( 40, 320, Config::get_theme()*4 + 0 );
    Resources::Game_maptile_addons()->put_screen( 40, 320, 6 );
    Resources::Font_small()->render(_("PRESS NUMBER KEYS FOR BOMBER POSITIONS"),
                                    110, 328, cbe::FontAlignment_0topleft);

    // show arrows
    Resources::Font_small()->render(_("THE FOLLOWING KEYS PRODUCE ARROWS"), 40,
                                    398, cbe::FontAlignment_0topleft);

    Resources::Game_maptiles()->put_screen( 150, 460, Config::get_theme()*4 + 0 );
    Resources::Game_maptile_addons()->put_screen( 150, 460, 3 );
    Resources::Font_small()->render(_("U"), 170, 428,
                                    cbe::FontAlignment_0topcenter);

    Resources::Game_maptiles()->put_screen( 110, 500, Config::get_theme()*4 + 0 );
    Resources::Game_maptile_addons()->put_screen( 110, 500, 2 );
    Resources::Font_small()->render(_("J"), 98, 510,
                                    cbe::FontAlignment_0topcenter);

    Resources::Game_maptiles()->put_screen( 150, 500, Config::get_theme()*4 + 0 );
    Resources::Game_maptile_addons()->put_screen( 150, 500, 1 );
    Resources::Font_small()->render(_("K"), 170, 550,
                                    cbe::FontAlignment_0topcenter);

    Resources::Game_maptiles()->put_screen( 190, 500, Config::get_theme()*4 + 0 );
    Resources::Game_maptile_addons()->put_screen( 190, 500, 4 );
    Resources::Font_small()->render(_("L"), 242, 510,
                                    cbe::FontAlignment_0topcenter);

    // misc stuff
    Resources::Font_small()->render(_("PRESS SPACE BAR FOR A HOLE"), 380, 448,
                                    cbe::FontAlignment_0topleft);
    Resources::Font_small()->render(_("PAGE UP AND DOWN SET NR OF PLAYERS"), 380,
                                    468, cbe::FontAlignment_0topleft);
    Resources::Font_small()->render(_("PRESS A FOR CHANGING THE AUTHOR"),	380,
                                    488, cbe::FontAlignment_0topleft);
    Resources::Font_small()->render(_("PRESSING F2 SWITCHES EDITOR MODE"), 380,
                                    508, cbe::FontAlignment_0topleft);

    CB_Flip();

    // wait for the "any key"
    CB_WaitForKeypress();
#endif
}

void MapEditor::clip_cursor()
{
    if (text_editor_mode)
    {
        if (cur_x <0)
        {
            cur_x = MAP_WIDTH-1;
            cur_y--;
        }
        if (cur_x > MAP_WIDTH-1)
        {
            cur_x=0;
            cur_y++;
        }
    }
    else
    {
        if (cur_x <0)
        {
            cur_x = 0;
        }
        if (cur_x > MAP_WIDTH-1)
        {
            cur_x = MAP_WIDTH-1;
        }
    }
    if (cur_y <0)
    {
        cur_y = 0;
    }
    if (cur_y > MAP_HEIGHT-1)
    {
        cur_y = MAP_HEIGHT-1;
    }
}
