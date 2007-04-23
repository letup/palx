/*
 * PAL library GRF format include file
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
 *���ɽ�����������GRF��ʽͷ�ļ�
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

/*	GRF(Game Resource File) �ļ���ʽ
����GRF �ļ�����������ɣ�������Index�������ݣ�Data����
�����������и�ʽ�Ĳ��֣���Ÿ������ļ�����Ϣ���������޸�ʽ�Ĳ��֣�������������ʽ���
���ݡ����������ж����ݵ����á�
�����������ļ������⣬��չ��Ϊ��.GRF�������������ִ洢��ʽ�����ɴ洢�������ݴ������
���ļ��У�λ������֮�󣻵����洢��ÿ���������ʹ�ö����ļ����ļ���Ϊ��NNNNNNNN������
�С�NNNNNNNN��Ϊʮ������������
���������洢�ڶ����ݽ����޸�ʱЧ�ʽϸߣ������ɴ洢���ڶ�ȡʱ�Եýϼ򵥡���˿��Ա༭
�޸ĵ�GRF�ļ�һ��Ϊ�����洢��
 */

#pragma once

#ifndef	PAL_LIBRARY_H
#	error	Please include pallib.h instead of this file!
#endif

/* ѹ���㷨�������� */
#define	GRF_COMPRESS_ALGORITHM_NONE		0	/* ��ѹ�� */
#define	GRF_COMPRESS_ALGORITHM_YJ1		1	/* YJ_1 */
#define	GRF_COMPRESS_ALGORITHM_YJ2		2	/* YJ_2 (WIN��) */

/* ��Դ���ͳ������� */
#define	GRF_RESOURCE_TYPE_NONE			0	/* ������ */

/* ������� */
#define	GRF_ATTRIBUTE_MINIMUM			0
#define	GRF_ATTRIBUTE_OFFSET			0
#define	GRF_ATTRIBUTE_LENGTH			1
#define	GRF_ATTRIBUTE_RESOURCETYPE		2
#define	GRF_ATTRIBUTE_COMPRESSALGORITHM	3
#define	GRF_ATTRIBUTE_NAMELENGTH		4
#define	GRF_ATTRIBUTE_NAME				5
#define	GRF_ATTRIBUTE_MAXIMUM			0

/* GRF �ļ����ඨ�� */
#define	GRF_TYPE_STANDALONE				0
#define	GRF_TYPE_INTEGRATE				1

/* ������볣������ */
#define	GRF_ERROR_CODE_SUCCESSFUL		0							/* �޴��󣨳ɹ��� */
#define	GRF_ERROR_CODE_BASE				0x80000000L					/* ���������ֵ */
#define	GRF_ERROR_CODE_INTERNALERROR	(GRF_ERROR_CODE_BASE + 0)	/* �ڲ������ڴ������� */
#define	GRF_ERROR_CODE_INVAILDPARAMETER	(GRF_ERROR_CODE_BASE + 1)	/* �Ƿ����� */
#define	GRF_ERROR_CODE_PATHNOTEXIST		(GRF_ERROR_CODE_BASE + 2)	/* ָ����·�������ڻ��޷����� */
#define	GRF_ERROR_CODE_FILEERROR		(GRF_ERROR_CODE_BASE + 3)	/* �ļ��������� */
#define	GRF_ERROR_CODE_INVAILDFILE		(GRF_ERROR_CODE_BASE + 4)	/* �Ƿ��ļ���ʽ */
#define	GRF_ERROR_CODE_ENTRYEXISTED		(GRF_ERROR_CODE_BASE + 5)	/* ָ�������Ѿ����� */
#define	GRF_ERROR_CODE_ENTRYNOTEXIST	(GRF_ERROR_CODE_BASE + 6)	/* ָ��������� */
#define	GRF_ERROR_CODE_ENTRYNOTSPECIFY	(GRF_ERROR_CODE_BASE + 7)	/* ��δѡ����ǰ�� */

#pragma pack(1)
typedef struct _GRF_HEADER
{
	char	Signature[4];		/* �ļ���־��ӦΪ'GRF\0' */
	uint32	FileLength;			/* �ļ��ܳ��� */
	uint32	EntryCount;			/* ������������ */
	uint32	DataOffset;			/* ������ʼƫ�ƣ�Ϊ 0 ��ʾ���ļ�ֻ������������������ִ���ڴ����� */
}	GRF_HEADER;

typedef struct _INDEX_ENTRY
{
	uint32	Offset;				/* �洢ƫ���� */
	uint32	Length;				/* �ļ����� */
	uint8	ResourceType;		/* ��Դ���� */
	uint8	CompressAlgorithm;	/* ѹ���㷨 */
	uint16	PathLength;			/* ·��ȫ������ */
	char	EntryPath[0];		/* ·��ȫ��������ڸ�Ŀ¼�� */
}	INDEX_ENTRY;
#pragma pack()

#include <stdio.h>

typedef struct _GRF_FILE
{
	int				fd;		/* �����ļ������� */
	char*			base;	/* ����·�� */
	void*			pie;	/* �洢������ */
	INDEX_ENTRY*	ieptr;	/* ������ָ�� */
	uint32			flag;	/* ��־λ */
	errno_t			error;	/* ������ */
	uint32			pos;	/* ��ǰ����λ�� */
}	GRFFILE;
