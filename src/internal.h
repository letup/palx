/***************************************************************************
 *   PALx: A platform independent port of classic RPG PAL                  *
 *   Copyleft (C) 2006 by Pal Lockheart                                    *
 *   palxex@gmail.com                                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, If not, see                          *
 *   <http://www.gnu.org/licenses/>.                                       *
 ***************************************************************************/
#ifndef CONSTANTS
#define CONSTANTS

#include "structs.h"
#include <vector>
#include <map>

struct Scene;
struct BattleScene;
class playrix;
class Game;
class global_init;
extern Scene *scene;
extern BattleScene *battle_scene;
extern playrix *rix;
extern global_init *global;

extern bool flag_battling;

extern int flag_to_load;
extern int rpg_to_load;
extern int map_toload;

extern int step_off_x,step_off_y;
extern int coordinate_x_max,coordinate_y_max;
extern int x_scrn_offset,y_scrn_offset;
extern int abstract_x_bak,abstract_y_bak;
extern int viewport_x_bak,viewport_y_bak;
extern int direction_offs[4][2];
extern bool key_enable;
extern int fadegap[6];

extern bool flag_parallel_mutex;
extern int redraw_flag;
extern int flag_pic_level;

extern int dialog_type;

extern uint32_t current_dialog_lines;
extern uint32_t glbvar_fontcolor;
extern uint32_t font_color_yellow;
extern uint32_t font_color_red;
extern uint32_t font_color_cyan;
extern uint32_t font_color_cyan_1;
extern uint32_t frame_pos_flag;
extern uint32_t dialog_x;
extern uint32_t dialog_y;
extern uint32_t frame_text_x;
extern uint32_t frame_text_y;

class sprite_prim;
extern std::vector<sprite_prim> mgos;
extern std::map<int,int> team_mgos;
extern std::map<int,int> npc_mgos;

extern void randomize();
extern float rnd0();
extern int rnd1(int);

extern int scale;
extern int x_off,y_off;
extern bool running;

extern void Load_Data();
extern void GameLoop_OneCycle(bool);
extern bool process_Menu();
extern void process_Explore();
extern uint16_t process_script(uint16_t script,int16_t object);
extern uint16_t process_autoscript(uint16_t script,int16_t object);

extern void load_team_mgo();
extern void load_NPC_mgo();
extern void setup_our_team_data_things();
extern void record_step();
extern void calc_trace_frames();
extern void team_walk_one_step();
extern void stop_and_update_frame();
extern void calc_followers_screen_pos();
extern int calc_faceto(int x_diff,int y_diff);
extern bool barrier_check(uint16_t self,int x,int y,bool =true);
extern bool no_barrier;

extern bool mutex_can_change_palette;
extern int mutex_paletting,mutex_blitting;
extern int shake_times,shake_grade;
extern void pal_fade_out(int gap);
extern void pal_fade_in(int gap);
extern void fade_inout(int);
class bitmap;
extern void crossFade_assimilate(int gap,int time,bitmap &dst,bitmap &jitter);
extern void crossFade_desault(int gap,int time,bitmap &dst,bitmap &jitter);
extern void CrossFadeOut(int u,int times,int gap,const bitmap &buf);
extern void crossFade_self(int gap,int time,bitmap &src);
extern void show_fbp(int,int);
extern void shake_screen();
extern void flush_screen();
extern void wave_screen(bitmap &buffer,bitmap &dst,int grade,int height);
extern int wave_progression;

extern int CARD;
extern int mutex_int;
extern void switch_proc();
extern void perframe_proc();

extern int RNG_num;
extern void play_RNG(int begin,int end,int gap);

extern bool prelimit_OK;

extern int compact_items();
extern void learnmagic(bool flag_dialog,int magic,int role);

#endif

