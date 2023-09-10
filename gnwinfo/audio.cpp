// SPDX-License-Identifier: Unlicense

#include "audio.h"
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <functiondiscoverykeys_devpkey.h>

LPWSTR get_default_id(IMMDeviceEnumerator* p)
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
get_device_volume(IMMDevice* p)
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

extern "C" GNW_AUDIO_DEV*
gnwinfo_get_audio(UINT* count)
{
	HRESULT hr = S_OK;
	IMMDeviceEnumerator* p_enum = NULL;
	IMMDeviceCollection* p_collection = NULL;
	GNW_AUDIO_DEV* dev = NULL;
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

	dev = (GNW_AUDIO_DEV*)calloc(*count, sizeof(GNW_AUDIO_DEV));
	if (!dev)
		goto fail;

	default_id = get_default_id(p_enum);

	for (ULONG i = 0; i < *count; i++)
	{
		IMMDevice* end_point = NULL;
		IPropertyStore* props = NULL;
		PROPVARIANT var;
		LPWSTR id = NULL;
		hr = p_collection->Item(i, &end_point);
		if (FAILED(hr))
			goto clean;

		dev[i].volume = get_device_volume(end_point);

		hr = end_point->GetId(&id);
		if (SUCCEEDED(hr))
		{
			if (default_id)
				dev[i].is_default = wcscmp(id, default_id) == 0;
#if 0
			wcscpy_s(dev[i].id, id);
#endif
			CoTaskMemFree(id);
		}

		hr = end_point->OpenPropertyStore(STGM_READ, &props);
		if (FAILED(hr))
			goto clean;
#if 0
		PropVariantInit(&var);
		hr = props->GetValue(PKEY_DeviceInterface_FriendlyName, &var);
		if (SUCCEEDED(hr) && var.vt != VT_EMPTY)
			wcscpy_s(dev[i].if_name, var.pwszVal);
		PropVariantClear(&var);

		PropVariantInit(&var);
		hr = props->GetValue(PKEY_Device_DeviceDesc, &var);
		if (SUCCEEDED(hr) && var.vt != VT_EMPTY)
			wcscpy_s(dev[i].desc, var.pwszVal);
		PropVariantClear(&var);
#endif
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
