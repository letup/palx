/*
 * PAL RLE format library
 *
 * Author: Yihua Lou <louyihua@21cn.com>
 *
 * Copyright 2006 - 2007 Yihua Lou
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
 * ��Ȩ���� 2006 - 2007 ¥�Ȼ�
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

#include <stdlib.h>
#include <memory.h>

#include "pallib.h"
using namespace Pal::Tools;

palerrno_t Pal::Tools::DecodeRle(const void* pSrcRle, void* pDest, int nDestWidth, int nDestHeight,
								 int x, int y, Pal::Tools::PDECODERLECALLBACK pCallback, void* pUserData)
{
	int sx, sy, dx, dy, maxy, maxx, offx;
	uint16 rle_width, rle_height;
	uint8  count, cnt;
	uint8* dest;
	const uint8* src;

	//����������
	if (NULL == pSrcRle || NULL == pDest || NULL == pCallback)
		return PAL_EMPTY_POINTER;

	//ȡ RLE ͼ��Ŀ�Ⱥ͸߶�
	rle_width = reinterpret_cast<const uint16*>(pSrcRle)[0];
	rle_height = reinterpret_cast<const uint16*>(pSrcRle)[1];
	src = reinterpret_cast<const uint8*>(pSrcRle) + 4;

	//��� RLE ͼ���ܷ���ʾ��ָ����ͼ����
	if (nDestWidth <= 0 || nDestHeight <= 0 ||
		x + rle_width < 0 || x >= nDestWidth ||
		y + rle_height < 0 || y >= nDestHeight)
		return PAL_OK;

	maxy = (y + rle_height > nDestHeight) ? nDestHeight : y + rle_height;
	maxx = (x + rle_width > nDestWidth) ? nDestWidth : x + rle_width;
	offx = nDestWidth - maxx;

	//�������� y < 0 ���µĲ�����ʾ�Ĳ���
	for(sy = 0, dy = y; dy < 0; dy++, sy++)
		for(sx = 0; sx < rle_width;)
		{
			count = *src++;
			if (count < 0x80)
			{
				src += count;
				sx += count;
			}
			else
				sx += count & 0x7f;
		}

	// ���Ŀ������
	for(dest = reinterpret_cast<uint8*>(pDest) + dy * nDestWidth; dy < maxy; dy++, sy++)
	{
		// �������� x < 0 ���µĲ�����ʾ�Ĳ���
		for(count = 0, dx = x; dx < 0;)
		{
			count = *src++;
			if (count < 0x80)
			{
				src += count;
				dx += count;
			}
			else
				dx += count & 0x7f;
		}

		if (x < 0)
		{
			// �Կ�Խ 0 ���ͼ��������⴦��
			if (dx > 0)
			{
				// ����Ŀ��������߽�
				if (count < 0x80)
					src -= dx;
				count = (count & 0x80) | dx;
				dx = 0;
			}
			else
				count = 0;
			sx = -x;
		}
		else
			sx = 0;

		// д��Ŀ������
		for(dest += dx;;)
		{
			if (count < 0x80)
			{
				// ����͸������
				for(cnt = 0; cnt < count && dx < maxx; cnt++, sx++, dx++, dest++)
				{
					uint8 sval = *src++;
					if (!pCallback(sval, dest, pUserData))
						// ʹ�ñ�׼��Ϊ
						*dest = sval;
				}
			}
			else
			{
				// ��͸������
				for(cnt = 0x80; cnt < count && dx < maxx; cnt++, sx++, dx++, dest++)
					pCallback(-1, dest, pUserData);
			}
			if (dx < maxx)
				// ȡ�ظ�����
				count = *src++;
			else
			{
				if (count < 0x80)
					src += count - cnt;
				sx += count - cnt;
				dest += offx;
				break;
			}
		}

		//����Դָ�룬ʹָ��ָ����һ��
		while(sx < rle_width)
		{
			count = *src++;
			if (count < 0x80)
			{
				src += count;
				sx += count;
			}
			else
				sx += count & 0x7f;
		}
	}

	return PAL_OK;
}

palerrno_t Pal::Tools::EncodeRle(const void* pSrc, int nSrcWidth, int nSrcHeight, void* pDestRle,
								 uint32& nDestLen, Pal::Tools::PENCODERLECALLBACK pCallback, void* pUserData)
{
	uint32 ptr;
	const uint8* src = reinterpret_cast<const uint8*>(pSrc);
	uint8* dest = reinterpret_cast<uint8*>(pDestRle);
	bool bprev, bcur;
	uint8 count;
	int x, y;

	//����������
	if (NULL == pSrc || NULL == pCallback)
		return PAL_EMPTY_POINTER;
	if (pDestRle && nDestLen < 6)
		return PAL_NOT_ENOUGH_SPACE;
	if (0 == nSrcWidth || 0 == nSrcHeight)
	{
		nDestLen = 0;
		return PAL_OK;
	}

	if (pDestRle)
	{
		uint32 nDestMaxLen = nDestLen;
		uint8* pcount;

		// ����ʵ�����
		reinterpret_cast<uint16*>(pDestRle)[0] = static_cast<uint16>(nSrcWidth);
		reinterpret_cast<uint16*>(pDestRle)[1] = static_cast<uint16>(nSrcHeight);
		dest += 4; pcount = dest++; count = 0; ptr = 5;

		bprev = pCallback(src[0], 0, 0, pUserData);
		for(y = 0; y < nSrcHeight && ptr < nDestMaxLen; y++)
		{
			for(x = 0; x < nSrcWidth && ptr < nDestMaxLen; x++, src++)
			{
				bcur = pCallback(*src, x, y, pUserData);
				if (bcur == bprev)
				{
					// ��ͬ��͸���ȣ������Ӽ���
					count++;
					if (!bcur)
					{
						// ��͸������д��Ŀ��ֵ
						*dest++ = *src; ptr++;
					}
					if (count == 0x7f)
					{
						// �������ļ���ֵ����д��
						*pcount = bcur ? 0xff : 0x7f;
						count = 0; pcount = dest++; ptr++;
					}
				}
				else
				{
					*pcount = (bprev ? 0x80 : 0x00) | count;
					bprev = bcur; count = 1;
					pcount = dest++; ptr++;
					if (!bcur && ptr < nDestMaxLen)
					{
						// ��͸��
						*dest++ = *src; ptr++;
					}
				}
			}
		}

		if (y < nSrcHeight || x < nSrcWidth)
			return PAL_NOT_ENOUGH_SPACE;
	}
	else
	{
		// ֻ���㳤��
		count = 0; ptr = 5;

		bprev = pCallback(src[0], 0, 0, pUserData);
		for(y = 0; y < nSrcHeight; y++)
		{
			for(x = 0; x < nSrcWidth; x++, src++)
			{
				bcur = pCallback(*src, x, y, pUserData);
				if (bcur == bprev)
				{
					// ��ͬ��͸���ȣ������Ӽ���
					count++;
					if (!bcur)
						ptr++;	// ��͸��
					if (count == 0x7f)
					{
						// �������ļ���ֵ
						count = 0; ptr++;
					}
				}
				else
				{
					bprev = bcur; count = 1; ptr++;
					if (!bcur)
						ptr++;	// ��͸��
				}
			}
		}
	}
	nDestLen = ptr;

	return PAL_OK;
}

/*
errno_t Pal::Tools::DecodeRLE(const void *Rle, void *Destination, sint32 Stride, sint32 Width, sint32 Height, sint32 x, sint32 y)
{
	sint32 sx, sy, dx, dy, temp;
	uint16 rle_width, rle_height;
	uint8  count, cnt;
	uint8* dest;
	uint8* ptr;

	//����������
	if (Rle == NULL || Destination == NULL)
		return EINVAL;
	//ȡ RLE ͼ��Ŀ�Ⱥ͸߶�
	rle_width = *(uint16*)Rle;
	rle_height = *((uint16*)Rle + 1);
	ptr = (uint8*)Rle + 4;
	//��� RLE ͼ���ܷ���ʾ��ָ����ͼ����
	if (Width <= 0 || Height <= 0 || x + rle_width < 0 || x >= Width || y + rle_height < 0 || y >= Height)
		return 0;

	//�������� y < 0 ���µĲ�����ʾ�Ĳ���
	for(sy = 0, dy = y; dy < 0; dy++, sy++)
		for(sx = 0; sx < rle_width;)
		{
			count = *ptr++;
			sx += count & 0x7f;
			if (count < 0x80)
				ptr += count;
		}
	//����Ŀ������ָ��
	dest = (uint8*)Destination + dy * Stride;
	//���Ŀ������
	for(; dy < Height && sy < rle_height; dy++, sy++, dest += Stride)
	{
		//�������� x < 0 ���µĲ�����ʾ�Ĳ���
		for(count = 0, dx = x; dx < 0;)
		{
			count = *ptr++;
			if (count < 0x80)
			{
				ptr += count;
				if ((dx += count) >= 0)
				{
					//��͸��������Ҫ������ʾ����ptr��count�ó���ȷ��ֵ
					ptr -= dx;
					count = dx;
					dx = 0;
				}
			}
			else
				dx += count & 0x7f;
		}
		//����Ƿ��Ѿ�Խ��Ŀ��ͼ��߽�
		if ((temp = Width - dx) > 0)
		{
			//���Ŀ��ͼ��
			if (count < 0x80 && count > 0)
			{
				//��͸������
				if (count > temp)
					cnt = temp;
				else
					cnt = count;
				//���
				memcpy(dest + dx, ptr, cnt);
				dx += cnt;
				//����Դָ��
				ptr += count;
			}
			//������ʣ�µĲ���
			for(sx = dx - x; sx < rle_width && dx < Width;)
			{
				//ȡ����
				count = *ptr++;
				if (count < 0x80)
				{
					//��͸�����֣�����ͬ��
					if (count > Width - dx)
						cnt = Width - dx;
					else
						cnt = count;
					//���
					memcpy(dest + dx, ptr, cnt);
					dx += cnt;
					//����Դָ���Դ x ֵ
					ptr += count;
					sx += count;
				}
				else
				{
					//͸������
					count &= 0x7f;
					//����Դָ���Դ��Ŀ�� x ֵ
					sx += count;
					dx += count;
				}
			}
		}
		//����Դָ�룬ʹָ��ָ����һ��
		while(sx < rle_width)
		{
			count = *ptr++;
			if (count < 0x80)
			{
				ptr += count;
				sx += count;
			}
			else
				sx += count & 0x7f;
		}
	}
	return 0;
}

errno_t Pal::Tools::EncodeRLE(const void *Source, const void *Base, sint32 Stride, sint32 Width, sint32 Height, void*& Destination, uint32& Length)
{
	sint32 i, j, count;
	uint32 length;
	uint8* src = (uint8*)Source;
	uint8* base = (uint8*)Base;
	uint8* temp;
	uint8* ptr;

	if (Source == NULL || Base == NULL)
		return EINVAL;
	if ((ptr = temp = (uint8*)malloc(Width * Height * 2)) == NULL)
		return ENOMEM;

	for(i = 0, ptr = temp + 4; i < Height; i++)
	{
		for(j = 0; j < Width;)
		{
			for(count = 0; j < Width && *src == *base; j++, base++, src++, count++);
			while(count > 0)
			{
				*ptr++ = (count > 0x7f) ? 0xff : count;
				count -= 0x7f;
			}
			for(count = 0; j < Width && *src != *base; j++, base++, src++, count++);
			while(count > 0)
			{
				if (count > 0x7f)
				{
					*ptr++ = 0x7f;
					memcpy(src - count, ptr, 0x7f);
					ptr += 0x7f;
				}
				else
				{
					*ptr++ = count;
					memcpy(src - count, ptr, count);
					ptr += count;
				}
				count -= 0x7f;
			}
		}
		src += Stride - Width;
		base += Stride - Width;
	}
	length = (uint32)(ptr - temp);

	if ((Destination = realloc(temp, length)) == NULL)
	{
		free(temp);
		return ENOMEM;
	}
	*((uint16*)Destination) = (uint16)Width;
	*((uint16*)Destination + 1) = (uint16)Height;
	Length = length;
	return 0;
}

errno_t Pal::Tools::EncodeRLE(const void *Source, const uint8 TransparentColor, sint32 Stride, sint32 Width, sint32 Height, void*& Destination, uint32& Length)
{
	sint32 i, j, count;
	uint32 length;
	uint8* src = (uint8*)Source;
	uint8* temp;
	uint8* ptr;

	if (Source == NULL)
		return EINVAL;
	if ((ptr = temp = (uint8*)malloc(Width * Height * 2 + 4)) == NULL)
		return ENOMEM;

	for(i = 0, ptr = temp + 4; i < Height; i++)
	{
		for(j = 0; j < Width;)
		{
			for(count = 0; j < Width && *src == TransparentColor; j++, src++, count++);
			while(count > 0)
			{
				*ptr++ = (count > 0x7f) ? 0xff : count;
				count -= 0x7f;
			}
			for(count = 0; j < Width && *src != TransparentColor; j++, src++, count++);
			while(count > 0)
			{
				if (count > 0x7f)
				{
					*ptr++ = 0x7f;
					memcpy(src - count, ptr, 0x7f);
					ptr += 0x7f;
				}
				else
				{
					*ptr++ = count;
					memcpy(src - count, ptr, count);
					ptr += count;
				}
				count -= 0x7f;
			}
		}
		src += Stride - Width;
	}
	length = (uint32)(ptr - temp);

	if ((Destination = realloc(temp, length)) == NULL)
	{
		free(temp);
		return ENOMEM;
	}
	*((uint16*)Destination) = (uint16)Width;
	*((uint16*)Destination + 1) = (uint16)Height;
	Length = length;
	return 0;
}
*/
