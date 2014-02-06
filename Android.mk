LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS :=

LOCAL_SRC_FILES:=                                      \
                  BandwidthController.cpp              \
                  CommandListener.cpp                  \
                  DnsProxyListener.cpp                 \
                  MDnsSdListener.cpp                   \
                  IdletimerController.cpp              \
                  NatController.cpp                    \
                  NetdCommand.cpp                      \
                  NetdConstants.cpp                    \
                  NetlinkHandler.cpp                   \
                  NetlinkManager.cpp                   \
                  PanController.cpp                    \
                  PppController.cpp                    \
                  ResolverController.cpp               \
                  SecondaryTableController.cpp         \
                  TetherController.cpp                 \
                  ThrottleController.cpp               \
                  oem_iptables_hook.cpp                \
                  logwrapper.c                         \
                  main.cpp                             \

ifeq ($(WLAN_ENABLE_STE_WIFI_TETHERING), true)
LOCAL_SRC_FILES+= \
                  SoftapController_ste.cpp

LOCAL_CFLAGS += -DENABLE_STE_CHANGES
else
LOCAL_SRC_FILES+= \
                  SoftapController.cpp
endif

LOCAL_MODULE:= netd

LOCAL_C_INCLUDES := $(KERNEL_HEADERS) \
                    $(LOCAL_PATH)/../bluetooth/bluedroid/include \
                    $(LOCAL_PATH)/../bluetooth/bluez-clean-headers \
                    external/mdnsresponder/mDNSShared \
                    external/openssl/include \
                    external/stlport/stlport \
                    bionic \
                    bionic/libc/private \
                    $(call include-path-for, libhardware_legacy)/hardware_legacy

LOCAL_CFLAGS := -Werror=format

ifdef WIFI_DRIVER_FW_STA_PATH
LOCAL_CFLAGS += -DWIFI_DRIVER_FW_STA_PATH=\"$(WIFI_DRIVER_FW_STA_PATH)\"
endif
ifdef WIFI_DRIVER_FW_AP_PATH
LOCAL_CFLAGS += -DWIFI_DRIVER_FW_AP_PATH=\"$(WIFI_DRIVER_FW_AP_PATH)\"
endif

LOCAL_SHARED_LIBRARIES := libstlport libsysutils libcutils libnetutils \
                          libcrypto libhardware_legacy libmdnssd

ifneq ($(BOARD_HOSTAPD_DRIVER),)
  LOCAL_CFLAGS += -DHAVE_HOSTAPD
endif

ifeq ($(WLAN_ENABLE_STE_WIFI_TETHERING), true)
LOCAL_SHARED_LIBRARIES += libhardware_legacy

ifeq ($(CN_ENABLE_FEATURE_MAD),true)
LOCAL_SRC_FILES += NetdStateReport_ste.c
LOCAL_C_INCLUDES += $(call include-path-for, dbus)
LOCAL_SHARED_LIBRARIES += libdbus
LOCAL_CFLAGS += -DMAD_AVAILABLE
endif #CN_ENABLE_FEATURE_MAD,true
endif #WLAN_ENABLE_STE_WIFI_TETHERING,true

ifeq ($(BOARD_HAVE_BLUETOOTH),true)
  LOCAL_SHARED_LIBRARIES := $(LOCAL_SHARED_LIBRARIES) libbluedroid
  LOCAL_CFLAGS := $(LOCAL_CFLAGS) -DHAVE_BLUETOOTH
endif

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_SRC_FILES:=          \
                  ndc.c \

LOCAL_MODULE:= ndc

LOCAL_C_INCLUDES := $(KERNEL_HEADERS)

LOCAL_CFLAGS :=

LOCAL_SHARED_LIBRARIES := libcutils

include $(BUILD_EXECUTABLE)
