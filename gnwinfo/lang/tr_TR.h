﻿// SPDX-License-Identifier: Unlicense
// This file should be saved in UTF-8 **with** BOM encoding

#include "../gettext.h"

static const char*
lang_tr_tr[N__MAX_] =
{
	[N__FONT_] = u8"Tahoma",
	[N__LANG_NAME_] = u8"Türkçe",
	[N__LOADING] = u8"Yükleniyor",
	[N__PLS_WAIT] = u8"Lütfen bekleyin ...",
	[N__OS] = u8"İşletim Sistemi",
	[N__PC] = u8"Bilgisayar",
	[N__CPU] = u8"İşlemci",
	[N__MEMORY] = u8"Bellek",
	[N__DISPLAY] = u8"Monitör",
	[N__STORAGE] = u8"Depolama",
	[N__NETWORK] = u8"Ağ",
	[N__AUDIO] = u8"Ses Aygıtları",
	[N__LOGIN] = u8"Kullanıcı Adı",
	[N__UPTIME] = u8"Çalışma Süresi",
	[N__VENDOR] = u8"Üretici Firma",
	[N__VERSION] = u8"Versiyon",
	[N__USAGE] = u8"Kullanım",
	[N__CACHE] = u8"Ön Bellek",
	[N__POWER_STAT] = u8"Güç Planı",
	[N__GOOD] = u8"İyi",
	[N__CAUTION] = u8"Dikkat",
	[N__BAD] = u8"Kötü",
	[N__UNKNOWN] = u8"Bilinmeyen",
	[N__CORES] = u8"Çekirdekler",
	[N__CORES_UPPER] = u8"Çekirdekler",
	[N__THREADS] = u8"İş parçacıkları",
	[N__THREADS_UPPER] = u8"İş parçacıkları",
	[N__SB] = u8"Güvenli Önyükleme",
	[N__SB_OFF] = u8"Güvenli Önyükleme kapalı",
	[N__TOTAL] = u8"Toplam",
	[N__FEATURES] = u8"Özellikler",
	[N__MULTIPLIER] = u8"Çarpan",
	[N__BUS_CLOCK] = u8"Veri yolu",
	[N__TEMPERATURE] = u8"Sıcaklık",
	[N__VOLTAGE] = u8"Voltaj",
	[N__CPU_POWER] = u8"Güç",
	[N__BRAND] = u8"Marka",
	[N__HYPERVISOR] = u8"Sanallaştırma",
	[N__CODE_NAME] = u8"Kod Adı",
	[N__TECHNOLOGY] = u8"Teknoloji",
	[N__FAMILY] = u8"Aile",
	[N__MODEL] = u8"Model",
	[N__STEPPING] = u8"Revizyon",
	[N__EXT_FAMILY] = u8"Genişletilmiş Seri",
	[N__EXT_MODEL] = u8"Genişletilmiş Model",
	[N__AFF_MASK] = u8"Affinite Maske",
	[N__ABOUT] = u8"Hakkında",
	[N__SETTINGS] = u8"Ayarlar",
	[N__HIDE_COMPONENTS] = u8"Donanımları gizle",
	[N__HIDE_BUILD_NUMBER] = u8"Yapı numarasını gizle",
	[N__HIDE_EDITION_ID] = u8"Sürüm kimliğini gizle",
	[N__HIDE_UPTIME] = u8"Çalışma süresini gizle",
	[N__HIDE_LOGIN] = u8"Giriş durumunu gizle",
	[N__HIDE_VENDOR] = u8"Üretici firma adını gizle",
	[N__HIDE_VERSION] = u8"Sürümü gizle",
	[N__HIDE_CACHE] = u8"Önbelleği gizle",
	[N__COMPACT_VIEW] = u8"Kompakt görünüm",
	[N__USE_BIT_UNITS] = u8"Bit birimlerini kullan",
	[N__HIDE_INACTIVE_NETWORK] = u8"Aktif olmayan ağı gizle",
	[N__HIDE_DETAILS] = u8"Detayları gizle",
	[N__HIDE_SMART] = u8"SMART'ı gizle",
	[N__DISPLAY_SMART_HEX] = u8"SMART'ı HEX formatında göster",
	[N__ADV_DISK_SEARCH] = u8"Gelişmiş disk araması",
	[N__HD204UI_WORKAROUND] = u8"HD204UI özel çözüm",
	[N__ADATA_SSD_WORKAROUND] = u8"ADATA SSD özel çözümü",
	[N__ATA_PASS_THROUGH] = u8"ATA doğrudan geçiş",
	[N__ENABLE_NVIDIA_CTRL] = u8"Nvidia denetleyicisini etkinleştir",
	[N__ENABLE_MARVELL_CTRL] = u8"Marvell denetleyicisini etkinleştir",
	[N__ENABLE_USB_SAT] = u8"USB SAT'ı etkinleştir",
	[N__ENABLE_USB_IO_DATA] = u8"USB I-O DATA'yı etkinleştir",
	[N__ENABLE_USB_SUNPLUS] = u8"USB Sunplus'ı etkinleştir",
	[N__ENABLE_USB_LOGITEC] = u8"USB Logitech'i etkinleştir",
	[N__ENABLE_USB_PROLIFIC] = u8"USB Prolific'i etkinleştir",
	[N__ENABLE_USB_JMICRON] = u8"USB JMicron'ı etkinleştir",
	[N__ENABLE_USB_CYPRESS] = u8"USB Cypress'i etkinleştir",
	[N__ENABLE_ASMEDIA_ASM1352R] = u8"ASMedia ASM1352R'yi etkinleştir",
	[N__ENABLE_USB_MEMORY] = u8"USB Belleğini etkinleştir",
	[N__ENABLE_NVME_JMICRON] = u8"NVMe JMicron'ı etkinleştir",
	[N__ENABLE_NVME_ASMEDIA] = u8"NVMe ASMedia'yı etkinleştir",
	[N__ENABLE_NVME_REALTEK] = u8"NVMe Realtek'i etkinleştir",
	[N__ENABLE_MEGARAID] = u8"MegaRAID'i etkinleştir",
	[N__ENABLE_INTEL_VROC] = u8"Intel VROC'u etkinleştir",
	[N__ENABLE_AMD_RC2] = u8"AMD RC2'yi etkinleştir",
	[N__ENABLE_REALTEK_9220DP] = u8"Realtek 9220DP'yi etkinleştir",
	[N__HIDE_RAID_VOLUME] = u8"RAID hacmini gizle",
	[N__WINDOW_RESTART] = u8"Pencere (Yeniden başlatma gerekli)",
	[N__ENABLE_PDH] = u8"PDH'yi etkinleştir",
	[N__BACKGROUND_INFO] = u8"Arka plan bilgisini göster",
	[N__SHOW_SENSITIVE_DATA] = u8"Hassas bilgileri göster",
	[N__DISABLE_DPI_SCALING] = u8"DPI ölçeklemeyi devre dışı bırak",
	[N__DISABLE_ANTIALIASING] = u8"Kenar yumuşatma (anti-aliasing) özelliğini devre dışı bırak",
	[N__WIDTH] = u8"#Genişlik",
	[N__HEIGHT] = u8"#Yükseklik",
	[N__ALPHA] = u8"#Saydamlık",
	[N__COLOR] = u8"Renk",
	[N__SAVE] = u8"Kaydet",
	[N__NO_DISKS_FOUND] = u8"Disk Bulunamadı",
	[N__REFRESH] = u8"Yenile",
	[N__FIRMWARE] = u8"Donanım yazılımı",
	[N__HEALTH_STATUS] = u8"Sağlık Durumu",
	[N__TOTAL_READS] = u8"Toplam okuma",
	[N__TOTAL_WRITES] = u8"Toplam yazma",
	[N__NAND_WRITES] = u8"NAND Toplam Yazma",
	[N__BUFFER_SIZE] = u8"Ön bellek miktarı",
	[N__S_N] = u8"Seri Numarası",
	[N__INTERFACE] = u8"Arayüz",
	[N__RPM] = u8"Devir Sayısı",
	[N__MODE] = u8"Mod",
	[N__POWER_ON_COUNT] = u8"Açılma Sayısı",
	[N__POWER_ON_HOURS] = u8"Açılma Süresi (Saat)",
	[N__DRIVE] = u8"Sürücü",
	[N__STANDARD] = u8"Standart",
	[N__ATTRIBUTE] = u8"Öznitelik",
	[N__PHYSICAL_MEMORY] = u8"Fiziksel Bellek",
	[N__PAGE_FILE] = u8"Sayfalama Dosyası",
	[N__SYSTEM_WORKING_SET] = u8"Sistem Çalışma Kümesi",
	[N__CLEAN_MEMORY] = u8"Belleği Temizle",
	[N__CREATE_PAGING_FILES] = u8"Sayfalama Dosyası Oluştur",
	[N__OK] = u8"Tamam",
	[N__SUCCESS] = u8"Başarılı",
	[N__FAILED] = u8"Başarısız",
	[N__HOSTNAME] = u8"Kullanıcı Adı",
	[N__RANDOM] = u8"Rastgele",
	[N__NETWORK_DRIVES] = u8"Ağ Sürücüleri",
	[N__NETWORK_DRIVE] = u8"Ağ sürücüsü",
	[N__MAX_CAPACITY] = u8"Maksimum Kapasite",
	[N__SLOTS] = u8"Yuva",
	[N__SOCKET] = u8"Soket",
	[N__BASE_CLOCK] = u8"Temel Saat Hızı",
	[N__HIDE_DISK_SPACE_BAR] = u8"Disk Alanı Çubuğunu Gizle",
	[N__FREE] = u8"Ücretsiz sürüm",
	[N__CLOSE] = u8"Çıkış"
};
