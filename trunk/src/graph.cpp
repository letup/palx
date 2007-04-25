#include "allegdef.h"

bitmap::bitmap(const uint8_t *src,int width,int height):
	bmp(create_bitmap(width,height))
{
	if(src)
		memcpy(bmp->dat,src,width*height);
}
bitmap::~bitmap()
{
	destroy_bitmap(bmp);
}
bool bitmap::blit_to(BITMAP *dest,int source_x,int source_y,int dest_x,int dest_y)
{
	int w=(dest->w-dest_x<bmp->w?dest->w-dest_x:bmp->w);
	int h=(dest->h-dest_y<bmp->h?dest->h-dest_y:bmp->h);
	blit(bmp,dest,source_x,source_y,dest_x,dest_y,w,h);
	return true;
}
sprite::sprite(uint8_t *src):buf(src)
{}
sprite::~sprite()
{}
bool sprite::blit_to(BITMAP *dest,int dest_x,int dest_y)
{
	Pal::Tools::DecodeRLE(buf,dest->dat,dest->w,dest->w,dest->h,dest_x,dest_y);
	return true;
}

ALFONT_FONT *ttfont::glb_font;