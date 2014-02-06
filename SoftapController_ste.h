/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Copyright (C) ST-Ericsson SA 2011
 *
 * ST-Ericsson rewritten implementation of SoftAPController class to
 * interact with wpa_supplicant instead of dedicated driver.
 *
 * Author: Bartosz Markowski (bartosz.markowski@tieto.com)
 */

#ifndef _SOFTAP_CONTROLLER_H
#define _SOFTAP_CONTROLLER_H

#define SOFTAP_MAX_BUFFER_SIZE  512
#define WPA_SUPPLICANT_DELAY    5000000

#include <linux/in.h>
#include <net/if.h>
#include <utils/List.h>

#define DISABLE_NET_CMD "DISABLE_NETWORK"
#define ENABLE_NET_CMD  "ENABLE_NETWORK"
#define ADD_NET_CMD     "ADD_NETWORK"
#define REMOVE_NET_CMD  "REMOVE_NETWORK"
#define SET_NET_CMD     "SET_NETWORK"
#define AP_SCAN         "SET ap_scan"
#define INTERFACE_NAME  "wlan0"
#define TERMINATE_CMD   "TERMINATE"

class SoftapController {
    char    mBuf[SOFTAP_MAX_BUFFER_SIZE];
    char    reply[SOFTAP_MAX_BUFFER_SIZE];
    size_t  reply_len;
    pid_t   mPid;
    int     networkID;
    static const char *freq_list[];
    pthread_t mDummyReader;

    int sendCmd(const char *cmd, const char *property, const char *value);
    int storeSettings();
    int restoreSettings();

public:
    SoftapController();
    virtual ~SoftapController();

    int startDriver(char *iface);
    int stopDriver(char *iface);
    int startSoftap();
    int stopSoftap();
    bool isSoftapStarted();
    int setSoftap(int argc, char *argv[]);
    int fwReloadSoftap(int argc, char *argv[]);
    int clientsSoftap(char **retbuf);
};

#endif
