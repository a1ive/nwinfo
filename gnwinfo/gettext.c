// SPDX-License-Identifier: Unlicense

#include "gnwinfo.h"
#include "gettext.h"

#define INVALID_N_ID u8"!!INVALID ID - FIX ME!!"

#include "lang/en_US.h"
#include "lang/it_IT.h"
#include "lang/ja_JP.h"
#include "lang/ko_KR.h"
#include "lang/pl_PL.h"
#include "lang/sv_SE.h"
#include "lang/tr_TR.h"
#include "lang/zh_CN.h"
#include "lang/zh_TW.h"

// https://learn.microsoft.com/en-us/openspecs/office_standards/ms-oe376/6c085406-a698-4e12-9d4d-c3b0ee3dbc4a
LANGID g_lang_id;

const char*
N_(GETTEXT_STR_ID id)
{
	if (g_lang_id == 0)
		g_lang_id = GetUserDefaultUILanguage();
	if (id >= N__MAX_)
		return INVALID_N_ID;

	const char* str = NULL;
	switch (g_lang_id)
	{
	case 2052: // Chinese - People's Republic of China
		str = lang_zh_cn[id];
		break;
	case 1028: // Chinese - Taiwan
		str = lang_zh_tw[id];
		break;
	case 1040: // Italian
		str = lang_it_it[id];
		break;
	case 1041: // Japanese
		str = lang_ja_jp[id];
		break;
	case 1042: // Korean
		str = lang_ko_kr[id];
		break;
	case 1045: // Polish
		str = lang_pl_pl[id];
		break;
	case 1053: // Swedish
		str = lang_sv_se[id];
		break;
	case 1055: // Turkish
		str = lang_tr_tr[id];
		break;
	}
	// 1033: English - United States
	if (str == NULL || str[0] == '\0')
		str = lang_en_us[id];
	return str;
}
