// SPDX-License-Identifier: Unlicense
#pragma once

// TCG TPM Vendor ID Registry Family 1.2 and 2.0
// TPM Capabilities Vendor ID (TPM_PT_MANUFACTURER / tpmVendorID)

struct TPM_VENDOR_ID_ENTRY
{
	const char* id;
	const char* name;
};

#ifdef TPM_IDS_IMPL
static const struct TPM_VENDOR_ID_ENTRY TPM_VENDOR_ID_LIST[] =
{
	{ "AMD", "AMD" },
	{ "ANT", "Ant Group" },
	{ "ATML", "Atmel" },
	{ "BRCM", "Broadcom" },
	{ "CSCO", "Cisco" },
	{ "FLYS", "Flyslice" },
	{ "ROCC", "Rockchip" },
	{ "GOOG", "Google" },
	{ "HPI", "HPI" },
	{ "HPE", "HPE" },
	{ "HISI", "Huawei" },
	{ "IBM", "IBM" },
	{ "IFX", "Infineon" },
	{ "INTC", "Intel" },
	{ "LEN", "Lenovo" },
	{ "MSFT", "Microsoft" },
	{ "NSM ", "National Semiconductor" },
	{ "NTZ", "Nationz" },
	{ "NSG", "NSING" },
	{ "NTC", "Nuvoton" },
	{ "QCOM", "Qualcomm" },
	{ "SMSN", "Samsung" },
	{ "SECE", "SecEdge" },
	{ "SNS", "Sinosun" },
	{ "SMSC", "SMSC" },
	{ "STM ", "STMicroelectronics" },
	{ "TXN", "Texas Instruments" },
	{ "WEC", "Winbond" },
	{ "SEAL", "Wisekey" },
	{ "SIM0", "Simulator 0" },
	{ "SIM1", "Simulator 1" },
	{ "SIM2", "Simulator 2" },
	{ "SIM3", "Simulator 3" },
	{ "SIM4", "Simulator 4" },
	{ "SIM5", "Simulator 5" },
	{ "SIM6", "Simulator 6" },
	{ "SIM7", "Simulator 7" },
	{ "TST0", "Test 0" },
	{ "TST1", "Test 1" },
	{ "TST2", "Test 2" },
	{ "TST3", "Test 3" },
	{ "TST4", "Test 4" },
	{ "TST5", "Test 5" },
	{ "TST6", "Test 6" },
	{ "TST7", "Test 7" },
};
#endif
