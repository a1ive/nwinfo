// SPDX-License-Identifier: Unlicense
#pragma once

#include <stdio.h>
#define VC_EXTRALEAN
#include <windows.h>

#include "nwapi.h"

#define NFLG_PLACEHOLDER		0x1		// Node is a placeholder with no attributes
#define NFLG_TABLE				0x2		// Node represents an array of tabular rows
#define NFLG_TABLE_ROW			0x4		// Node represents a row of tabular data
#define NFLG_ATTGROUP			0x8		// This node is a grouping of attributes belonging to the parent node

#define NAFLG_KEY				0x1		// Attribute is a key field for the parent node
#define NAFLG_ARRAY				0x4		// Attribute value is a multistring array terminated by a zero length string

#define NAFLG_FMT_STRING		0x0000
#define NAFLG_FMT_BOOLEAN		0x0100
#define NAFLG_FMT_NUMERIC		0x0200
#define NAFLG_FMT_NEED_QUOTE	0x0400
#define NAFLG_FMT_HUMAN_SIZE	0x0800
#define NAFLG_FMT_SENSITIVE		0x1000
#define NAFLG_FMT_KEY_QUOTE		0x2000

#define NAFLG_FMT_IPADDR		(NAFLG_FMT_NEED_QUOTE | NAFLG_FMT_STRING)
#define NAFLG_FMT_GUID			(NAFLG_FMT_NEED_QUOTE | NAFLG_FMT_STRING)
#define NAFLG_FMT_BASE64		(NAFLG_FMT_NEED_QUOTE | NAFLG_FMT_STRING)

#define NA_BOOL_TRUE			"Y"
#define NA_BOOL_FALSE			"N"

// Structures
typedef struct _NODE_ATT
{
	char* key; // alloc
	char* value; // alloc
	int flags;
} NODE_ATT, * PNODE_ATT;

typedef struct _NODE
{
	char* name; // alloc
	struct _NODE* parent; // ptr
	struct _NODE** children; // dynamic array
	struct _NODE_ATT* attributes; // string hash map
	int flags;
} NODE, * PNODE;

// Functions
LIBNW_API PNODE NWL_NodeAlloc(LPCSTR name, INT flags);
LIBNW_API VOID NWL_NodeFree(PNODE node, INT deep);

LIBNW_API INT NWL_NodeDepth(PNODE node);
LIBNW_API INT NWL_NodeChildCount(PNODE node);
LIBNW_API PNODE NWL_NodeEnumChild(PNODE parent, INT index);
LIBNW_API INT NWL_NodeAppendChild(PNODE parent, PNODE child);
LIBNW_API PNODE NWL_NodeAppendNew(PNODE parent, LPCSTR name, INT flags);
LIBNW_API PNODE NWL_NodeGetChild(PNODE parent, LPCSTR name);

LIBNW_API INT NWL_NodeAttrCount(PNODE node);
LIBNW_API PNODE_ATT NWL_NodeAttrEnum(PNODE node, INT index);
LIBNW_API LPCSTR NWL_NodeAttrGet(PNODE node, LPCSTR key);
LIBNW_API PNODE_ATT NWL_NodeAttrSet(PNODE node, LPCSTR key, LPCSTR value, INT flags);
LIBNW_API PNODE_ATT
NWL_NodeAttrSetf(PNODE node, LPCSTR key, INT flags, LPCSTR _Printf_format_string_ format, ...);

#define NWL_NodeAttrSetBool(node, key, value, flags) \
	NWL_NodeAttrSet((node), (key), ((value) ? NA_BOOL_TRUE : NA_BOOL_FALSE), (flags) | NAFLG_FMT_BOOLEAN)

LIBNW_API PNODE_ATT NWL_NodeAttrSetMulti(PNODE node, LPCSTR key, LPCSTR value, int flags);
LIBNW_API VOID NWL_NodeAppendMultiSz(LPSTR* lpmszMulti, LPCSTR szNew);

LIBNW_API VOID NWL_NodeAttrSetRaw(PNODE node, LPCSTR key, void* value, size_t len);
