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

#include "pallib.h"
using namespace Pal::Tools;

Pal::Tools::CGameResourceFile::CGameResourceFile()	:
m_iErrorCode(GRF_ERROR_CODE_SUCCESSFUL),
m_pszDataFileName(NULL), m_pszIndexFileName(NULL)
{
	memset(&m_Header, 0, sizeof(GRF_HEADER));
}

Pal::Tools::CGameResourceFile::~CGameResourceFile()
{
	if (m_pszIndexFileName)
		delete [] m_pszIndexFileName;
	if (m_pszDataFileName)
		delete [] m_pszDataFileName;
}

bool Pal::Tools::CGameResourceFile::SetResourceFilePathAndName(const char* pszFilePath,
															   const char* pszFileName,
															   bool bCreateOnNotExist)
{
	char* fulldataname;
	char* fullfilename;
	int iFileHandle;
	GRF_HEADER header;
	size_t pathlen, namelen, addlen = 0;

	// ��鴫����ļ����Ƿ�Ϊ��
	if (pszFileName == NULL || (namelen = strlen(pszFileName)) == 0)
	{
		m_iErrorCode = GRF_ERROR_CODE_INVAILDPARAMETER;
		return false;
	}

	// ���������·��
	if (pszFilePath != NULL && (pathlen = strlen(pszFilePath)) > 0)
	{
		char* curpath;
		char* newpath;
		char* pch;

		// �Դ����·�����м��
		if ((newpath = new char [pathlen + 2]) == NULL)
		{
			// û���㹻���ڴ����ڷ��仺����
			m_iErrorCode = GRF_ERROR_CODE_INTERNALERROR;
			return false;
		}
		strcpy(newpath, pszFilePath);
		newpath[pathlen + 1] = '\0';

		// ȡ��ǰ����Ŀ¼�Ա���
		if ((curpath = getcwd(NULL, 0)) == NULL)
		{
			// û���㹻���ڴ����ڷ��仺����
			delete [] newpath;

			m_iErrorCode = GRF_ERROR_CODE_INTERNALERROR;
			return false;
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
				bool flag = true;

				if (bCreateOnNotExist)
				{
					// ���Դ���Ŀ¼
#					if	defined(WIN32)
					if (mkdir(newpath) == 0)
#					else
					if (mkdir(newpath, 0777) == 0)
#					endif
						flag = false;
				}
				
				if (flag)
				{
					// ������Ŀ¼���򴴽�Ŀ¼ʱ����������Ӧ�����ش���
					m_iErrorCode = GRF_ERROR_CODE_PATHNOTEXIST;
					free(curpath);
					delete [] newpath;
					return false;
				}
			}

			// �ò�ɴ�����Ѿ������˸ò�Ŀ¼
			*(pch + 1) = ch1;

			// �Ƿ��Ѿ���鵽����·���Ľ�β
			if (ch0 == '\0')
				break;
		}

		// �����µ�ǰĿ¼����Ϊ֮ǰ��Ŀ¼
		chdir(curpath);

		free(curpath);
		delete [] newpath;
	}
	else
		pathlen = 0;

	// ���������·��������·���Ƿ��Էָ�����β
	if (pathlen > 0)
		addlen = (pszFilePath[pathlen - 1] == '\\' || pszFilePath[pathlen - 1] == '/') ? 0 : 1;

	// Ϊ�ļ�������·��������ռ�
	if ((fullfilename = new char [pathlen + namelen + addlen + 1]) == NULL)
	{
		m_iErrorCode = GRF_ERROR_CODE_INTERNALERROR;
		return false;
	}

	// ���ȸ���·����
	if (pathlen > 0)
		strcpy(fullfilename, pszFilePath);
	// ��ξ����Ƿ��Ϸָ���
	if (addlen > 0)
#		if	defined(WIN32)
		fullfilename[pathlen] = '\\';
#		else
		fullfilename[pathlen] = '/';
#		endif
	// ������ļ���
	strcpy(fullfilename + pathlen + addlen, pszFileName);

	// ͨ�����ļ�������ļ��Ƿ����
	if ((iFileHandle = open(fullfilename, O_BINARY | O_RDONLY)) == -1)
	{
		bool flag = true;

		if (bCreateOnNotExist)
		{
			// ���Դ����ļ�
			if ((iFileHandle = open(fullfilename, O_BINARY | O_WRONLY | O_CREAT, S_IREAD | S_IWRITE)) != -1)
			{
				strcpy(header.Signature, "GRF");
				header.FileLength = sizeof(GRF_HEADER);
				header.EntryCount = header.DataOffset = 0;
				if (write(iFileHandle, &header, sizeof(GRF_HEADER)) == sizeof(GRF_HEADER))
					flag = false;
			}
		}

		if (flag)
		{
			// �ļ������ڻ����Ǵ���ʧ�ܻ���д��ʧ��
			delete [] fullfilename;

			m_iErrorCode = GRF_ERROR_CODE_FILEERROR;
			return false;
		}
	}
	else
	{
		// �ļ����ڣ�����ļ��Ƿ�Ϸ�
		if (read(iFileHandle, &header, sizeof(GRF_HEADER)) < sizeof(GRF_HEADER) ||
			strncmp(header.Signature, "GRF", 4) != 0)
		{
			// �ļ����ڵ����Ϸ�
			m_iErrorCode = GRF_ERROR_CODE_INVAILDFILE;
			close(iFileHandle);
			delete [] fullfilename;
			return false;
		}
	}
	close(iFileHandle);

	// Ϊ�����������ļ�������ռ�
	if ((fulldataname = new char [pathlen + addlen + 9]) == NULL)
	{
		m_iErrorCode = GRF_ERROR_CODE_INTERNALERROR;
		delete [] fullfilename;
		return false;
	}
	if (pathlen > 0)
		strncpy(fulldataname, fullfilename, pathlen + addlen);
	strcpy(fulldataname + pathlen + addlen, "00000000");

	// ���ԭ���Ѿ��д洢���ļ������������ͷ���ռ�
	if (m_pszIndexFileName)
		delete [] m_pszIndexFileName;
	if (m_pszDataFileName)
		delete [] m_pszDataFileName;

	// ���ļ����洢�ڳ�Ա������
	m_pszIndexFileName = fullfilename;
	m_pszDataFileName = fulldataname;
	// ���ļ�ͷ���ݴ洢�ڳ�Ա������
	m_Header = header;

	m_iErrorCode = GRF_ERROR_CODE_SUCCESSFUL;
	return true;
}

void Pal::Tools::CGameResourceFile::IntegerToHexString(uint32 ui, char* pchBuffer)
{
	static const char* pchNumbers = "0123456789ABCDEF";
	char* pch = pchBuffer + 7;
	for(int i = 0; i < 8; i++, ui >>= 4)
		*pch-- = pchNumbers[ui & 0xF];
}

bool Pal::Tools::CGameResourceFile::LoadIndices(void*& pvIndices, uint32& uiLength)
{
	void* buf;
	uint32 len;
	int iFileHandle;

	// ����Ϊ��
	if (m_Header.EntryCount == 0)
	{
		uiLength = 0;
		pvIndices = NULL;
		return true;
	}

	// ����ռ�
	if (m_Header.DataOffset > 0)
		len = m_Header.DataOffset - sizeof(GRF_HEADER);
	else
		len = m_Header.FileLength - sizeof(GRF_HEADER);
	if ((buf = malloc(len)) == NULL)
	{
		m_iErrorCode = GRF_ERROR_CODE_INTERNALERROR;
		return false;
	}

	// ��ȡ����
	if ((iFileHandle = open(m_pszIndexFileName, O_BINARY | O_RDONLY)) == -1)
	{
		m_iErrorCode = GRF_ERROR_CODE_FILEERROR;
		free(buf);
		return false;
	}
	if (lseek(iFileHandle, sizeof(GRF_HEADER), SEEK_SET) < sizeof(GRF_HEADER) ||
		read(iFileHandle, buf, len) < (int)len)
	{
		m_iErrorCode = GRF_ERROR_CODE_INVAILDFILE;
		close(iFileHandle);
		free(buf);
		return false;
	}
	close(iFileHandle);

	pvIndices = buf;
	uiLength = len;

	m_iErrorCode = GRF_ERROR_CODE_SUCCESSFUL;
	return true;
}

bool Pal::Tools::CGameResourceFile::LoadEntryData(const INDEX_ENTRY& entry, uint32 uiOffset, void* pvData, uint32 uiLength)
{
	uint32 length;
	int iFileHandle;

	// ����Ϊ��
	if (m_Header.EntryCount == 0)
	{
		m_iErrorCode = GRF_ERROR_CODE_SUCCESSFUL;
		return true;
	}

	// ��������
	if (uiOffset >= entry.Length || uiLength == 0 || pvData == NULL)
	{
		m_iErrorCode = GRF_ERROR_CODE_INVAILDPARAMETER;
		return false;
	}

	// ���㳤��
	if (uiOffset + uiLength > entry.Length)
		length = entry.Length - uiOffset;
	else
		length = uiLength;

	// ��ȡ����
	if (m_Header.DataOffset > 0)
	{
		if ((iFileHandle = open(m_pszIndexFileName, O_BINARY | O_RDONLY)) == -1)
		{
			m_iErrorCode = GRF_ERROR_CODE_FILEERROR;
			return false;
		}
		if (lseek(iFileHandle, entry.Offset + uiOffset, SEEK_SET) < (long)(entry.Offset + uiOffset) ||
			read(iFileHandle, pvData, length) < (int)length)
		{
			m_iErrorCode = GRF_ERROR_CODE_INVAILDFILE;
			close(iFileHandle);
			return false;
		}
	}
	else
	{
		size_t len = strlen(m_pszDataFileName);
		IntegerToHexString(entry.FileIndex, m_pszDataFileName + len - 8);
		if ((iFileHandle = open(m_pszDataFileName, O_BINARY | O_RDONLY)) == -1)
		{
			m_iErrorCode = GRF_ERROR_CODE_FILEERROR;
			return false;
		}
		if (lseek(iFileHandle, uiOffset, SEEK_SET) < (long)uiOffset ||
			read(iFileHandle, pvData, length) < (int)length)
		{
			m_iErrorCode = GRF_ERROR_CODE_INVAILDFILE;
			close(iFileHandle);
			return false;
		}
	}
	close(iFileHandle);

	m_iErrorCode = GRF_ERROR_CODE_SUCCESSFUL;
	return true;
}
