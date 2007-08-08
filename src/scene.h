/***************************************************************************
 *   PALx: A platform independent port of classic RPG PAL   *
 *   Copyleft (C) 2006 by Pal Lockheart   *
 *   palxex@gmail.com   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#ifndef MAINLOOP_H
#define MAINLOOP_H

#include <list>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/multi_array.hpp>

#include "resource.h"
#include "allegdef.h"

struct scene_map:public bitmap
{
	virtual void change(int p)=0;
	scene_map(const uint8_t *a,int b,int c):bitmap(a,b,c){}
protected: 
	long len;
};
class fbp:public scene_map{
public: 
	fbp():scene_map(0,SCREEN_W,SCREEN_H){}
	void change(int p){
		memcpy(bmp->dat,FBP.decode(p,0),bmp->w*bmp->h);
	}
};
struct tile{
	boost::shared_ptr<sprite> image;
	bool blocked;
	int layer;
	bool valid;
	tile():image((sprite*)0),blocked(0),layer(0),valid(false){}
};
class palmap:public scene_map{
	boost::multi_array<tile,4> sprites;
	sprite &getsprite(int x,int y,int h,int l,uint8_t *src,bool throu,int layer);
public: 
	void make_tile(uint8_t*,int,int,int,BITMAP*);
	void make_onescreen(BITMAP *dest,int source_x,int source_y,int dest_x,int dest_y);
	void blit_to(BITMAP *dest,int sx,int sy,int dx,int dy);
	tile &gettile(int x,int y,int h,int l);
	palmap();
	void change(int p);
};

struct Scene{
	palmap scenemap;
	BITMAP *scene_buf;
	std::vector<EVENT_OBJECT>::iterator sprites_begin,sprites_end;
	typedef std::vector<boost::shared_ptr<sprite> > s_list;
	s_list active_list;
	position team_pos;
	Scene();
	~Scene();
	void clear_scanlines();
	void clear_active();
	void calc_team_walking(int key);
	void our_team_setdraw();
	void visible_NPC_movment_setdraw();
	void Redraw_Tiles_or_Fade_to_pic();
	void move_usable_screen();
	void get_sprites();
	void produce_one_screen();
	void scanline_draw_normal_scene(int);
};
struct BattleScene{
	fbp background;
	typedef std::list<sprite *> s_list;
	s_list active_list;
	void draw();
};


class cut_msg_impl
{
	FILE *fp;
	char *glb_buf;
	char buf[100];
public: 
	cut_msg_impl(const char *fname="m.msg")
		:fp(fopen(fname,"rb"))
	{
		long len;fseek(fp,0,SEEK_END);len=ftell(fp);rewind(fp);
		glb_buf=new char[len];
		fread(glb_buf,len,1,fp);
		fclose(fp);
	}
	~cut_msg_impl()
	{
		delete[] glb_buf;
	}
	char *operator()(int start,int end=-1)
	{
		if(end==-1)
			end=start+10;
		assert(end>start);assert(start>=0);
		memset(buf,0,sizeof(buf));
		memcpy(buf,glb_buf+start,end-start);
		return buf;
	}
};

#endif
