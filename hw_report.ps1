#
# SPDX-License-Identifier: Unlicense

# Set the working directory to the script's location
Set-Location -Path (Split-Path -Parent -Path $MyInvocation.MyCommand.Path)

# Check if the script is running with administrator privileges
if (!([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] 'Administrator')) {
	Start-Process -FilePath PowerShell.exe -Verb Runas -ArgumentList "-File `"$($MyInvocation.MyCommand.Path)`" `"$($MyInvocation.MyCommand.UnboundArguments)`""
	Exit
}

# Add the required assembly for GUI components
Add-Type -AssemblyName System.Windows.Forms

# Hide the powershell window: https://stackoverflow.com/a/27992426/1069307
Add-Type -Name Window -Namespace WinAPI -MemberDefinition '
	[DllImport("user32.dll")]
	public static extern bool ShowWindow(int handle, int state);'
$handle = (Get-Process -Id $PID).MainWindowHandle
[WinAPI.Window]::ShowWindow($handle, 0)

# Create a form to display the log
$logForm = New-Object System.Windows.Forms.Form
$logForm.Text = "Loading ..."
$logForm.Size = New-Object System.Drawing.Size(800, 600)
$logForm.StartPosition = "CenterScreen"
$logForm.TopMost = $true

# Add a text box to the log form
$logTextBox = New-Object System.Windows.Forms.TextBox
$logTextBox.Multiline = $true
$logTextBox.ScrollBars = "Vertical"
$logTextBox.ReadOnly = $true
$logTextBox.Dock = "Fill"
$logForm.Controls.Add($logTextBox)

# Show the log form
$logForm.Show()

# Define a helper function to log messages
function Log-Message {
	param([string]$message)
	$logTextBox.AppendText("[$(Get-Date -Format 'HH:mm:ss')] " + $message + "`r`n")
	[System.Windows.Forms.Application]::DoEvents()
}

try {
	# Determine the appropriate nwinfo executable based on the OS architecture
	Log-Message "Checking OS architecture..."
	if (([Environment]::Is64BitOperatingSystem) -And (Test-Path ".\nwinfox64.exe")) {
		$programPath = ".\nwinfox64.exe"
	} elseif (Test-Path ".\nwinfo.exe") {
		$programPath = ".\nwinfo.exe"
	} else {
		throw "nwinfo.exe not found."
	}
	Log-Message "Using $programPath..."

	Log-Message "Loading icon from $programPath..."
	$mainIcon = [System.Drawing.Icon]::ExtractAssociatedIcon((Resolve-Path $programPath).Path)

	# Define arguments to pass to the nwinfo executable
	$programArgs = @("--format=json",
		"--cp=utf8",
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
	Log-Message "Defining arguments $programArgs..."
	# Execute nwinfo and capture its output
	Log-Message "Executing nwinfo..."
	$processOutput = & $programPath $programArgs

	# Ensure the output is UTF-8 encoded
	# [Console]::OutputEncoding = [System.Text.Encoding]::UTF8
	Log-Message "Processing nwinfo output..."
	$utf8Bytes = [System.Text.Encoding]::Default.GetBytes($processOutput)
	$utf8Json = [System.Text.Encoding]::UTF8.GetString($utf8Bytes)

	# Parse the JSON output into a PowerShell object
	Log-Message "Parsing JSON..."
	$parsedJson = $utf8Json | ConvertFrom-Json

	# Initialize a dictionary to group SMBIOS data by Table Type
	Log-Message "Parsing SMBIOS..."
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

	# Prepare the hardware report text
	Log-Message "Generating hardware report..."
	$outputText = @()
	$outputText += "NWinfo Hardware Report - $(Get-Date -Format 'yyyyMMdd HH:mm:ss')"
	$outputText += "NWinfo $($parsedJson.'libnw') Copyright (c) 2024 A1ive"

	$outputText += "OS:"
	$systemTable = $parsedJson.'System'
	$outputText += "`t$($systemTable.'Os')"
	$outputText += "`t`t$($systemTable.'Build Number')"
	$outputText += "`t`t$($systemTable.'Edition') $($systemTable.'Processor Architecture')"

	$outputText += "CPU:"
	$cpuTable = $parsedJson.'CPUID'.'CPU0'
	$outputText += "`t$($cpuTable.'Brand')"
	$outputText += "`t`t$($cpuTable.'Code Name')"
	$outputText += "`t`t$($cpuTable.'Cores') cores $($cpuTable.'Logical CPUs') threads"
	$outputText += "`t`t$($cpuTable.'Temperature (C)')°C"
	if ($null -ne $dmiTable[4]) {
		$outputText += "`t`t$($dmiTable[4][0].'Socket Designation') socket"
	}

	$outputText += "RAM:"
	$maxCapacity = "unknown"
	$numSlots = "unknown"
	if ($null -ne $dmiTable[16]) {
		$maxCapacity = $dmiTable[16][0].'Max Capacity'
		$numSlots = $dmiTable[16][0].'Number of Slots'
	} elseif ($null -ne $dmiTable[5]) {
		$maxCapacity = "$($dmiTable[5][0].'Max Memory Module Size (MB)') MB"
		$numSlots = $dmiTable[5][0].'Number of Slots'
	}
	$outputText += "`tMax Capacity $maxCapacity $numSlots slots"
	if ($null -ne $dmiTable[17]) {
		foreach ($mdTable in $dmiTable[17]) {
			$outputText += "`t$($mdTable.'Device Type')-$($mdTable.'Speed (MT/s)') $($mdTable.'Device Size') $($mdTable.'Manufacturer')"
			$outputText += "`t`t$($mdTable.'Device Locator') $($mdTable.'Bank Locator') $($mdTable.'Form Factor')"
			$outputText += "`t`t$($mdTable.'Serial Number') $($mdTable.'Part Number')"
		}
	}

	$outputText += "Firmware:"
	$fwTable = $parsedJson.'UEFI'
	$outputText += "`t$($fwTable.'Firmware Type')"
	$outputText += "`tSecure Boot $($fwTable.'Secure Boot')"
	if ($null -ne $dmiTable[0]) {
		$outputText += "`t$($dmiTable[0][0].'Vendor')"
		$outputText += "`t$($dmiTable[0][0].'Version')"
		$outputText += "`t$($dmiTable[0][0].'Release Date') $($dmiTable[0][0].'System BIOS Version')"
	}

	$outputText += "Base Board:"
	if ($null -ne $dmiTable[2]) {
		$outputText += "`t$($dmiTable[2][0].'Manufacturer') $($dmiTable[2][0].'Product Name') $($dmiTable[2][0].'Version')"
		$outputText += "`t$($dmiTable[2][0].'Serial Number')"
	}

	$outputText += "Graphics:"
	foreach ($gpuTable in $parsedJson.'GPU') {
		$outputText += "`t$($gpuTable.'Memory Size') $($gpuTable.'Device')"
	}
	foreach ($displayTable in $parsedJson.'Display') {
		if ($null -ne $displayTable.'Manufacturer') {
			$outputText += "`t$($displayTable.'Manufacturer') $($displayTable.'ID') ($($displayTable.'Max Resolution')@$($displayTable.'Max Refresh Rate (Hz)')) $($displayTable.'Diagonal (in)')'"
		}
	}

	$outputText += "Network:"
	foreach ($netTable in $parsedJson.'Network') {
		$outputText += "`t$($netTable.'Description')"
	}

	$outputText += "Storage:"
	foreach ($diskTable in $parsedJson.'Disks') {
		$outputText += "`t$($diskTable.'HW Name')"
		$diskType = "HDD"
		if ($diskTable.'Path'.StartsWith("\\.\CdRom")) {
			$diskType = "CD-ROM"
		}
		elseif ($diskTable.'SSD' -eq "Yes") {
			$diskType = "SSD"
		}
		$outputText += "`t`t$($diskTable.'Size') $diskType $($diskTable.'Type')"
		if ($null -ne $diskTable.'Serial Number') {
			$outputText += "`t`t$($diskTable.'Serial Number')"
		}
		if ($null -ne $diskTable.'Health Status') {
			$outputText += "`t`t$($diskTable.'Temperature (C)')°C $($diskTable.'Health Status')"
		}
	}

	$outputText += "Audio:"
	$audioTable = $parsedJson.'Audio'
	foreach ($i in $audioTable.PSObject.Properties.Name) {
		if ($i -ne "Default") {
			$outputText += "`t$($audioTable.$i.'Name')"
		}
	}
} catch {
	[System.Windows.Forms.MessageBox]::Show($_, "Error", [System.Windows.Forms.MessageBoxButtons]::OK, [System.Windows.Forms.MessageBoxIcon]::Error)
	$logForm.Close()
	Exit
}

$logForm.Close()

# Write-Output $outputText

# Create a window to display the report
$mainForm = New-Object System.Windows.Forms.Form
$mainForm.Text = "Hardware Report"
$mainForm.Size = New-Object System.Drawing.Size(800, 600)
$mainForm.StartPosition = "CenterScreen"
# Set the form's icon from the nwinfo executable
$mainForm.Icon = $mainIcon

# Add a text box for the report
$textBox = New-Object System.Windows.Forms.TextBox
$textBox.Multiline = $true
$textBox.ScrollBars = "Vertical"
$textBox.ReadOnly = $true
$textBox.Dock = "Fill"
$textBox.Text = $outputText -join "`r`n"

$mainForm.Controls.Add($textBox)

# Show the form
$mainForm.ShowDialog()
