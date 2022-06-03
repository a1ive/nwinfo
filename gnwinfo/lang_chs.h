// SPDX-License-Identifier: Unlicense

#include "gnwinfo.h"

static GNW_LANG GNW_LANG_CHS[] =
{
	{"Loading, please wait ...", "加载中，请稍候 ..."},
	{"File", "文件"},
	{"Refresh", "刷新"},
	{"Export", "导出"},
	{"Exit", "退出"},
	{"Help", "帮助"},
	{"Homepage", "主页"},
	{"About", "关于"},

	{"Name", "名称"},
	{"Attribute", "属性"},
	{"Data", "数据"},

	{"ACPI Table", "ACPI 表"},
	{"Processor", "处理器"},
	{"Physical Storage", "磁盘"},
	{"Display Devices", "显示设备"},
	{"Network Adapter", "网络适配器"},
	{"PCI Devices", "PCI 设备"},
	{"SMBIOS", "SMBIOS"},
	{"Memory SPD", "内存 SPD"},
	{"Operating System", "操作系统"},
	{"USB Devices", "USB 设备"},

	{"Address", "地址"},
	{"Anycast Address", "任播地址"},
	{"Bank Locator", "Bank 定位"},
	{"Bits per color", "位/颜色"},
	{"Brand", "品牌"},
	{"Bus Clock (MHz)", "总线时钟频率 (MHz)"},
	{"CPU Clock (MHz)", "CPU 时钟频率 (MHz)"},
	{"Cache", "缓存"},
	{"Cache Speed (ns)", "缓存速度 (ns)"},
	{"Capacity", "容量"},
	{"Checksum", "校验码"},
	{"Checksum Status", "校验码状态"},
	{"Class", "种类"},
	{"Code Name", "代号"},
	{"Compatiable ID", "兼容 ID"},
	{"Computer Name", "计算机名"},
	{"Configuration Strings", "配置字符串"},
	{"Core Count", "核心数"},
	{"Core Voltage (V)", "核心电压 (V)"},
	{"Cores", "核心"},
	{"Current", "当前"},
	{"Current Language", "当前语言"},
	{"Current Speed (MHz)", "当前速度 (MHz)"},
	{"Current Speed (ns)", "当前速度 (ns)"},
	{"DHCP Enabled", "已启用 DHCP"},
	{"DHCP Server", "DHCP 服务器"},
	{"DMI Version", "DMI 版本"},
	{"DNS Domain", "DNS 域"},
	{"DNS Hostname", "DNS 主机名"},
	{"DNS Server", "DNS 服务器"},
	{"Date", "日期"},
	{"Description", "描述"},
	{"Device", "设备"},
	{"Device Locator", "设备定位"},
	{"Device Size", "设备大小"},
	{"Device Type", "设备类型"},
	{"Diagonal (in)", "对角线 (英寸)"},
	{"Drive Letter", "盘符"},
	{"Driver", "驱动"},
	{"Driver Path", "驱动路径"},
	{"Driver Version", "驱动版本"},
	{"EDID Version", "EDID 版本"},
	{"Ending Address", "结束地址"},
	{"Error Correction", "错误校正"},
	{"Ext.Family", "扩展系列"},
	{"Ext.Model", "扩展型号"},
	{"External Clock (MHz)", "外部时钟 (MHz)"},
	{"External Connector Type", "外部接口类型"},
	{"External Reference Designator", "外部参考代号"},
	{"Family", "系列"},
	{"Features", "特性"},
	{"Filesystem", "文件系统"},
	{"Firmware", "固件"},
	{"Free", "可用"},
	{"Free Space", "可用空间"},
	{"Function", "功能"},
	{"Gateway", "网关"},
	{"Group Name", "组名"},
	{"HW Name", "硬件名称"},
	{"HWID", "硬件 ID"},
	{"Height", "高度"},
	{"Height (mm)", "高度 (mm)"},
	{"Hypervisor", "虚拟机监控器"},
	{"ID", "ID"},
	{"Image", "图像"},
	{"Image Size (K)", "镜像大小 (K)"},
	{"Installable Languages", "可安装语言"},
	{"Installed Cache Size", "已安装缓存容量"},
	{"Installed Size (MB)", "已安装大小 (MB)"},
	{"Interface", "界面"},
	{"Internal Connector Type", "内部接口类型"},
	{"Internal Reference Designator", "内部参考代号"},
	{"Item Handle", "项目句柄"},
	{"Item Type", "项目类型"},
	{"L1 D", "一级 数据"},
	{"L1 I", "一级 指令"},
	{"L2", "二级"},
	{"L3", "三级"},
	{"Label", "卷标"},
	{"Language ID", "语言 ID"},
	{"Length", "长度"},
	{"Location", "位置"},
	{"Logical CPUs", "逻辑处理器"},
	{"MAC Address", "MAC 地址"},
	{"MBR Signature", "MBR 签名"},
	{"MTU (Byte)", "最大传输单元 (字节)"},
	{"Manufacturer", "制造商"},
	{"Max", "最大"},
	{"Max Cache Size", "最大缓存容量"},
	{"Max Capacity", "最大容量"},
	{"Max Speed (MHz)", "最大速度 (MHz)"},
	{"Memory Usage", "内存使用"},
	{"Memory Type", "内存类型"},
	{"Min", "最小"},
	{"Model", "型号"},
	{"Module Type", "模组类型"},
	{"Multicast Address", "多播地址"},
	{"Multiplier", "倍频"},
	{"Number of Devices", "设备数"},
	{"Number of Slots", "插槽数"},
	{"Number of Strings", "字符串数"},
	{"OEM String", "OEM 字符串"},
	{"OS", "操作系统"},
	{"On Board Devices", "板载设备"},
	{"Page Size", "页大小"},
	{"Paging File", "页面文件"},
	{"Partition Table", "分区表"},
	{"Path", "路径"},
	{"Physical Memory", "物理内存"},
	{"Pixel Clock", "像素时钟频率"},
	{"Port Type", "端口类型"},
	{"Power Status", "电源状态"},
	{"Processor Architecture", "处理器架构"},
	{"Processor Family", "处理器系列"},
	{"Processor Manufacturer", "处理器制造商"},
	{"Processor Version","处理器版本"},
	{"Product ID", "产品 ID"},
	{"Product Name", "产品名称"},
	{"Product Rev", "产品修订版"},
	{"Prog IF", "编程接口"},
	{"Protocol", "协议"},
	{"Receive Link Speed", "接收链接速率"},
	{"Received (Octets)", "已接收 (八比特组)"},
	{"Refresh Rate (Hz)", "刷新率"},
	{"Release Date", "发布日期"},
	{"Removable", "可移动"},
	{"Resolution", "分辨率"},
	{"Revision", "修订版"},
	{"SKU Number", "SKU 编号"},
	{"SMBIOS Version", "SMBIOS 版本"},
	{"Screen Size", "屏幕尺寸"},
	{"Secure Boot", "安全启动"},
	{"Sent (Octets)", "已发送 (八比特组)"},
	{"Serial Number", "序列号"},
	{"Signature", "签名"},
	{"Size", "大小"},
	{"Slot Designation", "槽位名称"},
	{"Socket Designation", "插座名称"},
	{"Speed (MHz)", "速度 (MHz)"},
	{"Speed (MT/s)", "速度 (MT/s)"},
	{"Start", "起始"},
	{"Starting Address", "起始地址"},
	{"Starting Segment", "起始段地址"},
	{"Status", "状态"},
	{"Stepping", "步进"},
	{"String", "字符串"},
	{"Subclass", "子类"},
	{"Subnet Mask", "子网掩码"},
	{"System BIOS Version", "系统 BIOS 版本"},
	{"System Directory", "系统目录"},
	{"Table Handle", "表句柄"},
	{"Table Length", "表长度"},
	{"Table Type", "表类型"},
	{"Temperature (C)", "温度 (C)"},
	{"Thread Count", "线程数"},
	{"Total", "总计"},
	{"Total CPUs", "总处理器数"},
	{"Total Space", "总空间"},
	{"Transmit Link Speed", "传输链接速率"},
	{"Type", "类型"},
	{"Unicast Address", "单播地址"},
	{"Uptime", "正常运行时间"},
	{"Username", "用户名"},
	{"Vendor", "供应商"},
	{"Version", "版本"},
	{"Video Input", "视频输入"},
	{"Voltage", "电压"},
	{"Width", "宽度"},
	{"Width (mm)", "宽度 (mm)"},
	{"Windows Directory", "Windows 目录"},
};
