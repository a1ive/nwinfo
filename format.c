// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "format.h"
#include "nwinfo.h"

// Base on https://github.com/cavaliercoder/sysinv

#define NODE_BUFFER_LEN	32767

static int indent_depth = 0;

static void fprintcx(FILE* file, LPCSTR s, int count)
{
	int i;
	for (i = 0; i < count; i++)
		fprintf(file, s);
}

PNODE node_alloc(LPCSTR name, int flags)
{
	PNODE node = NULL;
	SIZE_T size;

	// Calculate required size
	size = sizeof(NODE) + (sizeof(CHAR) * (strlen(name) + 1));

	// Allocate
	node = (PNODE)calloc(1, size);
	if (!node)
	{
		fprintf(stderr, "Failed to allocate memory for new node\n");
		exit(ERROR_OUTOFMEMORY);
	}
	node->Children = (PNODE_LINK)calloc(1, sizeof(NODE_LINK));
	node->Attributes = (PNODE_ATT_LINK)calloc(1, sizeof(NODE_ATT_LINK));

	// Copy node name
	node->Name = (LPSTR)(node + 1);
	strcpy_s(node->Name, NODE_BUFFER_LEN, name);

	// Set flags
	node->Flags = flags;

	return node;
}

void node_free(PNODE node, int deep)
{
	PNODE_ATT_LINK att;
	PNODE_LINK child;

	// Free attributes
	for (att = &node->Attributes[0]; att->LinkedAttribute; att++)
		free(att->LinkedAttribute);

	// Free children
	if (deep > 0)
	{
		for (child = &node->Children[0]; NULL != child->LinkedNode; child++)
			node_free(child->LinkedNode, deep);
	}

	free(node->Attributes);
	free(node->Children);
	free(node);
}

int node_depth(PNODE node)
{
	PNODE parent;
	int count = 0;
	for (parent = node; parent->Parent; parent = parent->Parent)
		count++;
	return count;
}

int node_child_count(PNODE node)
{
	int count = 0;
	PNODE_LINK link = node->Children;
	while (node->Children[count].LinkedNode)
		count++;
	return count;
}

int node_append_child(PNODE parent, PNODE child)
{
	int i, old_count, new_count;
	PNODE_LINK new_links;

	// Count old children
	old_count = node_child_count(parent);
	if (NULL == child)
		return old_count;

	new_count = old_count + 1;

	// Allocate new link list
	new_links = (PNODE_LINK)calloc(1ULL + new_count, sizeof(NODE_LINK));
	if (!new_links)
	{
		fprintf(stderr, "Failed to allocate memory for appending node\n");
		exit(ERROR_OUTOFMEMORY);
	}

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

PNODE node_append_new(PNODE parent, LPCSTR name, int flags)
{
	PNODE node = node_alloc(name, flags);
	node_append_child(parent, node);
	return node;
}

static PNODE_ATT node_alloc_att(LPCSTR key, LPCSTR value, int flags)
{
	PNODE_ATT att = NULL;
	LPCSTR nvalue = NULL;
	SIZE_T size;

	if (NULL == key)
		return att;

	nvalue = value ? value : "";

	size = sizeof(NODE_ATT) + strlen(key) + 1 + strlen(nvalue) + 1;

	att = (PNODE_ATT)calloc(1, size);
	if (!att)
	{
		fprintf(stderr, "Failed to allocate memory in node_alloc_att\n");
		exit(ERROR_OUTOFMEMORY);
	}

	att->Key = (LPSTR)(att + 1);
	strcpy_s(att->Key, NODE_BUFFER_LEN, key);

	att->Value = att->Key + strlen(key) + 1;
	strcpy_s(att->Value, NODE_BUFFER_LEN, nvalue);

	att->Flags = flags;

	return att;
}

int node_att_count(PNODE node)
{
	int count = 0;
	PNODE_ATT_LINK link = node->Attributes;
	while (node->Attributes[count].LinkedAttribute)
		count++;
	return count;
}

int node_att_indexof(PNODE node, LPCSTR key)
{
	int i;
	for (i = 0; node->Attributes[i].LinkedAttribute; i++)
	{
		if (strcmp(node->Attributes[i].LinkedAttribute->Key, key) == 0)
			return i;
	}
	return -1;
}

LPSTR node_att_get(PNODE node, LPCSTR key)
{
	int i = node_att_indexof(node, key);
	return (i < 0) ? NULL : node->Attributes[i].LinkedAttribute->Value;
}

PNODE_ATT node_att_set(PNODE node, LPCSTR key, LPCSTR value, int flags)
{
	int i, old_count, new_count, new_index;
	PNODE_ATT att;
	PNODE_ATT_LINK link = NULL;
	PNODE_ATT_LINK new_link = NULL;
	PNODE_ATT_LINK new_links = NULL;

	if (!nwinfo_human_size && (flags & NAFLG_FMT_HUMAN_SIZE))
		flags |= NAFLG_FMT_NUMERIC;
	// Count old attributes
	old_count = node_att_count(node);

	// Search for existing attribute
	new_index = node_att_indexof(node, key);
	if (new_index > -1)
		new_link = &node->Attributes[new_index];

	if (new_link)
	{
		// Replace attribute link with new value if value differs
		if (strcmp(new_link->LinkedAttribute->Value, value) != 0)
		{
			free(new_link->LinkedAttribute);
			new_link->LinkedAttribute = node_alloc_att(key, value, flags);
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
		{
			fprintf(stderr, "Failed to allocate memory in node_att_set\n");
			exit(ERROR_OUTOFMEMORY);
		}

		// Copy old child links
		for (i = 0; i < old_count; i++)
			new_links[i].LinkedAttribute = node->Attributes[i].LinkedAttribute;

		// Copy new attribute
		new_links[new_index].LinkedAttribute = node_alloc_att(key, value, flags);

		// Release old list
		free(node->Attributes);
		node->Attributes = new_links;

		att = new_links[new_index].LinkedAttribute;
	}

	return att;
}

static CHAR nsbuf[NODE_BUFFER_LEN];

PNODE_ATT node_att_setf(PNODE node, LPCSTR key, int flags, const char* format, ...)
{
	va_list ap;
	//ZeroMemory(nsbuf, sizeof(nsbuf));
	va_start(ap, format);
	vsnprintf(nsbuf, sizeof(nsbuf), format, ap);
	va_end(ap);
	return node_att_set(node, key, nsbuf, flags);
}

static SIZE_T json_escape_content(LPCTSTR input, LPSTR buffer, DWORD bufferSize)
{
	LPCTSTR cIn = input;
	LPSTR cOut = buffer;
	DWORD newBufferSize = 0;

	while (*cIn != '\0')
	{
		switch (*cIn)
		{
		case '"':
			memcpy(cOut, "\\\"", 2);
			cOut += 2;
			break;

		case '\\':
			memcpy(cOut, "\\\\", 2);
			cOut += 2;
			break;

		case '\r':
			break;

		case '\n':
			memcpy(cOut, "\\n", 2);
			cOut += 2;
			break;

		default:
			memcpy(cOut, cIn, 1);
			cOut += 1;
			break;
		}

		cIn++;
	}
	*cOut = '\0';

	return cOut - buffer;
}

int node_to_json(PNODE node, FILE* file, int flags)
{
	int i = 0;
	int nodes = 1;
	int atts = node_att_count(node);
	int children = node_child_count(node);
	int plural = 0;
	int indent = ((flags & NODE_JS_FLAG_NOWS) == 0) ? indent_depth : 0;
	LPCSTR nl = flags & NODE_JS_FLAG_NOWS ? "" : NODE_JS_DELIM_NL;
	LPCSTR space = flags & NODE_JS_FLAG_NOWS ? "" : NODE_JS_DELIM_SPACE;

	// Print header
	fprintcx(file, NODE_JS_DELIM_INDENT, indent);
	if (indent_depth > 0 && (node->Flags & NFLG_TABLE_ROW) == 0)
		fprintf(file, "\"%s\":%s", node->Name, space);

	if ((node->Flags & NFLG_TABLE) == 0)
		fprintf(file, "{");
	else
		fprintf(file, "[");

	// Print attributes
	if (atts > 0 && (node->Flags & NFLG_TABLE) == 0)
	{
		for (i = 0; i < atts; i++)
		{
			if (*node->Attributes[i].LinkedAttribute->Value != '\0')
			{
				if (plural)
					fprintf(file, ",");

				// Print attribute name
				fprintf(file, "%s", NODE_JS_DELIM_NL);
				fprintcx(file, NODE_JS_DELIM_INDENT, indent + 1);
				fprintf(file, "\"%s\":%s", node->Attributes[i].LinkedAttribute->Key, space);

				// Print value
				if (node->Attributes[i].LinkedAttribute->Flags & NAFLG_FMT_NUMERIC)
					fprintf(file, node->Attributes[i].LinkedAttribute->Value);
				else
				{
					json_escape_content(node->Attributes[i].LinkedAttribute->Value, nwinfo_buffer, NWINFO_BUFSZ);
					fprintf(file, "\"%s\"", nwinfo_buffer);
				}
				plural = 1;
			}
		}
	}

	// Print children
	if (children > 0)
	{
		indent_depth++;
		for (i = 0; i < children; i++)
		{
			if (plural)
				fprintf(file, ",");

			fprintf(file, NODE_JS_DELIM_NL);
			nodes += node_to_json(node->Children[i].LinkedNode, file, flags);
			plural = 1;
		}
		indent_depth--;
	}

	if (atts > 0 || children > 0)
	{
		fprintf(file, NODE_JS_DELIM_NL);
		fprintcx(file, NODE_JS_DELIM_INDENT, indent);
	}
	if ((node->Flags & NFLG_TABLE) == 0)
		fprintf(file, "}");
	else
		fprintf(file, "]");
	return nodes;
}

int node_to_yaml(PNODE node, FILE* file, int flags)
{
	int i = 0;
	int count = 1;
	int atts = node_att_count(node);
	int children = node_child_count(node);
	PNODE_ATT att = NULL;
	PNODE child = NULL;
	CHAR* attVal = NULL;

	if (!node->Parent)
		fprintf(file, "---%s", NODE_YAML_DELIM_NL);

	fprintcx(file, NODE_YAML_DELIM_INDENT, indent_depth);

	if (NFLG_TABLE_ROW & node->Flags)
		fprintf(file, "- ");

	fprintf(file, "%s:", node->Name);

	// Print attributes
	if (atts > 0)
	{
		fprintf(file, NODE_YAML_DELIM_NL);
		for (i = 0; i < atts; i++)
		{
			att = node->Attributes[i].LinkedAttribute;
			attVal = (att->Value && *att->Value != '\0') ? att->Value : "~";

			fprintcx(file, NODE_YAML_DELIM_INDENT, indent_depth + 1);
			if (att->Flags & NAFLG_FMT_NEED_QUOTE)
				fprintf(file, "%s: '%s'%s", att->Key, attVal, NODE_YAML_DELIM_NL);
			else
				fprintf(file, "%s: %s%s", att->Key, attVal, NODE_YAML_DELIM_NL);
		}
	}

	// Print children
	if (children > 0)
	{
		if (atts == 0)
			fprintf(file, NODE_YAML_DELIM_NL);
		indent_depth++;
		for (i = 0; i < children; i++)
		{
			child = node->Children[i].LinkedNode;
			node_to_yaml(child, file, 0);
		}
		indent_depth--;
	}
	else if (atts == 0)
	{
		fprintf(file, " ~%s", NODE_YAML_DELIM_NL);
	}

	return count;
}
