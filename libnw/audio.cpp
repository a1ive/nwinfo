// SPDX-License-Identifier: Unlicense

#include "libnw.h"
#include "audio.h"
#include "devtree.h"
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <stdlib.h>
#include <functiondiscoverykeys_devpkey.h>

#define HDA_IDS_IMPL
#include <hda_ids.h>

extern "C" LPCSTR NWL_Ucs2ToUtf8(LPCWSTR src);

typedef struct _AUDIO_CODEC_ENUM_CTX
{
	CHAR CodecHwid[DEVTREE_MAX_STR_LEN];
} AUDIO_CODEC_ENUM_CTX;

static void
AudioSetCodecName(PNODE node, LPCSTR hwid)
{
	uint16_t vid = 0;
	uint16_t did = 0;

	if (strlen(hwid) < 33)
		return;

	vid = (uint16_t)strtoul(hwid + 20, NULL, 16);
	did = (uint16_t)strtoul(hwid + 29, NULL, 16);

	NWL_NodeAttrSet(node, "Codec Vendor", "Unknown", 0);
	for (size_t i = 0; i < sizeof(HDA_VENDOR_IDS) / sizeof(HDA_VENDOR_IDS[0]); i++)
	{
		if (HDA_VENDOR_IDS[i].vid == vid)
		{
			NWL_NodeAttrSet(node, "Codec Vendor", HDA_VENDOR_IDS[i].name, 0);
			break;
		}
	}

	NWL_NodeAttrSet(node, "Codec Device", "HDMI/DP", 0);
	for (size_t i = 0; i < sizeof(HDA_DEVICE_IDS) / sizeof(HDA_DEVICE_IDS[0]); i++)
	{
		if (HDA_DEVICE_IDS[i].vid == vid && HDA_DEVICE_IDS[i].did == did)
		{
			NWL_NodeAttrSet(node, "Codec Device", HDA_DEVICE_IDS[i].name, 0);
			break;
		}
	}
}

static void CALLBACK
GetDeviceInfoAudioCodec(PNODE node, void* data, DEVINST devInst, DEVINST parentDevInst, LPCSTR hwIds)
{
	AUDIO_CODEC_ENUM_CTX* ctx = (AUDIO_CODEC_ENUM_CTX*)data;
	(void)node;
	(void)devInst;
	(void)parentDevInst;

	if (ctx->CodecHwid[0] != '\0')
		return;

	strncpy_s(ctx->CodecHwid, DEVTREE_MAX_STR_LEN, hwIds, _TRUNCATE);
}

static void CALLBACK
GetDeviceInfoAudioController(PNODE node, void* data, DEVINST devInst, DEVINST parentDevInst, LPCSTR hwIds)
{
	static const CHAR prefix[] = "HDAUDIO\\FUNC_01&VEN_";
	AUDIO_CODEC_ENUM_CTX* ctx = (AUDIO_CODEC_ENUM_CTX*)data;
	DEVTREE_ENUM_CTX codecCtx = { 0 };
	PNODE devTree;
	(void)node;
	(void)parentDevInst;

	(void)hwIds;

	if (ctx->CodecHwid[0] != '\0')
		return;

	strncpy_s(codecCtx.filter, DEVTREE_MAX_STR_LEN, prefix, _TRUNCATE);
	codecCtx.filterLen = sizeof(prefix) - 1;
	codecCtx.hub = "Devices";
	codecCtx.data = ctx;
	codecCtx.GetDeviceInfo = GetDeviceInfoAudioCodec;

	devTree = NWL_NodeAlloc("Device Tree", NFLG_TABLE);
	NWL_EnumerateDevices(devTree, &codecCtx, devInst, 0);
	NWL_NodeFree(devTree, 1);
}

static void
AudioSetCodecHwid(PNODE node, LPCSTR hwid, DEVINST devRoot)
{
	DEVTREE_ENUM_CTX ctx = { 0 };
	AUDIO_CODEC_ENUM_CTX data = { 0 };
	PNODE devTree;

	if (devRoot == 0)
		return;

	strncpy_s(ctx.filter, DEVTREE_MAX_STR_LEN, hwid, _TRUNCATE);
	ctx.filterLen = strlen(ctx.filter);
	ctx.hub = "Devices";
	ctx.data = &data;
	ctx.GetDeviceInfo = GetDeviceInfoAudioController;

	devTree = NWL_NodeAlloc("Device Tree", NFLG_TABLE);
	NWL_EnumerateDevices(devTree, &ctx, devRoot, 0);
	NWL_NodeFree(devTree, 1);

	if (data.CodecHwid[0] != '\0')
	{
		NWL_NodeAttrSet(node, "Codec HWID", data.CodecHwid, 0);
		AudioSetCodecName(node, data.CodecHwid);
	}
}

static LPWSTR AudioGetDefaultId(IMMDeviceEnumerator* p)
{
	HRESULT hr = NULL;
	IMMDevice* dev = NULL;
	LPWSTR id = NULL;
	hr = p->GetDefaultAudioEndpoint(eRender, eConsole, &dev);
	if (FAILED(hr))
		return NULL;
	hr = dev->GetId(&id);
	dev->Release();
	return SUCCEEDED(hr) ? id : NULL;
}

static float
AudioGetDeviceVolume(IMMDevice* p)
{
	float ret = 0.0f;
	HRESULT hr = NULL;
	IAudioEndpointVolume* vol = NULL;
	hr = p->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, (LPVOID*)&vol);
	if (FAILED(hr))
		return 0.0f;
	hr = vol->GetMasterVolumeLevelScalar(&ret);
	vol->Release();
	return SUCCEEDED(hr) ? ret : 0.0f;
}

NWLIB_AUDIO_DEV*
NWL_GetAudio(UINT* count)
{
	HRESULT hr = S_OK;
	IMMDeviceEnumerator* p_enum = NULL;
	IMMDeviceCollection* p_collection = NULL;
	NWLIB_AUDIO_DEV* dev = NULL;
	LPWSTR default_id = NULL;

	*count = 0;

	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL,
		CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&p_enum);
	if (FAILED(hr))
		goto fail;

	hr = p_enum->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &p_collection);
	if (FAILED(hr))
		goto fail;

	hr = p_collection->GetCount(count);
	if (FAILED(hr))
		goto fail;

	dev = (NWLIB_AUDIO_DEV*)calloc(*count, sizeof(NWLIB_AUDIO_DEV));
	if (!dev)
		goto fail;

	default_id = AudioGetDefaultId(p_enum);

	for (ULONG i = 0; i < *count; i++)
	{
		IMMDevice* end_point = NULL;
		IPropertyStore* props = NULL;
		PROPVARIANT var;
		LPWSTR id = NULL;
		hr = p_collection->Item(i, &end_point);
		if (FAILED(hr))
			goto clean;

		dev[i].volume = AudioGetDeviceVolume(end_point);

		hr = end_point->GetId(&id);
		if (SUCCEEDED(hr))
		{
			wcscpy_s(dev[i].id, id);
			if (default_id)
				dev[i].is_default = wcscmp(id, default_id) == 0;
			CoTaskMemFree(id);
		}

		hr = end_point->OpenPropertyStore(STGM_READ, &props);
		if (FAILED(hr))
			goto clean;

		PropVariantInit(&var);
		hr = props->GetValue(PKEY_Device_FriendlyName, &var);
		if (SUCCEEDED(hr) && var.vt != VT_EMPTY)
			wcscpy_s(dev[i].name, var.pwszVal);
		PropVariantClear(&var);
	clean:
		if (end_point)
			end_point->Release();
		if (props)
			props->Release();
	}

fail:
	if (default_id)
		CoTaskMemFree(default_id);
	if (p_enum)
		p_enum->Release();
	if (p_collection)
		p_collection->Release();
	return dev;
}

inline void PrintMMDevices(PNODE node)
{
	UINT count, i;
	NWLIB_AUDIO_DEV* dev = NWL_GetAudio(&count);
	if (!dev)
		return;
	for (i = 0; i < count; i++)
	{
		CHAR buf[32];
		PNODE pChild;
		snprintf(buf, 32, "%u", i);
		pChild = NWL_NodeAppendNew(node, buf, NFLG_ATTGROUP);
		NWL_NodeAttrSet(pChild, "Name", NWL_Ucs2ToUtf8(dev[i].name), 0);
		NWL_NodeAttrSet(pChild, "ID", NWL_Ucs2ToUtf8(dev[i].id), 0);
		NWL_NodeAttrSetf(pChild, "Volume", 0, "%.0f%%", 100.0f * dev[i].volume);
		if (dev[i].is_default)
			NWL_NodeAttrSetf(node, "Default", NAFLG_FMT_NUMERIC, "%u", i);
	}
	free(dev);
}

inline void PrintSoundCards(PNODE node)
{
	PNODE sdc = NWL_NodeAppendNew(node, "Sound Cards", NFLG_TABLE);
	DEVINST devRoot = 0;

	PNWL_ARG_SET pciClasses = NULL;
	NWL_ArgSetAddStr(&pciClasses, "0401"); // Multimedia audio controller
	NWL_ArgSetAddStr(&pciClasses, "0403"); // Audio device
	PNODE pci = NWL_EnumPci(NWL_NodeAlloc("PCI", NFLG_TABLE), pciClasses);
	NWL_ArgSetFree(pciClasses);

	if (CM_Locate_DevNodeW(&devRoot, NULL, CM_LOCATE_DEVNODE_NORMAL) != CR_SUCCESS)
		NWL_NodeAppendMultiSz(&NWLC->ErrLog, "CM_Locate_DevNodeW failed");

	INT count = NWL_NodeChildCount(pci);
	for (INT i = 0; i < count; i++)
	{
		PNODE dev = NWL_NodeEnumChild(pci, i);
		if (dev == NULL)
			continue;
		PNODE t = NWL_NodeAppendNew(sdc, "Device", NFLG_TABLE_ROW);
		const char* name = NWL_NodeAttrGet(dev, "Device");
		if (name[0] == '-' && name[1] == '\0')
			name = "High Definition Audio Controller";
		LPCSTR hwid = NWL_NodeAttrGet(dev, "HWID");
		NWL_NodeAttrSet(t, "HWID", hwid, 0);
		NWL_NodeAttrSet(t, "Vendor", NWL_NodeAttrGet(dev, "Vendor"), 0);
		NWL_NodeAttrSet(t, "Device", name, 0);
		AudioSetCodecHwid(t, hwid, devRoot);
	}
	NWL_NodeFree(pci, 1);
}

PNODE NW_Audio(BOOL bAppend)
{
	PNODE pNode = NWL_NodeAlloc("Audio", 0);
	if (bAppend)
		NWL_NodeAppendChild(NWLC->NwRoot, pNode);
	PrintMMDevices(pNode);
	PrintSoundCards(pNode);
	return pNode;
}
