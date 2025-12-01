// SPDX-License-Identifier: Unlicense

// Base on https://github.com/cavaliercoder/sysinv

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "libnw.h"
#include "utils.h"
#include "base64.h"

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

PNODE NWL_NodeAlloc(LPCSTR name, INT flags)
{
	PNODE node = NULL;

	NWL_Debug("NODE", "ALLOC [%s]", name);

	node = (PNODE)calloc(1, sizeof(NODE));
	if (!node)
		NWL_ErrExit(ERROR_OUTOFMEMORY, "Failed to allocate memory in "__FUNCTION__);

	node->name = _strdup(name);
	if (!node->name)
		NWL_ErrExit(ERROR_OUTOFMEMORY, "Failed to allocate memory in "__FUNCTION__);

	node->parent = NULL;
	node->children = NULL;
	node->attributes = NULL;
	node->flags = flags;

	return node;
}

VOID NWL_NodeFree(PNODE node, INT deep)
{
	if (!node)
		return;

	// Free children
	if (deep > 0 && node->children)
	{
		for (ptrdiff_t i = 0; i < arrlen(node->children); i++)
			NWL_NodeFree(node->children[i], deep);
	}

	// Free attributes
	if (node->attributes)
	{
		for (size_t i = 0; i < shlenu(node->attributes); i++)
		{
			// FUCK: false positive C6001 warnings
			free(node->attributes[i].key);
			free(node->attributes[i].value);
		}
		shfree(node->attributes);
	}

	if (node->children)
		arrfree(node->children);

	free(node->name);
	free(node);
}

INT NWL_NodeDepth(PNODE node)
{
	PNODE parent;
	int count = 0;
	for (parent = node; parent && parent->parent; parent = parent->parent)
		count++;
	return count;
}

INT NWL_NodeChildCount(PNODE node)
{
	if (!node || !node->children)
		return 0;
	return (INT)arrlen(node->children);
}

PNODE NWL_NodeEnumChild(PNODE parent, INT index)
{
	if (!parent || !parent->children)
		return NULL;

	if (index < 0 || (ptrdiff_t)index >= arrlen(parent->children))
		return NULL;

	return parent->children[index];
}

INT NWL_NodeAppendChild(PNODE parent, PNODE child)
{
	if (!parent || !child)
		return parent ? NWL_NodeChildCount(parent) : 0;

	NWL_Debug("NODE", "APPEND [%s] -> [%s]", parent->name, child->name);

	arrpush(parent->children, child);
	child->parent = parent;
	return (INT)arrlen(parent->children);
}

PNODE NWL_NodeAppendNew(PNODE parent, LPCSTR name, INT flags)
{
	PNODE node = NWL_NodeAlloc(name, flags);
	NWL_NodeAppendChild(parent, node);
	return node;
}

PNODE NWL_NodeGetChild(PNODE parent, LPCSTR name)
{
	if (!parent || !parent->children)
		return NULL;

	for (ptrdiff_t i = 0; i < arrlen(parent->children); ++i)
	{
		if (strcmp(parent->children[i]->name, name) == 0)
			return parent->children[i];
	}
	return NULL;
}

static void NWL_NodeAttrAllocStrings(LPCSTR key, LPCSTR value, int flags, char** outKey, char** outValue)
{
	char* k;
	char* v;
	const char* nvalue;
	size_t key_len, val_len;

	if (!key || !outKey || !outValue)
		return;

	key_len = strlen(key) + 1;
	nvalue = value ? value : "";
	val_len = strlen(nvalue) + 1;

	k = (char*)malloc(key_len);
	v = (char*)malloc(val_len);
	if (!k || !v)
		NWL_ErrExit(ERROR_OUTOFMEMORY, "Failed to allocate memory in " __FUNCTION__);

	memcpy(k, key, key_len);

	if (NWLC->HideSensitive && (flags & NAFLG_FMT_SENSITIVE))
	{
		memset(v, '*', val_len - 1);
		v[val_len - 1] = '\0';
	}
	else
	{
		memcpy(v, nvalue, val_len);
	}

	*outKey = k;
	*outValue = v;
}

INT NWL_NodeAttrCount(PNODE node)
{
	if (!node || !node->attributes)
		return 0;
	return (INT)shlenu(node->attributes);
}

static inline NODE_ATT* NWL_NodeAttrGetEntry(PNODE node, LPCSTR key)
{
	if (!node || !key || !node->attributes)
		return NULL;

	return shgetp_null(node->attributes, (char*)key);
}

LPCSTR NWL_NodeAttrGet(PNODE node, LPCSTR key)
{
	NODE_ATT* att = NWL_NodeAttrGetEntry(node, key);
	return att ? att->value : "-\0";
}

PNODE_ATT NWL_NodeAttrEnum(PNODE node, INT index)
{
	if (!node || !node->attributes)
		return NULL;

	if (index < 0 || (size_t)index >= shlenu(node->attributes))
		return NULL;

	return &node->attributes[index];
}

PNODE_ATT NWL_NodeAttrSet(PNODE node, LPCSTR key, LPCSTR value, INT flags)
{
	NODE_ATT* att;

	NWL_Debug("NODE", "SET <%s> = <%s>", key, value ? value : "(null)");

	if (!node || !key)
		return NULL;

	if (!NWLC->HumanSize && (flags & NAFLG_FMT_HUMAN_SIZE))
		flags |= NAFLG_FMT_NUMERIC;

	att = NWL_NodeAttrGetEntry(node, key);

	if (att)
	{
		const char* nvalue = value ? value : "";
		if ((att->flags & NAFLG_FMT_SENSITIVE) || (flags & NAFLG_FMT_SENSITIVE) ||
			strcmp(att->value, nvalue) != 0)
		{
			free(att->value);
			char* newValue = NULL;
			char* dummyKey = NULL;
			NWL_NodeAttrAllocStrings(att->key, value, flags, &dummyKey, &newValue);
			free(dummyKey);
			att->value = newValue;
		}
		att->flags = flags;
		return att;
	}
	else
	{
		NODE_ATT tmp = { 0 };
		NWL_NodeAttrAllocStrings(key, value, flags, &tmp.key, &tmp.value);
		tmp.flags = flags;

		shputs(node->attributes, tmp);

		return shgetp(node->attributes, tmp.key);
	}
}

PNODE_ATT
NWL_NodeAttrSetf(PNODE node, LPCSTR key, INT flags, LPCSTR _Printf_format_string_ format, ...)
{
	int sz;
	char* buf = NULL;
	PNODE_ATT att = NULL;
	va_list ap;

	va_start(ap, format);
	sz = _vscprintf(format, ap) + 1;
	if (sz <= 0)
	{
		va_end(ap);
		NWL_ErrExit(ERROR_INVALID_DATA, "Failed to calculate string length in " __FUNCTION__);
	}
	buf = (char*)calloc(sz, sizeof(CHAR));
	if (!buf)
	{
		va_end(ap);
		NWL_ErrExit(ERROR_OUTOFMEMORY, "Failed to allocate memory in " __FUNCTION__);
	}
	vsnprintf(buf, sz, format, ap);
	va_end(ap);

	att = NWL_NodeAttrSet(node, key, buf, flags);
	free(buf);
	return att;
}

VOID NWL_NodeAppendMultiSz(LPSTR* lpmszMulti, LPCSTR szNew)
{
	DWORD oldLength = 0;
	DWORD newSzLength = 0;
	DWORD count = 0;
	LPSTR mszMulti = NULL;
	LPSTR mszResult = NULL;

	if (szNew == NULL)
		return;

	newSzLength = (DWORD)strlen(szNew);

	// If old multistring was empty
	if (*lpmszMulti == NULL)
	{
		// Write new string with two null chars at the end
		*lpmszMulti = (LPSTR)calloc(newSzLength + 1 + 1, sizeof(CHAR));
		if (*lpmszMulti == NULL)
			NWL_ErrExit(ERROR_OUTOFMEMORY, "Failed to allocate memory in " __FUNCTION__);
		memcpy(*lpmszMulti, szNew, sizeof(CHAR) * newSzLength);
		return;
	}

	mszMulti = *lpmszMulti;

	// Iterate chars in old multistring until double null-char is found
	count = 0;
	for (oldLength = 1;
		!(mszMulti[oldLength - 1] == '\0' && mszMulti[oldLength] == '\0');
		oldLength++)
	{
		if (mszMulti[oldLength] == '\0')
			count++;
	}

	// Allocate memory: old + new string + 2 NULs
	mszResult = (LPSTR)calloc(oldLength + newSzLength + 1 + 1, sizeof(CHAR));
	if (mszResult == NULL)
		NWL_ErrExit(ERROR_OUTOFMEMORY, "Failed to allocate memory in " __FUNCTION__);

	// Copy values
	memcpy(mszResult, mszMulti, oldLength);
	memcpy(&mszResult[oldLength], szNew, newSzLength);

	// Release old pointer
	free(mszMulti);

	// Repoint
	*lpmszMulti = mszResult;
}

PNODE_ATT NWL_NodeAttrSetMulti(PNODE node, LPCSTR key, LPCSTR value, int flags)
{
	NODE_ATT* att;
	const char* nvalue;
	const char* c;
	size_t nvalue_len;
	size_t key_len;
	char* new_key = NULL;
	char* new_value = NULL;

	if (!node || !key)
		return NULL;

	nvalue = value ? value : "\0";

	for (c = nvalue; *c != '\0'; c += strlen(c) + 1)
		NWL_Debug("NODE", "MULTI <%s> += {%s}", key, c);
	nvalue_len = (size_t)(c - nvalue + 1);

	att = NWL_NodeAttrGetEntry(node, key);
	if (att)
	{
		free(att->value);

		new_value = (char*)malloc(nvalue_len);
		if (!new_value)
			NWL_ErrExit(ERROR_OUTOFMEMORY, "Failed to allocate memory in " __FUNCTION__);

		memcpy(new_value, nvalue, nvalue_len);

		att->value = new_value;
		att->flags = flags | NAFLG_ARRAY;
		return att;
	}

	key_len = strlen(key) + 1;

	new_key = (char*)malloc(key_len);
	if (!new_key)
		NWL_ErrExit(ERROR_OUTOFMEMORY, "Failed to allocate memory in " __FUNCTION__);

	new_value = (char*)malloc(nvalue_len);
	if (!new_value)
	{
		free(new_key);
		NWL_ErrExit(ERROR_OUTOFMEMORY, "Failed to allocate memory in " __FUNCTION__);
	}

	memcpy(new_key, key, key_len);
	memcpy(new_value, nvalue, nvalue_len);

	NODE_ATT tmp = { 0 };
	tmp.key = new_key;
	tmp.value = new_value;
	tmp.flags = flags | NAFLG_ARRAY;

	shputs(node->attributes, tmp);

	return shgetp(node->attributes, tmp.key);
}

VOID
NWL_NodeAttrSetRaw(PNODE node, LPCSTR key, void* value, size_t len)
{
	if (value == NULL || len == 0 || len >= UINT32_MAX)
		return;

	switch (NWLC->BinaryFormat)
	{
	case BIN_FMT_BASE64:
	{
		char* base64 = NWL_Base64Encode(value, len);
		if (!base64)
			NWL_ErrExit(ERROR_OUTOFMEMORY, "Failed to allocate memory in "__FUNCTION__);
		NWL_NodeAttrSet(node, key, base64, NAFLG_FMT_BASE64);
		free(base64);
		return;
	}
	case BIN_FMT_HEX:
	{
		const char* t = "0123456789ABCDEF";
		PNODE nhex = NWL_NodeAppendNew(node, key, NFLG_ATTGROUP);
		for (size_t i = 0; i < len; i += 0x10)
		{
			char base[9]; // "00000000"
			char hex[48]; // "00 11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF"
			char* ptr = hex;
			sprintf_s(base, sizeof(base), "%08zX", i);
			for (UINT16 j = 0; j < 0x10; j++)
			{
				if (i + j >= len)
				{
					*ptr++ = ' ';
					*ptr++ = ' ';
				}
				else
				{
					*ptr++ = t[((unsigned char*)value)[i + j] >> 4];
					*ptr++ = t[((unsigned char*)value)[i + j] & 0x0F];
				}
				if (j < 0x0F)
					*ptr++ = ' ';
				*ptr = '\0';
			}
			NWL_NodeAttrSet(nhex, base, hex, 0);
		}
		return;
	}
	case BIN_FMT_NONE:
	default:
		return;
	}
}
