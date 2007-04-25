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

static inline errno_t _icheckpath(const char* pszFilePath, char*& pszPath)
{
	size_t pathlen;
	char* newpath;

	if (pszFilePath != NULL && (pathlen = strlen(pszFilePath)) > 0)
	{
		char* curpath;
		char* pch;

		// �Դ����·�����м��
		if ((newpath = (char*)malloc(pathlen + 2)) == NULL)
			return ENOMEM;
		strcpy(newpath, pszFilePath);
		newpath[pathlen + 1] = '\0';

		// ȡ��ǰ����Ŀ¼�Ա���
		if ((curpath = getcwd(NULL, 0)) == NULL)
		{
			// û���㹻���ڴ����ڷ��仺����
			free(newpath);
			return ENOMEM;
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
				return ENOENT;
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
		pszPath = newpath;
		return 0;
	}
	else
	{
		if ((newpath = (char*)malloc(1)) == NULL)
			return ENOMEM;
		else
		{
			newpath[0] = '\0';
			pszPath = newpath;
			return 0;
		}
	}
}

static errno_t _iGRFseekfile(GRFFILE* stream, const char* name)
{
	INDEX_ENTRY* ptr;
	uint32 len;

	//������
	if (stream->pie == NULL || (len = (uint32)strlen(name)) == 0)
	{
		stream->error = EINVAL;
		return EINVAL;
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
			return 0;
		}
		else
			ptr = (INDEX_ENTRY*)((uint8*)(ptr + 1) + ptr->PathLength);
	}
	stream->error = ENOENT;
	return ENOENT;
}

//��������

errno_t Pal::Tools::GRF::GRFopen(const char* grffile, const char* base, bool create, bool truncate, GRFFILE*& stream)
{
	GRFFILE* grf;
	char* _base;
	void* ptr;
	int fd, _mode = O_BINARY;
	long flen;
	bool oflag;
	uint32 flag;
	errno_t err;

	//����������
	if (grffile == NULL)
		return EINVAL;
	if ((err = _icheckpath(base, _base)) != 0)
		return err;

	//���Դ��ļ�����ȷ���򿪷�ʽ
	if ((fd = open(grffile, O_BINARY | O_RDONLY)) == -1)
		oflag = true;
	else
	{
		GRF_HEADER hdr;
		memset(&hdr, 0, sizeof(GRF_HEADER));
		read(fd, &hdr, sizeof(GRF_HEADER));
		if (hdr.DataOffset == 0)
			oflag = true;
		else
			oflag = false;
		close(fd);
	}

	//���ô�ģʽ
	if (create)
		_mode |= O_CREAT;
	if (truncate)
	{
		_mode |= O_TRUNC;
		oflag = true;
	}
	_mode |= oflag ? O_RDWR : O_RDONLY;

	//���Դ������ļ�
	if ((fd = open(grffile, _mode, S_IREAD | S_IWRITE)) == -1)
	{
		err = errno;
		free(_base);
		return err;
	}
	//ȡ�ļ�����
	if (lseek(fd, 0, SEEK_END) == -1 ||
		(flen = tell(fd)) == -1)
	{
		err = errno;
		close(fd);
		free(_base);
		return err;
	}
	if (flen == 0)
	{
		//����Ϊ 0�������½���ض�
		//����ռ�
		if ((ptr = malloc(sizeof(GRF_HEADER))) == NULL)
		{
			err = errno;
			close(fd);
			free(_base);
			return err;
		}
		//�������
		memset(ptr, 0, sizeof(GRF_HEADER));
		strcpy((char*)ptr, "GRF");
		((GRF_HEADER*)ptr)->FileLength = sizeof(GRF_HEADER);
		//д���ļ�
		if (write(fd, ptr, sizeof(GRF_HEADER)) < sizeof(GRF_HEADER))
		{
			err = errno;
			free(ptr);
			close(fd);
			free(_base);
			return err;
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
			err = errno;
			close(fd);
			free(_base);
			return err;
		}
		len = hdr.DataOffset == 0 ? hdr.FileLength : hdr.DataOffset;
		if ((ptr = malloc(len)) == NULL)
		{
			err = errno;
			close(fd);
			free(_base);
			return err;
		}
		//��ȡ����
		if ((uint32)read(fd, ptr, len) < len)
		{
			err = errno;
			free(ptr);
			close(fd);
			free(_base);
			return err;
		}
		if (hdr.DataOffset == 0)
			flag = GRF_FLAG_STANDALONE;
		else
			flag = 0;
	}

	//Ϊ�ṹ����ռ䲢���
	if ((grf = (GRFFILE*)malloc(sizeof(GRFFILE))) == NULL)
	{
		err = errno;
		free(ptr);
		close(fd);
		free(_base);
		return err;
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
		return 0;
	}
}

errno_t Pal::Tools::GRF::GRFclose(GRFFILE* stream)
{
	int fd;

	if (stream == NULL)
		return EINVAL;
	GRFflush(stream);
	if (stream->base)
		free(stream->base);
	if (stream->pie)
		free(stream->pie);
	fd = stream->fd;
	free(stream);
	if (fd != -1)
	{
		close(fd);
		return errno;
	}
	else
		return 0;
}

errno_t Pal::Tools::GRF::GRFflush(GRFFILE* stream)
{
	if (stream == NULL)
		return EINVAL;
	if (stream->fd == -1)
	{
		stream->error = EBADF;
		return EBADF;
	}
	if (stream->pie == NULL)
	{
		stream->error = EINVAL;
		return EINVAL;
	}

	if ((stream->flag & GRF_FLAG_MODIFIED) != 0)
	{
		uint32 len = ((GRF_HEADER*)stream->pie)->FileLength;

		if (lseek(stream->fd, 0, SEEK_SET) == -1 ||
			(uint32)write(stream->fd, stream->pie, len) < len)
		{
			stream->error = errno;
			return stream->error;
		}
		stream->flag &= ~GRF_FLAG_MODIFIED;
	}

	return 0;
}

errno_t Pal::Tools::GRF::GRFgettype(GRFFILE* stream, int& type)
{
	if (stream == NULL)
		return EINVAL;

	if ((stream->flag & GRF_FLAG_STANDALONE) == 0)
		type = GRF_TYPE_INTEGRATE;
	else
		type = GRF_TYPE_STANDALONE;
	return 0;
}

errno_t Pal::Tools::GRF::GRFenumname(GRFFILE* stream, const char* prevname, char*& nextname)
{
	INDEX_ENTRY* ptr;
	char* name;
	errno_t err;

	//������
	if (stream == NULL)
		return EINVAL;

	//������
	if (prevname == NULL)
		ptr = (INDEX_ENTRY*)((GRF_HEADER*)stream->pie + 1);
	else
	{
		INDEX_ENTRY* oldptr = stream->ieptr;
		if ((err = _iGRFseekfile(stream, prevname)) != 0)
		{
			stream->error = err;
			return err;
		}
		else
		{
			ptr = (INDEX_ENTRY*)((uint8*)(stream->ieptr + 1) + stream->ieptr->PathLength);
			stream->ieptr = oldptr;
		}
	}

	if ((uint32)((uint8*)ptr - (uint8*)stream->pie) >= ((GRF_HEADER*)stream->pie)->FileLength)
	{
		if (nextname)
			free(nextname);
		nextname = NULL;
		return 0;
	}
	else if ((name = (char*)realloc(nextname, ptr->PathLength + 1)) == NULL)
	{
		stream->error = ENOMEM;
		return ENOMEM;
	}
	else
	{
		memcpy(name, ptr->EntryPath, stream->ieptr->PathLength);
		name[ptr->PathLength] = '\0';
		nextname = name;
		return 0;
	}
}

errno_t Pal::Tools::GRF::GRFerror(GRFFILE* stream)
{
	if (stream == NULL)
		return EINVAL;
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

errno_t Pal::Tools::GRF::GRFgetfilename(GRFFILE* stream, const char* name, char*& filename, bool& forcenew)
{
	char* path;
	errno_t err;

	//������
	if (stream == NULL)
		return EINVAL;
	if ((stream->flag & GRF_FLAG_STANDALONE) == 0 || name == NULL)
	{
		stream->error = EINVAL;
		return EINVAL;
	}
	//������
	if ((err = _iGRFseekfile(stream, name)) != 0)
	{
		stream->error = err;
		return err;
	}
	//�ҵ��Ϊ·������ռ�
	if ((path = (char*)malloc(strlen(stream->base) + strlen(name) + 1)) == NULL)
	{
		stream->error = errno;
		return stream->error;
	}

	//����·��
	strcpy(path, stream->base);
	strcat(path, name);
	filename = path;
	forcenew = ((stream->flag & GRF_FLAG_CREATENEW) != 0);
	return 0;
}

errno_t Pal::Tools::GRF::GRFopenfile(GRFFILE* stream, const char* name, const char* mode, FILE*& fpout)
{
	FILE* fp;
	char* path;
	errno_t err;

	//������
	if (stream == NULL)
		return EINVAL;
	if ((stream->flag & GRF_FLAG_STANDALONE) == 0 || name == NULL || mode == NULL)
	{
		stream->error = EINVAL;
		return EINVAL;
	}
	//������
	if ((err = _iGRFseekfile(stream, name)) != 0)
	{
		stream->error = err;
		return err;
	}
	//�ҵ��Ϊ·������ռ�
	if ((path = (char*)malloc(strlen(stream->base) + strlen(name) + 1)) == NULL)
	{
		stream->error = errno;
		return stream->error;
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
		return stream->error;
	}
	else
	{
		fpout = fp;
		return 0;
	}
}

errno_t Pal::Tools::GRF::GRFappendfile(GRFFILE* stream, const char* name)
{
	void* ptr;
	GRF_HEADER* hdr;
	size_t len;
	errno_t err;

	//������
	if (stream == NULL)
		return EINVAL;
	if ((stream->flag & GRF_FLAG_STANDALONE) == 0 || name == NULL)
	{
		stream->error = EINVAL;
		return EINVAL;
	}
	if ((len = strlen(name)) > 0xffff)
	{
		stream->error = ERANGE;
		return ERANGE;
	}
	//��������Ƿ��Ѵ���
	if ((err = _iGRFseekfile(stream, name)) == 0)
	{
		stream->error = EEXIST;
		return EEXIST;
	}
	else if (err != ENOENT)
		return err;

	//����ռ�
	hdr = (GRF_HEADER*)stream->pie;
	if ((ptr = realloc(stream->pie,	hdr->FileLength + sizeof(INDEX_ENTRY) + len)) == NULL)
	{
		stream->error = ENOMEM;
		return ENOMEM;
	}
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

	return 0;
}

errno_t Pal::Tools::GRF::GRFremovefile(GRFFILE* stream, const char* name)
{
	GRF_HEADER* hdr;
	INDEX_ENTRY* ptr;
	void* buf;
	size_t len, count;
	errno_t err;

	//������
	if (stream == NULL)
		return EINVAL;
	if ((stream->flag & GRF_FLAG_STANDALONE) == 0 || name == NULL)
	{
		stream->error = EINVAL;
		return EINVAL;
	}
	if ((len = strlen(name)) > 0xffff)
	{
		stream->error = ERANGE;
		return ERANGE;
	}
	//��������Ƿ����
	if ((err = _iGRFseekfile(stream, name)) != 0)
		return err;

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

	return 0;
}

errno_t Pal::Tools::GRF::GRFrenamefile(GRFFILE* stream, const char* oldname, const char* newname)
{
	INDEX_ENTRY* ptr;
	void* buf;
	size_t count, len, len0, len1, len2;
	errno_t err;

	//������
	if (stream == NULL)
		return EINVAL;
	if ((stream->flag & GRF_FLAG_STANDALONE) == 0 || oldname == NULL || newname == NULL)
	{
		stream->error = EINVAL;
		return EINVAL;
	}
	if ((len1 = strlen(oldname)) > 0xffff || (len2 = strlen(newname)) > 0xffff || len2 == 0)
	{
		stream->error = ERANGE;
		return ERANGE;
	}
	//���ԭ�ȵ������Ƿ����
	if ((err = _iGRFseekfile(stream, oldname)) != 0)
		return err;
	//����µ������Ƿ����
	if ((err = _iGRFseekfile(stream, newname)) == 0)
	{
		stream->error = EEXIST;
		return EEXIST;
	}
	else if (err != ENOENT)
		return err;

	//��������Ŀռ�
	len = ((GRF_HEADER*)stream->pie)->FileLength;
	len0 = len - len1 + len2;
	if ((buf = realloc(stream->pie, len0)) == NULL)
	{
		stream->error = ENOMEM;
		return ENOMEM;
	}
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

	return 0;
}

errno_t Pal::Tools::GRF::GRFgetfileattr(GRFFILE* stream, const char* name, int attr, void* value)
{
	errno_t err;

	//������
	if (stream == NULL)
		return EINVAL;
	if ((stream->flag & GRF_FLAG_STANDALONE) == 0 || name == NULL || value == NULL)
	{
		stream->error = EINVAL;
		return EINVAL;
	}
	if (attr < GRF_ATTRIBUTE_MINIMUM || attr > GRF_ATTRIBUTE_MAXIMUM)
	{
		stream->error = ERANGE;
		return ERANGE;
	}
	//��������Ƿ����
	if ((err = _iGRFseekfile(stream, name)) != 0)
		return err;

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
		stream->error = ERANGE;
		return ERANGE;
	}

	return 0;
}

errno_t Pal::Tools::GRF::GRFsetfileattr(GRFFILE* stream, const char* name, int attr, const void* value)
{
	errno_t err;

	//������
	if (stream == NULL)
		return EINVAL;
	if ((stream->flag & GRF_FLAG_STANDALONE) == 0 || name == NULL || value == NULL)
	{
		stream->error = EINVAL;
		return EINVAL;
	}
	if (attr < GRF_ATTRIBUTE_MINIMUM || attr > GRF_ATTRIBUTE_MAXIMUM)
	{
		stream->error = ERANGE;
		return ERANGE;
	}
	//��������Ƿ����
	if ((err = _iGRFseekfile(stream, name)) != 0)
		return err;

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
		stream->error = ERANGE;
		return ERANGE;
	}

	return 0;
}

//���ɰ汾����

errno_t Pal::Tools::GRF::GRFseekfile(GRFFILE* stream, const char* name)
{
	INDEX_ENTRY* ptr;
	errno_t err;

	//������
	if (stream == NULL)
		return EINVAL;
	if (name == NULL || (stream->flag & GRF_FLAG_STANDALONE) != 0)
	{
		stream->error = EINVAL;
		return EINVAL;
	}

	ptr = stream->ieptr;
	//������
	if ((err = _iGRFseekfile(stream, name)) == 0)
	{		
		if (lseek(stream->fd, stream->ieptr->Offset, SEEK_SET) == -1)
		{
			stream->ieptr = ptr;
			stream->error = ENOENT;
			return ENOENT;
		}
		else
		{
			stream->pos = stream->ieptr->Offset;
			return 0;
		}
	}
	else
		return err;
}

errno_t Pal::Tools::GRF::GRFeof(GRFFILE* stream)
{
	//������
	if (stream == NULL)
		return EINVAL;
	if ((stream->flag & GRF_FLAG_STANDALONE) != 0)
	{
		stream->error = EINVAL;
		return EINVAL;
	}
	return ((stream->flag & GRF_FLAG_EOF) != 0) ? EOF : 0;
}

errno_t Pal::Tools::GRF::GRFseek(GRFFILE* stream, long offset, int origin, long& newpos)
{
	long pos, npos;

	//������
	if (stream == NULL)
		return EINVAL;
	if (stream->fd == -1)
	{
		stream->error = EBADF;
		return EBADF;
	}
	if ((stream->flag & GRF_FLAG_STANDALONE) != 0)
	{
		stream->error = EINVAL;
		return EINVAL;
	}

	//������ʼ��
	if (origin == SEEK_SET)
		pos = (long)((GRF_HEADER*)stream->pie)->DataOffset + offset;
	else if (origin == SEEK_CUR)
		pos = (long)stream->pos + offset;
	else if (origin == SEEK_END)
		pos = (long)(stream->ieptr->Offset + stream->ieptr->Length) + offset;
	else
	{
		stream->error = ERANGE;
		return ERANGE;
	}

	//����λ��ֵ
	if (pos > (long)(stream->ieptr->Offset + stream->ieptr->Length))
		pos = (long)(stream->ieptr->Offset + stream->ieptr->Length);
	else if (pos < (long)(stream->ieptr->Offset))
		pos = (long)(stream->ieptr->Offset);

	//�ƶ��ļ�ָ��
	if ((npos = lseek(stream->fd, pos, SEEK_SET)) == -1)
	{
		stream->error = errno;
		return stream->error;
	}
	else
	{
		stream->pos = npos;
		newpos = (long)(npos - stream->ieptr->Offset);
		return 0;
	}
}

errno_t Pal::Tools::GRF::GRFtell(GRFFILE* stream, long& pos)
{
	//������
	if (stream == NULL)
		return EINVAL;
	if ((stream->flag & GRF_FLAG_STANDALONE) != 0)
	{
		stream->error = EINVAL;
		return EINVAL;
	}

	pos = (long)(stream->pos - stream->ieptr->Offset);
	return 0;
}

errno_t Pal::Tools::GRF::GRFread(GRFFILE* stream, void* buffer, uint32 size, uint32& actual)
{
	bool flag;
	uint32 ret;

	//������
	if (stream == NULL)
		return EINVAL;
	if (stream->fd == -1)
	{
		stream->error = EBADF;
		return EBADF;
	}
	if ((stream->flag & GRF_FLAG_STANDALONE) != 0)
	{
		stream->error = EINVAL;
		return EINVAL;
	}

	if (flag = (size + stream->pos > stream->ieptr->Offset + stream->ieptr->Length))
		size = stream->ieptr->Offset + stream->ieptr->Length - stream->pos;
	if ((ret = (uint32)read(stream->fd, buffer, size)) == -1)
	{
		stream->error = errno;
		return stream->error;
	}
	else
	{
		if (ret <= size && flag)
			stream->flag |= GRF_FLAG_EOF;
		else
			stream->flag &= ~GRF_FLAG_EOF;
		stream->pos += size;
		actual = ret;
		return 0;
	}
}

errno_t Pal::Tools::GRF::GRFgetattr(GRFFILE* stream, int attr, void* value)
{
	//������
	if (stream == NULL)
		return EINVAL;
	if ((stream->flag & GRF_FLAG_STANDALONE) != 0 || value == NULL)
	{
		stream->error = EINVAL;
		return EINVAL;
	}
	if (attr < GRF_ATTRIBUTE_MINIMUM || attr > GRF_ATTRIBUTE_MAXIMUM)
	{
		stream->error = ERANGE;
		return ERANGE;
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

	return 0;
}

errno_t Pal::Tools::GRF::GRFPackage(const char* pszGRF, const char* pszBasePath, const char* pszNewFile)
{
	int fdold, fdnew;
	size_t pathlen;
	char* name;
	void* buf;
	bool flag;
	long idxpos;
	GRF_HEADER hdr;
	INDEX_ENTRY cur, old;
	errno_t err;

	//����������
	if (pszGRF == NULL || pszNewFile == NULL || strlen(pszGRF) == 0 || strlen(pszNewFile) == 0)
		return EINVAL;
	if ((err = _icheckpath(pszBasePath, name)) != 0)
		return err;

	//��ԭ GRF �ļ�
	if ((fdold = open(pszGRF, O_BINARY | O_RDONLY)) == -1)
	{
		err = errno;
		free(name);
		return err;
	}
	else
	{
		//��� GRF �ļ�
		if (read(fdold, &hdr, sizeof(GRF_HEADER)) < sizeof(GRF_HEADER) ||
			memcmp(hdr.Signature, "GRF", 4) != 0 ||
			hdr.DataOffset != 0 || hdr.EntryCount == 0)
		{
			err = errno;
			close(fdold);
			free(name);
			return err;
		}
		else
			hdr.DataOffset = hdr.FileLength;
	}
	//�����µ� GRF �ļ�
	if ((fdnew = open(pszNewFile, O_CREAT | O_TRUNC | O_BINARY | O_RDWR, S_IREAD | S_IWRITE)) == -1)
	{
		err = errno;
		close(fdold);
		free(name);
		return err;
	}
	memset(&old, 0, sizeof(INDEX_ENTRY));
	old.Offset = hdr.DataOffset;
	pathlen = strlen(name);
	idxpos = sizeof(GRF_HEADER);

	//�������ݻ�����
	if ((buf = malloc(0x4000)) == NULL)
	{
		err = errno;
		close(fdnew);
		close(fdold);
		free(name);
		return err;
	}
	else
		flag = true;

	for(uint32 i = 0; i < hdr.EntryCount; i++, old = cur)
	{
		void* temp;
		int fddat;
		long ret, datalen = 0;

		//��ȡ������
		if (read(fdold, &cur, sizeof(INDEX_ENTRY)) < sizeof(INDEX_ENTRY))
		{
			err = errno;
			flag = false;
			break;
		}
		else
			cur.Offset = old.Offset + old.Length;
		//�������ֿռ�
		if ((temp = realloc(name, pathlen + cur.PathLength + 1)) == NULL)
		{
			err = errno;
			flag = false;
			break;
		}
		else
			name = (char*)temp;
		//��ȡ���ֲ�ƴ�ӵ�·����
		if (read(fdold, name + pathlen, cur.PathLength) < cur.PathLength)
		{
			err = errno;
			flag = false;
			break;
		}
		name[pathlen + cur.PathLength] = '\0';
		//�ƶ��ļ�ָ�뵽������
		if (lseek(fdnew, cur.Offset, SEEK_SET) == -1)
		{
			err = errno;
			flag = false;
			break;
		}
		//�������ļ�
		if ((fddat = open(name, O_BINARY | O_RDONLY)) == -1)
			continue;
		while((ret = read(fddat, buf, 0x4000)) > 0)
		{
			if (write(fdnew, buf, ret) < ret)
			{
				err = errno;
				flag = false;
				break;
			}
			datalen += ret;
		}
		close(fddat);
		cur.Length = datalen;
		//�ƶ��ļ�ָ�뵽������
		if (lseek(fdnew, idxpos, SEEK_SET) == -1)
		{
			err = errno;
			flag = false;
			break;
		}
		//д������
		if (write(fdnew, &cur, sizeof(INDEX_ENTRY)) < sizeof(INDEX_ENTRY) ||
			write(fdnew, name + pathlen, cur.PathLength) < cur.PathLength)
		{
			err = errno;
			flag = false;
			break;
		}
		idxpos += sizeof(INDEX_ENTRY) + cur.PathLength;
	}
	//�����ļ�ͷ
	hdr.FileLength = cur.Offset + cur.Length;
	if (lseek(fdnew, 0, SEEK_SET) != -1)
		write(fdnew, &hdr, sizeof(GRF_HEADER));

	//��β����
	free(buf);
	close(fdnew);
	close(fdold);
	free(name);
	return flag ? 0 : err;
}

errno_t Pal::Tools::GRF::GRFExtract(const char* pszGRF, const char* pszBasePath, const char* pszNewFile)
{
	int fdold, fdnew;
	size_t pathlen;
	char* name;
	void* buf;
	bool flag;
	long idxpos;
	GRF_HEADER hdr;
	INDEX_ENTRY cur;
	errno_t err;

	//����������
	if (pszGRF == NULL || pszNewFile == NULL || strlen(pszGRF) == 0 || strlen(pszNewFile) == 0)
		return EINVAL;
	if ((err = _icheckpath(pszBasePath, name)) != 0)
		return err;

	//��ԭ GRF �ļ�
	if ((fdold = open(pszGRF, O_BINARY | O_RDONLY)) == -1)
	{
		err = errno;
		free(name);
		return err;
	}
	else
	{
		//��� GRF �ļ�
		if (read(fdold, &hdr, sizeof(GRF_HEADER)) < sizeof(GRF_HEADER) ||
			memcmp(hdr.Signature, "GRF", 4) != 0 ||
			hdr.DataOffset == 0 || hdr.EntryCount == 0)
		{
			err = errno;
			close(fdold);
			free(name);
			return err;
		}
		else
			hdr.DataOffset = hdr.FileLength;
	}
	//�����µ� GRF �ļ�
	if ((fdnew = open(pszNewFile, O_CREAT | O_TRUNC | O_BINARY | O_RDWR, S_IREAD | S_IWRITE)) == -1)
	{
		err = errno;
		close(fdold);
		free(name);
		return err;
	}
	pathlen = strlen(name);
	idxpos = sizeof(GRF_HEADER);

	//�������ݻ�����
	if ((buf = malloc(0x4000)) == NULL)
	{
		err = errno;
		close(fdnew);
		close(fdold);
		free(name);
		return err;
	}
	else
		flag = true;

	for(uint32 i = 0; i < hdr.EntryCount; i++)
	{
		void* temp;
		int fddat;
		long ret, datalen;

		//�ƶ��ļ�ָ�뵽������
		if (lseek(fdold, idxpos, SEEK_SET) == -1)
		{
			err = errno;
			flag = false;
			break;
		}
		//��ȡ������
		if (read(fdold, &cur, sizeof(INDEX_ENTRY)) < sizeof(INDEX_ENTRY))
		{
			err = errno;
			flag = false;
			break;
		}
		//�������ֿռ�
		if ((temp = realloc(name, pathlen + cur.PathLength + 1)) == NULL)
		{
			err = errno;
			flag = false;
			break;
		}
		else
			name = (char*)temp;
		//��ȡ���ֲ�ƴ�ӵ�·����
		if (read(fdold, name + pathlen, cur.PathLength) < cur.PathLength)
		{
			err = errno;
			flag = false;
			break;
		}
		name[pathlen + cur.PathLength] = '\0';
		//�ƶ��ļ�ָ�뵽������
		if (lseek(fdold, cur.Offset, SEEK_SET) == -1)
		{
			err = errno;
			flag = false;
			break;
		}
		//�������ļ�
		if ((fddat = open(name, O_BINARY | O_WRONLY | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE)) == -1)
			continue;
		//д������
		for(datalen = cur.Length; datalen > 0;)
		{
			if (datalen >= 0x4000)
				ret = read(fdold, buf, 0x4000);
			else
				ret = read(fdold, buf, datalen);
			if (ret > 0)
			{
				if (write(fddat, buf, ret) < ret)
				{
					err = errno;
					flag = false;
					break;
				}
				datalen -= ret;
			}
			else
				break;
		}
		close(fddat);
		cur.Offset = cur.Length = 0;
		//�ƶ��ļ�ָ�뵽������
		if (lseek(fdnew, idxpos, SEEK_SET) == -1)
		{
			err = errno;
			flag = false;
			break;
		}
		//д������
		if (write(fdnew, &cur, sizeof(INDEX_ENTRY)) < sizeof(INDEX_ENTRY) ||
			write(fdnew, name + pathlen, cur.PathLength) < cur.PathLength)
		{
			err = errno;
			flag = false;
			break;
		}
		idxpos += sizeof(INDEX_ENTRY) + cur.PathLength;
	}
	//�����ļ�ͷ
	hdr.FileLength = idxpos;
	hdr.DataOffset = 0;
	if (lseek(fdnew, 0, SEEK_SET) != -1)
		write(fdnew, &hdr, sizeof(GRF_HEADER));

	//��β����
	free(buf);
	close(fdnew);
	close(fdold);
	free(name);
	return flag ? 0 : err;
}
