// SPDX-License-Identifier: Unlicense

#include "gnwinfo.h"
#include "gettext.h"

#define INVALID_N_ID u8"!!INVALID ID - FIX ME!!"

#include "lang/ar_AA.h"
#include "lang/de_DE.h"
#include "lang/el_GR.h"
#include "lang/en_US.h"
#include "lang/es_ES.h"
#include "lang/fr_FR.h"
#include "lang/it_IT.h"
#include "lang/ja_JP.h"
#include "lang/ko_KR.h"
#include "lang/pl_PL.h"
#include "lang/pt_BR.h"
#include "lang/ru_RU.h"
#include "lang/sl_SI.h"
#include "lang/sv_SE.h"
#include "lang/tr_TR.h"
#include "lang/zh_CN.h"
#include "lang/zh_HK.h"
#include "lang/zh_TW.h"

// https://learn.microsoft.com/en-us/openspecs/office_standards/ms-oe376/6c085406-a698-4e12-9d4d-c3b0ee3dbc4a
LANGID g_lang_id;

LANGID g_supported_lang_ids[] =
{
	1033, // English - United States
	2052, // Chinese - People's Republic of China
	1028, // Chinese - Taiwan
	1031, // German - Germany
	1032, // Greek
	1034, // Spanish - Spain (Traditional Sort)
	1036, // French - France
	1040, // Italian
	1041, // Japanese
	1042, // Korean
	1045, // Polish
	1046, // Portuguese - Brazil
	1049, // Russian
	1053, // Swedish
	1055, // Turkish
	1060, // Slovenian
	3076, // Chinese - Hong Kong SAR
	1025, // Arabic - Saudi Arabia
};
const size_t g_supported_lang_num = ARRAYSIZE(g_supported_lang_ids);

const char*
gnwinfo_get_lang_str(LANGID lang, GETTEXT_STR_ID id)
{
	if (id < 0 || id >= N__MAX_)
		return INVALID_N_ID;
	const char* str = NULL;

	switch (PRIMARYLANGID(lang))
	{
	case LANG_ARABIC:
		str = lang_ar_aa[id];
		break;
	case LANG_CHINESE:
		switch (SUBLANGID(lang))
		{
		case SUBLANG_CHINESE_TRADITIONAL:
			str = lang_zh_tw[id];
			break;
		case SUBLANG_CHINESE_HONGKONG:
		case SUBLANG_CHINESE_MACAU:
			str = lang_zh_hk[id];
			break;
		case SUBLANG_CHINESE_SIMPLIFIED:
		case SUBLANG_CHINESE_SINGAPORE:
		default:
			str = lang_zh_cn[id];
			break;
		}
		break;
	case LANG_ENGLISH:
		str = lang_en_us[id];
		break;
	case LANG_FRENCH:
		str = lang_fr_fr[id];
		break;
	case LANG_GERMAN:
		str = lang_de_de[id];
		break;
	case LANG_GREEK:
		str = lang_el_gr[id];
		break;
	case LANG_ITALIAN:
		str = lang_it_it[id];
		break;
	case LANG_JAPANESE:
		str = lang_ja_jp[id];
		break;
	case LANG_KOREAN:
		str = lang_ko_kr[id];
		break;
	case LANG_POLISH:
		str = lang_pl_pl[id];
		break;
	case LANG_PORTUGUESE:
		str = lang_pt_br[id];
		break;
	case LANG_RUSSIAN:
		str = lang_ru_ru[id];
		break;
	case LANG_SLOVENIAN:
		str = lang_sl_si[id];
		break;
	case LANG_SPANISH:
		str = lang_es_es[id];
		break;
	case LANG_SWEDISH:
		str = lang_sv_se[id];
		break;
	case LANG_TURKISH:
		str = lang_tr_tr[id];
		break;
	}

	if (str == NULL || str[0] == '\0')
		str = lang_en_us[id];
	return str;
}

const char*
N_(GETTEXT_STR_ID id)
{
	if (g_lang_id == 0)
		g_lang_id = GetUserDefaultUILanguage();
	return gnwinfo_get_lang_str(g_lang_id, id);
}
