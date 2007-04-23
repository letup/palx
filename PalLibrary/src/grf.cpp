/*
 * PAL library GRF format base class
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
 *���ɽ����������� GRF ��ʽ������
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

#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <memory.h>
#include <string.h>
#if	defined(WIN32)
#include <direct.h>
#else
#include <unistd.h>
#endif

#include "pallib.h"

#define	GRF_FLAG_EOF			0x1L	//����EOF
#define	GRF_FLAG_STANDALONE		0x2L	//��������&�洢
#define	GRF_FLAG_CREATENEW		0x4L	//�������ļ�
#define	GRF_FLAG_MODIFIED		0x8L	//���޸�

#if	defined(WIN32)
#	define	commit(x)	_commit(x)
#else
#	define	commit(x)	fdatasync(x)
#endif

//�ڲ�ʹ�ú���

static inline char* _icheckpath(const char* pszFilePath)
{
	size_t pathlen;
	char* newpath;

	if (pszFilePath != NULL && (pathlen = strlen(pszFilePath)) > 0)
	{
		char* curpath;
		char* pch;

		// �Դ����·�����м��
		if ((newpath = (char*)malloc(pathlen + 2)) == NULL)
			return NULL;
		strcpy(newpath, pszFilePath);
		newpath[pathlen + 1] = '\0';

		// ȡ��ǰ����Ŀ¼�Ա���
		if ((curpath = getcwd(NULL, 0)) == NULL)
		{
			// û���㹻���ڴ����ڷ��仺����
			free(newpath);
			return NULL;
		}	

		// ���·����ÿһ��
		for(pch = newpath; ; pch++)
		{
			char ch0 = *pch, ch1 = *(pch + 1);

			// �����Ĳ���·���ָ�������������������һ���ַ�
			if (ch0 != '\\' && ch0 != '/' && ch0 != '\0')
				continue;

			// ��·����ʱ�޶�����ǰ����λ��
			*(pch + 1) = '\0';

			// ���ò��Ƿ�ɴ�
			if (chdir(newpath) != 0)
			{
				free(curpath);
				free(newpath);
				return NULL;
			}

			// �ò�ɴ�����Ѿ������˸ò�Ŀ¼
			*(pch + 1) = ch1;

			// �Ƿ��Ѿ���鵽����·���Ľ�β
			if (ch0 == '\0')
				break;
		}

		// ���½���ǰĿ¼����Ϊ֮ǰ��Ŀ¼
		chdir(curpath);
		free(curpath);

		//Ϊ·��ĩβ���ָ���
		if (newpath[pathlen - 1] != '\\' && newpath[pathlen - 1] != '/')
		{
#if	defined(WIN32) || defined(DOS)
			newpath[pathlen] = '\\';
#else
			newpath[pathlen] = '/';
#endif
		}
		return newpath;
	}
	else
	{
		if ((newpath = (char*)malloc(1)) == NULL)
			return NULL;
		else
		{
			newpath[0] = '\0';
			return newpath;
		}
	}
}

static bool _iGRFseekfile(GRFFILE* stream, const char* name)
{
	INDEX_ENTRY* ptr;
	uint32 len;

	//������
	if (stream->pie == NULL || (len = (uint32)strlen(name)) == 0)
	{
		stream->error = EINVAL;
		return false;
	}

	//�ƶ�ָ������ʼ
	ptr = (INDEX_ENTRY*)((GRF_HEADER*)stream->pie + 1);

	//��ʼ����ָ������
	for(uint32 index = 0; index < ((GRF_HEADER*)stream->pie)->EntryCount; index++)
	{
		//�Ƚ�����
		if (len == ptr->PathLength && strncmp(ptr->EntryPath, name, len) == 0)
		{
			stream->ieptr = ptr;
			return true;
		}
		else
			ptr = (INDEX_ENTRY*)((uint8*)(ptr + 1) + ptr->PathLength);
	}
	return false;
}

//��������

bool Pal::Tools::GRF::GRFopen(const char* grffile, const char* base, int mode, GRFFILE*& stream)
{
	GRFFILE* grf;
	char* _base;
	void* ptr;
	int fd, _mode = O_BINARY | O_RDWR;
	long flen;
	uint32 flag;

	//����������
	if (grffile == NULL || (_base = _icheckpath(base)) == NULL)
		return false;

	//���ô�ģʽ
	if ((mode & O_CREAT) != 0)
		_mode &= O_CREAT;
	if ((mode & O_TRUNC) != 0)
		_mode &= O_TRUNC;
	//���Դ������ļ�
	if ((fd = open(grffile, _mode, S_IREAD | S_IWRITE)) == -1)
	{
		free(_base);
		return false;
	}
	//ȡ�ļ�����
	if (lseek(fd, 0, SEEK_END) == -1 ||
		(flen = tell(fd)) == -1)
	{
		close(fd);
		free(_base);
		return false;
	}
	if (flen == 0)
	{
		//����Ϊ 0�������½���ض�
		//����ռ�
		if ((ptr = malloc(sizeof(GRF_HEADER))) == NULL)
		{
			close(fd);
			free(_base);
			return false;
		}
		//�������
		memset(ptr, 0, sizeof(GRF_HEADER));
		strcpy((char*)ptr, "GRF");
		((GRF_HEADER*)ptr)->FileLength = sizeof(GRF_HEADER);
		//д���ļ�
		if (write(fd, ptr, sizeof(GRF_HEADER)) < sizeof(GRF_HEADER))
		{
			free(ptr);
			close(fd);
			free(_base);
			return false;
		}
		commit(fd);
		//���ñ�־
		flag = GRF_FLAG_STANDALONE | GRF_FLAG_CREATENEW;
	}
	else
	{
		//�������ļ�
		GRF_HEADER hdr;
		uint32 len;

		//�жϸ�ʽ������ռ�
		if (read(fd, &hdr, sizeof(GRF_HEADER)) < sizeof(GRF_HEADER) ||
			strcmp(hdr.Signature, "GRF") != 0 || lseek(fd, 0, SEEK_SET) == -1)
		{
			close(fd);
			free(_base);
			return false;
		}
		len = hdr.DataOffset == 0 ? hdr.FileLength : hdr.DataOffset;
		if ((ptr = malloc(len)) == NULL)
		{
			close(fd);
			free(_base);
			return false;
		}
		//��ȡ����
		if ((uint32)read(fd, ptr, len) < len)
		{
			free(ptr);
			close(fd);
			free(_base);
			return false;
		}
		if (hdr.DataOffset == 0)
			flag = GRF_FLAG_STANDALONE;
		else
			flag = 0;
	}

	//Ϊ�ṹ����ռ䲢���
	if ((grf = (GRFFILE*)malloc(sizeof(GRFFILE))) == NULL)
	{
		free(ptr);
		close(fd);
		free(_base);
		return false;
	}
	else
	{
		memset(grf, 0, sizeof(GRFFILE));
		grf->fd = fd;
		grf->base = _base;
		grf->pie = ptr;
		grf->ieptr = (INDEX_ENTRY*)((GRF_HEADER*)ptr + 1);
		grf->flag = flag;
		grf->pos = ((GRF_HEADER*)ptr)->DataOffset;
		stream = grf;
		return true;
	}
}

bool Pal::Tools::GRF::GRFclose(GRFFILE* stream)
{
	int fd;

	if (stream == NULL)
		return false;
	GRFflush(stream);
	if (stream->base)
		free(stream->base);
	if (stream->pie)
		free(stream->pie);
	fd = stream->fd;
	free(stream);
	if (fd != -1)
		return (close(fd) != -1);
	else
		return true;
}

bool Pal::Tools::GRF::GRFflush(GRFFILE* stream)
{
	if (stream == NULL)
		return false;
	if (stream->fd == -1)
	{
		stream->error = EBADF;
		return false;
	}
	if (stream->pie == NULL)
	{
		stream->error = EINVAL;
		return false;
	}

	if ((stream->flag & GRF_FLAG_MODIFIED) != 0)
	{
		uint32 len = ((GRF_HEADER*)stream->pie)->FileLength;

		if (lseek(stream->fd, 0, SEEK_SET) == -1 ||
			(uint32)write(stream->fd, stream->pie, len) < len)
			return false;
		stream->flag &= ~GRF_FLAG_MODIFIED;
	}

	return true;
}

bool Pal::Tools::GRF::GRFgettype(GRFFILE* stream, int& type)
{
	if (stream == NULL)
		return false;

	if ((stream->flag & GRF_FLAG_STANDALONE) == 0)
		type = GRF_TYPE_INTEGRATE;
	else
		type = GRF_TYPE_STANDALONE;
	return true;
}

bool Pal::Tools::GRF::GRFenumname(GRFFILE* stream, const char* prevname, char*& nextname)
{
	INDEX_ENTRY* ptr;
	char* name;

	//������
	if (stream == NULL)
		return false;

	//������
	if (prevname == NULL)
		ptr = (INDEX_ENTRY*)((GRF_HEADER*)stream->pie + 1);
	else
	{
		INDEX_ENTRY* oldptr = stream->ieptr;
		if (!_iGRFseekfile(stream, prevname))
			return false;
		else
		{
			ptr = (INDEX_ENTRY*)((uint8*)(stream->ieptr + 1) + stream->ieptr->PathLength);
			stream->ieptr = oldptr;
		}
	}

	if ((uint32)((uint8*)ptr - (uint8*)stream->pie) >= ((GRF_HEADER*)stream->pie)->FileLength)
	{
		nextname = NULL;
		return true;
	}
	else if ((name = (char*)malloc(ptr->PathLength + 1)) == NULL)
		return false;
	else
	{
		memcpy(name, ptr->EntryPath, stream->ieptr->PathLength);
		name[ptr->PathLength] = '\0';
		nextname = name;
		return true;
	}
}

int Pal::Tools::GRF::GRFerror(GRFFILE* stream)
{
	if (stream == NULL)
		return 0;
	return stream->error;
}

void Pal::Tools::GRF::GRFclearerr(GRFFILE* stream)
{
	if (stream == NULL)
		return;
	stream->error = 0;
	stream->flag &= ~GRF_FLAG_EOF;
}

//�����汾����

bool Pal::Tools::GRF::GRFopenfile(GRFFILE* stream, const char* name, const char* mode, FILE*& fpout)
{
	FILE* fp;
	char* path;

	//������
	if (stream == NULL)
		return false;
	if ((stream->flag & GRF_FLAG_STANDALONE) == 0 || name == NULL || mode == NULL)
	{
		stream->error = EINVAL;
		return false;
	}
	//������
	if (!_iGRFseekfile(stream, name))
		return false;
	//�ҵ��Ϊ·������ռ�
	if ((path = (char*)malloc(strlen(stream->base) + strlen(name) + 1)) == NULL)
	{
		stream->error = errno;
		return false;
	}

	//����·��
	strcpy(path, stream->base);
	strcat(path, name);
	//����Ӧ�ļ�
	if (stream->flag & GRF_FLAG_CREATENEW)
		fp = fopen(path, "wb+");
	else
		fp = fopen(path, mode);
	free(path);
	if (fp == NULL)
	{
		stream->error = errno;
		return false;
	}
	else
	{
		fpout = fp;
		return true;
	}
}

bool Pal::Tools::GRF::GRFappendfile(GRFFILE* stream, const char* name)
{
	void* ptr;
	GRF_HEADER* hdr;
	size_t len;

	//������
	if (stream == NULL)
		return false;
	if ((stream->flag & GRF_FLAG_STANDALONE) == 0 || name == NULL ||
		(len = strlen(name)) > 0xffff)
	{
		stream->error = EINVAL;
		return false;
	}
	//��������Ƿ��Ѵ���
	if (_iGRFseekfile(stream, name))
	{
		stream->error = EEXIST;
		return false;
	}
	else if (stream->error != ENOENT)
		return false;

	//����ռ�
	hdr = (GRF_HEADER*)stream->pie;
	if ((ptr = realloc(stream->pie,	hdr->FileLength + sizeof(INDEX_ENTRY) + len)) == NULL)
		return false;
	else
		stream->pie = ptr;
	//����ֵ
	stream->ieptr = (INDEX_ENTRY*)((uint8*)stream->pie + hdr->FileLength);
	stream->ieptr->Offset = stream->ieptr->Length = 0;
	stream->ieptr->ResourceType = GRF_RESOURCE_TYPE_NONE;
	stream->ieptr->CompressAlgorithm= GRF_COMPRESS_ALGORITHM_NONE;
	stream->ieptr->PathLength = (uint16)len;
	hdr->FileLength += sizeof(INDEX_ENTRY) + (uint32)len;
	hdr->EntryCount++;
	stream->flag |= GRF_FLAG_MODIFIED;

	return true;
}

bool Pal::Tools::GRF::GRFremovefile(GRFFILE* stream, const char* name)
{
	GRF_HEADER* hdr;
	INDEX_ENTRY* ptr;
	void* buf;
	size_t len, count;

	//������
	if (stream == NULL)
		return false;
	if ((stream->flag & GRF_FLAG_STANDALONE) == 0 || name == NULL ||
		(len = strlen(name)) > 0xffff)
	{
		stream->error = EINVAL;
		return false;
	}
	//��������Ƿ����
	if (!_iGRFseekfile(stream, name))
		return false;

	//�Ƴ�����
	hdr = (GRF_HEADER*)stream->pie;
	ptr = (INDEX_ENTRY*)((uint8*)(stream->ieptr + 1) + len);
	count = (size_t)hdr->FileLength - (size_t)((uint8*)ptr - (uint8*)stream->pie);
	memmove(stream->ieptr, ptr, count);
	hdr->FileLength -= sizeof(INDEX_ENTRY) + (uint32)len;
	hdr->EntryCount--;
	stream->flag |= GRF_FLAG_MODIFIED;

	//��������Ŀռ�
	if ((buf = realloc(stream->pie, hdr->FileLength)) != NULL)
		stream->pie = buf;

	return true;
}

bool Pal::Tools::GRF::GRFrenamefile(GRFFILE* stream, const char* oldname, const char* newname)
{
	INDEX_ENTRY* ptr;
	void* buf;
	size_t count, len, len0, len1, len2;

	//������
	if (stream == NULL)
		return false;
	if ((stream->flag & GRF_FLAG_STANDALONE) == 0 || oldname == NULL || newname == NULL ||
		(len1 = strlen(oldname)) > 0xffff || (len2 = strlen(newname)) > 0xffff || len2 == 0)
	{
		stream->error = EINVAL;
		return false;
	}
	//���ԭ�ȵ������Ƿ����
	if (!_iGRFseekfile(stream, oldname))
		return false;
	//����µ������Ƿ����
	if (_iGRFseekfile(stream, newname))
	{
		stream->error = EEXIST;
		return false;
	}
	else if (stream->error != ENOENT)
		return false;

	//��������Ŀռ�
	len = ((GRF_HEADER*)stream->pie)->FileLength;
	len0 = len - len1 + len2;
	if ((buf = realloc(stream->pie, len0)) == NULL)
		return false;
	else
		stream->pie = buf;

	//Ϊ�������Ƴ��ռ�
	ptr = stream->ieptr + 1;
	count = len - len1 - (size_t)((uint8*)ptr - (uint8*)stream->pie);
	memmove((uint8*)ptr + len2, (uint8*)ptr + len1, count);
	//����������
	memcpy(ptr, newname, len2);
	((GRF_HEADER*)stream->pie)->FileLength = (uint32)len0;
	stream->flag |= GRF_FLAG_MODIFIED;

	return true;
}

bool Pal::Tools::GRF::GRFgetfileattr(GRFFILE* stream, const char* name, int attr, void* value)
{
	//������
	if (stream == NULL)
		return false;
	if ((stream->flag & GRF_FLAG_STANDALONE) == 0 || name == NULL || value == NULL ||
		attr < GRF_ATTRIBUTE_MINIMUM || attr > GRF_ATTRIBUTE_MAXIMUM)
	{
		stream->error = EINVAL;
		return false;
	}
	//��������Ƿ����
	if (!_iGRFseekfile(stream, name))
		return false;

	//������������ȷ����Ϊ
	switch(attr)
	{
	case GRF_ATTRIBUTE_RESOURCETYPE:
		*(uint8*)value = stream->ieptr->ResourceType;
		break;
	case GRF_ATTRIBUTE_COMPRESSALGORITHM:
		*(uint8*)value = stream->ieptr->CompressAlgorithm;
		break;
	case GRF_ATTRIBUTE_NAMELENGTH:
		*(uint16*)value = stream->ieptr->PathLength;
		break;
	default:
		return false;
	}

	return true;
}

bool Pal::Tools::GRF::GRFsetfileattr(GRFFILE* stream, const char* name, int attr, const void* value)
{
	//������
	if (stream == NULL)
		return false;
	if ((stream->flag & GRF_FLAG_STANDALONE) == 0 || name == NULL || value == NULL ||
		attr < GRF_ATTRIBUTE_MINIMUM || attr > GRF_ATTRIBUTE_MAXIMUM)
	{
		stream->error = EINVAL;
		return false;
	}
	//��������Ƿ����
	if (!_iGRFseekfile(stream, name))
		return false;

	//������������ȷ����Ϊ
	switch(attr)
	{
	case GRF_ATTRIBUTE_RESOURCETYPE:
		stream->ieptr->ResourceType = *(uint8*)value;
		break;
	case GRF_ATTRIBUTE_COMPRESSALGORITHM:
		stream->ieptr->CompressAlgorithm = *(uint8*)value;
		break;
	default:
		return false;
	}

	return true;
}

//���ɰ汾����

bool Pal::Tools::GRF::GRFseekfile(GRFFILE* stream, const char* name)
{
	INDEX_ENTRY* ptr;

	//������
	if (stream == NULL)
		return false;
	if (name == NULL || (stream->flag & GRF_FLAG_STANDALONE) != 0)
	{
		stream->error = EINVAL;
		return false;
	}

	ptr = stream->ieptr;
	//������
	if (_iGRFseekfile(stream, name))
	{		
		if (lseek(stream->fd, stream->ieptr->Offset, SEEK_SET) == -1)
		{
			stream->ieptr = ptr;
			return false;
		}
		else
		{
			stream->pos = stream->ieptr->Offset;
			return true;
		}
	}
	else
		return false;
}

bool Pal::Tools::GRF::GRFeof(GRFFILE* stream)
{
	//������
	if (stream == NULL)
		return false;
	if ((stream->flag & GRF_FLAG_STANDALONE) != 0)
	{
		stream->error = EINVAL;
		return false;
	}
	return ((stream->flag & GRF_FLAG_EOF) != 0);
}

bool Pal::Tools::GRF::GRFseek(GRFFILE* stream, long offset, int origin, long& newpos)
{
	long pos, npos;

	//������
	if (stream == NULL)
		return false;
	if (stream->fd == -1)
	{
		stream->error = EBADF;
		return false;
	}
	if ((stream->flag & GRF_FLAG_STANDALONE) != 0)
	{
		stream->error = EINVAL;
		return false;
	}

	//������ʼ��
	if (origin == SEEK_SET)
		pos = (long)((GRF_HEADER*)stream->pie)->DataOffset + offset;
	else if (origin == SEEK_CUR)
		pos = (long)stream->pos + offset;
	else if (origin == SEEK_END)
		pos = (long)(stream->ieptr->Offset + stream->ieptr->Length) + offset;
	else
		return false;

	//����λ��ֵ
	if (pos > (long)(stream->ieptr->Offset + stream->ieptr->Length))
		pos = (long)(stream->ieptr->Offset + stream->ieptr->Length);
	else if (pos < (long)(stream->ieptr->Offset))
		pos = (long)(stream->ieptr->Offset);

	//�ƶ��ļ�ָ��
	if ((npos = lseek(stream->fd, pos, SEEK_SET)) == -1)
		return false;
	else
	{
		stream->pos = npos;
		newpos = (long)(npos - stream->ieptr->Offset);
		return true;
	}
}

bool Pal::Tools::GRF::GRFtell(GRFFILE* stream, long& pos)
{
	//������
	if (stream == NULL)
		return false;
	if ((stream->flag & GRF_FLAG_STANDALONE) != 0)
	{
		stream->error = EINVAL;
		return false;
	}

	pos = (long)(stream->pos - stream->ieptr->Offset);
	return true;
}

bool Pal::Tools::GRF::GRFread(GRFFILE* stream, void* buffer, uint32 size, uint32& actual)
{
	bool flag;
	uint32 ret;

	//������
	if (stream == NULL)
		return false;
	if (stream->fd == -1)
	{
		stream->error = EBADF;
		return false;
	}
	if ((stream->flag & GRF_FLAG_STANDALONE) != 0)
	{
		stream->error = EINVAL;
		return false;
	}

	if (flag = (size + stream->pos > stream->ieptr->Offset + stream->ieptr->Length))
		size = stream->ieptr->Offset + stream->ieptr->Length - stream->pos;
	if ((ret = (uint32)read(stream->fd, buffer, size)) == -1)
		return false;
	else
	{
		if (ret <= size && flag)
			stream->flag |= GRF_FLAG_EOF;
		else
			stream->flag &= ~GRF_FLAG_EOF;
		stream->pos += size;
		actual = ret;
		return true;
	}
}

bool Pal::Tools::GRF::GRFgetattr(GRFFILE* stream, int attr, void* value)
{
	//������
	if (stream == NULL)
		return false;
	if ((stream->flag & GRF_FLAG_STANDALONE) != 0 || value == NULL ||
		attr < GRF_ATTRIBUTE_MINIMUM || attr > GRF_ATTRIBUTE_MAXIMUM)
	{
		stream->error = EINVAL;
		return false;
	}

	//������������ȷ����Ϊ
	switch(attr)
	{
	case GRF_ATTRIBUTE_OFFSET:
		*(uint32*)value = stream->ieptr->Offset;
		break;
	case GRF_ATTRIBUTE_LENGTH:
		*(uint32*)value = stream->ieptr->Length;
		break;
	case GRF_ATTRIBUTE_RESOURCETYPE:
		*(uint8*)value = stream->ieptr->ResourceType;
		break;
	case GRF_ATTRIBUTE_COMPRESSALGORITHM:
		*(uint8*)value = stream->ieptr->CompressAlgorithm;
		break;
	case GRF_ATTRIBUTE_NAMELENGTH:
		*(uint16*)value = stream->ieptr->PathLength;
		break;
	case GRF_ATTRIBUTE_NAME:
		memcpy(value, stream->ieptr->EntryPath, stream->ieptr->PathLength);
		((char*)value)[stream->ieptr->PathLength] = '\0';
		break;
	}

	return true;
}
/*
bool Pal::Tools::GRF::GRFPackage(const char* pszGRF, const char* pszBasePath, const char* pszNewFile)
{
	GRFFILE* grf;
	int grftype, fd;
	char* name = NULL;
	void* buf;
	GRF_HEADER* hdr;
	INDEX_ENTRY* ptr;
	INDEX_ENTRY* ptrend;

	//����������
	if (pszGRF == NULL || pszNewFile == NULL || strlen(pszGRF) == 0 || strlen(pszNewFile) == 0)
		return false;

	//�� GRF �ļ�
	if (!Pal::Tools::GRF::GRFopen(pszGRF, pszBasePath, 0, grf))
		return false;
	//��� GRF �ļ�����
	hdr = (GRF_HEADER*)grf->pie;
	Pal::Tools::GRF::GRFgettype(grf, grftype);
	if (grftype != GRF_TYPE_STANDALONE || hdr->EntryCount == 0)
	{
		Pal::Tools::GRF::GRFclose(grf);
		return true;
	}
	else
	{
		ptr = (INDEX_ENTRY*)(hdr + 1);
		ptrend = (INDEX_ENTRY*)((uint8*)grf->pie + hdr->FileLength);
	}

	//�����µ� GRF �ļ�
	if ((fd = open(pszNewFile, O_CREAT | O_TRUNC | O_BINARY | O_RDWR, S_IREAD | S_IWRITE)) == -1)
	{
		Pal::Tools::GRF::GRFclose(grf);
		return false;
	}

	//�ƶ��ļ�ָ�뵽������
	if (lseek(fd, hdr->FileLength, SEEK_SET) == -1)
	{
		close(fd);
		Pal::Tools::GRF::GRFclose(grf);
		return false;
	}
	//�����ļ�ͷ
	hdr->DataOffset = hdr->FileLength;
	ptr->Offset = hdr->DataOffset;

	//�������ݻ�����
	if ((buf = malloc(0x4000)) == NULL)
	{
		close(fd);
		Pal::Tools::GRF::GRFclose(grf);
		return false;
	}
	//�������ֻ�����
	if ((name = malloc(ptr->PathLength + 1)) == NULL)
	{
		free(buf);
		close(fd);
		Pal::Tools::GRF::GRFclose(grf);
		return false;
	}

	while(ptr < ptrend)
	{
		//ȡ������
		FILE* fp;
		void* temp;
		int dfd;
		uint32 ret, len = 0;

		if ((temp = realloc(name, ptr->PathLength + 1)) == NULL)
		{
			free(name);
			free(buf);
			close(fd);
			Pal::Tools::GRF::GRFclose(grf);
			return false;
		}
		name = (char*)temp;
		memcpy(name, ptr->EntryPath, ptr->PathLength);
		name[ptr->PathLength] = '\0';
		//���ļ�
		if (!Pal::Tools::GRF::GRFopenfile(grf, name, "r+", fp))
			continue;
		setbuf(fp, NULL);
		dfd = fileno(fp);
		while(true)
		{
			if ((ret = (uint32)read(dfd, buf, 0x4000)) == -1)
				break;
			if (ret > 0)
			{
				if (write(fd, buf, ret) == -1)
					break;
				len += ret;
			}
			if (eof(fd))
				break;
		}

	}

	return true;
}

bool Pal::Tools::GRF::GRFExtract(const char* pszGRF, const char* pszBasePath, const char* pszNewFile)
{
	char* pszOldLocale;
	char* pszFile;
	size_t pathlen, namelen;

	//����������
	if (pszGRF == NULL || pszNewFile == NULL || strlen(pszGRF) == 0 || strlen(pszNewFile) == 0)
		return false;

	//���Ƚ�·�������Ƶ���ʱ������
	if (pszBasePath == NULL || (pathlen = strlen(pszBasePath)) == 0)
	{
		pszFile = NULL;
		pathlen = 0;
	}
	else
	{
		if ((pszFile = (char*)malloc(pathlen + 1)) == NULL)
			return false;
		strcpy(pszFile, pszBasePath);
	}

	return true;
}
*/
