# SPDX-License-Identifier: Unlicense

param (
	[Parameter(Mandatory=$true, HelpMessage="Target folder path")]
	[string]$TargetFolder
)

# Define the list of files to copy or download
$FilesToCopy = @(
	# Common files for all flavors
	"LICENSE",
	"README.pdf",
	"hw_report.ps1",
	"libnw\jep106.ids",
	"gnwinfo\gnwinfo.ini",
	"ioctl\PawnIOSetup.exe",
	"ioctl\pawn\AMDFamily0F.bin",
	"ioctl\pawn\AMDFamily10.bin",
	"ioctl\pawn\AMDFamily17.bin",
	"ioctl\pawn\IntelMSR.bin",
	"ioctl\pawn\RyzenSMU.bin",
	"ioctl\pawn\SmbusI801.bin",
	"ioctl\pawn\SmbusPIIX4.bin"
)

$FilesToDownload = @(
	"https://raw.githubusercontent.com/pciutils/pciids/master/pci.ids",
	"https://raw.githubusercontent.com/vcrhonek/hwdata/master/pnp.ids",
	"https://raw.githubusercontent.com/vcrhonek/hwdata/master/usb.ids"
)

# Function to handle local file copying
function Copy-LocalFile {
	param (
		[string]$SourceFile,
		[string]$DestinationFolder
	)

	try {
		if (-Not (Test-Path -Path $DestinationFolder)) {
			throw "Target folder does not exist: $DestinationFolder"
		}

		$DestinationFile = Join-Path -Path $DestinationFolder -ChildPath (Split-Path -Leaf $SourceFile)
		Copy-Item -Path $SourceFile -Destination $DestinationFile -Force

		Write-Output "Successfully copied local file $SourceFile to $DestinationFolder"
	} catch {
		Write-Error "Error copying local file: $_"
	}
}

# Function to handle file downloading
function Download-File {
	param (
		[string]$Url,
		[string]$DestinationFolder
	)

	try {
		if (-Not (Test-Path -Path $DestinationFolder)) {
			throw "Target folder does not exist: $DestinationFolder"
		}

		$DestinationFile = Join-Path -Path $DestinationFolder -ChildPath (Split-Path -Leaf $Url)
		Invoke-WebRequest -Uri $Url -OutFile $DestinationFile

		Write-Output "Successfully downloaded file from $Url to $DestinationFolder"
	} catch {
		Write-Error "Error downloading file: $_"
	}
}

if (-Not (Test-Path -Path $TargetFolder)) {
	try {
		New-Item -Path $TargetFolder -ItemType Directory -Force
		Write-Output "Target folder $TargetFolder has been created"
	} catch {
		Write-Error "Error creating target folder: $_"
		exit 1
	}
}

foreach ($File in $FilesToCopy) {
	if (Test-Path -Path $File) {
		Copy-LocalFile -SourceFile $File -DestinationFolder $TargetFolder
	} else {
		Write-Error "Local file does not exist: $File"
	}
}

foreach ($Url in $FilesToDownload) {
	Download-File -Url $Url -DestinationFolder $TargetFolder
}

# Rename vendor names in pci.ids
$PciIdsPath = Join-Path -Path $TargetFolder -ChildPath "pci.ids"
if (Test-Path -Path $PciIdsPath) {
	try {
		$content = Get-Content -Path $PciIdsPath
		$content = $content `
			-replace "NVIDIA Corporation", "NVIDIA" `
			-replace "Intel Corporation", "Intel" `
			-replace "Advanced Micro Devices, Inc. \[(.*?)\]", '$1'
		Set-Content -Path $PciIdsPath -Value $content
		Write-Output "Successfully modified $PciIdsPath"
	} catch {
		Write-Error "Error modifying ${PciIdsPath}: $_"
	}
}

# Add version info and remove invalid vendor codename from pnp.ids
$PnpIdsPath = Join-Path -Path $TargetFolder -ChildPath "pnp.ids"
if (Test-Path -Path $PnpIdsPath) {
	try {
		Add-Content -Path $PnpIdsPath -Value "# Version: $(Get-Date -Format 'yyyy.MM.dd')"
		$content = Get-Content -Path $PnpIdsPath -Raw
		$content = $content -replace "Invalid Vendor Codename - ", ""
		Set-Content -Path $PnpIdsPath -Value $content
		Write-Output "Successfully modified $PnpIdsPath"
	} catch {
		Write-Error "Error modifying ${PnpIdsPath}: $_"
	}
}
