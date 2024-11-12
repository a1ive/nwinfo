[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

$programPath = ".\nwinfo.exe"
$programArgs = @("--format=json",
	"--cp=utf8",
	"--human",
	"--sys",
	"--cpu",
	"--smbios",
	"--net=phys",
	"--disk=no-smart",
	"--audio"
	)

$processOutput = & $programPath $programArgs

$parsedJson = $processOutput | ConvertFrom-Json

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

Write-Output "Network:"
foreach ($net in $parsedJson.'Network') {
	Write-Output "`t$($net.'Description')"
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
}

Write-Output "Audio:"
$audioTable = $parsedJson.'Audio'
foreach ($i in $audioTable.PSObject.Properties.Name) {
	if ($i -ne "Default") {
		Write-Output "`t$($audioTable.$i.'Name')"
	}
}
