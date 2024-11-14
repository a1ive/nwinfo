# SPDX-License-Identifier: Unlicense

param (
	[Parameter(Mandatory=$true, HelpMessage="Target folder path")]
	[string]$TargetFolder
)

# Define the list of files to copy or download
$FilesToCopy = @(
	"LICENSE",
	"hw_report.ps1",
	"libnw\jep106.ids",
	"winring0\HwRwDrv.sys",
	"winring0\HwRwDrvx64.sys"
)

$FilesToDownload = @(
	"https://raw.githubusercontent.com/pciutils/pciids/master/pci.ids",
	"https://raw.githubusercontent.com/vcrhonek/hwdata/master/pnp.ids",
	"http://www.linux-usb.org/usb.ids",
	"https://github.com/a1ive/libcdi/releases/download/latest/libcdi.zip"
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

# Extract libcdi.zip and copy dlls
$LibcdiZipPath = Join-Path -Path $TargetFolder -ChildPath "libcdi.zip"
if (Test-Path -Path $LibcdiZipPath) {
	$ExtractPath = Join-Path -Path $TargetFolder -ChildPath "dll"
	try {
		# Extract the zip file
		Expand-Archive -Path $LibcdiZipPath -DestinationPath $ExtractPath -Force
		# Copy the DLL files
		$LibcdiX86Dll = Join-Path -Path $ExtractPath -ChildPath "x86\libcdi.dll"
		$LibcdiX64Dll = Join-Path -Path $ExtractPath -ChildPath "x64\libcdi.dll"
		if (Test-Path -Path $LibcdiX86Dll) {
			Copy-Item -Path $LibcdiX86Dll -Destination (Join-Path -Path $TargetFolder -ChildPath "libcdi.dll") -Force
		} else {
			Write-Error "File not found: $LibcdiX86Dll"
		}
		if (Test-Path -Path $LibcdiX64Dll) {
			Copy-Item -Path $LibcdiX64Dll -Destination (Join-Path -Path $TargetFolder -ChildPath "libcdix64.dll") -Force
		} else {
			Write-Error "File not found: $LibcdiX64Dll"
		}
		# Clean up extracted files
		Remove-Item -Path $ExtractPath -Recurse -Force
		Remove-Item -Path $LibcdiZipPath -Force
		Write-Output "Successfully extracted and copied DLLs from $LibcdiZipPath"
	} catch {
		Write-Error "Error extracting or copying DLLs from ${LibcdiZipPath}: $_"
	}
}
