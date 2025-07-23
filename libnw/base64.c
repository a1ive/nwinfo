// SPDX-License-Identifier: Unlicense

#include <stdlib.h>
#include "base64.h"

static char Base64Table[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"abcdefghijklmnopqrstuvwxyz"
	"0123456789+/";

char*
NWL_Base64Encode(uint8_t* pSrc, size_t szSrc)
{
	size_t szDest;
	size_t szLeft;
	char* pDest;
	char* p;

	if (pSrc == NULL)
		return NULL;

	if (szSrc == 0)
		szDest = 1;
	else
		szDest = ((szSrc + 2) / 3) * 4 + 1;

	pDest = (char*)malloc(szDest);
	if (pDest == NULL)
		return NULL;
	p = pDest;

	// Special case
	if (szSrc == 0)
	{
		*p = '\0';
		return pDest;
	}

	szLeft = szSrc;

	// Encode 24 bits (three bytes) into 4 ascii characters
	while (szLeft >= 3)
	{
		*p++ = Base64Table[(pSrc[0] & 0xfc) >> 2];
		*p++ = Base64Table[((pSrc[0] & 0x03) << 4) + ((pSrc[1] & 0xf0) >> 4)];
		*p++ = Base64Table[((pSrc[1] & 0x0f) << 2) + ((pSrc[2] & 0xc0) >> 6)];
		*p++ = Base64Table[(pSrc[2] & 0x3f)];
		szLeft -= 3;
		pSrc += 3;
	}

	// Handle the remainder, and add padding '=' characters as necessary.
	switch (szLeft)
	{
	case 0:
		break;
	case 1:
		*p++ = Base64Table[(pSrc[0] & 0xfc) >> 2];
		*p++ = Base64Table[((pSrc[0] & 0x03) << 4)];
		*p++ = '=';
		*p++ = '=';
		break;
	case 2:
		*p++ = Base64Table[(pSrc[0] & 0xfc) >> 2];
		*p++ = Base64Table[((pSrc[0] & 0x03) << 4) + ((pSrc[1] & 0xf0) >> 4)];
		*p++ = Base64Table[((pSrc[1] & 0x0f) << 2)];
		*p++ = '=';
		break;
	}

	*p = '\0';
	return pDest;
}
