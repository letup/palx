/*
 * PAL RLE format library
 * 
 * Author: Lou Yihua <louyihua@21cn.com>
 *
 * Copyright 2007 Lou Yihua
 *
 * This file is part of PAL library.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

 *
 *���ɽ���������RLE��ʽ�����
 *
 * ���ߣ� ¥�Ȼ� <louyihua@21cn.com>
 *
 * ��Ȩ���� 2007 ¥�Ȼ�
 *
 * ���ļ��ǡ��ɽ������������һ���֡�
 *
 * �������������������������������������������GNU��ͨ�ù������֤��
 * �����޸ĺ����·�����һ���򡣻��������֤2.1�棬���ߣ��������ѡ������
 * �ν��µİ汾��������һ���Ŀ����ϣ�������ã���û���κε���������û���ʺ�
 * �ض�Ŀ�������ĵ���������ϸ����������GNU��ͨ�ù������֤��
 * 
 * ��Ӧ���Ѿ��Ϳ�һ���յ�һ��GNU��ͨ�ù������֤�Ŀ����������û�У�д�Ÿ�
 * �����������᣺51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#include "pallib.h"
using namespace Pal::Tools;

bool Pal::Tools::DecodeRLE(const void *Rle, void *Destination, sint32 Width, sint32 Height, sint32 x, sint32 y)
{
	sint32 sx, sy, dx, dy, i, j, width, height;
	uint8 count;
	uint8* src;
	uint8* dst;
	uint8* dest;
	uint16	rle_width = *(uint16*)Rle,
			rle_height = *((uint16*)Rle + 1);
	uint8* ptr = (uint8*)Rle + 4;
	uint8* temp;

	if (x + rle_width < 0 || x >= Width || y + rle_height < 0 || y >= Height)
		return true;
	if ((temp = new uint8 [rle_width * rle_height]) == NULL)
		return false;

	if (x < 0)
		sx = 0, dx = -x;
	else
		sx = x, dx = 0;
	if (y < 0)
		sy = 0, dy = -y;
	else
		sy = y, dy = 0;
	width = (Width - sx < rle_width - dx) ? (Width - sx) : (rle_width - dx);
	height = (Height - sy < rle_height - dy) ? (Height - sy) : (rle_height - dy);

	src = (uint8 *)Destination + (sy * Width) + sx;
	dst = temp + (dy * rle_width) + dy;
	for(i = 0; i < height; i++)
	{
		memcpy(dst, src, width);
		src += Width; dst += rle_width;
	}

	for(dest = temp, i = 0; i < rle_height; i++)
		for(j = 0; j < rle_width;)
		{
			count = *ptr++;
			if (count < 0x80)
			{
				memcpy(dest, ptr, count);
				ptr += count;
				j   += count;
			}
			else
				j   += count & 0x7f;
			dest += count & 0x7f;
		}

	src = temp + (dy * rle_width) + dy;
	dst = (uint8 *)Destination + (sy * Width) + sx;
	for(i = 0; i < height; i++)
	{
		memcpy(dst, src, width);
		src += rle_width; dst += Width;
	}

	delete [] temp;
	return true;
}

bool Pal::Tools::EncodeRLE(const void *Source, const void *Base, sint32 Width, sint32 Height, void*& Destination, uint32& Length)
{
	sint32 i, j, length = 0;
	bool flag;
	uint8 count;
	uint8* src = (uint8*)Source;
	uint8* base = (uint8*)Base;
	uint8* temp;
	uint8* ptr;

	if ((ptr = temp = new uint8 [Width * Height * 2]) == NULL)
		return false;

	for(i = 0; i < Height; i++)
	{
		flag = (*src == *base); count = 0;
		for(j = 0; j < Width; j++)
		{
			if (flag)
				if (*src++ == *base++)
				{
					if (count < 0x7f)
						count++;
					else
					{
						*ptr++ = 0xff;
						count = 1;
					}
				}
				else
				{
					*ptr++ = count | 0x80;
					flag = false;
					count = 1;
				}
			else
				if (*src++ != *base++)
				{
					if (count < 0x7f)
						count++;
					else
					{
						*ptr++ = 0x7f;
						memcpy(ptr, src - 0x7f, 0x7f);
						ptr += 0x7f;
						count = 1;
					}
				}
				else
				{
					*ptr++ = count;
					memcpy(ptr, src - count, count);
					ptr += count;
					flag = true;
					count = 1;
				}
		}
	}

	if ((Destination = malloc(length + 4)) == NULL)
	{
		delete [] temp;
		return false;
	}
	*((uint16*)Destination) = (uint16)Width;
	*((uint16*)Destination + 1) = (uint16)Height;
	memcpy((uint8*)Destination + 4, temp, length);
	Length = length + 4;
	delete [] temp;
	return true;
}

bool Pal::Tools::EncodeRLE(const void *Source, const uint8 TransparentColor, sint32 Width, sint32 Height, void*& Destination, uint32& Length)
{
	sint32 i, j, length = 0;
	bool flag;
	uint8 count;
	uint8* src = (uint8*)Source;
	uint8* temp;
	uint8* ptr;

	if ((ptr = temp = new uint8 [Width * Height * 2]) == NULL)
		return false;

	for(i = 0; i < Height; i++)
	{
		flag = (*src == TransparentColor); count = 0;
		for(j = 0; j < Width; j++)
		{
			if (flag)
				if (*src++ == TransparentColor)
				{
					if (count < 0x7f)
						count++;
					else
					{
						*ptr++ = 0xff;
						count = 1;
					}
				}
				else
				{
					*ptr++ = count | 0x80;
					flag = false;
					count = 1;
				}
			else
				if (*src++ != TransparentColor)
				{
					if (count < 0x7f)
						count++;
					else
					{
						*ptr++ = 0x7f;
						memcpy(ptr, src - 0x7f, 0x7f);
						ptr += 0x7f;
						count = 1;
					}
				}
				else
				{
					*ptr++ = count;
					memcpy(ptr, src - count, count);
					ptr += count;
					flag = true;
					count = 1;
				}
		}
	}

	if ((Destination = malloc(length + 4)) == NULL)
	{
		delete [] temp;
		return false;
	}
	*((uint16*)Destination) = (uint16)Width;
	*((uint16*)Destination + 1) = (uint16)Height;
	memcpy((uint8*)Destination + 4, temp, length);
	Length = length + 4;
	delete [] temp;
	return true;
}
