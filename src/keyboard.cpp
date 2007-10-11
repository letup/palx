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
#include "allegdef.h"
#include "game.h"
#include <stack>

int scancode_translate(int allegro_scancode)
{
	static class __scancode_map{
		std::map<int,int> mymap;
	public:
		__scancode_map(){
			mymap[0x48]=KEY_UP;
			mymap[0x4b]=KEY_LEFT;
			mymap[0x4d]=KEY_RIGHT;
			mymap[0x50]=KEY_DOWN;
		}
		int operator[](int n){
		    if(mymap.find(n)!=mymap.end())
                return mymap[n];
            else
                return -1;
		}
	}scancode_map;
	return scancode_map[allegro_scancode];
}

int mykey[256];
int mykey_lowlevel[256];
std::stack<int> keys;

VKEY get_key(bool clear)
{
	VKEY keygot=VK_NONE;
	int k;
	if(keypressed()){
		switch(k=(readkey()>>8)){
		case KEY_ESC:
		case KEY_INSERT:
		case KEY_ALT:
		case KEY_ALTGR:
			keygot = VK_MENU;
			break;
		case KEY_ENTER:
		case KEY_SPACE:
		case KEY_LCONTROL:
		case KEY_RCONTROL:
			keygot = VK_EXPLORE;
			break;
		case KEY_LEFT:
			keygot = VK_LEFT;
			break;
		case KEY_UP:
			keygot = VK_UP;
			break;
		case KEY_RIGHT:
			keygot = VK_RIGHT;
			break;
		case KEY_DOWN:
			keygot = VK_DOWN;
			break;
		case KEY_PGUP:
			keygot = VK_PGUP;
			break;
		case KEY_PGDN:
			keygot = VK_PGDN;
			break;
		case KEY_R:
			keygot = VK_REPEAT;
			break;
		case KEY_A:
			keygot = VK_AUTO;
			break;
		case KEY_D:
			keygot = VK_DEFEND;
			break;
		case KEY_E:
			keygot = VK_USE;
			break;
		case KEY_W:
			keygot = VK_THROW;
			break;
		case KEY_Q:
			keygot = VK_QUIT;
			break;
		case KEY_S:
			keygot = VK_STATUS;
			break;
		case KEY_F:
			keygot = VK_FORCE;
			break;
		case KEY_P:
		case KEY_PRTSCR:
			keygot = VK_PRINTSCREEN;
			break;
		default:
			keygot = VK_NONE;
			if(!clear)
				simulate_keypress(k<<8);
		}
		if(clear)
			clear_keybuf();
	}
	return keygot;
}
int make_layer(int key)
{
	if(2<=key && key<=3)
		return 4-key;
	return 0;
}
extern int x_off,y_off;
void reproduct_key();
VKEY get_key_lowlevel()
{
	VKEY keygot=VK_NONE;
	if(key[KEY_ESC] || key[KEY_INSERT])// || mykey[KEY_ALT] || mykey[KEY_ALTGR])
		keygot = VK_MENU;
	else if(key[KEY_ENTER] || key[KEY_SPACE] || key[KEY_LCONTROL] || key[KEY_RCONTROL])
		keygot = VK_EXPLORE;
	/*else if
		case KEY_PGUP:
			keygot = VK_PGUP;
			break;
		case KEY_PGDN:
			keygot = VK_PGDN;
			break;
		case KEY_R:
			keygot = VK_REPEAT;
			break;
		case KEY_A:
			keygot = VK_AUTO;
			break;
		case KEY_D:
			keygot = VK_DEFEND;
			break;
		case KEY_E:
			keygot = VK_USE;
			break;
		case KEY_W:
			keygot = VK_THROW;
			break;
		case KEY_Q:
			keygot = VK_QUIT;
			break;
		case KEY_S:
			keygot = VK_STATUS;
			break;
		case KEY_F:
			keygot = VK_FORCE;
			break;
		case KEY_P:
		case KEY_PRTSCR:
			keygot = VK_PRINTSCREEN;
			break;
		default:
			keygot = VK_NONE;
			if(!clear)
				simulate_keypress(k<<8);
		}
		if(clear)
			clear_keybuf();
	}*/
	/*reproduct_key();
	int key_updown=0,key_leftright=0;
	int up=make_layer(mykey[KEY_UP]),down=make_layer(mykey[KEY_DOWN]),left=make_layer(mykey[KEY_LEFT]),right=make_layer(mykey[KEY_RIGHT]);
	if(!up && !down)
		key_updown=0,
		key_leftright=(left>right?-1:(right>left?1:0));
	if(!left && !right)
		key_leftright=0,
		key_updown=(up>down?-1:(down>up?1:0));

	if(up==2)
		key_updown=-1,key_leftright=0;
	if(down==2)
		key_updown=1,key_leftright=0;
	if(left==2)
		key_updown=0,key_leftright=-1;
	if(right==2)
		key_updown=0,key_leftright=1;

	if(!right && key_leftright==1)
		key_leftright=0;
	if(!left && key_leftright==-1)
		key_leftright=0;
	if(!down && key_updown==1)
		key_updown=0;
	if(!up && key_updown==-1)
		key_updown=0;

	x_off=((key_updown<0||key_leftright>0)?1:((key_updown>0||key_leftright<0)?-1:0));
	y_off=((key_updown>0||key_leftright>0)?1:((key_updown<0||key_leftright<0)?-1:0));*/

	if(!keys.empty())
	{
		x_off=((keys.top()==scancode_translate(res::setup.key_left)||keys.top()==scancode_translate(res::setup.key_down))?-1:((keys.top()==scancode_translate(res::setup.key_right)||keys.top()==scancode_translate(res::setup.key_up))?1:0));
		y_off=((keys.top()==scancode_translate(res::setup.key_down)||keys.top()==scancode_translate(res::setup.key_right))?1:((keys.top()==scancode_translate(res::setup.key_left)||keys.top()==scancode_translate(res::setup.key_up))?-1:0));
	}
	else
		x_off=0,y_off=0;
	return keygot;
}

void key_watcher(int scancode)
{
	/*memset(mykey,0,sizeof(mykey));
	for(int i=0;i<127;i++)
		if(key[i])
			mykey[i]=2;
	if(scancode<127 && mykey[scancode]==2)	return;*/
	if(scancode>127){
		std::stack<int> another;
		for(int i=0;i<keys.size();){
			if(keys.top()!=(scancode&0x7f))
				another.push(keys.top());
			keys.pop();
		}
		for(int i=0;i<another.size();){
			keys.push(another.top());
			another.pop();
		}
	}else
		if(keys.empty() || keys.top()!=scancode)
			keys.push(scancode);
	mykey_lowlevel[scancode&0x7f]=(scancode>127?2:1);
}
END_OF_FUNCTION(key_watcher);
void reproduct_key()
{
	for(int i=0;i<127;i++)
	{
		int &key=mykey[i],&scan_key=mykey_lowlevel[i];
		if(scan_key==0)
			key=0;
		else if(scan_key==1)
			if(key<2)
				key=2;
			else
				key=2;
		else
			if(key<2)
				key=0;
			else
				key=1;
	}
}
