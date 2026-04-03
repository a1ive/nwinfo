// SPDX-License-Identifier: Unlicense
#pragma once

#include <stdint.h>

// https://en.wikipedia.org/wiki/List_of_Intel_chipsets
// https://en.wikipedia.org/wiki/List_of_AMD_chipsets
// https://en.wikipedia.org/wiki/Comparison_of_Nvidia_nForce_chipsets
// https://github.com/LibreHardwareMonitor/LibreHardwareMonitor/blob/master/LibreHardwareMonitorLib/Hardware/Motherboard/Identification.cs
// https://github.com/lzhoang2801/Hardware-Sniffer/blob/main/Scripts/datasets/chipset_data.py

struct CHIPSET_ID_ENTRY
{
	uint16_t id;
	const char* name;
};

#ifdef CHIPSET_IDS_IMPL

static const struct CHIPSET_ID_ENTRY INTEL_ISA_LIST[] =
{
	{ 0x0284u, "Comet Lake-LP" }, // 400 series
	{ 0x0285u, "Comet Lake" }, // 400 series
	{ 0x0684u, "H470" }, // 400 series
	{ 0x0685u, "Z490" }, // 400 series
	{ 0x0687u, "Q470" }, // 400 series
	{ 0x068Cu, "QM480" }, // 400 series
	{ 0x068Du, "HM470" }, // 400 series
	{ 0x068Eu, "WM490" }, // 400 series
	{ 0x0697u, "W480" }, // 400 series
	{ 0x069Au, "H420E" },
	{ 0x19DCu, "Atom C3000" },
	{ 0x1C40u, "Cougar Point" },
	{ 0x1C41u, "Cougar Point Mobile SFF" },
	{ 0x1C42u, "Cougar Point" },
	{ 0x1C43u, "Cougar Point Mobile" },
	{ 0x1C44u, "Z68" }, // 6 / C200
	{ 0x1C45u, "Cougar Point" },
	{ 0x1C46u, "P67" }, // 6 / C200
	{ 0x1C47u, "UM67" }, // 6 / C200
	{ 0x1C48u, "Cougar Point" },
	{ 0x1C49u, "HM65" }, // 6 / C200
	{ 0x1C4Au, "H67" }, // 6 / C200
	{ 0x1C4Bu, "HM67" }, // 6 / C200
	{ 0x1C4Cu, "Q65" }, // 6 / C200
	{ 0x1C4Du, "QS67" }, // 6 / C200
	{ 0x1C4Eu, "Q67" }, // 6 / C200
	{ 0x1C4Fu, "QM67" }, // 6 / C200
	{ 0x1C50u, "B65" }, // 6 / C200
	{ 0x1C51u, "Cougar Point" },
	{ 0x1C52u, "C202" }, // 6 / C200
	{ 0x1C53u, "Cougar Point" },
	{ 0x1C54u, "C204" }, // 6 / C200
	{ 0x1C55u, "Cougar Point" },
	{ 0x1C56u, "C206" }, // 6 / C200
	{ 0x1C57u, "Cougar Point" },
	{ 0x1C58u, "B65" },
	{ 0x1C59u, "HM67" },
	{ 0x1C5Au, "Q67" },
	{ 0x1C5Bu, "Cougar Point" },
	{ 0x1C5Cu, "H61" },
	{ 0x1C5Du, "Cougar Point" },
	{ 0x1C5Eu, "Cougar Point" },
	{ 0x1C5Fu, "Cougar Point" },
	{ 0x1D40u, "C600/X79" },
	{ 0x1D41u, "C600/X79" },
	{ 0x1E41u, "Panther Point" },
	{ 0x1E42u, "Panther Point" },
	{ 0x1E43u, "Panther Point" },
	{ 0x1E44u, "Z77" },
	{ 0x1E45u, "Panther Point" },
	{ 0x1E46u, "Z75" },
	{ 0x1E47u, "Q77" },
	{ 0x1E48u, "Q75" },
	{ 0x1E49u, "B75" },
	{ 0x1E4Au, "H77" },
	{ 0x1E4Bu, "Panther Point" },
	{ 0x1E4Cu, "Panther Point" },
	{ 0x1E4Du, "Panther Point" },
	{ 0x1E4Eu, "Panther Point" },
	{ 0x1E4Fu, "Panther Point" },
	{ 0x1E50u, "Panther Point" },
	{ 0x1E51u, "Panther Point" },
	{ 0x1E52u, "Panther Point" },
	{ 0x1E53u, "C216" },
	{ 0x1E54u, "Panther Point" },
	{ 0x1E55u, "QM77" },
	{ 0x1E56u, "QS77" },
	{ 0x1E57u, "HM77" },
	{ 0x1E58u, "UM77" },
	{ 0x1E59u, "HM76" },
	{ 0x1E5Au, "Panther Point" },
	{ 0x1E5Bu, "UM77" },
	{ 0x1E5Cu, "Panther Point" },
	{ 0x1E5Du, "HM75" },
	{ 0x1E5Eu, "HM70" },
	{ 0x1E5Fu, "NM70" },
	{ 0x2310u, "DH89xxCC" },
	{ 0x2390u, "DH895XCC" },
	{ 0x2410u, "82801AA" },
	{ 0x2420u, "82801AB" },
	{ 0x2440u, "82801BA" },
	{ 0x244Cu, "82801BAM" },
	{ 0x2450u, "82801E" },
	{ 0x2480u, "82801CA" },
	{ 0x248Cu, "82801CAM" },
	{ 0x24C0u, "82801DB/DBL (ICH4/ICH4-L)" },
	{ 0x24CCu, "82801DBM (ICH4-M)" },
	{ 0x24D0u, "82801EB/ER (ICH5/ICH5R)" },
	{ 0x24DCu, "82801EB (ICH5)" },
	{ 0x25A1u, "6300ESB" },
	{ 0x2640u, "82801FB/FR (ICH6/ICH6R)" },
	{ 0x2641u, "82801FBM (ICH6M)" },
	{ 0x2642u, "82801FW/FRW (ICH6W/ICH6RW)" },
	{ 0x2670u, "631xESB/632xESB/3100" },
	{ 0x27B0u, "82801GH (ICH7DH)" },
	{ 0x27B8u, "82801GB/GR (ICH7)" },
	{ 0x27B9u, "82801GBM (ICH7-M)" },
	{ 0x27BCu, "NM10" },
	{ 0x27BDu, "82801GHM (ICH7-M DH)" },
	{ 0x2810u, "82801HB/HR (ICH8/R)" },
	{ 0x2811u, "82801HEM (ICH8M-E)" },
	{ 0x2812u, "82801HH (ICH8DH)" },
	{ 0x2814u, "82801HO (ICH8DO)" },
	{ 0x2815u, "82801HM (ICH8M)" },
	{ 0x2912u, "82801IH (ICH9DH)" },
	{ 0x2914u, "82801IO (ICH9DO)" },
	{ 0x2916u, "82801IR (ICH9R)" },
	{ 0x2917u, "82801IM (ICH9M-E)" },
	{ 0x2918u, "82801IB (ICH9)" },
	{ 0x2919u, "82801IM (ICH9M)" },
	{ 0x3197u, "Gemini Lake" },
	{ 0x31E8u, "Gemini Lake" },
	{ 0x3482u, "Ice Lake-LP" },
	{ 0x3882u, "Ice Lake" },
	{ 0x3A14u, "82801JDO (ICH10DO)" },
	{ 0x3A16u, "82801JIR (ICH10R)" },
	{ 0x3A18u, "82801JIB (ICH10)" },
	{ 0x3A1Au, "82801JD (ICH10D)" },
	{ 0x3B00u, "5/3400" },
	{ 0x3B01u, "5/ Mobile" },
	{ 0x3B02u, "P55" },
	{ 0x3B03u, "PM55" },
	{ 0x3B06u, "H55" },
	{ 0x3B07u, "QM57" },
	{ 0x3B08u, "H57" },
	{ 0x3B09u, "HM55" },
	{ 0x3B0Au, "Q57" },
	{ 0x3B0Bu, "HM57" },
	{ 0x3B0Fu, "QS57" },
	{ 0x3B10u, "5/3400" },
	{ 0x3B11u, "5/3400" },
	{ 0x3B12u, "3400" },
	{ 0x3B13u, "5/3400" },
	{ 0x3B14u, "3420" },
	{ 0x3B15u, "5/3400" },
	{ 0x3B16u, "3450" },
	{ 0x3B17u, "5/3400" },
	{ 0x3B18u, "5/3400" },
	{ 0x3B19u, "5/3400" },
	{ 0x3B1Au, "5/3400" },
	{ 0x3B1Bu, "5/3400" },
	{ 0x3B1Cu, "5/3400" },
	{ 0x3B1Du, "5/3400" },
	{ 0x3B1Eu, "5/3400" },
	{ 0x3B1Fu, "5/3400" },
	{ 0x4381u, "Rocket Lake-S" }, // 500 series
	{ 0x4382u, "Rocket Lake" }, // 500 series Mobile
	{ 0x4383u, "Rocket Lake" }, // 500 series Server
	{ 0x4384u, "Q570" }, // 500 series
	{ 0x4385u, "Z590" }, // 500 series
	{ 0x4386u, "H570" }, // 500 series
	{ 0x4387u, "B560" }, // 500 series
	{ 0x4388u, "H510" }, // 500 series
	{ 0x4389u, "WM590" }, // 500 series
	{ 0x438Au, "QM580" }, // 500 series
	{ 0x438Bu, "HM570" }, // 500 series
	{ 0x438Cu, "C252" }, // 500 series
	{ 0x438Du, "C256" }, // 500 series
	{ 0x438Eu, "H310D" },
	{ 0x438Fu, "W580" }, // 500 series
	{ 0x4390u, "RM590E" },
	{ 0x4391u, "R580E" },
	{ 0x4B00u, "Elkhart Lake" },
	{ 0x4D87u, "Jasper Lake" },
	{ 0x5031u, "EP80579" },
	{ 0x5181u, "Alder Lake" },
	{ 0x5182u, "Alder Lake" },
	{ 0x5187u, "Alder Lake" },
	{ 0x519Du, "Raptor Lake-P" },
	{ 0x519Eu, "Raptor Lake-PX" },
	{ 0x51A4u, "Alder Lake-P" },
	{ 0x51AAu, "Alder Lake" },
	{ 0x51ABu, "Alder Lake" },
	{ 0x5481u, "Alder Lake-N" },
	{ 0x5795u, "Granite Rapids"},
	{ 0x7727u, "Arrow Lake-H" },
	{ 0x7730u, "Arrow Lake-H" },
	{ 0x7746u, "Arrow Lake-H" },
	{ 0x7A04u, "Z790" }, // 700 series
	{ 0x7A05u, "H770" }, // 700 series
	{ 0x7A06u, "B760" }, // 700 series
	{ 0x7A0Cu, "HM770" }, // 700 series
	{ 0x7A0Du, "WM790" }, // 700 series
	{ 0x7A13u, "C266" }, // 700 series
	{ 0x7A14u, "C262" }, // 700 series
	{ 0x7A83u, "Q670" }, // 600 series
	{ 0x7A84u, "Z690" }, // 600 series
	{ 0x7A85u, "H670" }, // 600 series
	{ 0x7A86u, "B660" }, // 600 series
	{ 0x7A87u, "H610" }, // 600 series
	{ 0x7A88u, "W680" }, // 600 series
	{ 0x7A8Au, "W790" }, // 700 series
	{ 0x7A8Cu, "HM670" }, // 600 series
	{ 0x7A8Du, "WM690" }, // 600 series
	{ 0x7A90u, "R680E" },
	{ 0x7A91u, "Q670E" },
	{ 0x7A92u, "H610E" },
	{ 0x7E01u, "Meteor Lake-P" },
	{ 0x7F03u, "Q870" }, // 800 series
	{ 0x7F04u, "Z890" }, // 800 series
	{ 0x7F06u, "B860" }, // 800 series
	{ 0x7F07u, "H810" }, // 800 series
	{ 0x7F08u, "W880" }, // 800 series
	{ 0x7F0Cu, "HM870" }, // 800 series
	{ 0x7F0Du, "WM880" }, // 800 series
	{ 0x8186u, "Atom E6xx" },
	{ 0x8C40u, "8/C220" },
	{ 0x8C41u, "8/C220 Mobile" },
	{ 0x8C42u, "8/C220 Desktop" },
	{ 0x8C43u, "8/C220" },
	{ 0x8C44u, "Z87" },
	{ 0x8C45u, "8/C220" },
	{ 0x8C46u, "Z85" },
	{ 0x8C47u, "8/C220" },
	{ 0x8C48u, "8/C220" },
	{ 0x8C49u, "HM86" },
	{ 0x8C4Au, "H87" },
	{ 0x8C4Bu, "HM87" },
	{ 0x8C4Cu, "Q85" },
	{ 0x8C4Du, "8/C220" },
	{ 0x8C4Eu, "Q87" },
	{ 0x8C4Fu, "QM87" },
	{ 0x8C50u, "B85" },
	{ 0x8C51u, "8/C220" },
	{ 0x8C52u, "C222" },
	{ 0x8C53u, "8/C220" },
	{ 0x8C54u, "C224" },
	{ 0x8C55u, "8/C220" },
	{ 0x8C56u, "C226" },
	{ 0x8C57u, "8/C220" },
	{ 0x8C58u, "8/C220 WS" },
	{ 0x8C59u, "8/C220" },
	{ 0x8C5Au, "8/C220" },
	{ 0x8C5Bu, "8/C220" },
	{ 0x8C5Cu, "H81" },
	{ 0x8C5Du, "8/C220" },
	{ 0x8C5Eu, "8/C220" },
	{ 0x8C5Fu, "8/C220" },
	{ 0x8CC1u, "9/ Mobile" }, // 9 Series
	{ 0x8CC2u, "9/ Desktop" }, // 9 Series
	{ 0x8CC3u, "HM97" },
	{ 0x8CC4u, "Z97" },
	{ 0x8CC5u, "QM97" },
	{ 0x8CC6u, "H97" },
	{ 0x8D40u, "C610/X99" },
	{ 0x8D41u, "C610/X99" },
	{ 0x8D42u, "C610/X99" },
	{ 0x8D43u, "C610/X99" },
	{ 0x8D44u, "C610/X99" },
	{ 0x8D45u, "C610/X99" },
	{ 0x8D46u, "C610/X99" },
	{ 0x8D47u, "C610/X99" },
	{ 0x8D48u, "C610/X99" },
	{ 0x8D49u, "C610/X99" },
	{ 0x8D4Au, "C610/X99" },
	{ 0x8D4Bu, "C610/X99" },
	{ 0x8D4Cu, "C610/X99" },
	{ 0x8D4Du, "C610/X99" },
	{ 0x8D4Eu, "C610/X99" },
	{ 0x8D4Fu, "C610/X99" },
	{ 0x9C40u, "Lynx Point" }, // 8 Series
	{ 0x9C41u, "Lynx Point" },
	{ 0x9C42u, "Lynx Point" },
	{ 0x9C43u, "Lynx Point" },
	{ 0x9C44u, "Lynx Point" },
	{ 0x9C45u, "Lynx Point" },
	{ 0x9C46u, "Lynx Point" },
	{ 0x9C47u, "Lynx Point" },
	{ 0x9CC1u, "Wildcat Point-LP" },
	{ 0x9CC2u, "Wildcat Point-LP" },
	{ 0x9CC3u, "Wildcat Point-LP" },
	{ 0x9CC5u, "Wildcat Point-LP" },
	{ 0x9CC6u, "Wildcat Point-LP" },
	{ 0x9CC7u, "Wildcat Point-LP" },
	{ 0x9CC9u, "Wildcat Point-LP" },
	{ 0x9D43u, "Sunrise Point-LP" },
	{ 0x9D46u, "Sunrise Point-LP" },
	{ 0x9D48u, "Sunrise Point-LP" },
	{ 0x9D4Bu, "Sunrise Point-LP" },
	{ 0x9D4Eu, "Sunrise Point" },
	{ 0x9D50u, "Sunrise Point" },
	{ 0x9D56u, "Sunrise Point-LP" },
	{ 0x9D58u, "Sunrise Point-LP" },
	{ 0x9D4Eu, "Sunrise Point" },
	{ 0x9D84u, "Cannon Point-LP" }, // 300 series
	{ 0xA082u, "Tiger Lake-LP" }, // 500 series
	{ 0xA140u, "Sunrise Point-H" },
	{ 0xA141u, "Sunrise Point-H" },
	{ 0xA142u, "Sunrise Point-H" },
	{ 0xA143u, "H110" }, // 100 / C230
	{ 0xA144u, "H170" }, // 100 / C230
	{ 0xA145u, "Z170" }, // 100 / C230
	{ 0xA146u, "Q170" }, // 100 / C230
	{ 0xA147u, "Q150" }, // 100 / C230
	{ 0xA148u, "B150" }, // 100 / C230
	{ 0xA149u, "C236" }, // 100 / C230
	{ 0xA14Au, "C232" }, // 100 / C230
	{ 0xA14Bu, "Sunrise Point-H" },
	{ 0xA14Cu, "Sunrise Point-H" },
	{ 0xA14Du, "QM170" }, // 100 / C230
	{ 0xA14Eu, "HM170" }, // 100 / C230
	{ 0xA14Fu, "Sunrise Point-H" },
	{ 0xA150u, "CM236" }, // 100 / C230
	{ 0xA151u, "QMS180" },
	{ 0xA152u, "HM175" }, // 100 / C230
	{ 0xA153u, "QM175" }, // 100 / C230
	{ 0xA154u, "CM238" }, // 100 / C230
	{ 0xA155u, "QMU185" },
	{ 0xA156u, "Sunrise Point-H" },
	{ 0xA157u, "Sunrise Point-H" },
	{ 0xA158u, "Sunrise Point-H" },
	{ 0xA159u, "Sunrise Point-H" },
	{ 0xA15Au, "Sunrise Point-H" },
	{ 0xA15Bu, "Sunrise Point-H" },
	{ 0xA15Cu, "Sunrise Point-H" },
	{ 0xA15Du, "Sunrise Point-H" },
	{ 0xA15Eu, "Sunrise Point-H" },
	{ 0xA15Fu, "Sunrise Point-H" },
	{ 0xA1C1u, "C621" },
	{ 0xA1C2u, "C622" },
	{ 0xA1C3u, "C624" },
	{ 0xA1C4u, "C625" },
	{ 0xA1C5u, "C626" },
	{ 0xA1C6u, "C627" },
	{ 0xA1C7u, "C628" },
	{ 0xA1C8u, "Lewisburg" },
	{ 0xA1CBu, "Lewisburg" },
	{ 0xA242u, "Lewisburg" },
	{ 0xA243u, "Lewisburg" },
	{ 0xA2C4u, "H270" }, // 200 / X299 / Z370
	{ 0xA2C5u, "Z270" }, // 200 / X299 / Z370
	{ 0xA2C6u, "Q270" }, // 200 / X299 / Z370
	{ 0xA2C7u, "Q250" }, // 200 / X299 / Z370
	{ 0xA2C8u, "B250" }, // 200 / X299 / Z370
	{ 0xA2C9u, "Z370" }, // 200 / X299 / Z370
	{ 0xA2CAu, "H310" },
	{ 0xA2CCu, "B365" },
	{ 0xA2D2u, "X299" }, // 200 / X299 / Z370
	{ 0xA2D3u, "C422" },
	{ 0xA303u, "H310" }, // 300 / C240
	{ 0xA304u, "H370" }, // 300 / C240
	{ 0xA305u, "Z390" }, // 300 / C240
	{ 0xA306u, "Q370" }, // 300 / C240
	{ 0xA308u, "B360" }, // 300 / C240
	{ 0xA309u, "C246" }, // 300 / C240
	{ 0xA30Au, "C242" }, // 300 / C240
	{ 0xA30Cu, "QM370" }, // 300 / C240
	{ 0xA30Du, "HM370" }, // 300 / C240
	{ 0xA30Eu, "CM246" }, // 300 / C240
	{ 0xA313u, "Cannon Lake" },
	{ 0xA3C8u, "B460" },
	{ 0xA3DAu, "H410" },
	{ 0xA806u, "Lunar Lake-M" },
	
};

static const char* INTEL_CHIPSET_LIST[] =
{
	// 900 Series
	"Z990",
	"W980",
	"Q970",
	"Z970",
	"B960",
	// 800 Series
	"Z890",
	"B860",
	"H810",
	"HM870",
	"WM880",
	// 600/700 Series
	"Z690",
	"W680",
	"Q670",
	"H670",
	"B660",
	"H610",
	"R680E",
	"Q670E",
	"H610E",
	"Z790",
	"H770",
	"B760",
	"HM670",
	"WM690",
	"HM770",
	"WM790",
	// 400/500 Series
	"H410",
	"B460",
	"H470",
	"Q470",
	"Z490",
	"W480",
	"H420E",
	"Q470E",
	"W480E",
	"H510",
	"B560",
	"H570",
	"Z590",
	"W580",
	// 100/200/300 Series
	"H110",
	"B150",
	"Q150",
	"H170",
	"Q170",
	"Z170",
	"B250",
	"Q250",
	"H270",
	"Q270",
	"Z270",
	"Z370",
	"H310",
	"B360",
	"B365",
	"H370",
	"Q370",
	"Z390",
	"C232",
	"C236",
	"C242",
	"C246",
	"HM170",
	"QM170",
	"CM236",
	"QMS180",
	"QMU185",
	"HM175",
	"QM175",
	"CM238",
	"HM370",
	"QM370",
	"CM246",
	// 5/6/7/8/9 Series
	"X299",
	"C422",
	"C621",
	"C622",
	"C624",
	"C625",
	"C626",
	"C627",
	"C628",
};

static const char* AMD_CHIPSET_LIST[] =
{
	// sTR5
	"TRX50"
	"WRX90",
	// sWRX8
	"WRX80",
	// sTRX4
	"TRX40",
	// TR4
	"X399",
	// AM5
	"A620A",
	"A620",
	"B650E",
	"B650",
	"X670E",
	"X670",
	"B840",
	"B850",
	"X870E",
	"X870",
	// AM4
	"A300",
	"X300",
	"PRO 500", // ?
	"A320",
	"B350",
	"X370",
	"B450",
	"X470",
	"A520",
	"B550",
	"X570",
	// AM1
	"A55T",
	"A50M",
	"A60M",
	"A68M",
	"A70M",
	"A76M",
	"A45",
	"A55",
	"A58",
	"A68H",
	"A75",
	"A78",
	"A85X",
	"A88X",
	"A55E",
	"A77E",
};

#endif
