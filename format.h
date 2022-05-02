// SPDX-License-Identifier: Unlicense
#pragma once

#include <stdio.h>
#include <stdint.h>
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

#define NAFLG_FMT_IPADDR		(NAFLG_FMT_NEED_QUOTE | NAFLG_FMT_STRING)
#define NAFLG_FMT_GUID			(NAFLG_FMT_NEED_QUOTE | NAFLG_FMT_STRING)

// Macros for printing nodes to JSON
#define NODE_JS_FLAG_NOWS		0x2		// No whitespace
#define NODE_JS_DELIM_NL		"\n"	// New line for JSON output
#define NODE_JS_DELIM_INDENT	"  "	// Tab token for JSON output
#define NODE_JS_DELIM_SPACE		" "	// Space used between keys and values

// Macros for printing nodes to YAML
#define NODE_YAML_DELIM_NL		"\n"	// New line token for YAML output
#define NODE_YAML_DELIM_INDENT	"    "	// Tab toekn for YAML output

// Function macros
#define node_att_set_bool(node, key, value, flags) \
	node_att_set(node, key, (value ? "Yes" : "No"), flags | NAFLG_FMT_BOOLEAN)

// Structures
typedef struct _NODE
{
	char* Name;						// Name of the node
	struct _NODE_ATT_LINK* Attributes;	// Array of attributes linked to the node
	struct _NODE* Parent;				// Parent node
	struct _NODE_LINK* Children;		// Array of linked child nodes
	int Flags;							// Node configuration flags
} NODE, * PNODE;

typedef struct _NODE_LINK
{
	struct _NODE* LinkedNode;			// Node attached to this node
} NODE_LINK, * PNODE_LINK;

typedef struct _NODE_ATT
{
	char* Key;						// Attribute name
	char* Value;						// Attribute value string (may be null separated multistring if NAFLG_ARRAY is set)
	int Flags;							// Attribute configuration flags
} NODE_ATT, * PNODE_ATT;

typedef struct _NODE_ATT_LINK
{
	struct _NODE_ATT* LinkedAttribute;	// Attribute linked to this node
} NODE_ATT_LINK, * PNODE_ATT_LINK;

// Functions
PNODE node_alloc(LPCSTR name, int flags);
void node_free(PNODE node, int deep);

int node_depth(PNODE node);
int node_child_count(PNODE node);
int node_append_child(PNODE parent, PNODE child);
PNODE node_append_new(PNODE parent, LPCSTR name, int flags);

int node_att_count(PNODE node);
int node_att_indexof(PNODE node, LPCSTR key);
PNODE_ATT node_att_set(PNODE node, LPCSTR key, LPCSTR value, int flags);
PNODE_ATT node_att_setf(PNODE node, LPCSTR key, int flags, const char* format, ...);
LPSTR node_att_get(PNODE node, LPCSTR key);

int node_to_json(PNODE node, FILE* file, int flags);
int node_to_yaml(PNODE node, FILE* file, int flags);
