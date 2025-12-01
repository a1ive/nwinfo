// SPDX-License-Identifier: Unlicense

// Base on https://github.com/cavaliercoder/sysinv

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "libnw.h"
#include "utils.h"

// Macros for printing nodes to JSON
#define NODE_JS_DELIM_NL		"\n"	// New line for JSON output
#define NODE_JS_DELIM_INDENT	"  "	// Tab token for JSON output
#define NODE_JS_BOOL_TRUE		"true"	// JSON boolean true
#define NODE_JS_BOOL_FALSE		"false"	// JSON boolean false

// Macros for printing nodes to YAML
#define NODE_YAML_DELIM_NL		"\n"	// New line token for YAML output
#define NODE_YAML_DELIM_INDENT	"    "	// Tab token for YAML output
// YAML 1.2 only supports "true" and "false" as boolean values
#define NODE_YAML_BOOL_TRUE		"true"	// YAML boolean true
#define NODE_YAML_BOOL_FALSE	"false"	// YAML boolean false

// Macros for printing nodes to LUA
#define NODE_LUA_DELIM_NL		"\n"	// New line token for LUA output
#define NODE_LUA_DELIM_INDENT	"  "	// Tab token for LUA output
#define NODE_LUA_BOOL_TRUE		"true"	// LUA boolean true
#define NODE_LUA_BOOL_FALSE		"false"	// LUA boolean false

// Macros for printing nodes to TREE
#define NODE_TREE_DELIM_NL		"\n"	// New line token for TREE output
#define NODE_TREE_BOOL_TRUE		"Yes"	// TREE boolean true
#define NODE_TREE_BOOL_FALSE	"No"	// TREE boolean false
#define NODE_TREE_DELIM_INDENT	" |"
#define NODE_TREE_BRANCH		" *"
#define NODE_TREE_LEAF			"-"
#define NODE_TREE_SUB			"\\"

static int indent_depth = 0;

static void fprintcx(FILE* file, LPCSTR s, int count)
{
	int i;
	for (i = 0; i < count; i++)
		fputs(s, file);
}

// CP_UTF8 -> CP_ACP
static char* NWL_NodeMbsDup(LPCSTR old_str, SIZE_T* new_size)
{
	char* new_str = NULL;
	LPCWSTR wstr = NULL;
	int size;
	if (!old_str)
		old_str = "";
	if (NWLC->CodePage == CP_UTF8)
		goto fail;
	wstr = NWL_Utf8ToUcs2(old_str);
	size = WideCharToMultiByte(NWLC->CodePage, 0, wstr, -1, NULL, 0, NULL, NULL);
	if (size <= 0)
		goto fail;
	new_str = (char*)calloc(size, sizeof(char));
	if (!new_str)
		goto fail;
	WideCharToMultiByte(NWLC->CodePage, 0, wstr, -1, new_str, size, NULL, NULL);
	if (new_size)
		*new_size = size;
	return new_str;
fail:
	if (new_size)
		*new_size = strlen(old_str) + 1;
	return _strdup(old_str);
}

static SIZE_T JsonEscapeContent(LPCSTR input, LPSTR buffer, DWORD bufferSize)
{
	CHAR* mbs = NWL_NodeMbsDup(input, NULL);
	LPCSTR cIn = mbs;
	LPSTR cOut = buffer;

	while (*cIn != '\0')
	{
		// truncate
		if (2UL + (cOut - buffer)>= bufferSize)
			break;
		switch (*cIn)
		{
		case '\"':
			memcpy(cOut, "\\\"", 2);
			cOut += 2;
			break;
		case '\\':
			memcpy(cOut, "\\\\", 2);
			cOut += 2;
			break;
#if 0 //?
		case '/':
			memcpy(cOut, "\\/", 2);
			cOut += 2;
			break;
#endif
		case '\n':
			memcpy(cOut, "\\n", 2);
			cOut += 2;
			break;
		case '\r':
			memcpy(cOut, "\\r", 2);
			cOut += 2;
			break;
		case '\t':
			memcpy(cOut, "\\t", 2);
			cOut += 2;
			break;
		default:
			if ((*cIn > 0x00 && *cIn < 0x21) || *cIn == 0x7f)
				memcpy(cOut, " ", 1);
			else
				memcpy(cOut, cIn, 1);
			cOut += 1;
			break;
		}
		cIn++;
	}
	*cOut = '\0';
	free(mbs);
	return cOut - buffer;
}

static SIZE_T YamlEscapeContent(LPCSTR input, LPSTR buffer, DWORD bufferSize)
{
	CHAR* mbs = NWL_NodeMbsDup(input, NULL);
	LPCSTR cIn = mbs;
	LPSTR cOut = buffer;

	while (*cIn != '\0')
	{
		// truncate
		if (2UL + (cOut - buffer) >= bufferSize)
			break;
		switch (*cIn)
		{
		case '\'':
			memcpy(cOut, "\'\'", 2);
			cOut += 2;
			break;
		case '\n':
			memcpy(cOut, "\\n", 2);
			cOut += 2;
			break;
		case '\r':
			memcpy(cOut, "\\r", 2);
			cOut += 2;
			break;
		case '\t':
			memcpy(cOut, "\\t", 2);
			cOut += 2;
			break;
		default:
			if ((*cIn > 0x00 && *cIn < 0x21) || *cIn == 0x7f)
				memcpy(cOut, " ", 1);
			else
				memcpy(cOut, cIn, 1);
			cOut += 1;
			break;
		}
		cIn++;
	}
	*cOut = '\0';
	free(mbs);
	return cOut - buffer;
}

static SIZE_T LuaEscapeContent(LPCSTR input, LPSTR buffer, DWORD bufferSize)
{
	CHAR* mbs = NWL_NodeMbsDup(input, NULL);
	LPCSTR cIn = mbs;
	LPSTR cOut = buffer;

	while (*cIn != '\0')
	{
		// truncate
		if (2UL + (cOut - buffer) >= bufferSize)
			break;
		switch (*cIn)
		{
		case '\"':
			memcpy(cOut, "\\\"", 2);
			cOut += 2;
			break;
		case '\'':
			memcpy(cOut, "\\\'", 2);
			cOut += 2;
			break;
		case '\\':
			memcpy(cOut, "\\\\", 2);
			cOut += 2;
			break;
		case '\n':
			memcpy(cOut, "\\n", 2);
			cOut += 2;
			break;
		case '\r':
			memcpy(cOut, "\\r", 2);
			cOut += 2;
			break;
		case '\t':
			memcpy(cOut, "\\t", 2);
			cOut += 2;
			break;
		default:
			if ((*cIn > 0x00 && *cIn < 0x21) || *cIn == 0x7f)
				memcpy(cOut, " ", 1);
			else
				memcpy(cOut, cIn, 1);
			cOut += 1;
			break;
		}
		cIn++;
	}
	*cOut = '\0';
	free(mbs);
	return cOut - buffer;
}

static SIZE_T TreeEscapeContent(LPCSTR input, LPSTR buffer, DWORD bufferSize)
{
	CHAR* mbs = NWL_NodeMbsDup(input, NULL);
	LPCSTR cIn = mbs;
	LPSTR cOut = buffer;

	while (*cIn != '\0')
	{
		// truncate
		if (2UL + (cOut - buffer) >= bufferSize)
			break;

		// For TREE format, we replace control characters with a space to avoid breaking the layout.
		if ((*cIn > 0x00 && *cIn < 0x20) || *cIn == 0x7f)
		{
			*cOut = ' ';
			cOut++;
		}
		else
		{
			*cOut = *cIn;
			cOut++;
		}
		cIn++;
	}
	*cOut = '\0';
	free(mbs);
	return cOut - buffer;
}

static SIZE_T HtmlEscapeContent(LPCSTR input, LPSTR buffer, DWORD bufferSize)
{
	CHAR* mbs = NWL_NodeMbsDup(input, NULL);
	LPCSTR cIn = mbs;
	LPSTR cOut = buffer;

	while (*cIn != '\0')
	{
		// worst case: 1 char -> 6 chars (")
		if (6UL + (cOut - buffer) >= bufferSize)
			break;

		switch (*cIn)
		{
		case '&':
			memcpy(cOut, "&amp;", 5);
			cOut += 5;
			break;
		case '\"':
			memcpy(cOut, "&quot;", 6);
			cOut += 6;
			break;
		case '\'':
			memcpy(cOut, "&apos;", 6);
			cOut += 6;
			break;
		case '<':
			memcpy(cOut, "&lt;", 4);
			cOut += 4;
			break;
		case '>':
			memcpy(cOut, "&gt;", 4);
			cOut += 4;
			break;
		case '\n':
			memcpy(cOut, "&nbsp;", 6);
			cOut += 6;
			break;
		default:
			if ((*cIn > 0x00 && *cIn < 0x20) || *cIn == 0x7f)
			{
				*cOut = ' ';
			}
			else
			{
				*cOut = *cIn;
			}
			cOut++;
			break;
		}
		cIn++;
	}
	*cOut = '\0';
	free(mbs);
	return cOut - buffer;
}

static INT NWL_NodeToJson(PNODE node, FILE* file)
{
	int i = 0;
	int nodes = 1;
	int atts = NWL_NodeAttrCount(node);
	int children = NWL_NodeChildCount(node);
	int plural = 0;
	int indent = indent_depth;

	// Print header
	fprintcx(file, NODE_JS_DELIM_INDENT, indent);
	if (indent_depth > 0 && (node->flags & NFLG_TABLE_ROW) == 0)
		fprintf(file, "\"%s\": ", node->name);

	if ((node->flags & NFLG_TABLE) == 0)
		fputs("{", file);
	else
		fputs("[", file);

	// Print attributes
	if (atts > 0 && (node->flags & NFLG_TABLE) == 0)
	{
		for (i = 0; i < atts; i++)
		{
			PNODE_ATT att = NWL_NodeAttrEnum(node, i);
			if (!att)
				continue;

			if (att->value && *att->value != '\0')
			{
				if (plural)
					fputs(",", file);

				// Print attribute name
				fputs(NODE_JS_DELIM_NL, file);
				fprintcx(file, NODE_JS_DELIM_INDENT, indent + 1);
				fprintf(file, "\"%s\": ", att->key);

				// Print value
				if (att->flags & NAFLG_ARRAY)
				{
					char* c;
					fputs("[ ", file);
					for (c = att->value; *c != '\0'; c += strlen(c) + 1)
					{
						if (c != att->value)
							fputs(", ", file);
						JsonEscapeContent(c, NWLC->NwBuf, NWINFO_BUFSZ);
						fprintf(file, "\"%s\"", NWLC->NwBuf);
					}
					fputs(" ]", file);
				}
				else if (att->flags & NAFLG_FMT_NUMERIC)
					fputs(att->value, file);
				else if (att->flags & NAFLG_FMT_BOOLEAN)
				{
					if (strcmp(att->value, NA_BOOL_TRUE) == 0)
						fputs(NODE_JS_BOOL_TRUE, file);
					else
						fputs(NODE_JS_BOOL_FALSE, file);
				}
				else
				{
					JsonEscapeContent(att->value, NWLC->NwBuf, NWINFO_BUFSZ);
					fprintf(file, "\"%s\"", NWLC->NwBuf);
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
			PNODE child = NWL_NodeEnumChild(node, i);
			if (!child)
				continue;

			if (plural)
				fputs(",", file);

			fputs(NODE_JS_DELIM_NL, file);
			nodes += NWL_NodeToJson(child, file);
			plural = 1;
		}
		indent_depth--;
	}

	if (atts > 0 || children > 0)
	{
		fputs(NODE_JS_DELIM_NL, file);
		fprintcx(file, NODE_JS_DELIM_INDENT, indent);
	}
	if ((node->flags & NFLG_TABLE) == 0)
		fputs("}", file);
	else
		fputs("]", file);
	return nodes;
}

static INT NWL_NodeToYaml(PNODE node, FILE* file)
{
	int i = 0;
	int count = 1;
	int atts = NWL_NodeAttrCount(node);
	int children = NWL_NodeChildCount(node);
	PNODE child = NULL;

	if (!node->parent)
		fputs("---"NODE_YAML_DELIM_NL, file);

	fprintcx(file, NODE_YAML_DELIM_INDENT, indent_depth);

	if (NFLG_TABLE_ROW & node->flags)
		fputs("- ", file);

	fprintf(file, "%s:", node->name);

	// Print attributes
	if (atts > 0)
	{
		fputs(NODE_YAML_DELIM_NL, file);
		for (i = 0; i < atts; i++)
		{
			PNODE_ATT att = NWL_NodeAttrEnum(node, i);
			if (!att)
				continue;

			fprintcx(file, NODE_YAML_DELIM_INDENT, indent_depth + 1);
			fprintf(file, "%s: ", att->key);
			if (att->flags & NAFLG_ARRAY)
			{
				char* c;
				fputs("[ ", file);
				for (c = att->value; *c != '\0'; c += strlen(c) + 1)
				{
					if (c != att->value)
						fputs(", ", file);
					YamlEscapeContent(c, NWLC->NwBuf, NWINFO_BUFSZ);
					fprintf(file, "\'%s\'", NWLC->NwBuf);
				}
				fputs(" ]", file);
			}
			else
			{
				CHAR* attVal = (att->value && *att->value != '\0') ? att->value : "~";
				if (att->flags & NAFLG_FMT_NUMERIC)
					fputs(attVal, file);
				else if (att->flags & NAFLG_FMT_BOOLEAN)
				{
					if (strcmp(attVal, NA_BOOL_TRUE) == 0)
						fputs(NODE_YAML_BOOL_TRUE, file);
					else
						fputs(NODE_YAML_BOOL_FALSE, file);
				}
				else
				{
					YamlEscapeContent(attVal, NWLC->NwBuf, NWINFO_BUFSZ);
					fprintf(file, "\'%s\'", NWLC->NwBuf);
				}
			}
			fputs(NODE_YAML_DELIM_NL, file);
		}
	}

	// Print children
	if (children > 0)
	{
		if (atts == 0)
			fputs(NODE_YAML_DELIM_NL, file);
		indent_depth++;
		for (i = 0; i < children; i++)
		{
			child = NWL_NodeEnumChild(node, i);
			if (!child)
				continue;
			NWL_NodeToYaml(child, file);
		}
		indent_depth--;
	}
	else if (atts == 0)
	{
		fputs(" ~"NODE_YAML_DELIM_NL, file);
	}

	return count;
}

static INT NWL_NodeToLua(PNODE node, FILE* file)
{
	int i = 0;
	int nodes = 1;
	int atts = NWL_NodeAttrCount(node);
	int children = NWL_NodeChildCount(node);
	int plural = 0;
	int indent = indent_depth;

	if (!node->parent)
		fputs("_NWINFO = ", file);

	// Print header
	fprintcx(file, NODE_LUA_DELIM_INDENT, indent);
	if (indent_depth > 0 && (node->flags & NFLG_TABLE_ROW) == 0)
		fprintf(file, "[\"%s\"] = ", node->name);

	//if ((node->flags & NFLG_TABLE) == 0)
	fputs("{", file);

	// Print attributes
	if (atts > 0 && (node->flags & NFLG_TABLE) == 0)
	{
		for (i = 0; i < atts; i++)
		{
			PNODE_ATT att = NWL_NodeAttrEnum(node, i);
			if (!att)
				continue;
			if (att->value && *att->value != '\0')
			{
				if (plural)
					fputs(",", file);

				// Print attribute name
				fputs(NODE_LUA_DELIM_NL, file);
				fprintcx(file, NODE_LUA_DELIM_INDENT, indent + 1);
				fprintf(file, "[\"%s\"] = ", att->key);

				// Print value
				if (att->flags & NAFLG_ARRAY)
				{
					char* c;
					fputs("{ ", file);
					for (c = att->value; *c != '\0'; c += strlen(c) + 1)
					{
						if (c != att->value)
							fputs(", ", file);
						LuaEscapeContent(c, NWLC->NwBuf, NWINFO_BUFSZ);
						fprintf(file, "\"%s\"", NWLC->NwBuf);
					}
					fputs(" }", file);
				}
				else if (att->flags & NAFLG_FMT_BOOLEAN)
				{
					if (strcmp(att->value, NA_BOOL_TRUE) == 0)
						fputs(NODE_LUA_BOOL_TRUE, file);
					else
						fputs(NODE_LUA_BOOL_FALSE, file);
				}
				else
				{
					LuaEscapeContent(att->value, NWLC->NwBuf, NWINFO_BUFSZ);
					fprintf(file, "\"%s\"", NWLC->NwBuf);
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
			PNODE child = NWL_NodeEnumChild(node, i);
			if (!child)
				continue;

			if (plural)
				fputs(",", file);

			fputs(NODE_LUA_DELIM_NL, file);
			nodes += NWL_NodeToLua(child, file);
			plural = 1;
		}
		indent_depth--;
	}

	if (atts > 0 || children > 0)
	{
		fputs(NODE_LUA_DELIM_NL, file);
		fprintcx(file, NODE_LUA_DELIM_INDENT, indent);
	}
	//if ((node->flags & NFLG_TABLE) == 0)
	fputs("}", file);
	return nodes;
}

static int NWL_NodeToTree(PNODE node, FILE* file)
{
	int i = 0;
	int count = 1;
	int atts = NWL_NodeAttrCount(node);
	int children = NWL_NodeChildCount(node);

	// Print node name with indentation
	fprintcx(file, NODE_TREE_DELIM_INDENT, indent_depth);
	if (node->parent)
		fputs(NODE_TREE_BRANCH, file);
	fprintf(file, "%s", node->name);
	fputs(NODE_TREE_DELIM_NL, file);

	// Increase indent for attributes and children
	indent_depth++;

	// Print attributes
	if (atts > 0)
	{
		for (i = 0; i < atts; i++)
		{
			PNODE_ATT att = NWL_NodeAttrEnum(node, i);
			if (!att)
				continue;

			// Skip attributes with no value
			if (!att->value || *att->value == '\0')
				continue;

			fprintcx(file, NODE_TREE_DELIM_INDENT, indent_depth);
			fputs(NODE_TREE_LEAF, file);
			fprintf(file, "%s: ", att->key);

			// Print value
			if (att->flags & NAFLG_ARRAY)
			{
				char* c;
				int first = 1;
				for (c = att->value; *c != '\0'; c += strlen(c) + 1)
				{
					if (!first)
						fputs(", ", file);
					TreeEscapeContent(c, NWLC->NwBuf, NWINFO_BUFSZ);
					fprintf(file, "%s", NWLC->NwBuf);
					first = 0;
				}
			}
			else if (att->flags & NAFLG_FMT_BOOLEAN)
			{
				if (strcmp(att->value, NA_BOOL_TRUE) == 0)
					fputs(NODE_TREE_BOOL_TRUE, file);
				else
					fputs(NODE_TREE_BOOL_FALSE, file);
			}
			else
			{
				TreeEscapeContent(att->value, NWLC->NwBuf, NWINFO_BUFSZ);
				fprintf(file, "%s", NWLC->NwBuf);
			}
			fputs(NODE_TREE_DELIM_NL, file);
		}
	}

	// Print children recursively
	if (children > 0)
	{
		fprintcx(file, NODE_TREE_DELIM_INDENT, indent_depth);
		fputs(NODE_TREE_SUB NODE_TREE_DELIM_NL, file);
		for (i = 0; i < children; i++)
		{
			PNODE child = NWL_NodeEnumChild(node, i);
			if (!child)
				continue;
			count += NWL_NodeToTree(child, file);
		}
	}

	// Decrease indent after processing this node and its children
	indent_depth--;

	return count;
}

static int NWL_NodeToHtml(PNODE node, FILE* file)
{
	int i = 0;
	int count = 1;
	int atts = NWL_NodeAttrCount(node);
	int children = NWL_NodeChildCount(node);

	// Print HTML header for the root node
	if (!node->parent)
	{
		fputs("<!DOCTYPE html>\n"
			"<html>\n<head>\n"
			"<meta charset=\"UTF-8\">\n"
			"<title>NWinfo Report</title>\n"
			"<style>\n"
			"body { font-family: sans-serif; background-color: #f4f4f4; color: #333; }\n"
			"details { border: 1px solid #ccc; border-radius: 4px; margin-bottom: 5px; margin-left: 2em; overflow: hidden; }\n"
			"details[open] { background-color: #fff; }\n"
			"summary { font-weight: bold; cursor: pointer; padding: 8px; background-color: #e9e9e9; }\n"
			"summary:hover { background-color: #ddd; }\n"
			"ul { list-style-type: none; padding: 0px 10px; margin: 0; }\n"
			"li { padding: 5px 0; border-bottom: 1px solid #eee; }\n"
			"li:last-child { border-bottom: none; }\n"
			".key { color: #005a9c; font-weight: bold; }\n"
			".value { color: #333; }\n"
			".attr-list { padding: 10px; }\n"
			"</style>\n"
			"</head>\n<body>\n", file);
	}

	fprintcx(file, "  ", indent_depth);
	// Use <details> for collapsible sections. Open top-level nodes by default.
	fprintf(file, "<details %s>\n", (indent_depth < 2) ? "open" : "");

	// Node name in <summary>
	fprintcx(file, "  ", indent_depth + 1);
	fputs("<summary>", file);
	HtmlEscapeContent(node->name, NWLC->NwBuf, NWINFO_BUFSZ);
	fprintf(file, "%s</summary>\n", NWLC->NwBuf);

	// Increase indent for attributes and children
	indent_depth++;

	// Print attributes in an unordered list
	if (atts > 0)
	{
		fprintcx(file, "  ", indent_depth);
		fputs("<div class=\"attr-list\">\n", file);
		fprintcx(file, "  ", indent_depth);
		fputs("<ul>\n", file);
		for (i = 0; i < atts; i++)
		{
			PNODE_ATT att = NWL_NodeAttrEnum(node, i);
			if (!att)
				continue;

			if (!att->value || *att->value == '\0')
				continue;

			fprintcx(file, "  ", indent_depth + 1);
			fputs("<li>", file);
			// Key
			HtmlEscapeContent(att->key, NWLC->NwBuf, NWINFO_BUFSZ);
			fprintf(file, "<span class=\"key\">%s:</span> ", NWLC->NwBuf);
			// Value
			fputs("<span class=\"value\">", file);
			if (att->flags & NAFLG_ARRAY)
			{
				char* c;
				int first = 1;
				for (c = att->value; *c != '\0'; c += strlen(c) + 1)
				{
					if (!first)
						fputs(", ", file);
					HtmlEscapeContent(c, NWLC->NwBuf, NWINFO_BUFSZ);
					fprintf(file, "%s", NWLC->NwBuf);
					first = 0;
				}
			}
			else if (att->flags & NAFLG_FMT_BOOLEAN)
			{
				if (strcmp(att->value, NA_BOOL_TRUE) == 0)
					fputs(NODE_TREE_BOOL_TRUE, file); // "Yes"
				else
					fputs(NODE_TREE_BOOL_FALSE, file); // "No"
			}
			else
			{
				HtmlEscapeContent(att->value, NWLC->NwBuf, NWINFO_BUFSZ);
				fprintf(file, "%s", NWLC->NwBuf);
			}
			fputs("</span></li>\n", file);
		}
		fprintcx(file, "  ", indent_depth);
		fputs("</ul>\n", file);
		fprintcx(file, "  ", indent_depth);
		fputs("</div>\n", file);
	}

	// Print children recursively
	if (children > 0)
	{
		for (i = 0; i < children; i++)
		{
			PNODE child = NWL_NodeEnumChild(node, i);
			if (!child)
				continue;
			count += NWL_NodeToHtml(child, file);
		}
	}

	// Decrease indent after processing this node and its children
	indent_depth--;

	fprintcx(file, "  ", indent_depth);
	fputs("</details>\n", file);

	// Print HTML footer for the root node
	if (!node->parent)
	{
		fputs("</body>\n</html>\n", file);
	}

	return count;
}

VOID NW_Export(PNODE node, FILE* file)
{
	indent_depth = 0;
	switch (NWLC->NwFormat)
	{
	case FORMAT_YAML:
		NWL_NodeToYaml(node, file);
		break;
	case FORMAT_JSON:
		NWL_NodeToJson(node, file);
		break;
	case FORMAT_LUA:
		NWL_NodeToLua(node, file);
		break;
	case FORMAT_TREE:
		NWL_NodeToTree(node, file);
		break;
	case FORMAT_HTML:
		NWL_NodeToHtml(node, file);
		break;
	}
}
