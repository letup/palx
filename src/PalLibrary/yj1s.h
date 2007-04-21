/*
 * PAL DOS compress format (YJ_1) streaming library
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
 *《仙剑奇侠传》DOS版压缩格式(YJ_1)流式处理库
 *
 * 作者： 楼奕华 <louyihua@21cn.com>
 *
 * 版权所有 2007 楼奕华
 *
 * 本文件是《仙剑奇侠传》库的一部分。
 *
 * 这个库属于自由软件，你可以遵照自由软件基金会出版的GNU次通用公共许可证条
 * 款来修改和重新发布这一程序。或者用许可证2.1版，或者（根据你的选择）用任
 * 何较新的版本。发布这一库的目的是希望它有用，但没有任何担保。甚至没有适合
 * 特定目的隐含的担保。更详细的情况请参阅GNU次通用公共许可证。
 * 
 * 你应该已经和库一起收到一份GNU次通用公共许可证的拷贝。如果还没有，写信给
 * 自由软件基金会：51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

struct YJ1_Decode_State
{
	TreeNode*	root;

	uint8*		pIBuffer;
	uint32		uiILength;
	uint32		uiIGrowBy;
	uint32		uiIEnd;
	uint8*		pOBuffer;
	uint32		uiOEnd;
	uint32		uiOTop;

	YJ_1_FILEHEADER		header;

	bool		bInitialed;
	bool		bInput;
	bool		bFinished;

	uint16		inputstep;
	uint16		curblk;
};

bool Pal::Tools::DecodeYJ1StreamInitialize(void*& pvState, uint32 uiGrowBy)
{
	struct YJ1_Decode_State* state;

	if (uiGrowBy < 0x1000)
		uiGrowBy = 0x1000;
	if ((state = new YJ1_Decode_State) == NULL)
		return false;
	memset(state, 0, sizeof(struct YJ1_Decode_State));
	if ((state->pIBuffer = new uint8 [uiGrowBy]) == NULL)
	{
		delete state;
		return false;
	}
	if ((state->pOBuffer = new uint8 [0x4000]) == NULL)
	{
		delete [] state->pIBuffer;
		delete state;
		return false;
	}
	state->bInitialed = true;
	state->uiILength = uiGrowBy;
	state->uiIGrowBy = uiGrowBy;
	pvState = state;
	return true;
}

bool Pal::Tools::DecodeYJ1StreamInput(void* pvState, const void* Source, uint32 SourceLength)
{
	struct YJ1_Decode_State* state = (struct YJ1_Decode_State*)pvState;

	if (state && state->bInitialed && !state->bFinished)
	{
		uint8* src;

		if (Source == NULL || SourceLength == 0)
			return true;
		if (state->inputstep == 0)
		{
			if (SourceLength < sizeof(YJ_1_FILEHEADER))
				return false;
			state->header = *((PYJ_1_FILEHEADER)Source);
			if (state->header.Signature != '1_JY')
				return false;
			SourceLength -= sizeof(YJ_1_FILEHEADER);
			src = (uint8*)Source + sizeof(YJ_1_FILEHEADER);
			state->inputstep = 1;
		}
		else
			src = (uint8*)Source;
		if (state->inputstep == 1)
		{
			uint16 tree_len = ((uint16)state->header.HuffmanTreeLength) << 1;
			uint16 flag_len = ((tree_len & 0xf) ? (tree_len >> 4) + 1 : (tree_len >> 4)) << 1;
			if (state->uiIEnd + SourceLength < (uint32)tree_len + (uint32)flag_len)
			{
				if (SourceLength > 0)
				{
					memcpy(state->pIBuffer + state->uiIEnd, src, SourceLength);
					state->uiIEnd += SourceLength;
					src += SourceLength; SourceLength = 0;
				}
			}
			else
			{
				memcpy(state->pIBuffer + state->uiIEnd, src, tree_len + flag_len - state->uiIEnd);
				src += tree_len + flag_len - state->uiIEnd;
				SourceLength -= tree_len + flag_len - state->uiIEnd;
				state->uiIEnd = tree_len + flag_len;
			}
			if (state->uiIEnd >= (uint32)tree_len + (uint32)flag_len)
			{
				uint32 bitptr = 0;
				uint8* flag = state->pIBuffer + tree_len;

				if ((state->root = new TreeNode [tree_len + 1]) == NULL)
					return false;
				state->root[0].leaf = false;
				state->root[0].value = 0;
				state->root[0].left = state->root + 1;
				state->root[0].right = state->root + 2;
				for(uint16 i = 1; i <= tree_len; i++)
				{
					state->root[i].leaf = !get_bits(flag, bitptr, 1);
					state->root[i].value = state->pIBuffer[i - 1];
					if (state->root[i].leaf)
						state->root[i].left = state->root[i].right = NULL;
					else
					{
						state->root[i].left =  state->root + (state->root[i].value << 1) + 1;
						state->root[i].right = state->root[i].left + 1;
					}
				}
				state->inputstep = 2;
				state->uiIEnd = 0;
			}
		}
		if (state->inputstep == 2)
		{
			if (state->uiIEnd + SourceLength >= state->uiILength)
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
		}
		return true;
	}
	else
		return false;
}

static void DecodeYJ1StreamProcess(void* pvState)
{
	struct YJ1_Decode_State* state = (struct YJ1_Decode_State*)pvState;
	TreeNode* node;
	uint8* src = state->pIBuffer;
	uint8* dest = state->pOBuffer;
	uint32 bitptr;
	PYJ_1_BLOCKHEADER header;

	header = (PYJ_1_BLOCKHEADER)src; src += 4;
	if (!header->CompressedLength)
	{
		uint16 hul = header->UncompressedLength;
		while(hul--)
			*dest++ = *src++;
		return;
	}
	src += 20; bitptr = 0;
	for(;;)
	{
		uint16 loop;
		if ((loop = get_loop(src, bitptr, header)) == 0)
			break;

		while(loop--)
		{
			node = state->root;
			for(; !node->leaf;)
			{
				if (get_bits(src, bitptr, 1))
					node = node->right;
				else
					node = node->left;
			}
			*dest++ = node->value;
		}

		if ((loop = get_loop(src, bitptr, header)) == 0)
			break;

		while(loop--)
		{
			uint32 pos, count;
			count = get_count(src, bitptr, header);
			pos = get_bits(src, bitptr, 2);
			pos = get_bits(src, bitptr, header->LZSSOffsetCodeLengthTable[pos]);
			while(count--)
			{
				*dest = *(dest - pos);
				dest++;
			}
		}
	}
	return;
}

bool Pal::Tools::DecodeYJ1StreamOutput(void* pvState, void* Destination, uint32& Length)
{
	struct YJ1_Decode_State* state = (struct YJ1_Decode_State*)pvState;

	if (state && state->bInitialed)
	{

		uint32 len = 0;
		uint8* dest = (uint8*)Destination;

		if (Destination == NULL || Length == 0 || state->inputstep < 2)
		{
			Length = 0;
			return true;
		}

		if (state->uiOEnd < state->uiOTop)
		{
			uint32 len1 = state->uiOTop - state->uiOEnd;
			if (Length >= len1)
			{
				memcpy(dest, state->pOBuffer + state->uiOEnd, len1);
				len = len1;
				state->uiOTop = state->uiOEnd = 0;
			}
			else
			{
				memcpy(dest, state->pOBuffer + state->uiOEnd, Length);
				state->uiOEnd += Length;
				return true;
			}
		}

		while(state->uiIEnd >= 4 && state->curblk < state->header.BlockCount)
		{
			YJ_1_BLOCKHEADER hdr = *((PYJ_1_BLOCKHEADER)state->pIBuffer);
			uint32 length = hdr.CompressedLength;
			if (length == 0)
				length = hdr.UncompressedLength;
			else if (state->uiIEnd < sizeof(YJ_1_BLOCKHEADER))
				break;
			if (state->uiIEnd >= length)
			{
				DecodeYJ1StreamProcess(state);
				state->uiOTop = hdr.UncompressedLength;
				state->curblk++;
				if (state->uiIEnd > length && state->curblk < state->header.BlockCount)
					memcpy(state->pIBuffer, state->pIBuffer + length, state->uiIEnd - length);
				state->uiIEnd -= length;
				if (Length - len >= hdr.UncompressedLength)
				{
					memcpy(dest + len, state->pOBuffer, hdr.UncompressedLength);
					len += hdr.UncompressedLength;
					state->uiOTop = state->uiOEnd = 0;
				}
				else
				{
					memcpy(dest + len, state->pOBuffer, Length - len);
					state->uiOEnd = Length - len;
					return true;
				}
			}
			else
				break;
		}
		Length = len;
		return true;
	}
	else
		return false;
}

bool Pal::Tools::DecodeYJ1StreamFinished(void* pvState, uint32& AvailableLength)
{
	struct YJ1_Decode_State* state = (struct YJ1_Decode_State*)pvState;

	if (state && state->bInitialed)
	{
		AvailableLength = 0x4000 - state->uiOEnd;
		if (state->inputstep == 2)
			return state->curblk == state->header.BlockCount;
		else
			return false;
	}
	else
		return false;
}

bool Pal::Tools::DecodeYJ1StreamFinalize(void* pvState)
{
	struct YJ1_Decode_State* state = (struct YJ1_Decode_State*)pvState;

	if (state && state->bInitialed)
	{
		if (state->root)
			delete [] state->root;
		delete [] state->pOBuffer;
		delete [] state->pIBuffer;
		delete state;
		return true;
	}
	else
		return false;
}
