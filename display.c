// SPDX-License-Identifier: Unlicense

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <winuser.h>
#include "nwinfo.h"

void nwinfo_display(void)
{
	DWORD iDevNum = 0;
	DEVMODEA DevMode = { .dmSize = sizeof(DEVMODEA) };
	DISPLAY_DEVICEA DisplayDevice = { .cb = sizeof(DISPLAY_DEVICEA) };

	while (EnumDisplayDevicesA(NULL, iDevNum, &DisplayDevice, EDD_GET_DEVICE_INTERFACE_NAME))
	{
		DWORD State = DisplayDevice.StateFlags;
		printf("%s\n", DisplayDevice.DeviceName);
		printf("  %s\n", DisplayDevice.DeviceString);
		printf("  Device state: %s%s%s%s%s%s\n",
			(State & DISPLAY_DEVICE_ACTIVE) ? "active" : "deactive",
			(State & DISPLAY_DEVICE_MIRRORING_DRIVER) ? " mirroring" : "",
			(State & DISPLAY_DEVICE_MODESPRUNED) ? " modespruned" : "",
			(State & DISPLAY_DEVICE_PRIMARY_DEVICE) ? " primary" : "",
			(State & DISPLAY_DEVICE_REMOVABLE) ? " removable" : "",
			(State & DISPLAY_DEVICE_VGA_COMPATIBLE) ? " vga" : "");
		if (EnumDisplaySettingsExA(DisplayDevice.DeviceName, ENUM_CURRENT_SETTINGS, &DevMode, 0))
		{
			printf("  Current mode: %ux%u, %u Hz\n", DevMode.dmPelsWidth, DevMode.dmPelsHeight, DevMode.dmDisplayFrequency);
		}
		iDevNum++;
	}
}
