// SPDX-License-Identifier: Unlicense

#include "gnwinfo.h"
#include "gettext.h"

#define INVALID_N_ID u8"!!INVALID ID - FIX ME!!"

#include "lang/en_US.h"
#include "lang/fr_FR.h"
#include "lang/it_IT.h"
#include "lang/ja_JP.h"
#include "lang/ko_KR.h"
#include "lang/pl_PL.h"
#include "lang/pt_BR.h"
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
	1036, // French - France
	1040, // Italian
	1041, // Japanese
	1042, // Korean
	1045, // Polish
	1046, // Portuguese - Brazil
	1053, // Swedish
	1055, // Turkish
	1060, // Slovenian
	3076, // Chinese - Hong Kong SAR
};
const size_t g_supported_lang_num = ARRAYSIZE(g_supported_lang_ids);

const char*
gnwinfo_get_lang_str(LANGID lang, GETTEXT_STR_ID id)
{
	if (id < 0 || id >= N__MAX_)
		return INVALID_N_ID;
	const char* str = NULL;
	switch (lang)
	{
	case 2052: // Chinese - People's Republic of China
	case 4100: // Chinese - Singapore
		str = lang_zh_cn[id];
		break;
	case 1028: // Chinese - Taiwan
		str = lang_zh_tw[id];
		break;
	case 1033: // English - United States
	case 2057: // English - United Kingdom
	case 3081: // English - Australia
	case 4105: // English - Canada
	case 5129: // English - New Zealand
	case 6153: // English - Ireland
	case 18441:// English - Singapore
		str = lang_en_us[id];
		break;
	case 1036: // French - France
	case 2060: // French - Belgium
	case 3084: // French - Canada
	case 4108: // French - Switzerland
		str = lang_fr_fr[id];
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
	case 1046: // Portuguese - Brazil
	case 2070: // Portuguese - Portugal
		str = lang_pt_br[id];
		break;
	case 1053: // Swedish
		str = lang_sv_se[id];
		break;
	case 1055: // Turkish
		str = lang_tr_tr[id];
		break;
	case 1060: // Slovenian
		str = lang_sl_si[id];
		break;
	case 3076: // Chinese - Hong Kong SAR
	case 5124: // Chinese - Macao SAR
		str = lang_zh_hk[id];
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
