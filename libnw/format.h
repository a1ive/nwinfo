// SPDX-License-Identifier: Unlicense
#pragma once

#include <stdio.h>
#define VC_EXTRALEAN
#include <windows.h>

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

#define NAFLG_FMT_IPADDR		(NAFLG_FMT_NEED_QUOTE | NAFLG_FMT_STRING)
#define NAFLG_FMT_GUID			(NAFLG_FMT_NEED_QUOTE | NAFLG_FMT_STRING)

// Structures
typedef struct _NODE
{
	CHAR* Name;						// Name of the node
	struct _NODE_ATT_LINK* Attributes;	// Array of attributes linked to the node
	struct _NODE* Parent;				// Parent node
	struct _NODE_LINK* Children;		// Array of linked child nodes
	INT Flags;							// Node configuration flags
} NODE, * PNODE;

typedef struct _NODE_LINK
{
	struct _NODE* LinkedNode;			// Node attached to this node
} NODE_LINK, * PNODE_LINK;

typedef struct _NODE_ATT
{
	char* Key;						// Attribute name
	char* Value;						// Attribute value string (may be null separated multistring if NAFLG_ARRAY is set)
	INT Flags;							// Attribute configuration flags
} NODE_ATT, * PNODE_ATT;

typedef struct _NODE_ATT_LINK
{
	struct _NODE_ATT* LinkedAttribute;	// Attribute linked to this node
} NODE_ATT_LINK, * PNODE_ATT_LINK;

// Functions
PNODE NWL_NodeAlloc(LPCSTR name, INT flags);
VOID NWL_NodeFree(PNODE node, INT deep);

INT NWL_NodeDepth(PNODE node);
INT NWL_NodeChildCount(PNODE node);
INT NWL_NodeAppendChild(PNODE parent, PNODE child);
PNODE NWL_NodeAppendNew(PNODE parent, LPCSTR name, INT flags);
PNODE NWL_NodeGetChild(PNODE parent, LPCSTR name);

INT NWL_NodeAttrCount(PNODE node);
LPCSTR NWL_NodeAttrGet(PNODE node, LPCSTR key);
PNODE_ATT NWL_NodeAttrSet(PNODE node, LPCSTR key, LPCSTR value, INT flags);
PNODE_ATT
NWL_NodeAttrSetf(PNODE node, LPCSTR key, INT flags, LPCSTR _Printf_format_string_ format, ...);

#define NWL_NodeAttrSetBool(node, key, value, flags) \
	NWL_NodeAttrSet(node, key, (value ? "Yes" : "No"), flags | NAFLG_FMT_BOOLEAN)

PNODE_ATT NWL_NodeAttrSetMulti(PNODE node, LPCSTR key, LPCSTR value, int flags);
VOID NWL_NodeAppendMultiSz(LPSTR* lpmszMulti, LPCSTR szNew);
