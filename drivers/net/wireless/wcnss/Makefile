
# Makefile for WCNSS triple-play driver

# [WLAN][SHARP] 2015.04.28 Remedy for memory allocation failure Start
ccflags-y += -D SH_WIFI_CUSTOMIZE
# [WLAN][SHARP] 2015.04.28 Remedy for memory allocation failure End

wcnsscore-objs += wcnss_wlan.o qcomwlan_secif.o wcnss_vreg.o

obj-$(CONFIG_WCNSS_CORE) += wcnsscore.o
obj-$(CONFIG_WCNSS_MEM_PRE_ALLOC) += wcnss_prealloc.o
