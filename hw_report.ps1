#
# SPDX-License-Identifier: Unlicense

# Set the working directory to the script's location
if ($PSScriptRoot) { Set-Location -Path $PSScriptRoot }

# Check if the script is running with administrator privileges
if (!([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] 'Administrator')) {
	Start-Process -FilePath PowerShell.exe -Verb Runas -ArgumentList "-File `"$($MyInvocation.MyCommand.Path)`" `"$($MyInvocation.MyCommand.UnboundArguments)`""
	Exit
}

# Add required assemblies for WPF
Add-Type -AssemblyName PresentationFramework
Add-Type -AssemblyName PresentationCore
Add-Type -AssemblyName WindowsBase
Add-Type -AssemblyName System.Windows.Forms

# Hide the powershell window: https://stackoverflow.com/a/27992426/1069307
if (-not ("WinAPI.Window" -as [type])) {
	Add-Type -Name Window -Namespace WinAPI -MemberDefinition @'
		[DllImport("user32.dll")]
		public static extern bool ShowWindow(int handle, int state);
'@
}
$handle = (Get-Process -Id $PID).MainWindowHandle
[WinAPI.Window]::ShowWindow($handle, 0)

# Global variable to store report text
$script:reportText = ""

# XAML for loading dialog
$loadingXaml = @'
<Window xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
        xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
        Title="Loading..." Height="400" Width="600" 
        WindowStartupLocation="CenterScreen" 
        Background="White" ResizeMode="CanMinimize" Topmost="True">
    <Grid>
        <ScrollViewer VerticalScrollBarVisibility="Auto" Margin="10">
            <TextBlock Name="LogTextBlock" TextWrapping="Wrap" 
                      Foreground="Black" FontFamily="Consolas" FontSize="12"
                      Background="Transparent"/>
        </ScrollViewer>
    </Grid>
</Window>
'@

# Create loading window
$loadingReader = [System.Xml.XmlNodeReader]::new([xml]$loadingXaml)
$loadingWindow = [Windows.Markup.XamlReader]::Load($loadingReader)
$logTextBlock = $loadingWindow.FindName("LogTextBlock")

# Show loading window
$loadingWindow.Show()

# Helper function to log messages
function Write-Message {
	param([string]$message)
	$timestamp = Get-Date -Format 'HH:mm:ss'
	$logTextBlock.Text += "[$timestamp] $message`n"
	[System.Windows.Threading.Dispatcher]::CurrentDispatcher.Invoke([Action] {}, [System.Windows.Threading.DispatcherPriority]::Background)
}

# Helper function to extract icons from DLL
Add-Type -ReferencedAssemblies 'System.Drawing' -TypeDefinition @"
using System;
using System.Runtime.InteropServices;
using System.Drawing;

public class IconExtractor {
	[DllImport("shell32.dll", CharSet = CharSet.Auto)]
	private static extern IntPtr ExtractIcon(IntPtr hInst, string lpszExeFileName, int nIconIndex);
	
	[DllImport("user32.dll", CharSet = CharSet.Auto)]
	private static extern bool DestroyIcon(IntPtr handle);
	
	[DllImport("gdi32.dll")]
	private static extern bool DeleteObject(IntPtr hObject);
	
	public static Icon ExtractIconFromFile(string file, int index) {
		IntPtr hIcon = ExtractIcon(IntPtr.Zero, file, index);
		if (hIcon == IntPtr.Zero) return null;
		Icon icon = (Icon)Icon.FromHandle(hIcon).Clone();
		DestroyIcon(hIcon);
		return icon;
	}
	
	public static void DeleteBitmap(IntPtr hBitmap) {
		DeleteObject(hBitmap);
	}
}
"@

# Helper function to get system icon from imageres.dll
function Get-SystemIcon {
	param([string]$iconType)
	
	# Icon indices in imageres.dll
	$iconMap = @{
		"OS"         = 249
		"CPU"        = 144
		"RAM"        = 142
		"Firmware"   = 29
		"Base Board" = 87
		"Graphics"   = 96
		"Network"    = 138
		"Storage"    = 25
		"Audio"      = 103
	}
	
	$dllPath = "$env:SystemRoot\System32\imageres.dll"
	$iconIndex = $iconMap[$iconType]
	
	$icon = [IconExtractor]::ExtractIconFromFile($dllPath, $iconIndex)
	
	if ($null -ne $icon) {
		# Convert to BitmapSource for WPF
		$bitmap = $icon.ToBitmap()
		$hBitmap = $bitmap.GetHbitmap()
		
		$imageSource = [System.Windows.Interop.Imaging]::CreateBitmapSourceFromHBitmap(
			$hBitmap,
			[System.IntPtr]::Zero,
			[System.Windows.Int32Rect]::Empty,
			[System.Windows.Media.Imaging.BitmapSizeOptions]::FromEmptyOptions()
		)
		
		# Clean up - use DeleteObject for HBITMAP
		[IconExtractor]::DeleteBitmap($hBitmap)
		$bitmap.Dispose()
		$icon.Dispose()
		
		return $imageSource
	}
	
	return $null
}

try {
	# Determine the appropriate nwinfo executable based on the OS architecture
	Write-Message "Checking OS architecture..."
	if ([Environment]::Is64BitOperatingSystem) {
		if (Test-Path ".\nwinfo.exe") {
			$programPath = ".\nwinfo.exe"
		}
		else {
			throw "nwinfo.exe not found."
		}
	}
 else {
		if (Test-Path ".\nwinfox86.exe") {
			$programPath = ".\nwinfox86.exe"
		}
		elseif (Test-Path ".\nwinfo.exe") {
			$programPath = ".\nwinfo.exe"
		}
		else {
			throw "nwinfo.exe not found."
		}
	}
	Write-Message "Using $programPath..."

	Write-Message "Loading icon from $programPath..."
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
	Write-Message "Defining arguments..."
	
	# Execute nwinfo
	Write-Message "Executing nwinfo..."
	$psi = New-Object System.Diagnostics.ProcessStartInfo
	$psi.FileName = (Resolve-Path $programPath).Path
	$psi.Arguments = $programArgs -join " "
	$psi.RedirectStandardOutput = $true
	$psi.UseShellExecute = $false
	$psi.StandardOutputEncoding = [System.Text.Encoding]::UTF8

	$proc = [System.Diagnostics.Process]::Start($psi)
	$processOutput = $proc.StandardOutput.ReadToEnd()
	$proc.WaitForExit()

	if ($proc.HasExited -and $proc.ExitCode -ne 0) {
		throw "nwinfo exited with code $($proc.ExitCode)."
	}

	Write-Message "Processing nwinfo output..."
	$utf8Json = $processOutput

	# Parse the JSON output into a PowerShell object
	Write-Message "Parsing JSON..."
	$parsedJson = $utf8Json | ConvertFrom-Json -ErrorAction Stop

	# Parse SMBIOS
	Write-Message "Parsing SMBIOS..."
	$dmiTable = @{}
	foreach ($smbiosTable in $parsedJson.'SMBIOS') {
		$tableType = $smbiosTable.'Table Type'
		if ($null -ne $tableType) {
			if (-not $dmiTable.ContainsKey($tableType)) {
				$dmiTable[$tableType] = @()
			}
			$dmiTable[$tableType] += $smbiosTable
		}
	}

	# Generate report
	Write-Message "Generating hardware report..."
	$outputText = @()
	$outputText += "NWinfo Hardware Report - $(Get-Date -Format 'yyyyMMdd HH:mm:ss')"
	$outputText += "NWinfo $($parsedJson.'libnw') Copyright (c) 2024 A1ive"
	$outputText += ""

	$outputText += "OS:"
	$systemTable = $parsedJson.'System'
	$outputText += "`t$($systemTable.'Os')"
	$outputText += "`t`t$($systemTable.'Build Number') $($systemTable.'UBR')"
	$outputText += "`t`t$($systemTable.'Edition') $($systemTable.'Processor Architecture')"
	$outputText += "`t`t$($systemTable.'Username') $($systemTable.'Computer Name')"
	$outputText += ""

	$outputText += "CPU:"
	$cpuTable = $parsedJson.'CPUID'.'CPU0'
	$outputText += "`t$($cpuTable.'Brand')"
	$outputText += "`t`t$($cpuTable.'Code Name')"
	$outputText += "`t`t$($cpuTable.'Cores') cores $($cpuTable.'Logical CPUs') threads"
	$outputText += "`t`t$($cpuTable.'Temperature (C)')$([char]0xB0)C $($cpuTable.'Core Voltage (V)')V"
	if ($null -ne $dmiTable[4]) {
		$outputText += "`t`t$($dmiTable[4][0].'Processor Upgrade') ($($dmiTable[4][0].'Socket Designation'))"
	}
	$outputText += ""

	$outputText += "RAM:"
	$maxCapacity = "unknown"
	$numSlots = "unknown"
	if ($null -ne $dmiTable[16]) {
		$maxCapacity = $dmiTable[16][0].'Max Capacity'
		$numSlots = $dmiTable[16][0].'Number of Slots'
	}
	elseif ($null -ne $dmiTable[5]) {
		$maxCapacity = "$($dmiTable[5][0].'Max Memory Module Size (MB)') MB"
		$numSlots = $dmiTable[5][0].'Number of Slots'
	}
	$outputText += "`tMax Capacity $maxCapacity $numSlots slots"
	if ($null -ne $dmiTable[17]) {
		foreach ($mdTable in $dmiTable[17]) {
			if ($null -eq $mdTable.'Device Size') { continue }
			$outputText += "`t$($mdTable.'Device Type')-$($mdTable.'Speed (MT/s)') $($mdTable.'Device Size') $($mdTable.'Manufacturer')"
			$outputText += "`t`t$($mdTable.'Device Locator') $($mdTable.'Bank Locator') $($mdTable.'Form Factor')"
			$outputText += "`t`t$($mdTable.'Serial Number') $($mdTable.'Part Number')"
		}
	}
	$outputText += ""

	$outputText += "Firmware:"
	$fwTable = $parsedJson.'UEFI'
	$outputText += "`t$($fwTable.'Firmware Type')"
	$outputText += "`tSecure Boot $($fwTable.'Secure Boot')"
	if ($null -ne $dmiTable[0]) {
		$outputText += "`t$($dmiTable[0][0].'Vendor')"
		$outputText += "`t$($dmiTable[0][0].'Version')"
		$outputText += "`t$($dmiTable[0][0].'Release Date') $($dmiTable[0][0].'Platform Firmware Version')"
	}
	$outputText += ""

	$outputText += "Base Board:"
	if ($null -ne $dmiTable[2]) {
		$outputText += "`t$($dmiTable[2][0].'Manufacturer') $($dmiTable[2][0].'Product Name') $($dmiTable[2][0].'Version')"
		$outputText += "`t$($dmiTable[2][0].'Serial Number')"
	}
	$outputText += ""

	$outputText += "Graphics:"
	foreach ($gpuTable in $parsedJson.'GPU') {
		$outputText += "`t$($gpuTable.'Total Memory') $($gpuTable.'Device')"
	}
	foreach ($displayTable in $parsedJson.'Display') {
		if ($null -ne $displayTable.'Manufacturer') {
			$outputText += "`t$($displayTable.'Manufacturer') $($displayTable.'ID') ($($displayTable.'Max Resolution')@$($displayTable.'Max Refresh Rate (Hz)')Hz) $($displayTable.'Diagonal (in)')'"
		}
	}
	$outputText += ""

	$outputText += "Network:"
	foreach ($netTable in $parsedJson.'Network') {
		$outputText += "`t$($netTable.'Description')"
	}
	$outputText += ""

	$outputText += "Storage:"
	foreach ($diskTable in $parsedJson.'Disks') {
		$outputText += "`t$($diskTable.'HW Name')"
		$diskType = "HDD"
		if ($diskTable.'Path'.StartsWith("\\.\CdRom")) {
			$diskType = "CD-ROM"
		}
		elseif ($diskTable.'SSD' -eq $true) {
			$diskType = "SSD"
		}
		$outputText += "`t`t$($diskTable.'Size') $diskType $($diskTable.'Type')"
		if ($null -ne $diskTable.'Serial Number') {
			$outputText += "`t`t$($diskTable.'Serial Number')"
		}
		if ($null -ne $diskTable.'Health Status') {
			$outputText += "`t`t$($diskTable.'Temperature (C)')$([char]0xB0)C $($diskTable.'Health Status')"
		}
	}
	$outputText += ""

	$outputText += "Audio:"
	$audioTable = $parsedJson.'Audio'
	foreach ($i in $audioTable.PSObject.Properties.Name) {
		if ($i -ne "Default") {
			$outputText += "`t$($audioTable.$i.'Name')"
		}
	}

	$script:reportText = $outputText -join "`r`n"

}
catch {
	[System.Windows.MessageBox]::Show($_, "Error", [System.Windows.MessageBoxButton]::OK, [System.Windows.MessageBoxImage]::Error)
	$loadingWindow.Close()
	Exit
}

$loadingWindow.Close()

# Main WPF Window XAML
$mainXaml = @'
<Window xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
        xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
        Title="Hardware Report" Height="700" Width="900"
        WindowStartupLocation="CenterScreen"
        Background="#FFF0F0F0">
    <Window.Resources>
        <!-- Expander Style -->
        <Style x:Key="ExpanderStyle" TargetType="Expander">
            <Setter Property="Background" Value="White"/>
            <Setter Property="Margin" Value="10,5"/>
            <Setter Property="BorderBrush" Value="#FFCCCCCC"/>
            <Setter Property="BorderThickness" Value="1"/>
            <Setter Property="IsExpanded" Value="True"/>
            <Setter Property="Effect">
                <Setter.Value>
                    <DropShadowEffect Color="#40000000" Direction="270" ShadowDepth="1" 
                                    BlurRadius="4" Opacity="0.3"/>
                </Setter.Value>
            </Setter>
        </Style>
        
        <!-- Card Style -->
        <Style x:Key="CardStyle" TargetType="Border">
            <Setter Property="Background" Value="White"/>
            <Setter Property="CornerRadius" Value="4"/>
            <Setter Property="Margin" Value="10,5"/>
            <Setter Property="Padding" Value="15,10"/>
            <Setter Property="BorderBrush" Value="#FFCCCCCC"/>
            <Setter Property="BorderThickness" Value="1"/>
            <Setter Property="Effect">
                <Setter.Value>
                    <DropShadowEffect Color="#40000000" Direction="270" ShadowDepth="1" 
                                    BlurRadius="4" Opacity="0.3"/>
                </Setter.Value>
            </Setter>
        </Style>
        
        <!-- Section Header Style -->
        <Style x:Key="SectionHeaderStyle" TargetType="TextBlock">
            <Setter Property="FontSize" Value="16"/>
            <Setter Property="FontWeight" Value="SemiBold"/>
            <Setter Property="Foreground" Value="#FF0066CC"/>
            <Setter Property="Margin" Value="0,0,0,0"/>
        </Style>
        
        <!-- Detail Text Style -->
        <Style x:Key="DetailTextStyle" TargetType="TextBlock">
            <Setter Property="FontSize" Value="12"/>
            <Setter Property="Foreground" Value="Black"/>
            <Setter Property="Margin" Value="30,2,0,2"/>
            <Setter Property="TextWrapping" Value="Wrap"/>
        </Style>
        
        <!-- Icon Style -->
        <Style x:Key="IconStyle" TargetType="Image">
            <Setter Property="Width" Value="20"/>
            <Setter Property="Height" Value="20"/>
            <Setter Property="VerticalAlignment" Value="Center"/>
            <Setter Property="Margin" Value="0,0,8,0"/>
        </Style>
    </Window.Resources>
    
    <DockPanel>
        <!-- Menu Bar -->
        <Menu DockPanel.Dock="Top" Height="22">
            <MenuItem Header="File">
                <MenuItem Name="SaveMenuItem" Header="Save Report..."/>
                <Separator/>
                <MenuItem Name="ExitMenuItem" Header="Exit"/>
            </MenuItem>
            <MenuItem Header="Edit">
                <MenuItem Name="CopyMenuItem" Header="Copy to Clipboard"/>
            </MenuItem>
        </Menu>
        
        <!-- Main Content -->
        <ScrollViewer VerticalScrollBarVisibility="Auto" Background="#FFF0F0F0">
            <StackPanel Name="MainPanel" Margin="15">
                <!-- Title -->
                <TextBlock Text="Hardware Report" FontSize="24" FontWeight="Bold" 
                          Foreground="#FF003366" Margin="10,10,10,5"/>
                <TextBlock Name="TimestampText" FontSize="11" Foreground="#FF666666" 
                          Margin="10,0,10,20"/>
            </StackPanel>
        </ScrollViewer>
    </DockPanel>
</Window>
'@

# Create main window
$mainReader = [System.Xml.XmlNodeReader]::new([xml]$mainXaml)
$mainWindow = [Windows.Markup.XamlReader]::Load($mainReader)

# Get controls
$mainPanel = $mainWindow.FindName("MainPanel")
$timestampText = $mainWindow.FindName("TimestampText")
$saveMenuItem = $mainWindow.FindName("SaveMenuItem")
$exitMenuItem = $mainWindow.FindName("ExitMenuItem")
$copyMenuItem = $mainWindow.FindName("CopyMenuItem")

# Set icon
$mainWindow.Icon = [System.Windows.Interop.Imaging]::CreateBitmapSourceFromHIcon(
	$mainIcon.Handle,
	[System.Windows.Int32Rect]::Empty,
	[System.Windows.Media.Imaging.BitmapSizeOptions]::FromEmptyOptions()
)

# Parse and display report
$lines = $script:reportText -split "`r`n"
$timestampText.Text = $lines[0]
$currentCard = $null

for ($i = 2; $i -lt $lines.Count; $i++) {
	$line = $lines[$i]
	
	if ($line -eq "") { continue }
	
	# Check if it's a section header (OS:, CPU:, etc.)
	if ($line -match '^(OS|CPU|RAM|Firmware|Base Board|Graphics|Network|Storage|Audio):$') {
		$sectionName = $matches[1]
		
		# Create expander for collapsible section
		$expander = New-Object System.Windows.Controls.Expander
		$expander.Style = $mainWindow.Resources["ExpanderStyle"]
		
		# Header with icon
		$headerPanel = New-Object System.Windows.Controls.StackPanel
		$headerPanel.Orientation = "Horizontal"
		
		# Get and add icon
		$iconSource = Get-SystemIcon $sectionName
		if ($null -ne $iconSource) {
			$iconImage = New-Object System.Windows.Controls.Image
			$iconImage.Style = $mainWindow.Resources["IconStyle"]
			$iconImage.Source = $iconSource
			$headerPanel.AddChild($iconImage)
		}
		
		$headerText = New-Object System.Windows.Controls.TextBlock
		$headerText.Style = $mainWindow.Resources["SectionHeaderStyle"]
		$headerText.Text = $sectionName
		
		$headerPanel.AddChild($headerText)
		$expander.Header = $headerPanel
		
		# Create content panel
		$contentPanel = New-Object System.Windows.Controls.StackPanel
		$contentPanel.Margin = New-Object System.Windows.Thickness(15, 10, 15, 10)
		$expander.Content = $contentPanel
		
		$mainPanel.AddChild($expander)

		$currentCard = $contentPanel
	}
	elseif ($null -ne $currentCard) {
		# Add detail line
		$detailText = New-Object System.Windows.Controls.TextBlock
		$detailText.Style = $mainWindow.Resources["DetailTextStyle"]
		$detailText.Text = $line.TrimStart("`t")
		
		# Adjust margin based on indent level
		$indentLevel = ($line -replace '[^\t].*$', '').Length
		$detailText.Margin = New-Object System.Windows.Thickness((15 + $indentLevel * 20), 3, 0, 3)
		
		$currentCard.AddChild($detailText)
	}
}

# Menu event handlers
$saveMenuItem.Add_Click({
		$saveDialog = New-Object System.Windows.Forms.SaveFileDialog
		$saveDialog.Filter = "Text Files (*.txt)|*.txt|All Files (*.*)|*.*"
		$saveDialog.FileName = "HardwareReport_$(Get-Date -Format 'yyyyMMdd_HHmmss').txt"
	
		if ($saveDialog.ShowDialog() -eq [System.Windows.Forms.DialogResult]::OK) {
			try {
				[System.IO.File]::WriteAllText($saveDialog.FileName, $script:reportText, [System.Text.Encoding]::UTF8)
				[System.Windows.MessageBox]::Show("Report saved successfully!", "Success", 
					[System.Windows.MessageBoxButton]::OK, [System.Windows.MessageBoxImage]::Information)
			}
			catch {
				[System.Windows.MessageBox]::Show("Failed to save report: $_", "Error", 
					[System.Windows.MessageBoxButton]::OK, [System.Windows.MessageBoxImage]::Error)
			}
		}
	})

$copyMenuItem.Add_Click({
		try {
			[System.Windows.Clipboard]::SetText($script:reportText)
			[System.Windows.MessageBox]::Show("Report copied to clipboard!", "Success", 
				[System.Windows.MessageBoxButton]::OK, [System.Windows.MessageBoxImage]::Information)
		}
		catch {
			[System.Windows.MessageBox]::Show("Failed to copy to clipboard: $_", "Error", 
				[System.Windows.MessageBoxButton]::OK, [System.Windows.MessageBoxImage]::Error)
		}
	})

$exitMenuItem.Add_Click({
		$mainWindow.Close()
	})

# Show main window
$null = $mainWindow.ShowDialog()
