#include "allegdef.h"
#include "internal.h"
#include "game.h"
#include "timing.h"
#include "UI.h"

uint32_t current_dialog_lines = 0;
uint32_t glbvar_fontcolor  = 0x4F;
uint32_t font_color_yellow = 0x2D;
uint32_t font_color_red    = 0x1A;
uint32_t font_color_cyan   = 0x8D;
uint32_t font_color_cyan_1 = 0x8C;
uint32_t frame_pos_flag = 1;
uint32_t dialog_x = 12;
uint32_t dialog_y = 8;
uint32_t frame_text_x = 0x2C;
uint32_t frame_text_y = 0x1A;

uint32_t icon_x=0;
uint32_t icon_y=0;
uint32_t icon=0;

void show_wait_icon()
{
	if(frame_pos_flag)
		game->message_handles.getsprite(icon)->blit_to(screen,icon_x,icon_y);
	wait_for_key();
	current_dialog_lines=0;
	icon=0;
}

void dialog_string(const char *str,int x,int y,int color,bool shadow,BITMAP *bmp)
{
	ttfont(str).blit_to(bmp,x,y,color,shadow);
}

void draw_oneline_m_text(char *str,int x,int y)
{
	static char word[3];
	int text_x=x;
	for(int i=0,len=(int)strlen(str);i<len;i++)
		switch(str[i]){
			case '-':
				std::swap(glbvar_fontcolor, font_color_cyan);
				break;
			case '\'':
				std::swap(glbvar_fontcolor, font_color_red);
				break;
			case '\"':
				std::swap(glbvar_fontcolor, font_color_yellow);
				break;
			case '$':
				break;
			case '~':
				break;
			case ')':
				icon=1;
				break;
			case '(':
				icon=2;
				break;
			default:
				strncpy(word,str+i,2);
				dialog_string(word,text_x,y,glbvar_fontcolor,true);
				icon_x=text_x+=16;
				i++;
	}
	icon_y=y;
}