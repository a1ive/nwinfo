// SPDX-License-Identifier: MPL-2.0
#include "lhm.h"

#using <mscorlib.dll>
#using <System.dll>

#include <msclr/auto_gcroot.h>
#include <msclr/marshal_cppstd.h>
#include <array>
#include <string>
#include <vector>
#include <cwchar>

using namespace System;
using namespace System::Collections;
using namespace System::IO;
using namespace System::Reflection;

namespace
{
	msclr::auto_gcroot<Object^> computer_handle;
	std::wstring last_error;
	std::vector<LhmSensorInfo> sensor_cache;

	struct ToggleSetting
	{
		const wchar_t* name;
		bool enabled;
	};

	static std::array<ToggleSetting, 8> toggle_names = { {
		{ L"IsBatteryEnabled", true },
		{ L"IsControllerEnabled", true },
		{ L"IsCpuEnabled", true },
		{ L"IsGpuEnabled", true },
		{ L"IsMemoryEnabled", true },
		{ L"IsMotherboardEnabled", true },
		{ L"IsNetworkEnabled", true },
		{ L"IsStorageEnabled", true }
	} };
}

static void SetError(const std::wstring& message)
{
	last_error = message;
}

static void SetError(String^ message)
{
	last_error = msclr::interop::marshal_as<std::wstring>(message);
}

static std::wstring ToStdWString(String^ managed_value)
{
	return msclr::interop::marshal_as<std::wstring>(managed_value);
}

const wchar_t* __stdcall LhmGetLastError(void)
{
	return last_error.c_str();
}

static Assembly^ LoadLibreHardwareMonitor()
{
	String^ base_dir = AppDomain::CurrentDomain->BaseDirectory;
	String^ dll_path = Path::Combine(base_dir, "LibreHardwareMonitorLib.dll");
	if (File::Exists(dll_path))
	{
		return Assembly::LoadFrom(dll_path);
	}

	throw gcnew FileNotFoundException("LibreHardwareMonitorLib.dll not found. Copy it next to the executable.");
}

static Object^ GetProperty(Object^ target, String^ property_name)
{
	return target->GetType()->GetProperty(property_name)->GetValue(target, nullptr);
}

static void SetBooleanProperty(Object^ target, String^ property_name, bool value)
{
	target->GetType()->GetProperty(property_name)->SetValue(target, value, nullptr);
}

static Object^ Invoke(Object^ target, String^ method_name)
{
	return target->GetType()->GetMethod(method_name, Type::EmptyTypes)->Invoke(target, nullptr);
}

static Nullable<float> ToNullableFloat(Object^ managed_value)
{
	if (managed_value == nullptr)
	{
		return Nullable<float>();
	}

	return safe_cast<Nullable<float>>(managed_value);
}

static void UpdateHardware(Object^ hardware)
{
	Invoke(hardware, "Update");

	auto sub_hardware = safe_cast<IEnumerable^>(GetProperty(hardware, "SubHardware"));
	for each (Object ^ child in sub_hardware)
	{
		UpdateHardware(child);
	}
}

static void CopyToBuffer(const std::wstring& src, wchar_t* dest)
{
	if (!dest)
	{
		return;
	}

	wcsncpy_s(dest, LHM_STR_MAX, src.c_str(), _TRUNCATE);
}

static void CollectSensors(Object^ hardware, const std::wstring& hardware_name, std::vector<LhmSensorInfo>& output)
{
	UpdateHardware(hardware);

	auto sensors = safe_cast<IEnumerable^>(GetProperty(hardware, "Sensors"));
	for each (Object ^ sensor in sensors)
	{
		Nullable<float> value_opt = ToNullableFloat(GetProperty(sensor, "Value"));
		Nullable<float> min_opt = ToNullableFloat(GetProperty(sensor, "Min"));
		Nullable<float> max_opt = ToNullableFloat(GetProperty(sensor, "Max"));

		LhmSensorInfo sensor_record{};
		CopyToBuffer(hardware_name, sensor_record.hardware);
		CopyToBuffer(ToStdWString(GetProperty(sensor, "Identifier")->ToString()), sensor_record.id);
		CopyToBuffer(ToStdWString(safe_cast<String^>(GetProperty(sensor, "Name"))), sensor_record.name);
		CopyToBuffer(ToStdWString(GetProperty(sensor, "SensorType")->ToString()), sensor_record.type);

		sensor_record.has_value = value_opt.HasValue;
		sensor_record.value = value_opt.HasValue ? value_opt.Value : 0.0f;
		sensor_record.has_min = min_opt.HasValue;
		sensor_record.min = min_opt.HasValue ? min_opt.Value : 0.0f;
		sensor_record.has_max = max_opt.HasValue;
		sensor_record.max = max_opt.HasValue ? max_opt.Value : 0.0f;

		output.push_back(sensor_record);
	}

	auto sub_hardware = safe_cast<IEnumerable^>(GetProperty(hardware, "SubHardware"));
	for each (Object ^ child in sub_hardware)
	{
		std::wstring child_name = ToStdWString(safe_cast<String^>(GetProperty(child, "Name")));
		CollectSensors(child, child_name, output);
	}
}

bool __stdcall LhmInitialize(void)
{
	try
	{
		Assembly^ lhm_assembly = LoadLibreHardwareMonitor();
		Type^ computer_type = lhm_assembly->GetType("LibreHardwareMonitor.Hardware.Computer", true);
		Object^ computer_instance = Activator::CreateInstance(computer_type);

		for (const auto& toggle : toggle_names)
		{
			String^ property_name = gcnew String(toggle.name);
			SetBooleanProperty(computer_instance, property_name, toggle.enabled);
		}

		Invoke(computer_instance, "Open");

		computer_handle = computer_instance;
		last_error.clear();
		return true;
	}
	catch (Exception^ ex)
	{
		SetError(ex->Message);
		return false;
	}
}

size_t __stdcall LhmGetToggleCount(void)
{
	return toggle_names.size();
}

bool __stdcall LhmGetToggleInfo(size_t index, const wchar_t** out_name, bool* out_enabled)
{
	if (out_name == nullptr || out_enabled == nullptr)
	{
		SetError(L"outName and outEnabled must not be null");
		return false;
	}

	if (index >= toggle_names.size())
	{
		SetError(L"toggle index out of range");
		return false;
	}

	*out_name = toggle_names[index].name;
	*out_enabled = toggle_names[index].enabled;
	last_error.clear();
	return true;
}

bool __stdcall LhmSetToggleEnabled(size_t index, bool enabled)
{
	if (index >= toggle_names.size())
	{
		SetError(L"toggle index out of range");
		return false;
	}

	const bool previous = toggle_names[index].enabled;
	toggle_names[index].enabled = enabled;

	if (computer_handle.get() != nullptr)
	{
		try
		{
			String^ property_name = gcnew String(toggle_names[index].name);
			SetBooleanProperty(computer_handle.get(), property_name, enabled);
		}
		catch (Exception^ ex)
		{
			toggle_names[index].enabled = previous;
			SetError(ex->Message);
			return false;
		}
	}

	last_error.clear();
	return true;
}

bool __stdcall LhmEnumerateSensors(const LhmSensorInfo** sensors, size_t* out_count)
{
	if (sensors == nullptr || out_count == nullptr)
	{
		SetError(L"sensors and outCount must not be null");
		return false;
	}

	*sensors = nullptr;
	*out_count = 0;

	if (computer_handle.get() == nullptr)
	{
		SetError(L"LhmInitialize must be called before enumerating sensors.");
		return false;
	}

	try
	{
		sensor_cache.clear();

		auto hardware_list = safe_cast<IEnumerable^>(GetProperty(computer_handle.get(), "Hardware"));
		for each (Object ^ hardware in hardware_list)
		{
			std::wstring hardware_name = ToStdWString(safe_cast<String^>(GetProperty(hardware, "Name")));
			CollectSensors(hardware, hardware_name, sensor_cache);
		}

		*out_count = sensor_cache.size();
		*sensors = sensor_cache.empty() ? nullptr : sensor_cache.data();

		last_error.clear();
		return true;
	}
	catch (Exception^ ex)
	{
		sensor_cache.clear();
		*sensors = nullptr;
		*out_count = 0;
		SetError(ex->Message);
		return false;
	}
}

void __stdcall LhmShutdown(void)
{
	sensor_cache.clear();
	std::vector<LhmSensorInfo>().swap(sensor_cache);

	try
	{
		if (computer_handle.get() != nullptr)
		{
			Invoke(computer_handle.get(), "Close");
			computer_handle.reset();
		}

		last_error.clear();
	}
	catch (Exception^ ex)
	{
		SetError(ex->Message);
	}
}

#pragma managed(push, off)
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
#pragma managed(pop)
