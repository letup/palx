/*
 * PAL library GRF format Editor class
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
 *���ɽ����������� GRF ��ʽ Editor ��
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

Pal::Tools::CGameResourceFileEditor::CGameResourceFileEditor()	:
CGameResourceFile(), m_uiMaxFileIndex(0)
{}

Pal::Tools::CGameResourceFileEditor::~CGameResourceFileEditor()
{
	for(std::vector<std::list<IndexEntry> >::size_type i = 0;
		i < m_IndexVector.size(); i++)
		for(std::list<IndexEntry>::iterator iter = m_IndexVector[i].begin();
			iter != m_IndexVector[i].end(); iter++)
		{
			free(iter->pEntry);
			if (iter->pvData)
				free(iter->pvData);
		}
}

Pal::Tools::CGameResourceFileEditor* Pal::Tools::CGameResourceFileEditor::CreateEditor(const char* pszFilePath,
																						const char* pszFileName)
{
	void* pvIndices;
	uint32 uiIndexLength;
	INDEX_ENTRY* ptr;
	INDEX_ENTRY* pentry;
	IndexEntry entry;
	Pal::Tools::CGameResourceFileEditor* editor = new Pal::Tools::CGameResourceFileEditor();

	// �޷�������
	if (editor == NULL)
		return NULL;

	// û���ҵ���Դ
	if (!editor->SetResourceFilePathAndName(pszFilePath, pszFileName, true))
	{
		delete editor;
		return NULL;
	}

	// ���ǿɱ༭������
	if (editor->m_Header.DataOffset != 0)
	{
		delete editor;
		return NULL;
	}

	// �޷���������
	if (!editor->LoadIndices(pvIndices, uiIndexLength))
	{
		delete editor;
		return NULL;
	}

	// ���ù�ϣ���С
	editor->m_IndexVector.resize(0x1000);
	entry.pvData = NULL;
	ptr = (INDEX_ENTRY*)pvIndices;
	for(uint32 i = 0; i < editor->m_Header.EntryCount; i++)
	{
		uint32 code;
		std::map<uint32, std::list<IndexEntry> >::iterator iter;

		// �õ����Ĵ洢�ļ����
		if (editor->m_uiMaxFileIndex < ptr->FileIndex)
			editor->m_uiMaxFileIndex = ptr->FileIndex;

		// ����ռ�
		if ((pentry = (INDEX_ENTRY*)malloc(sizeof(INDEX_ENTRY) + ptr->PathLength * sizeof(wchar_t))) == NULL)
		{
			free(pvIndices);
			delete editor;
			return NULL;
		}
		memcpy(pentry, ptr, sizeof(INDEX_ENTRY) + ptr->PathLength * sizeof(wchar_t));
		entry.pEntry = pentry;

		// �����ϣ��
		code = CalculateHashCode(ptr->EntryPath, ptr->PathLength) & 0xFFFL;
		editor->m_IndexVector[code].push_back(entry);

		// ����洢�ļ���
		if ((iter = editor->m_StoreMap.find(ptr->FileIndex)) == editor->m_StoreMap.end())
		{
			editor->m_StoreMap.insert(std::pair<uint32, std::list<IndexEntry> >(ptr->FileIndex, std::list<IndexEntry>()));
			iter = editor->m_StoreMap.find(ptr->FileIndex);
		}
		iter->second.push_back(entry);
		ptr = (INDEX_ENTRY*)((uint8*)(ptr + 1) + ptr->PathLength * sizeof(wchar_t));
	}
	editor->m_iCurrentEntry = editor->m_EmptyList.end();

	free(pvIndices);

	return editor;
}

bool Pal::Tools::CGameResourceFileEditor::CreateEntry(const wchar_t* pwszEntryName)
{
	INDEX_ENTRY* pentry;
	IndexEntry entry;
	uint32 code, length;

	// ����������
	if (pwszEntryName == NULL || (length = (uint32)wcslen(pwszEntryName)) == 0)
	{
		m_iErrorCode = GRF_ERROR_CODE_INVAILDPARAMETER;
		return false;
	}
	code = CalculateHashCode(pwszEntryName) & 0xFFFL;

	// ����Ƿ����ͬ����
	if (!m_IndexVector[code].empty())
	{
		for(std::list<IndexEntry>::iterator iter =
			m_IndexVector[code].begin(); iter != m_IndexVector[code].end(); iter++)
			if (length = iter->pEntry->PathLength &&
				wcsncmp(iter->pEntry->EntryPath, pwszEntryName, iter->pEntry->PathLength) == 0)
			{
				m_iErrorCode = GRF_ERROR_CODE_ENTRYEXISTED;
				return false;
			}
	}

	// ����ռ�
	if ((pentry = (INDEX_ENTRY*)malloc(sizeof(INDEX_ENTRY) + length * sizeof(wchar_t))) == NULL)
	{
		m_iErrorCode = GRF_ERROR_CODE_INTERNALERROR;
		return false;
	}
	memset(pentry, 0, sizeof(INDEX_ENTRY));

	// ������ֵ
	wcsncpy(pentry->EntryPath, pwszEntryName, length);
	pentry->PathLength = length;
	m_Header.EntryCount++;

	// �������
	entry.pvData = NULL;
	entry.pEntry = pentry;
	m_IndexVector[code].push_back(entry);

	m_iErrorCode = GRF_ERROR_CODE_SUCCESSFUL;
	return true;
}

bool Pal::Tools::CGameResourceFileEditor::RemoveEntry(const wchar_t* pwszEntryName)
{
	uint32 code, length;

	// ����������
	if (pwszEntryName == NULL || (length = (uint32)wcslen(pwszEntryName)) == 0)
	{
		m_iErrorCode = GRF_ERROR_CODE_INVAILDPARAMETER;
		return false;
	}
	code = CalculateHashCode(pwszEntryName) & 0xFFFL;

	if (!m_IndexVector[code].empty())
	{
		for(std::list<IndexEntry>::iterator iter =
			m_IndexVector[code].begin(); iter != m_IndexVector[code].end(); iter++)
			if (length = iter->pEntry->PathLength &&
				wcsncmp(iter->pEntry->EntryPath, pwszEntryName, iter->pEntry->PathLength) == 0)
			{
				std::map<uint32, std::list<IndexEntry> >::iterator slist;

				// ��鱻ɾ�����Ƿ�ǰ��
				if (m_iCurrentEntry->pEntry == iter->pEntry)
					m_iCurrentEntry = m_EmptyList.end();

				// �Ӵ洢�ļ����ñ���ɾ������
				if ((slist = m_StoreMap.find(iter->pEntry->FileIndex)) != m_StoreMap.end())
				{
					for(std::list<IndexEntry>::iterator iter1 = slist->second.begin();
						iter1 != slist->second.end(); iter1++)
						if (iter1->pEntry == iter->pEntry)
						{
							slist->second.erase(iter1);
							break;
						}

					// �����Ӧ�Ĵ洢�ļ��Ƿ�Ӧ����ɾ��
					if (slist->second.empty())
						m_RemoveSet.insert(iter->pEntry->FileIndex);
				}

				// �ͷŸ�����ռ�ռ�
				free(iter->pEntry);

				// ������ӹ�ϣ����ɾ��
				m_IndexVector[code].erase(iter);

				m_Header.EntryCount--;

				m_iErrorCode = GRF_ERROR_CODE_SUCCESSFUL;
				return true;
			}
	}

	m_iErrorCode = GRF_ERROR_CODE_ENTRYNOTEXIST;
	return false;
}

bool Pal::Tools::CGameResourceFileEditor::Seek(const wchar_t* pwszEntryName)
{
	uint32 code, length;

	// ����������
	if (pwszEntryName == NULL || (length = (uint32)wcslen(pwszEntryName)) == 0)
	{
		m_iErrorCode = GRF_ERROR_CODE_INVAILDPARAMETER;
		return false;
	}
	code = CalculateHashCode(pwszEntryName) & 0xFFFL;

	if (!m_IndexVector[code].empty())
	{
		for(std::list<IndexEntry>::iterator iter =
			m_IndexVector[code].begin(); iter != m_IndexVector[code].end(); iter++)
			if (length == iter->pEntry->PathLength &&
				wcsncmp(iter->pEntry->EntryPath, pwszEntryName, iter->pEntry->PathLength) == 0)
			{
				m_iCurrentEntry = iter;
				m_iErrorCode = GRF_ERROR_CODE_SUCCESSFUL;
				return true;
			}
	}

	m_iErrorCode = GRF_ERROR_CODE_ENTRYNOTEXIST;
	return false;
}

bool Pal::Tools::CGameResourceFileEditor::GetResourceType(uint8& type)
{
	if (m_iCurrentEntry == m_EmptyList.end())
	{
		m_iErrorCode = GRF_ERROR_CODE_ENTRYNOTSPECIFY;
		return false;
	}
	else
	{
		type = m_iCurrentEntry->pEntry->ResourceType;

		m_iErrorCode = GRF_ERROR_CODE_SUCCESSFUL;
		return true;
	}
}

bool Pal::Tools::CGameResourceFileEditor::GetCompressAlgorithm(uint8& algo)
{
	if (m_iCurrentEntry == m_EmptyList.end())
	{
		m_iErrorCode = GRF_ERROR_CODE_ENTRYNOTSPECIFY;
		return false;
	}
	else
	{
		algo = m_iCurrentEntry->pEntry->CompressAlgorithm;

		m_iErrorCode = GRF_ERROR_CODE_SUCCESSFUL;
		return true;
	}
}

bool Pal::Tools::CGameResourceFileEditor::GetNameLength(uint32& uiLength)
{
	if (m_iCurrentEntry == m_EmptyList.end())
	{
		m_iErrorCode = GRF_ERROR_CODE_ENTRYNOTSPECIFY;
		return false;
	}
	else
	{
		uiLength = m_iCurrentEntry->pEntry->PathLength;

		m_iErrorCode = GRF_ERROR_CODE_SUCCESSFUL;
		return true;
	}
}

bool Pal::Tools::CGameResourceFileEditor::GetEntryName(wchar_t* pwszEntryName, uint32 uiLength)
{
	// �����Ƿ�
	if (pwszEntryName == NULL || uiLength == 0)
	{
		m_iErrorCode = GRF_ERROR_CODE_INVAILDPARAMETER;
		return false;
	}

	// ��û��ָ����ǰ��
	if (m_iCurrentEntry == m_EmptyList.end())
	{
		m_iErrorCode = GRF_ERROR_CODE_ENTRYNOTSPECIFY;
		return false;
	}

	// �������֣�ȷ����'\0'��β
	if (uiLength > m_iCurrentEntry->pEntry->PathLength)
	{
		wcsncpy(pwszEntryName, m_iCurrentEntry->pEntry->EntryPath, m_iCurrentEntry->pEntry->PathLength);
		pwszEntryName[m_iCurrentEntry->pEntry->PathLength] = L'\0';
	}
	else
	{
		wcsncpy(pwszEntryName, m_iCurrentEntry->pEntry->EntryPath, uiLength - 1);
		pwszEntryName[uiLength - 1] = L'\0';
	}

	m_iErrorCode = GRF_ERROR_CODE_SUCCESSFUL;
	return true;
}

bool Pal::Tools::CGameResourceFileEditor::GetDataLength(uint32& uiLength)
{
	if (m_iCurrentEntry == m_EmptyList.end())
	{
		m_iErrorCode = GRF_ERROR_CODE_ENTRYNOTSPECIFY;
		return false;
	}
	else
	{
		uiLength = m_iCurrentEntry->pEntry->Length;

		m_iErrorCode = GRF_ERROR_CODE_SUCCESSFUL;
		return true;
	}
}

bool Pal::Tools::CGameResourceFileEditor::GetData(uint32 uiOffset, void* pvData, uint32 uiLength)
{
	if (m_iCurrentEntry == m_EmptyList.end())
	{
		m_iErrorCode = GRF_ERROR_CODE_ENTRYNOTSPECIFY;
		return false;
	}
	else
	{
		if (m_iCurrentEntry->pvData != NULL)
		{
			uint32 len;
			if (pvData == NULL || uiOffset >= m_iCurrentEntry->pEntry->Length)
			{
				m_iErrorCode = GRF_ERROR_CODE_INVAILDPARAMETER;
				return false;
			}
			if (uiOffset + uiLength > m_iCurrentEntry->pEntry->Length)
				len = m_iCurrentEntry->pEntry->Length - uiOffset;
			else
				len = uiLength;
			memcpy(pvData, (uint8*)m_iCurrentEntry->pvData + uiOffset, len);

			m_iErrorCode = GRF_ERROR_CODE_SUCCESSFUL;
			return true;
		}
		else
			return LoadEntryData(*(m_iCurrentEntry->pEntry), uiOffset, pvData, uiLength);
	}
}

bool Pal::Tools::CGameResourceFileEditor::SetResourceType(uint8 type)
{
	if (m_iCurrentEntry == m_EmptyList.end())
	{
		m_iErrorCode = GRF_ERROR_CODE_ENTRYNOTSPECIFY;
		return false;
	}
	else
	{
		m_iCurrentEntry->pEntry->ResourceType = type;

		m_iErrorCode = GRF_ERROR_CODE_SUCCESSFUL;
		return true;
	}
}

bool Pal::Tools::CGameResourceFileEditor::SetCompressAlgorithm(uint8 algo)
{
	if (m_iCurrentEntry == m_EmptyList.end())
	{
		m_iErrorCode = GRF_ERROR_CODE_ENTRYNOTSPECIFY;
		return false;
	}
	else
	{
		m_iCurrentEntry->pEntry->CompressAlgorithm = algo;

		m_iErrorCode = GRF_ERROR_CODE_SUCCESSFUL;
		return true;
	}
}

bool Pal::Tools::CGameResourceFileEditor::SetEntryName(const wchar_t* pwszEntryName)
{
	INDEX_ENTRY* pentry;
	uint32 code, length;

	// �����Ƿ�
	if (pwszEntryName == NULL || (length = (uint32)wcslen(pwszEntryName)) == 0)
	{
		m_iErrorCode = GRF_ERROR_CODE_INVAILDPARAMETER;
		return false;
	}
	code = CalculateHashCode(pwszEntryName) & 0xFFFL;

	// ��û��ָ����ǰ��
	if (m_iCurrentEntry == m_EmptyList.end())
	{
		m_iErrorCode = GRF_ERROR_CODE_ENTRYNOTSPECIFY;
		return false;
	}

	// ���·���ռ�
	if ((pentry = (INDEX_ENTRY*)realloc(m_iCurrentEntry->pEntry, sizeof(INDEX_ENTRY) + length * sizeof(wchar_t))) == NULL)
	{
		m_iErrorCode = GRF_ERROR_CODE_INTERNALERROR;
		return false;
	}

	// ��������
	pentry->PathLength = length;
	wcsncpy(pentry->EntryPath, pwszEntryName, length);
	m_iCurrentEntry->pEntry = pentry;

	m_iErrorCode = GRF_ERROR_CODE_SUCCESSFUL;
	return true;
}

bool Pal::Tools::CGameResourceFileEditor::SetDataLength(uint32 uiLength)
{
	if (m_iCurrentEntry == m_EmptyList.end())
	{
		m_iErrorCode = GRF_ERROR_CODE_ENTRYNOTSPECIFY;
		return false;
	}
	else
	{
		std::map<uint32, std::list<IndexEntry> >::iterator slist;
		void* pvNewData;

		// ����ռ�
		if ((pvNewData = realloc(m_iCurrentEntry->pvData, uiLength)) == NULL && uiLength != 0)
		{
			m_iErrorCode = GRF_ERROR_CODE_INTERNALERROR;
			return false;
		}

		// ������������ͬһ�洢�ļ�����
		if ((slist = m_StoreMap.find(m_iCurrentEntry->pEntry->FileIndex)) != m_StoreMap.end())
		{
			std::list<IndexEntry>::iterator iter;
			for(iter = slist->second.begin(); iter != slist->second.end(); iter++)
			{
				iter->pvData = pvNewData;
				iter->pEntry->Length = uiLength;
			}
		}
		
		m_iErrorCode = GRF_ERROR_CODE_SUCCESSFUL;
		return true;
	}
}

bool Pal::Tools::CGameResourceFileEditor::SetData(uint32 uiOffset, const void* pvData, uint32 uiLength)
{
	if (m_iCurrentEntry == m_EmptyList.end())
	{
		m_iErrorCode = GRF_ERROR_CODE_ENTRYNOTSPECIFY;
		return false;
	}
	else if (pvData == NULL || uiLength == 0 || uiOffset > m_iCurrentEntry->pEntry->Length)
	{
		m_iErrorCode = GRF_ERROR_CODE_INVAILDPARAMETER;
		return false;
	}
	else
	{
		// ���ռ��Ƿ��ã��������з���
		if (uiOffset + uiLength > m_iCurrentEntry->pEntry->Length &&
			!SetDataLength(uiOffset + uiLength))
		{
			m_iErrorCode = GRF_ERROR_CODE_INTERNALERROR;
			return false;
		}

		// ��������
		memcpy((uint8*)m_iCurrentEntry->pvData + uiOffset, pvData, uiLength);

		m_iErrorCode = GRF_ERROR_CODE_SUCCESSFUL;
		return true;
	}
}

bool Pal::Tools::CGameResourceFileEditor::SetData(const wchar_t* pwszEntryName)
{
	uint32 length;

	if (m_iCurrentEntry == m_EmptyList.end())
	{
		m_iErrorCode = GRF_ERROR_CODE_ENTRYNOTSPECIFY;
		return false;
	}
	else if (pwszEntryName == NULL || (length = (uint32)wcslen(pwszEntryName)) == 0)
	{
		m_iErrorCode = GRF_ERROR_CODE_INVAILDPARAMETER;
		return false;
	}
	else
	{
		uint32 code = CalculateHashCode(pwszEntryName) & 0xFFFL;

		// ���Ҷ�Ӧ��
		if (!m_IndexVector[code].empty())
		{
			for(std::list<IndexEntry>::iterator iter =
				m_IndexVector[code].begin(); iter != m_IndexVector[code].end(); iter++)
				if (length == iter->pEntry->PathLength &&
					wcsncmp(iter->pEntry->EntryPath, pwszEntryName, iter->pEntry->PathLength) == 0)
				{
					// �ҵ���Ӧ�������ô洢�ļ��Ƿ���ͬ
					if (m_iCurrentEntry->pEntry->FileIndex != iter->pEntry->FileIndex)
					{
						std::map<uint32, std::list<IndexEntry> >::iterator slist;

						// �������õĴ洢�ļ�
						if ((slist = m_StoreMap.find(m_iCurrentEntry->pEntry->FileIndex)) != m_StoreMap.end())
						{
							for(std::list<IndexEntry>::iterator iter1 = slist->second.begin();
								iter1 != slist->second.end(); iter1++)
								if (iter1->pEntry == m_iCurrentEntry->pEntry)
								{
									// �ҵ��洢����б���ɾ��
									slist->second.erase(iter1);
									break;
								}

							// �����Ӧ�Ĵ洢�ļ��Ƿ�Ӧ����ɾ��
							if (slist->second.empty())
								m_RemoveSet.insert(m_iCurrentEntry->pEntry->FileIndex);
						}

						// ����Ƿ��Ѿ�д��������
						if (m_iCurrentEntry->pvData != NULL)
							free(m_iCurrentEntry->pvData);
						m_iCurrentEntry->pvData = iter->pvData;
						m_iCurrentEntry->pEntry->Length = iter->pEntry->Length;
						m_iCurrentEntry->pEntry->FileIndex = iter->pEntry->FileIndex;

						// �Ƿ���Ҫ���洢�ļ��ż�¼
						if ((slist = m_StoreMap.find(iter->pEntry->FileIndex)) != m_StoreMap.end())
							slist->second.push_back(*m_iCurrentEntry);
					}

					m_iErrorCode = GRF_ERROR_CODE_SUCCESSFUL;
					return true;
				}
		}

		m_iErrorCode = GRF_ERROR_CODE_ENTRYNOTEXIST;
		return false;
	}
}

bool Pal::Tools::CGameResourceFileEditor::SaveChanges()
{
	std::vector<std::list<IndexEntry> >::size_type i;
	std::list<IndexEntry>::iterator iter, iter1;
	int iFileHandle;
	size_t len = strlen(m_pszDataFileName);

	// ����д������
	for(i = 0; i < m_IndexVector.size(); i++)
		for(iter = m_IndexVector[i].begin(); iter != m_IndexVector[i].end(); iter++)
		{
			std::map<uint32, std::list<IndexEntry> >::iterator slist;
			void* pvData = iter->pvData;

			// �����������Ҫд��
			if (pvData)
			{
				// �����ļ����������´��������������
				if (iter->pEntry->FileIndex == 0)
					IntegerToHexString(iter->pEntry->FileIndex, m_pszDataFileName + len - 8);
				else
					IntegerToHexString(m_uiMaxFileIndex + 1, m_pszDataFileName + len - 8);

				// ���Դ������ļ�
				if ((iFileHandle = open(m_pszDataFileName, O_BINARY | O_WRONLY | O_CREAT | O_TRUNC)) != -1)
				{
					m_iErrorCode = GRF_ERROR_CODE_FILEERROR;
					return false;
				}

				// д������
				if (write(iFileHandle, pvData, iter->pEntry->Length) < (int)iter->pEntry->Length)
				{
					close(iFileHandle);
					m_iErrorCode = GRF_ERROR_CODE_FILEERROR;
					return false;
				}

				// �ر��ļ�
				close(iFileHandle);
			}

			// �����ļ��洢���
			if (iter->pEntry->FileIndex == 0)
				iter->pEntry->FileIndex = ++m_uiMaxFileIndex;

			// ����������ͬһ�洢�ļ����������ÿ�
			if ((slist = m_StoreMap.find(iter->pEntry->FileIndex)) != m_StoreMap.end())
				for(iter1 = slist->second.begin(); iter1 != slist->second.end(); iter1++)
					iter1->pvData = NULL;

			// �ͷ����ݿռ�
			free(pvData);
		}

	// ���д�����������ȳ��Դ������ļ�
	if ((iFileHandle = open(m_pszIndexFileName, O_BINARY | O_WRONLY | O_CREAT | O_TRUNC)) == -1)
	{
		m_iErrorCode = GRF_ERROR_CODE_FILEERROR;
		return false;
	}

	// ���ļ�ͷ������д����������
	if (write(iFileHandle, &m_Header, sizeof(GRF_HEADER)) < sizeof(GRF_HEADER))
	{
		close(iFileHandle);
		m_iErrorCode = GRF_ERROR_CODE_FILEERROR;
		return false;
	}
	for(i = 0; i < m_IndexVector.size(); i++)
		for(iter = m_IndexVector[i].begin(); iter != m_IndexVector[i].end(); iter++)
			if (write(iFileHandle, iter->pEntry, sizeof(INDEX_ENTRY) + iter->pEntry->Length * sizeof(wchar_t)) <
				(int)(sizeof(INDEX_ENTRY) + iter->pEntry->Length * sizeof(wchar_t)))
			{
				close(iFileHandle);
				m_iErrorCode = GRF_ERROR_CODE_FILEERROR;
				return false;
			}

	// �ر������ļ�
	close(iFileHandle);

	// ���ɾ����Щ���õĴ洢�ļ�
	for(std::set<uint32>::iterator iter2 = m_RemoveSet.begin(); iter2 != m_RemoveSet.end(); iter2++)
	{
		int ret;

		IntegerToHexString(*iter2, m_pszDataFileName + len - 8);
		ret = unlink(m_pszDataFileName);
	}

	m_iErrorCode = GRF_ERROR_CODE_SUCCESSFUL;
	return true;
}

uint32 Pal::Tools::CGameResourceFileEditor::CalculateHashCode(const wchar_t* pwszString)
{
	size_t len;
	wchar_t code;

	if (pwszString == NULL || (len = wcslen(pwszString)) == 0)
		return 0;

	code = pwszString[0];
	for(size_t i = 1; i < len; i++)
		code ^= pwszString[i];

	return (uint32)code;
}

uint32 Pal::Tools::CGameResourceFileEditor::CalculateHashCode(const wchar_t* pwszString, uint32 uiLength)
{
	wchar_t code;

	if (pwszString == NULL || uiLength == 0)
		return 0;

	code = pwszString[0];
	for(uint32 i = 1; i < uiLength; i++)
		code ^= pwszString[i];

	return (uint32)code;
}
