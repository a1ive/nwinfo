// SPDX-License-Identifier: Unlicense

#include <initguid.h>

#include "libnw.h"
#include "audio.h"
#include "devtree.h"
#include "utils.h"
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <functiondiscoverykeys_devpkey.h>

#define HDA_IDS_IMPL
#include <hda_ids.h>

DEFINE_GUID(IID_IMMDeviceEnumerator,
	0xa95664d2, 0x9614, 0x4f35, 0xa7, 0x46, 0xde, 0x8d, 0xb6, 0x36, 0x17, 0xe6);
DEFINE_GUID(CLSID_MMDeviceEnumerator,
	0xbcde0395, 0xe52f, 0x467c, 0x8e, 0x3d, 0xc4, 0x57, 0x92, 0x91, 0x69, 0x2e);
DEFINE_GUID(IID_IAudioEndpointVolume,
	0x5cdf2c82, 0x841e, 0x4546, 0x97, 0x22, 0x0c, 0xf7, 0x40, 0x78, 0x22, 0x9a);
DEFINE_GUID(IID_IAudioMeterInformation,
	0xc02216f6, 0x8c67, 0x4b5b, 0x9d, 0x00, 0xd0, 0x08, 0xe7, 0x3e, 0x00, 0x64);

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
	HRESULT hr = S_OK;
	IMMDevice* dev = NULL;
	LPWSTR id = NULL;
	hr = p->lpVtbl->GetDefaultAudioEndpoint(p, eRender, eConsole, &dev);
	if (FAILED(hr))
		return NULL;
	hr = dev->lpVtbl->GetId(dev, &id);
	dev->lpVtbl->Release(dev);
	return SUCCEEDED(hr) ? id : NULL;
}

static float
AudioGetDeviceVolume(IMMDevice* p)
{
	float ret = 0.0f;
	HRESULT hr = S_OK;
	IAudioEndpointVolume* vol = NULL;
	hr = p->lpVtbl->Activate(p, &IID_IAudioEndpointVolume, CLSCTX_INPROC_SERVER, NULL, (LPVOID*)&vol);
	if (FAILED(hr))
		return 0.0f;
	hr = vol->lpVtbl->GetMasterVolumeLevelScalar(vol, &ret);
	vol->lpVtbl->Release(vol);
	return SUCCEEDED(hr) ? ret : 0.0f;
}

static void
AudioGetDeviceLoudness(IMMDevice* p, NWLIB_AUDIO_DEV* dev)
{
	HRESULT hr = S_OK;
	IAudioMeterInformation* meter = NULL;
	float peak = 0.0f;

	if (p == NULL || dev == NULL)
		return;

	dev->loudness_percent = 0.0f;

	hr = p->lpVtbl->Activate(p, &IID_IAudioMeterInformation, CLSCTX_INPROC_SERVER, NULL, (LPVOID*)&meter);
	if (FAILED(hr))
		return;

	hr = meter->lpVtbl->GetPeakValue(meter, &peak);
	if (FAILED(hr))
		goto clean;

	dev->loudness_percent = peak * 100.0f;

clean:
	if (meter)
		meter->lpVtbl->Release(meter);
}

NWLIB_AUDIO_DEV*
NWL_GetAudio(UINT* count)
{
	HRESULT hr = S_OK;
	IMMDeviceEnumerator* p_enum = NULL;
	IMMDeviceCollection* p_collection = NULL;
	NWLIB_AUDIO_DEV* dev = NULL;
	LPWSTR default_id = NULL;

	if (count == NULL)
		return NULL;

	*count = 0;

	hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL,
		CLSCTX_ALL, &IID_IMMDeviceEnumerator, (void**)&p_enum);
	if (FAILED(hr))
		goto fail;

	hr = p_enum->lpVtbl->EnumAudioEndpoints(p_enum, eRender, DEVICE_STATE_ACTIVE, &p_collection);
	if (FAILED(hr))
		goto fail;

	hr = p_collection->lpVtbl->GetCount(p_collection, count);
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
		hr = p_collection->lpVtbl->Item(p_collection, i, &end_point);
		if (FAILED(hr))
			goto clean;

		dev[i].volume = AudioGetDeviceVolume(end_point);

		hr = end_point->lpVtbl->GetId(end_point, &id);
		if (SUCCEEDED(hr))
		{
			wcscpy_s(dev[i].id, _countof(dev[i].id), id);
			if (default_id)
				dev[i].is_default = wcscmp(id, default_id) == 0;
			CoTaskMemFree(id);
		}
		if (dev[i].is_default)
			AudioGetDeviceLoudness(end_point, &dev[i]);

		hr = end_point->lpVtbl->OpenPropertyStore(end_point, STGM_READ, &props);
		if (FAILED(hr))
			goto clean;

		PropVariantInit(&var);
		hr = props->lpVtbl->GetValue(props, &PKEY_Device_FriendlyName, &var);
		if (SUCCEEDED(hr) && var.vt == VT_LPWSTR && var.pwszVal != NULL)
			wcscpy_s(dev[i].name, _countof(dev[i].name), var.pwszVal);
		PropVariantClear(&var);
	clean:
		if (end_point)
			end_point->lpVtbl->Release(end_point);
		if (props)
			props->lpVtbl->Release(props);
	}

fail:
	if (default_id)
		CoTaskMemFree(default_id);
	if (p_enum)
		p_enum->lpVtbl->Release(p_enum);
	if (p_collection)
		p_collection->lpVtbl->Release(p_collection);
	return dev;
}

static void PrintMMDevices(PNODE node)
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
		NWL_NodeAttrSetf(pChild, "Loudness Percent", 0, "%.2f%%", dev[i].loudness_percent);
	}
	free(dev);
}

static void PrintSoundCards(PNODE node)
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
