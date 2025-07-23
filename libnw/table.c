// SPDX-License-Identifier: Unlicense

// Base on https://github.com/cavaliercoder/sysinv

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "libnw.h"
#include "utils.h"
#include "base64.h"

PNODE NWL_NodeAlloc(LPCSTR name, INT flags)
{
	PNODE node = NULL;
	SIZE_T size;
	SIZE_T name_len = strlen(name) + 1;

	NWL_Debugf("[DBG] ALLOC [%s]\n", name);

	// Calculate required size
	size = sizeof(NODE) + (sizeof(CHAR) * name_len);

	// Allocate
	node = (PNODE)calloc(1, size);
	if (!node)
		NWL_ErrExit(ERROR_OUTOFMEMORY, "Failed to allocate memory in "__FUNCTION__);
	node->Children = (PNODE_LINK)calloc(1, sizeof(NODE_LINK));
	node->Attributes = (PNODE_ATT_LINK)calloc(1, sizeof(NODE_ATT_LINK));

	// Copy node name
	node->Name = (LPSTR)(node + 1);
	strcpy_s(node->Name, name_len, name);

	// Set flags
	node->Flags = flags;

	return node;
}

VOID NWL_NodeFree(PNODE node, INT deep)
{
	PNODE_ATT_LINK att = NULL;
	PNODE_LINK child;

	// Free attributes
	for (att = &node->Attributes[0]; att->LinkedAttribute; att++)
		free(att->LinkedAttribute);

	// Free children
	if (deep > 0)
	{
		for (child = &node->Children[0]; NULL != child->LinkedNode; child++)
			NWL_NodeFree(child->LinkedNode, deep);
	}

	free(node->Attributes);
	free(node->Children);
	free(node);
}

INT NWL_NodeDepth(PNODE node)
{
	PNODE parent;
	int count = 0;
	for (parent = node; parent->Parent; parent = parent->Parent)
		count++;
	return count;
}

INT NWL_NodeChildCount(PNODE node)
{
	int count = 0;
	while (node->Children[count].LinkedNode)
		count++;
	return count;
}

INT NWL_NodeAppendChild(PNODE parent, PNODE child)
{
	int i, old_count, new_count;
	PNODE_LINK new_links;

	// Count old children
	old_count = NWL_NodeChildCount(parent);
	if (NULL == child)
		return old_count;

	NWL_Debugf("[DBG] APPEND [%s] -> [%s]\n", parent->Name, child->Name);

	new_count = old_count + 1;

	// Allocate new link list
	new_links = (PNODE_LINK)calloc(1ULL + new_count, sizeof(NODE_LINK));
	if (!new_links)
		NWL_ErrExit(ERROR_OUTOFMEMORY, "Failed to allocate memory in "__FUNCTION__);

	// Copy old child links
	for (i = 0; i < old_count; i++)
		new_links[i].LinkedNode = parent->Children[i].LinkedNode;

	// Copy new children
	new_links[new_count - 1].LinkedNode = child;

	// Release old list
	free(parent->Children);
	parent->Children = new_links;

	// Update parent pointer
	child->Parent = parent;

	return new_count;
}

PNODE NWL_NodeAppendNew(PNODE parent, LPCSTR name, INT flags)
{
	PNODE node = NWL_NodeAlloc(name, flags);
	NWL_NodeAppendChild(parent, node);
	return node;
}

PNODE NWL_NodeGetChild(PNODE parent, LPCSTR name)
{
	PNODE_LINK link;
	for (link = &parent->Children[0]; link->LinkedNode != NULL; link++)
	{
		if (strcmp(link->LinkedNode->Name, name) == 0)
			return link->LinkedNode;
	}
	return NULL;
}

static PNODE_ATT NWL_NodeAllocAttr(LPCSTR key, LPCSTR value, int flags)
{
	PNODE_ATT att = NULL;
	LPCSTR nvalue = NULL;
	SIZE_T size;
	SIZE_T key_len, nvalue_len;

	if (NULL == key)
		return att;

	key_len = strlen(key) + 1;
	nvalue = value ? value : "";
	nvalue_len = strlen(nvalue) + 1;
	size = sizeof(NODE_ATT) + key_len + nvalue_len;

	att = (PNODE_ATT)calloc(1, size);
	if (!att)
		NWL_ErrExit(ERROR_OUTOFMEMORY, "Failed to allocate memory in "__FUNCTION__);

	att->Key = (LPSTR)(att + 1);
	memcpy(att->Key, key, key_len);

	att->Value = att->Key + key_len;
	if (NWLC->HideSensitive && (flags & NAFLG_FMT_SENSITIVE))
		memset(att->Value, '*', nvalue_len);
	else
		memcpy(att->Value, nvalue, nvalue_len);

	att->Flags = flags;

	return att;
}

static PNODE_ATT NWL_NodeAllocAttrMulti(LPCSTR key, LPCSTR value, int flags)
{
	PNODE_ATT att = NULL;
	LPCSTR nvalue = NULL;
	SIZE_T size;
	SIZE_T key_len, nvalue_len = 0;
	LPCSTR c;

	if (NULL == key)
		return att;

	nvalue = value ? value : "\0";

	// Calculate size of value
	for (c = &nvalue[0]; *c != '\0'; c += strlen(c) + 1)
		NWL_Debugf("[DBG] MULTI <%s> += {%s}\n", key, c);
	nvalue_len = c - nvalue + 1;

	key_len = strlen(key) + 1;
	size = sizeof(NODE_ATT) + key_len + nvalue_len;

	att = (PNODE_ATT)calloc(1, size);
	if (!att)
		NWL_ErrExit(ERROR_OUTOFMEMORY, "Failed to allocate memory in "__FUNCTION__);

	att->Key = (LPSTR)(att + 1);
	strcpy_s(att->Key, key_len, key);

	att->Value = att->Key + key_len;
	memcpy(att->Value, nvalue, nvalue_len);

	att->Flags = flags | NAFLG_ARRAY;

	return att;
}

INT NWL_NodeAttrCount(PNODE node)
{
	int count = 0;
	PNODE_ATT_LINK link = node->Attributes;
	while (node->Attributes[count].LinkedAttribute)
		count++;
	return count;
}

static INT NWL_NodeAttrGetIndex(PNODE node, LPCSTR key)
{
	int i;
	for (i = 0; node->Attributes[i].LinkedAttribute; i++)
	{
		if (strcmp(node->Attributes[i].LinkedAttribute->Key, key) == 0)
			return i;
	}
	return -1;
}

LPCSTR NWL_NodeAttrGet(PNODE node, LPCSTR key)
{
	int i = -1;
	if (node)
		i = NWL_NodeAttrGetIndex(node, key);
	return (i < 0) ? "-\0" : node->Attributes[i].LinkedAttribute->Value;
}

PNODE_ATT NWL_NodeAttrSet(PNODE node, LPCSTR key, LPCSTR value, INT flags)
{
	int i, old_count, new_count, new_index;
	PNODE_ATT att;
	PNODE_ATT_LINK link = NULL;
	PNODE_ATT_LINK new_link = NULL;
	PNODE_ATT_LINK new_links = NULL;

	NWL_Debugf("[DBG] SET <%s> = <%s>\n", key, value);

	if (!NWLC->HumanSize && (flags & NAFLG_FMT_HUMAN_SIZE))
		flags |= NAFLG_FMT_NUMERIC;
	// Count old attributes
	old_count = NWL_NodeAttrCount(node);

	// Search for existing attribute
	new_index = NWL_NodeAttrGetIndex(node, key);
	if (new_index > -1)
		new_link = &node->Attributes[new_index];

	if (new_link)
	{
		// Replace attribute link with new value if value differs
		if (strcmp(new_link->LinkedAttribute->Value, value) != 0)
		{
			free(new_link->LinkedAttribute);
			new_link->LinkedAttribute = NWL_NodeAllocAttr(key, value, flags);
		}
		else
		{
			// Only update flags if value is identical
			new_link->LinkedAttribute->Flags = flags;
		}
		att = new_link->LinkedAttribute;
	}
	else
	{
		// Reallocate link list and add new attribute
		new_index = old_count;
		new_count = old_count + 1;

		// Allocate new link list
		new_links = (PNODE_ATT_LINK)calloc(1ULL + new_count, sizeof(NODE_ATT_LINK));
		if (!new_links)
			NWL_ErrExit(ERROR_OUTOFMEMORY, "Failed to allocate memory in "__FUNCTION__);

		// Copy old child links
		for (i = 0; i < old_count; i++)
			new_links[i].LinkedAttribute = node->Attributes[i].LinkedAttribute;

		// Copy new attribute
		new_links[new_index].LinkedAttribute = NWL_NodeAllocAttr(key, value, flags);

		// Release old list
		free(node->Attributes);
		node->Attributes = new_links;

		att = new_links[new_index].LinkedAttribute;
	}

	return att;
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
		NWL_ErrExit(ERROR_INVALID_DATA, "Failed to calculate string length in "__FUNCTION__);
	}
	buf = calloc(sizeof(CHAR), sz);
	if (!buf)
	{
		va_end(ap);
		NWL_ErrExit(ERROR_OUTOFMEMORY, "Failed to allocate memory in "__FUNCTION__);
	}
	vsnprintf(buf, sz, format, ap);
	va_end(ap);
	att = NWL_NodeAttrSet(node, key, buf, flags);
	free(buf);
	return att;
}

PNODE_ATT NWL_NodeAttrSetMulti(PNODE node, LPCSTR key, LPCSTR value, int flags)
{
	int i, old_count, new_count, new_index;
	PNODE_ATT att;
	PNODE_ATT_LINK link = NULL;
	PNODE_ATT_LINK new_link = NULL;
	PNODE_ATT_LINK new_links = NULL;
	LPSTR temp = NULL;

	// Create new attribute
	att = NWL_NodeAllocAttrMulti(key, value, flags);

	// Count old attributes
	old_count = NWL_NodeAttrCount(node);

	// Search for existing attribute
	new_index = NWL_NodeAttrGetIndex(node, key);
	if (new_index > -1)
		new_link = &node->Attributes[new_index];

	if (new_link)
	{
		free(new_link->LinkedAttribute);
		new_link->LinkedAttribute = att;
	}
	else
	{
		// Reallocate link list and add new attribute
		new_index = old_count;
		new_count = old_count + 1;

		// Allocate new link list
		new_links = (PNODE_ATT_LINK)calloc(1ULL + new_count, sizeof(NODE_ATT_LINK));
		if (!new_links)
			NWL_ErrExit(ERROR_OUTOFMEMORY, "Failed to allocate memory in "__FUNCTION__);

		// Copy old child links
		for (i = 0; i < old_count; i++)
			new_links[i].LinkedAttribute = node->Attributes[i].LinkedAttribute;

		// Copy new attribute
		new_links[new_index].LinkedAttribute = att;

		// Release old list
		free(node->Attributes);
		node->Attributes = new_links;
	}

	return att;
}

VOID NWL_NodeAppendMultiSz(LPSTR* lpmszMulti, LPCSTR szNew)
{
	DWORD oldLength = 0;
	DWORD newSzLength = 0;
	DWORD count = 0;
	LPSTR mszMulti = NULL;
	LPSTR mszResult = NULL;
	LPCSTR c = NULL;

	if (NULL == szNew)
		return;

	newSzLength = (DWORD)strlen(szNew);

	// If old multistring was empty
	if (NULL == *lpmszMulti)
	{
		// Write new string with two null chars at the end
		*lpmszMulti = calloc(newSzLength + 1 + 1, sizeof(CHAR));
		if (NULL == *lpmszMulti)
			NWL_ErrExit(ERROR_OUTOFMEMORY, "Failed to allocate memory in "__FUNCTION__);
		memcpy(*lpmszMulti, szNew, sizeof(CHAR) * newSzLength);
		return;
	}

	mszMulti = *lpmszMulti;

	// Iterate chars in old multistring until double null-char is found
	count = 0;
	for (oldLength = 1;
		!(('\0' == mszMulti[oldLength - 1]) && ('\0' == mszMulti[oldLength]));
		oldLength++)
	{
		if ('\0' == mszMulti[oldLength])
			count++;
	}

	// Allocate memory
	mszResult = calloc(oldLength + newSzLength + 1 + 1, sizeof(CHAR));
	if (NULL == mszResult)
		NWL_ErrExit(ERROR_OUTOFMEMORY, "Failed to allocate memory in "__FUNCTION__);

	// Copy values
	memcpy(mszResult, mszMulti, oldLength);
	memcpy(&mszResult[oldLength], szNew, newSzLength);

	// Release old pointer
	free(mszMulti);

	// Repoint
	*lpmszMulti = mszResult;
}

PNODE_ATT NWL_NodeAttrSetRaw(PNODE node, LPCSTR key, void* value, size_t len, INT flags)
{
	PNODE_ATT att = NULL;
	char* base64 = NULL;
	flags |= NAFLG_FMT_BASE64;
	base64 = NWL_Base64Encode(value, len);
	if (!base64)
		NWL_ErrExit(ERROR_OUTOFMEMORY, "Failed to allocate memory in "__FUNCTION__);
	att = NWL_NodeAttrSet(node, key, base64, flags);
	free(base64);
	return att;
}
