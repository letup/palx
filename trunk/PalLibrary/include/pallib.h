/*
 * PAL library common include file
 * 
 * Author: Lou Yihua <louyihua@21cn.com>
 *
 * Copyright 2006 - 2007 Lou Yihua
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
 *���ɽ����������⹫��ͷ�ļ�
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

#pragma once

#ifndef	PAL_LIBRARY_H
#	define	PAL_LIBRARY_H
#endif

#include "config.h"
#include "grf.h"

#if	defined(__cplusplus)

namespace Pal
{
	namespace Tools
	{
		bool DecodeYJ1(const void* Source, void*& Destination, uint32& Length);
		bool EncodeYJ1(const void* Source, uint32 SourceLength, void*& Destination, uint32& Length);
		bool DecodeYJ2(const void* Source, void*& Destination, uint32& Length);
		bool EncodeYJ2(const void* Source, uint32 SourceLength, void*& Destination, uint32& Length, bool bCompatible = true);
		bool DecodeRNG(const void* Source, void* PrevFrame);
		bool EncodeRNG(const void* PrevFrame, const void* CurFrame, void*& Destination, uint32& Length);
		bool DecodeRLE(const void* Rle, void* Destination, sint32 Width, sint32 Height, sint32 x, sint32 y);
		bool EncodeRLE(const void* Source, const void *Base, sint32 Width, sint32 Height, void*& Destination, uint32& Length);
		bool EncodeRLE(const void* Source, uint8 TransparentColor, sint32 Width, sint32 Height, void*& Destination, uint32& Length);

		bool DecodeYJ1StreamInitialize(void*& pvState, uint32 uiGrowBy = 0x10000);
		bool DecodeYJ1StreamInput(void* pvState, const void* Source, uint32 SourceLength);
		bool DecodeYJ1StreamOutput(void* pvState, void* Destination, uint32& Length);
		bool DecodeYJ1StreamFinished(void* pvState, uint32& AvailableLength);
		bool DecodeYJ1StreamFinalize(void* pvState);
		bool DecodeYJ1StreamReset(void* pvState);

		bool DecodeYJ2StreamInitialize(void*& pvState, uint32 uiGrowBy = 0x10000);
		bool DecodeYJ2StreamInput(void* pvState, const void* Source, uint32 SourceLength);
		bool DecodeYJ2StreamOutput(void* pvState, void* Destination, uint32& Length);
		bool DecodeYJ2StreamFinished(void* pvState, uint32& AvailableLength);
		bool DecodeYJ2StreamFinalize(void* pvState);
		bool DecodeYJ2StreamReset(void* pvState);

		bool EncodeYJ2StreamInitialize(void*& pvState, uint32 uiSourceLength, uint32 uiGrowBy = 0x10000, bool bCompatible = true);
		bool EncodeYJ2StreamInput(void* pvState, const void* Source, uint32 SourceLength, bool bFinished = false);
		bool EncodeYJ2StreamOutput(void* pvState, void* Destination, uint32& Length, uint32& Bits);
		bool EncodeYJ2StreamFinished(void* pvState);
		bool EncodeYJ2StreamFinalize(void* pvState);

		namespace GRF
		{
			bool GRFopen(const char* grffile, const char* base, int mode, GRFFILE*& stream);
			bool GRFclose(GRFFILE* stream);
			bool GRFflush(GRFFILE* stream);
			bool GRFgettype(GRFFILE* stream, int& type);
			bool GRFenumname(GRFFILE* stream, const char* prevname, char*& nextname);
			int GRFerror(GRFFILE* stream);
			void GRFclearerr(GRFFILE* stream);

			//ֻ�ṩ�������汾�� GRF �ļ�ʹ��
			bool GRFopenfile(GRFFILE* stream, const char* name, const char* mode, FILE*& fpout);
			bool GRFappendfile(GRFFILE* stream, const char* name);
			bool GRFremovefile(GRFFILE* stream, const char* name);
			bool GRFrenamefile(GRFFILE* stream, const char* oldname, const char* newname);
			bool GRFgetfileattr(GRFFILE* stream, const char* name, int attr, void* value);
			bool GRFsetfileattr(GRFFILE* stream, const char* name, int attr, const void* value);

			//ֻ�ṩ�����ɰ汾�� GRF �ļ�ʹ��
			bool GRFseekfile(GRFFILE* stream, const char* name);
			bool GRFeof(GRFFILE* stream);
			bool GRFseek(GRFFILE* stream, long offset, int origin, long& newpos);
			bool GRFtell(GRFFILE* stream, long& pos);
			bool GRFread(GRFFILE* stream, void* buffer, uint32 size, uint32& actual);
			bool GRFgetattr(GRFFILE* stream, int attr, void* value);

			bool GRFPackage(const char* pszGRF, const char* pszBasePath, const char* pszNewFile);
			bool GRFExtract(const char* pszGRF, const char* pszBasePath, const char* pszNewFile);
		}
	}
}

#else

//���º�����������ָ����� NULL ��ʾ�����⣬������Է��� -1 ��ʾ����

int decodeyj1(const void* Source, void** Destination, uint32* Length);
int encodeyj1(const void* Source, uint32 SourceLength, void** Destination, uint32* Length);
int decodeyj2(const void* Source, void** Destination, uint32* Length);
int encodeyj2(const void* Source, uint32 SourceLength, void** Destination, uint32* Length, int bCompatible);
int decoderng(const void* Source, void* PrevFrame);
int encoderng(const void* PrevFrame, const void* CurFrame, void** Destination, uint32* Length);
int decoderle(const void* Rle, void* Destination, sint32 Width, sint32 Height, sint32 x, sint32 y);
int encoderle(const void* Source, const void *Base, sint32 Width, sint32 Height, void** Destination, uint32* Length);
int encoderlet(const void* Source, uint8 TransparentColor, sint32 Width, sint32 Height, void** Destination, uint32* Length);

int decodeyj1streaminitialize(void** pvState, uint32 uiGrowBy);
int decodeyj1streaminput(void* pvState, const void* Source, uint32 SourceLength);
int decodeyj1streamoutput(void* pvState, void* Destination, uint32* Length);
int decodeyj1streamfinished(void* pvState, uint32* AvailableLength);
int decodeyj1streamfinalize(void* pvState);
int decodeyj1streamreset(void* pvState);

int decodeyj2streaminitialize(void** pvState, uint32 uiGrowBy);
int decodeyj2streaminput(void* pvState, const void* Source, uint32 SourceLength);
int decodeyj2streamoutput(void* pvState, void* Destination, uint32* Length);
int decodeyj2streamfinished(void* pvState, uint32* AvailableLength);
int decodeyj2streamfinalize(void* pvState);
int decodeyj2streamreset(void* pvState);

int encodeyj2streaminitialize(void** pvState, uint32 uiSourceLength, uint32 uiGrowBy, int bCompatible);
int encodeyj2streaminput(void* pvState, const void* Source, uint32 SourceLength, int bFinished);
int encodeyj2streamoutput(void* pvState, void* Destination, uint32* Length, uint32* Bits);
int encodeyj2streamfinished(void* pvState);
int encodeyj2streamfinalize(void* pvState);

GRFFILE* grfopen(const char* grffile, const char* base, int mode);
int grfclose(GRFFILE* stream);
int grfflush(GRFFILE* stream);
int grfgettype(GRFFILE* stream);
char* grfenumname(GRFFILE* stream, const char* prevname);
int grferror(GRFFILE* stream);
void grfclearerr(GRFFILE* stream);

FILE* grfopenfile(GRFFILE* stream, const char* name, const char* mode);
int grfappendfile(GRFFILE* stream, const char* name);
int grfremovefile(GRFFILE* stream, const char* name);
int grfrenamefile(GRFFILE* stream, const char* oldname, const char* newname);
int grfgetfileattr(GRFFILE* stream, const char* name, int attr, void* value);
int grfsetfileattr(GRFFILE* stream, const char* name, int attr, const void* value);

int grfseekfile(GRFFILE* stream, const char* name);
int grfeof(GRFFILE* stream);
long grfseek(GRFFILE* stream, long offset, int origin);
long grftell(GRFFILE* stream);
long grfread(GRFFILE* stream, void* buffer, long size);
int grfgetattr(GRFFILE* stream, int attr, void* value);

int grfpackage(const char* pszGRF, const char* pszBasePath, const char* pszNewFile);
int grfextract(const char* pszGRF, const char* pszBasePath, const char* pszNewFile);

#endif
