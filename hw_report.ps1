# SPDX-License-Identifier: Unlicense

#[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

if (([Environment]::Is64BitOperatingSystem) -And (Test-Path ".\nwinfox64.exe")) {
	$programPath = ".\nwinfox64.exe"
} else {
	$programPath = ".\nwinfo.exe"
}

$programArgs = @("--format=json",
	"--cp=ansi",
	"--human",
	"--sys",
	"--cpu",
	"--uefi",
	"--gpu",
	"--display",
	"--smbios",
	"--net=phys",
	"--disk=phys",
	"--audio"
	)

$processOutput = & $programPath $programArgs

$parsedJson = $processOutput | ConvertFrom-Json

$dmiTable = @{}
foreach ($smbiosTable in $parsedJson.'SMBIOS') {
	$tableType = $smbiosTable.'Table Type'
	if ($tableType -ne $null) {
		if (-not $dmiTable.ContainsKey($tableType)) {
			$dmiTable[$tableType] = @()
		}
		$dmiTable[$tableType] += $smbiosTable
	}
}

Write-Output "NWinfo Hardware Report - $(Get-Date -Format 'yyyyMMdd HH:mm:ss')"
Write-Output "NWinfo $($parsedJson.'libnw') Copyright (c) 2024 A1ive"

Write-Output "OS:"
$systemTable = $parsedJson.'System'
Write-Output "`t$($systemTable.'Os')"
Write-Output "`t`t$($systemTable.'Build Number')"
Write-Output "`t`t$($systemTable.'Edition') $($systemTable.'Processor Architecture')"

Write-Output "CPU:"
$cpuTable = $parsedJson.'CPUID'.'CPU0'
Write-Output "`t$($cpuTable.'Brand')"
Write-Output "`t`t$($cpuTable.'Code Name')"
Write-Output "`t`t$($cpuTable.'Cores') cores $($cpuTable.'Logical CPUs') threads"
Write-Output "`t`t$($cpuTable.'Temperature (C)')°C"
if ($null -ne $dmiTable[4]) {
	Write-Output "`t`t$($dmiTable[4][0].'Socket Designation') socket"
}

Write-Output "RAM:"
$maxCapacity = "unknown"
$numSlots = "unknown"
if ($null -ne $dmiTable[16]) {
	$maxCapacity = $dmiTable[16][0].'Max Capacity'
	$numSlots = $dmiTable[16][0].'Number of Slots'
} elseif ($null -ne $dmiTable[5]) {
	$maxCapacity = "$($dmiTable[5][0].'Max Memory Module Size (MB)') MB"
	$numSlots = $dmiTable[5][0].'Number of Slots'
}
Write-Output "`tMax Capacity $maxCapacity $numSlots slots"
if ($null -ne $dmiTable[17]) {
	foreach ($mdTable in $dmiTable[17]) {
		Write-Output "`t$($mdTable.'Device Type')-$($mdTable.'Speed (MT/s)') $($mdTable.'Device Size') $($mdTable.'Manufacturer')"
		Write-Output "`t`t$($mdTable.'Device Locator') $($mdTable.'Bank Locator') $($mdTable.'Form Factor')"
		Write-Output "`t`t$($mdTable.'Serial Number') $($mdTable.'Part Number')"
	}
}

Write-Output "Firmware:"
$fwTable = $parsedJson.'UEFI'
Write-Output "`t$($fwTable.'Firmware Type')"
Write-Output "`tSecure Boot $($fwTable.'Secure Boot')"
if ($null -ne $dmiTable[0]) {
	Write-Output "`t$($dmiTable[0][0].'Vendor')"
	Write-Output "`t$($dmiTable[0][0].'Version')"
	Write-Output "`t$($dmiTable[0][0].'Release Date') $($dmiTable[0][0].'System BIOS Version')"
}

Write-Output "Base Board:"
if ($null -ne $dmiTable[2]) {
	Write-Output "`t$($dmiTable[2][0].'Manufacturer') $($dmiTable[2][0].'Product Name') $($dmiTable[2][0].'Version')"
	Write-Output "`t$($dmiTable[2][0].'Serial Number')"
}

Write-Output "Graphics:"
foreach ($gpuTable in $parsedJson.'GPU') {
	Write-Output "`t$($gpuTable.'Memory Size') $($gpuTable.'Device')"
}
foreach ($displayTable in $parsedJson.'Display') {
	if ($null -ne $displayTable.'Manufacturer') {
		Write-Output "`t$($displayTable.'Manufacturer') $($displayTable.'ID') ($($displayTable.'Max Resolution')@$($displayTable.'Max Refresh Rate (Hz)')) $($displayTable.'Diagonal (in)')'"
	}
}

Write-Output "Network:"
foreach ($netTable in $parsedJson.'Network') {
	Write-Output "`t$($netTable.'Description')"
}

Write-Output "Storage:"
foreach ($diskTable in $parsedJson.'Disks') {
	Write-Output "`t$($diskTable.'HW Name')"
	$diskType = "HDD"
	if ($diskTable.'Path'.StartsWith("\\.\CdRom")) {
		$diskType = "CD-ROM"
	}
	elseif ($diskTable.'SSD' -eq "Yes") {
		$diskType = "SSD"
	}
	Write-Output "`t`t$($diskTable.'Size') $diskType $($diskTable.'Type')"
	if ($null -ne $diskTable.'Serial Number') {
		Write-Output "`t`t$($diskTable.'Serial Number')"
	}
	if ($null -ne $diskTable.'Health Status') {
		Write-Output "`t`t$($diskTable.'Temperature (C)')°C $($diskTable.'Health Status')"
	}
}

Write-Output "Audio:"
$audioTable = $parsedJson.'Audio'
foreach ($i in $audioTable.PSObject.Properties.Name) {
	if ($i -ne "Default") {
		Write-Output "`t$($audioTable.$i.'Name')"
	}
}
