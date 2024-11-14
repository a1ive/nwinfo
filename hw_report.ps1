# SPDX-License-Identifier: Unlicense

[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

$programPath = ".\nwinfo.exe"
$programArgs = @("--format=json",
	"--cp=utf8",
	"--human",
	"--sys",
	"--cpu",
	"--smbios",
	"--net=phys",
	"--disk=no-smart,phys",
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
Write-Output "`t`t$($cpuTable.'Temperature (C)') °C"
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

Write-Output "Network:"
foreach ($netTable in $parsedJson.'Network') {
	Write-Output "`t$($netTable.'Description')"
}

Write-Output "Disk:"
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
	Write-Output "`t`t$($diskTable.'Serial Number')"
}

Write-Output "Audio:"
$audioTable = $parsedJson.'Audio'
foreach ($i in $audioTable.PSObject.Properties.Name) {
	if ($i -ne "Default") {
		Write-Output "`t$($audioTable.$i.'Name')"
	}
}
