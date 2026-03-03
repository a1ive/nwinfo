// SPDX-License-Identifier: Unlicense
#pragma once

// https://en.wikipedia.org/wiki/List_of_Intel_chipsets
// https://en.wikipedia.org/wiki/List_of_AMD_chipsets
// https://en.wikipedia.org/wiki/Comparison_of_Nvidia_nForce_chipsets
// https://github.com/LibreHardwareMonitor/LibreHardwareMonitor/blob/master/LibreHardwareMonitorLib/Hardware/Motherboard/Identification.cs
// https://github.com/lzhoang2801/Hardware-Sniffer/blob/main/Scripts/datasets/chipset_data.py

struct CHIPSET_ID_ENTRY
{
	char id[5];
	const char* name;
};

#ifdef CHIPSET_IDS_IMPL

static const struct CHIPSET_ID_ENTRY INTEL_ISA_LIST[] =
{
	{ "0284", "Comet Lake-LP" }, // 400 series
	{ "0285", "Comet Lake" }, // 400 series
	{ "0684", "H470" }, // 400 series
	{ "0685", "Z490" }, // 400 series
	{ "0687", "Q470" }, // 400 series
	{ "068C", "QM480" }, // 400 series
	{ "068D", "HM470" }, // 400 series
	{ "068E", "WM490" }, // 400 series
	{ "0697", "W480" }, // 400 series
	{ "069A", "H420E" },
	{ "19DC", "Atom C3000" },
	{ "1C40", "Cougar Point" },
	{ "1C41", "Cougar Point Mobile SFF" },
	{ "1C42", "Cougar Point" },
	{ "1C43", "Cougar Point Mobile" },
	{ "1C44", "Z68" }, // 6 / C200
	{ "1C45", "Cougar Point" },
	{ "1C46", "P67" }, // 6 / C200
	{ "1C47", "UM67" }, // 6 / C200
	{ "1C48", "Cougar Point" },
	{ "1C49", "HM65" }, // 6 / C200
	{ "1C4A", "H67" }, // 6 / C200
	{ "1C4B", "HM67" }, // 6 / C200
	{ "1C4C", "Q65" }, // 6 / C200
	{ "1C4D", "QS67" }, // 6 / C200
	{ "1C4E", "Q67" }, // 6 / C200
	{ "1C4F", "QM67" }, // 6 / C200
	{ "1C50", "B65" }, // 6 / C200
	{ "1C51", "Cougar Point" },
	{ "1C52", "C202" }, // 6 / C200
	{ "1C53", "Cougar Point" },
	{ "1C54", "C204" }, // 6 / C200
	{ "1C55", "Cougar Point" },
	{ "1C56", "C206" }, // 6 / C200
	{ "1C57", "Cougar Point" },
	{ "1C58", "B65" },
	{ "1C59", "HM67" },
	{ "1C5A", "Q67" },
	{ "1C5B", "Cougar Point" },
	{ "1C5C", "H61" },
	{ "1C5D", "Cougar Point" },
	{ "1C5E", "Cougar Point" },
	{ "1C5F", "Cougar Point" },
	{ "1D40", "C600/X79" },
	{ "1D41", "C600/X79" },
	{ "1E41", "Panther Point" },
	{ "1E42", "Panther Point" },
	{ "1E43", "Panther Point" },
	{ "1E44", "Z77" },
	{ "1E45", "Panther Point" },
	{ "1E46", "Z75" },
	{ "1E47", "Q77" },
	{ "1E48", "Q75" },
	{ "1E49", "B75" },
	{ "1E4A", "H77" },
	{ "1E4B", "Panther Point" },
	{ "1E4C", "Panther Point" },
	{ "1E4D", "Panther Point" },
	{ "1E4E", "Panther Point" },
	{ "1E4F", "Panther Point" },
	{ "1E50", "Panther Point" },
	{ "1E51", "Panther Point" },
	{ "1E52", "Panther Point" },
	{ "1E53", "C216" },
	{ "1E54", "Panther Point" },
	{ "1E55", "QM77" },
	{ "1E56", "QS77" },
	{ "1E57", "HM77" },
	{ "1E58", "UM77" },
	{ "1E59", "HM76" },
	{ "1E5A", "Panther Point" },
	{ "1E5B", "UM77" },
	{ "1E5C", "Panther Point" },
	{ "1E5D", "HM75" },
	{ "1E5E", "HM70" },
	{ "1E5F", "NM70" },
	{ "2310", "DH89xxCC" },
	{ "2390", "DH895XCC" },
	{ "2410", "82801AA" },
	{ "2420", "82801AB" },
	{ "2440", "82801BA" },
	{ "244C", "82801BAM" },
	{ "2450", "82801E" },
	{ "2480", "82801CA" },
	{ "248C", "82801CAM" },
	{ "24C0", "82801DB/DBL (ICH4/ICH4-L)" },
	{ "24CC", "82801DBM (ICH4-M)" },
	{ "24D0", "82801EB/ER (ICH5/ICH5R)" },
	{ "24DC", "82801EB (ICH5)" },
	{ "25A1", "6300ESB" },
	{ "2640", "82801FB/FR (ICH6/ICH6R)" },
	{ "2641", "82801FBM (ICH6M)" },
	{ "2642", "82801FW/FRW (ICH6W/ICH6RW)" },
	{ "2670", "631xESB/632xESB/3100" },
	{ "27B0", "82801GH (ICH7DH)" },
	{ "27B8", "82801GB/GR (ICH7)" },
	{ "27B9", "82801GBM (ICH7-M)" },
	{ "27BC", "NM10" },
	{ "27BD", "82801GHM (ICH7-M DH)" },
	{ "2810", "82801HB/HR (ICH8/R)" },
	{ "2811", "82801HEM (ICH8M-E)" },
	{ "2812", "82801HH (ICH8DH)" },
	{ "2814", "82801HO (ICH8DO)" },
	{ "2815", "82801HM (ICH8M)" },
	{ "2912", "82801IH (ICH9DH)" },
	{ "2914", "82801IO (ICH9DO)" },
	{ "2916", "82801IR (ICH9R)" },
	{ "2917", "82801IM (ICH9M-E)" },
	{ "2918", "82801IB (ICH9)" },
	{ "2919", "82801IM (ICH9M)" },
	{ "3197", "Gemini Lake" },
	{ "31E8", "Gemini Lake" },
	{ "3482", "Ice Lake-LP" },
	{ "3882", "Ice Lake" },
	{ "3A14", "82801JDO (ICH10DO)" },
	{ "3A16", "82801JIR (ICH10R)" },
	{ "3A18", "82801JIB (ICH10)" },
	{ "3A1A", "82801JD (ICH10D)" },
	{ "3B00", "5/3400" },
	{ "3B01", "5/ Mobile" },
	{ "3B02", "P55" },
	{ "3B03", "PM55" },
	{ "3B06", "H55" },
	{ "3B07", "QM57" },
	{ "3B08", "H57" },
	{ "3B09", "HM55" },
	{ "3B0A", "Q57" },
	{ "3B0B", "HM57" },
	{ "3B0F", "QS57" },
	{ "3B10", "5/3400" },
	{ "3B11", "5/3400" },
	{ "3B12", "3400" },
	{ "3B13", "5/3400" },
	{ "3B14", "3420" },
	{ "3B15", "5/3400" },
	{ "3B16", "3450" },
	{ "3B17", "5/3400" },
	{ "3B18", "5/3400" },
	{ "3B19", "5/3400" },
	{ "3B1A", "5/3400" },
	{ "3B1B", "5/3400" },
	{ "3B1C", "5/3400" },
	{ "3B1D", "5/3400" },
	{ "3B1E", "5/3400" },
	{ "3B1F", "5/3400" },
	{ "4381", "Rocket Lake Desktop" }, // 500 series
	{ "4382", "Rocket Lake Mobile" }, // 500 series
	{ "4383", "Rocket Lake Server" }, // 500 series
	{ "4384", "Q570" }, // 500 series
	{ "4385", "Z590" }, // 500 series
	{ "4386", "H570" }, // 500 series
	{ "4387", "B560" }, // 500 series
	{ "4388", "H510" }, // 500 series
	{ "4389", "WM590" }, // 500 series
	{ "438A", "QM580" }, // 500 series
	{ "438B", "HM570" }, // 500 series
	{ "438C", "C252" }, // 500 series
	{ "438D", "C256" }, // 500 series
	{ "438E", "H310D" },
	{ "438F", "W580" }, // 500 series
	{ "4390", "RM590E" },
	{ "4391", "R580E" },
	{ "4B00", "Elkhart Lake" },
	{ "4D87", "Jasper Lake" },
	{ "5031", "EP80579" },
	{ "5181", "Alder Lake" },
	{ "5182", "Alder Lake" },
	{ "5187", "Alder Lake" },
	{ "519D", "Raptor Lake-P" },
	{ "519E", "Raptor Lake-PX" },
	{ "51A4", "Alder Lake-P" },
	{ "51AA", "Alder Lake" },
	{ "51AB", "Alder Lake" },
	{ "5481", "Alder Lake-N" },
	{ "5795", "Granite Rapids"},
	{ "7727", "Arrow Lake-H" },
	{ "7730", "Arrow Lake-H" },
	{ "7746", "Arrow Lake-H" },
	{ "7A04", "Z790" }, // 700 series
	{ "7A05", "H770" }, // 700 series
	{ "7A06", "B760" }, // 700 series
	{ "7A0C", "HM770" }, // 700 series
	{ "7A0D", "WM790" }, // 700 series
	{ "7A13", "C266" }, // 700 series
	{ "7A14", "C262" }, // 700 series
	{ "7A83", "Q670" }, // 600 series
	{ "7A84", "Z690" }, // 600 series
	{ "7A85", "H670" }, // 600 series
	{ "7A86", "B660" }, // 600 series
	{ "7A87", "H610" }, // 600 series
	{ "7A88", "W680" }, // 600 series
	{ "7A8A", "W790" }, // 700 series
	{ "7A8C", "HM670" }, // 600 series
	{ "7A8D", "WM690" }, // 600 series
	{ "7A90", "R680E" },
	{ "7A91", "Q670E" },
	{ "7A92", "H610E" },
	{ "7E01", "Meteor Lake-P" },
	{ "7F03", "Q870" }, // 800 series
	{ "7F04", "Z890" }, // 800 series
	{ "7F06", "B860" }, // 800 series
	{ "7F07", "H810" }, // 800 series
	{ "7F08", "W880" }, // 800 series
	{ "7F0C", "HM870" }, // 800 series
	{ "7F0D", "WM880" }, // 800 series
	{ "8186", "Atom E6xx" },
	{ "8C40", "8/C220" },
	{ "8C41", "8/C220 Mobile" },
	{ "8C42", "8/C220 Desktop" },
	{ "8C43", "8/C220" },
	{ "8C44", "Z87" },
	{ "8C45", "8/C220" },
	{ "8C46", "Z85" },
	{ "8C47", "8/C220" },
	{ "8C48", "8/C220" },
	{ "8C49", "HM86" },
	{ "8C4A", "H87" },
	{ "8C4B", "HM87" },
	{ "8C4C", "Q85" },
	{ "8C4D", "8/C220" },
	{ "8C4E", "Q87" },
	{ "8C4F", "QM87" },
	{ "8C50", "B85" },
	{ "8C51", "8/C220" },
	{ "8C52", "C222" },
	{ "8C53", "8/C220" },
	{ "8C54", "C224" },
	{ "8C55", "8/C220" },
	{ "8C56", "C226" },
	{ "8C57", "8/C220" },
	{ "8C58", "8/C220 WS" },
	{ "8C59", "8/C220" },
	{ "8C5A", "8/C220" },
	{ "8C5B", "8/C220" },
	{ "8C5C", "H81" },
	{ "8C5D", "8/C220" },
	{ "8C5E", "8/C220" },
	{ "8C5F", "8/C220" },
	{ "8CC1", "9/ Mobile" }, // 9 Series
	{ "8CC2", "9/ Desktop" }, // 9 Series
	{ "8CC3", "HM97" },
	{ "8CC4", "Z97" },
	{ "8CC5", "QM97" },
	{ "8CC6", "H97" },
	{ "8D40", "C610/X99" },
	{ "8D41", "C610/X99" },
	{ "8D42", "C610/X99" },
	{ "8D43", "C610/X99" },
	{ "8D44", "C610/X99" },
	{ "8D45", "C610/X99" },
	{ "8D46", "C610/X99" },
	{ "8D47", "C610/X99" },
	{ "8D48", "C610/X99" },
	{ "8D49", "C610/X99" },
	{ "8D4A", "C610/X99" },
	{ "8D4B", "C610/X99" },
	{ "8D4C", "C610/X99" },
	{ "8D4D", "C610/X99" },
	{ "8D4E", "C610/X99" },
	{ "8D4F", "C610/X99" },
	{ "9C40", "Lynx Point" }, // 8 Series
	{ "9C41", "Lynx Point" },
	{ "9C42", "Lynx Point" },
	{ "9C43", "Lynx Point" },
	{ "9C44", "Lynx Point" },
	{ "9C45", "Lynx Point" },
	{ "9C46", "Lynx Point" },
	{ "9C47", "Lynx Point" },
	{ "9CC1", "Wildcat Point-LP" },
	{ "9CC2", "Wildcat Point-LP" },
	{ "9CC3", "Wildcat Point-LP" },
	{ "9CC5", "Wildcat Point-LP" },
	{ "9CC6", "Wildcat Point-LP" },
	{ "9CC7", "Wildcat Point-LP" },
	{ "9CC9", "Wildcat Point-LP" },
	{ "9D43", "Sunrise Point-LP" },
	{ "9D46", "Sunrise Point-LP" },
	{ "9D48", "Sunrise Point-LP" },
	{ "9D4B", "Sunrise Point-LP" },
	{ "9D4E", "Sunrise Point" },
	{ "9D50", "Sunrise Point" },
	{ "9D56", "Sunrise Point-LP" },
	{ "9D58", "Sunrise Point-LP" },
	{ "9D4E", "Sunrise Point" },
	{ "9D84", "Cannon Point-LP" }, // 300 series
	{ "A082", "Tiger Lake-LP" },
	{ "A140", "Sunrise Point-H" },
	{ "A141", "Sunrise Point-H" },
	{ "A142", "Sunrise Point-H" },
	{ "A143", "H110" }, // 100 / C230
	{ "A144", "H170" }, // 100 / C230
	{ "A145", "Z170" }, // 100 / C230
	{ "A146", "Q170" }, // 100 / C230
	{ "A147", "Q150" }, // 100 / C230
	{ "A148", "B150" }, // 100 / C230
	{ "A149", "C236" }, // 100 / C230
	{ "A14A", "C232" }, // 100 / C230
	{ "A14B", "Sunrise Point-H" },
	{ "A14C", "Sunrise Point-H" },
	{ "A14D", "QM170" }, // 100 / C230
	{ "A14E", "HM170" }, // 100 / C230
	{ "A14F", "Sunrise Point-H" },
	{ "A150", "CM236" }, // 100 / C230
	{ "A151", "QMS180" },
	{ "A152", "HM175" }, // 100 / C230
	{ "A153", "QM175" }, // 100 / C230
	{ "A154", "CM238" }, // 100 / C230
	{ "A155", "QMU185" },
	{ "A156", "Sunrise Point-H" },
	{ "A157", "Sunrise Point-H" },
	{ "A158", "Sunrise Point-H" },
	{ "A159", "Sunrise Point-H" },
	{ "A15A", "Sunrise Point-H" },
	{ "A15B", "Sunrise Point-H" },
	{ "A15C", "Sunrise Point-H" },
	{ "A15D", "Sunrise Point-H" },
	{ "A15E", "Sunrise Point-H" },
	{ "A15F", "Sunrise Point-H" },
	{ "A1C1", "C621" },
	{ "A1C2", "C622" },
	{ "A1C3", "C624" },
	{ "A1C4", "C625" },
	{ "A1C5", "C626" },
	{ "A1C6", "C627" },
	{ "A1C7", "C628" },
	{ "A1C8", "Lewisburg" },
	{ "A1CB", "Lewisburg" },
	{ "A242", "Lewisburg" },
	{ "A243", "Lewisburg" },
	{ "A2C4", "H270" }, // 200 / X299 / Z370
	{ "A2C5", "Z270" }, // 200 / X299 / Z370
	{ "A2C6", "Q270" }, // 200 / X299 / Z370
	{ "A2C7", "Q250" }, // 200 / X299 / Z370
	{ "A2C8", "B250" }, // 200 / X299 / Z370
	{ "A2C9", "Z370" }, // 200 / X299 / Z370
	{ "A2CA", "H310" },
	{ "A2CC", "B365" },
	{ "A2D2", "X299" }, // 200 / X299 / Z370
	{ "A2D3", "C422" },
	{ "A303", "H310" }, // 300 / C240
	{ "A304", "H370" }, // 300 / C240
	{ "A305", "Z390" }, // 300 / C240
	{ "A306", "Q370" }, // 300 / C240
	{ "A308", "B360" }, // 300 / C240
	{ "A309", "C246" }, // 300 / C240
	{ "A30A", "C242" }, // 300 / C240
	{ "A30C", "QM370" }, // 300 / C240
	{ "A30D", "HM370" }, // 300 / C240
	{ "A30E", "CM246" }, // 300 / C240
	{ "A313", "Cannon Lake" },
	{ "A3C8", "B460" },
	{ "A3DA", "H410" },
	{ "A806", "Lunar Lake-M" },
	
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
