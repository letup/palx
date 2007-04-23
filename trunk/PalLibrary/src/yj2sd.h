/*
 * PAL WIN95 compress format (YJ_2) streaming library
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
 *���ɽ���������WIN95��ѹ����ʽ(YJ_2)��ʽ�����
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

struct YJ2_Decode_State
{
	Tree		tree;
	TreeNode*	node;
	uint8*		dest;

	uint8*		pIBuffer;
	uint32		uiILength;
	uint32		uiIGrowBy;
	uint32		uiIEnd;
	uint8*		pOBuffer;
	uint32		uiOStart;
	uint32		uiOEnd;

	uint32		ptr;
	uint32		start;
	uint32		len;
	uint32		length;

	uint16		step;
	uint16		val;
	uint32		avail;
	sint32		pos;

	bool		bInitialed;
	bool		bInput;
	bool		bFinished;
};

bool Pal::Tools::DecodeYJ2StreamInitialize(void*& pvState, uint32 uiGrowBy)
{
	struct YJ2_Decode_State* state;

	if (uiGrowBy < 0x100)
		uiGrowBy = 0x100;
	if ((state = new YJ2_Decode_State) == NULL)
		return false;
	memset(state, 0, sizeof(struct YJ2_Decode_State));
	if (!build_tree(state->tree))
	{
		delete state;
		return false;
	}
	if ((state->pIBuffer = new uint8 [uiGrowBy]) == NULL)
	{
		delete [] state->tree.list;
		delete [] state->tree.node;
		delete state;
		return false;
	}
	if ((state->dest = state->pOBuffer = new uint8 [0x2050]) == NULL)
	{
		delete [] state->pIBuffer;
		delete [] state->tree.list;
		delete [] state->tree.node;
		delete state;
		return false;
	}
	state->bInitialed = true;
	state->uiILength = uiGrowBy;
	state->uiIGrowBy = uiGrowBy;
	pvState = state;
	return true;
}

bool Pal::Tools::DecodeYJ2StreamInput(void* pvState, const void* Source, uint32 SourceLength)
{
	struct YJ2_Decode_State* state = (struct YJ2_Decode_State*)pvState;

	if (state && state->bInitialed && !state->bFinished)
	{
		uint8* src;

		if (Source == NULL || SourceLength == 0)
			return true;
		if (!state->bInput)
		{
			if (state->uiIEnd + SourceLength < 4)
			{
				memcpy(state->pIBuffer + state->uiIEnd, Source, SourceLength);
				state->uiIEnd += SourceLength;
				return true;
			}
			else
				memcpy(state->pIBuffer + state->uiIEnd, Source, 4 - state->uiIEnd);
			state->length = *((uint32*)state->pIBuffer);
			SourceLength -= 4 - state->uiIEnd;
			src = (uint8*)Source + 4 - state->uiIEnd;
			state->uiIEnd = 0;
			state->bInput = true;
		}
		else
			src = (uint8*)Source;
		if (state->uiIEnd + SourceLength > state->uiILength)
		{
			uint8* pbuf;
			uint32 len = (state->uiIEnd + SourceLength) / state->uiIGrowBy + state->uiIGrowBy;
			if ((pbuf = new uint8 [state->uiILength + len]) == NULL)
				return false;
			memcpy(pbuf, state->pIBuffer, state->uiIEnd);
			delete [] state->pIBuffer;
			state->pIBuffer = pbuf;
			state->uiILength += len;
		}
		memcpy(state->pIBuffer + state->uiIEnd, src, SourceLength);
		state->uiIEnd += SourceLength;
		return true;
	}
	else
		return false;
}

static bool DecodeYJ2StreamProcess(void* pvState)
{
	struct YJ2_Decode_State* state = (struct YJ2_Decode_State*)pvState;
	uint32 slen = state->uiIEnd << 3;

	while(state->ptr < slen)
	{
		uint32 temp, tmp;
		sint32 i;
		switch(state->step)
		{
		case 0:
			state->node = state->tree.node + 0x280;
			state->step = 1;
		case 1:
			while(state->node->value > 0x140 && state->ptr < slen)
			{
				if (bt(state->pIBuffer, state->ptr))
					state->node = state->node->right;
				else
					state->node = state->node->left;
				state->ptr++;
			}
			if (state->node->value > 0x140)
				return true;
			state->val = state->node->value;
			if (state->tree.node[0x280].weight == 0x8000)
			{
				for(i = 0; i < 0x141; i++)
					if (state->tree.list[i]->weight & 0x1)
						adjust_tree(state->tree, i);
				for(i = 0; i <= 0x280; i++)
					state->tree.node[i].weight >>= 1;
			}
			adjust_tree(state->tree, state->val);
			if (state->val > 0xff)
				state->step = 2;
			else
			{
				*state->dest++ = (uint8)state->val;
				state->len++;
				state->uiOEnd++;
				state->step = 0;
				if (state->uiOEnd >= 0x2000)
					return true;
				else
					break;
			}
		case 2:
			for(i = state->pos, temp = state->avail; i < 8 && state->ptr < slen; i++, state->ptr++)
				temp |= (uint32)bt(state->pIBuffer, state->ptr) << i;
			state->avail = temp;
			state->pos = i;
			if (i < 8)
				return true;
			else
				state->step = 3;
		case 3:
			temp = state->avail;
			tmp = temp & 0xff;
			for(i = state->pos; i < data2[tmp & 0xf] + 6 && state->ptr < slen; i++, state->ptr++)
				temp |= (uint32)bt(state->pIBuffer, state->ptr) << i;
			state->avail = temp;
			state->pos = i;
			if (i < data2[tmp & 0xf] + 6)
				return true;
			else
			{
				uint32 pos;
				uint8* pre;
				temp >>= data2[tmp & 0xf];
				pos = (temp & 0x3f) | ((uint32)data1[tmp] << 6);
				if (pos == 0xfff)
				{
					state->bInput = false;
					state->bFinished = true;
					return true;
				}
				pre = state->dest - pos - 1;
				for(i = 0; i < state->val - 0xfd; i++)
					*state->dest++ = *pre++;
				state->len += state->val - 0xfd;
				state->uiOEnd += state->val - 0xfd;
				state->pos = 0;
				state->avail = 0;
				state->step = 0;
				if (state->uiOEnd >= 0x2000)
					return true;
				else
					break;
			}
		}
	}
	return true;
}

bool Pal::Tools::DecodeYJ2StreamOutput(void* pvState, void* Destination, uint32& Length)
{
	struct YJ2_Decode_State* state = (struct YJ2_Decode_State*)pvState;

	if (state && state->bInitialed)
	{

		uint32 count, len1, start;
		uint32 len = 0;
		uint8* dest = (uint8*)Destination;

		if (Destination == NULL || Length == 0)
		{
			Length = 0;
			return true;
		}

		while(len < Length && state->uiIEnd > 0 && !state->bFinished)
		{
			if (!DecodeYJ2StreamProcess(state))
				return false;
			if (Length - len > state->uiOEnd - state->uiOStart)
				count = state->uiOEnd - state->uiOStart;
			else
				count = Length - len;
			memcpy(dest + len, state->pOBuffer + state->uiOStart, count);
			len += count;
			state->start += count;
			state->uiOStart += count;
			if ((len1 = state->ptr >> 3) < state->uiIEnd)
			{
				memmove(state->pIBuffer, state->pIBuffer + len1, state->uiIEnd - len1);
				state->ptr &= 0x7;
				state->uiIEnd -= len1;
			}
			else
				state->ptr = state->uiIEnd = 0;
			if (state->uiOEnd >= 0x1000)
			{
				if (state->uiOStart > state->uiOEnd - 0x1000)
				{
					start = state->uiOEnd - 0x1000;
					count = 0x1000;
				}
				else
				{
					start = state->uiOStart;
					count = state->uiOEnd - state->uiOStart;
				}
				memmove(state->pOBuffer, state->pOBuffer + start, count);
				state->dest -= start;
				state->uiOStart -= start;
				state->uiOEnd -= start;
			}
		}
		Length = len;
		return true;
	}
	else
		return false;
}

bool Pal::Tools::DecodeYJ2StreamFinished(void* pvState, uint32& AvailableLength)
{
	struct YJ2_Decode_State* state = (struct YJ2_Decode_State*)pvState;

	if (state && state->bInitialed)
	{
		AvailableLength = state->len - state->start;
		return state->bFinished;
	}
	else
		return false;
}

bool Pal::Tools::DecodeYJ2StreamFinalize(void* pvState)
{
	struct YJ2_Decode_State* state = (struct YJ2_Decode_State*)pvState;

	if (state && state->bInitialed)
	{
		delete [] state->pOBuffer;
		delete [] state->pIBuffer;
		delete [] state->tree.list;
		delete [] state->tree.node;
		delete state;
		return true;
	}
	else
		return false;
}

bool Pal::Tools::DecodeYJ2StreamReset(void* pvState)
{
	struct YJ2_Decode_State* state = (struct YJ2_Decode_State*)pvState;

	if (state && state->bInitialed)
	{
		uint32 uiGrowBy = state->uiIGrowBy;
		uint32 uiLength = state->uiILength;
		uint8* pIBuffer = state->pIBuffer;
		uint8* pOBuffer = state->pOBuffer;

		delete [] state->tree.list;
		delete [] state->tree.node;
		memset(state, 0, sizeof(struct YJ2_Decode_State));
		if (!build_tree(state->tree))
		{
			delete [] pOBuffer;
			delete [] pIBuffer;
			delete state;
			return false;
		}
		state->dest = state->pOBuffer = pOBuffer;
		state->pIBuffer = pIBuffer;
		state->uiIGrowBy = uiGrowBy;
		state->uiILength = uiLength;
		state->bInitialed = true;
		return true;
	}
	else
		return false;
}