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
 *
 */

#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <linux/wireless.h>

#include <cutils/log.h>
#include <cutils/properties.h>

#include "SoftapController_ste.h"
#include "NetdStateReport_ste.h"
#include "wifi.h"

#include "private/android_filesystem_config.h"

#define LOG_TAG "STE SoftAPCtrl"

const char* SoftapController::freq_list[] = {
    "2412", "2417", "2422", "2427",
    "2432", "2437", "2442", "2447",
    "2452", "2457", "2462", "2467",
    "2472", "2484"
};

static const char SUPP_CONFIG_FILE[]    = "/data/misc/wifi/wpa_supplicant.conf";
static const char SUPP_CONFIG_STORAGE[] = "/data/misc/wifi/wpa_supplicant.storage";


/***************************** PRIVATE FUNCTIONS *****************************/
static void *dummyReader(void *unused)
{
    char buf[4096];
    ALOGD("%s: ENTER", __func__);
    while (1) {
	/*Default Interface added*/
        wifi_wait_for_event(NULL,buf, sizeof(buf));
        if (strstr(buf, "CTRL-EVENT-TERMINATING") == buf)
            break;
    }
    ALOGD("%s: LEAVE", __func__);
    return 0;
}

/*
 * Function: sendCmd()
 * Description:
 *  Send command toward wpa_supplicant's ctrl_iface
 */
int SoftapController::sendCmd(const char *cmd,
                              const char *property,
                              const char *value) {
    int ret;

    ALOGD("%s: ENTER", __func__);

    if (property != NULL && value != NULL) {
        if (strcmp(property, "ssid") == 0 || strcmp(property, "psk") == 0) {
            /* set_network <network id> <property> "<value>" */
            ret = sprintf(mBuf, "%s %d %s \"%s\"", cmd, networkID, property, value);
        } else {
            /* set_network <network id> <property> <value> */
            ret = sprintf(mBuf, "%s %d %s %s", cmd, networkID, property, value);
        }
    }
    else if (value != NULL) {
        /* e.g ap_scan <value> */
        ret = sprintf(mBuf, "%s %s", cmd, value);
    }
    else if (!strcmp(cmd, TERMINATE_CMD)) {
        ret = sprintf(mBuf, "%s", cmd);
    }
    else {
        /* e.g enable_network <network_if> */
        ret = sprintf(mBuf, "%s %d", cmd, networkID);
    }

    if (ret > 0 && (ret = wifi_command(INTERFACE_NAME, mBuf, reply, &reply_len)) == 0) {
        //Check if response was 'OK'
        if(strcmp(reply, "OK") == 0) {
            ret = 0;
        } else {
            ret = -1;
        }
    }

    ALOGD("Command [%s] Reply [%s]", mBuf, reply);
    return ret;
}

int SoftapController::storeSettings() {
    char buf[2048];
    int srcfd, destfd;
    int nread;

    ALOGD("%s: ENTER", __func__);

    srcfd = open(SUPP_CONFIG_FILE, O_RDWR);
    if (srcfd < 0) {
        return 0;
    }

    destfd = open(SUPP_CONFIG_STORAGE, O_CREAT|O_WRONLY, 0660);
    if (destfd < 0) {
        close(srcfd);
        return -1;
    }

    while ((nread = read(srcfd, buf, sizeof(buf))) != 0) {
        if (nread < 0) {
            close(srcfd);
            close(destfd);
            unlink(SUPP_CONFIG_STORAGE);
            return -1;
        }
        write(destfd, buf, nread);
    }

    close(destfd);
    close(srcfd);

    unlink(SUPP_CONFIG_FILE);

    return 0;
}


int SoftapController::restoreSettings() {
    char buf[2048];
    int srcfd, destfd;
    int nread;

    ALOGD("%s: ENTER", __func__);

    srcfd = open(SUPP_CONFIG_STORAGE, O_RDWR);
    if (srcfd < 0) {
        return 0;
    }

    destfd = open(SUPP_CONFIG_FILE, O_CREAT|O_WRONLY|O_TRUNC, 0660);

    if (destfd < 0) {
        close(srcfd);
        return -1;
    }

    while ((nread = read(srcfd, buf, sizeof(buf))) != 0) {
        if (nread < 0) {
            close(srcfd);
            close(destfd);
            unlink(SUPP_CONFIG_FILE);
            return -1;
        }
        write(destfd, buf, nread);
    }

    close(destfd);
    close(srcfd);

    if (chown(SUPP_CONFIG_FILE, AID_SYSTEM, AID_WIFI) < 0) {
        unlink(SUPP_CONFIG_FILE);
        return -1;
    }

    unlink(SUPP_CONFIG_STORAGE);
    return 0;
}

/****************************** PUBLIC FUNCTIONS *****************************/

SoftapController::SoftapController() {
    mPid = 0;
    networkID = -1;
    reply_len = sizeof(reply) - 1;
    memset(mBuf, 0, sizeof(mBuf));
    memset(reply, 0, sizeof(reply));
    ALOGD("SoftapController()");
}

SoftapController::~SoftapController() {
    ALOGD("~SoftapController()");
}


/*
 * Function: startDriver()
 * Description:
 *  Driver is already loaded at this point.
 *  Try to start and connect to supplicant
 *
 */
int SoftapController::startDriver(char *iface) {
    int ret;

    ALOGD("%s: ENTER", __func__);

    //Save wpa_supplicant STA settings
    if (ret = storeSettings()) {
        ALOGD("%s: Unable to store STA mode settings: [%d]", __func__, ret);
        goto error;
    }

    //Start wpa_supplicant
    if( (ret = wifi_start_supplicant(0)) == 0) {
        int connectTries = 0;

        while(true) {
            if(wifi_connect_to_supplicant(INTERFACE_NAME))
            {
                if (connectTries++ < 3) {
                    usleep(WPA_SUPPLICANT_DELAY);
                }
                else {
                    ALOGD("%s: Connection to supplicant failed!", __func__);
                    ret = -1;
                    break;
                }
            } else {
                break;
            }
        }

        if ((ret = pthread_create(&mDummyReader, NULL, dummyReader, NULL)))
            ALOGE("%s: Unable to start dummyReader (%d -> %s)",
                 __func__, ret, strerror(ret));

    } else {
        ALOGD("%s: Unable to start supplicant: [%d]", __func__, ret);
        goto error;
    }

error:
    return ret;
}

/*
 * Function: stopDriver()
 * Description:
 */
int SoftapController::stopDriver(char *iface) {
    int ret;

    ALOGD("%s: ENTER", __func__);

    //Close supplicant connection
    wifi_close_supplicant_connection(INTERFACE_NAME);

    // cleanup dummy reader
    if ((ret = pthread_join(mDummyReader, 0)))
        ALOGE("%s: failed to join dummyReader (%d - %s)",
             __func__, ret, strerror(ret));

    //Stop wpa_supplicant
    ret = wifi_stop_supplicant();

    //restore STA mode settings
    ret |= restoreSettings();

    ALOGD("%s: ret=%d", __func__, ret);
    return ret;
}

/*
 * Function: startSoftap()
 * Description:
 *
 */
int SoftapController::startSoftap() {
    int ret;

    ALOGD("%s: ENTER", __func__);

    if (mPid) {
        ALOGE("startSoftap() already started EXIT");
        ret = 0;
    } else {
        ret = sendCmd(AP_SCAN, NULL, "2");
        if ((ret = sendCmd(ENABLE_NET_CMD, NULL, NULL)) == 0) {
            mPid = 1;

            /* Notify MAD of new Softap state */
            sendSoftapStateReport(true);
        }
    }

    ALOGD("%s: ret=%d", __func__, ret);
    return ret;
}

/*
 * Function: stopSoftap()
 * Description:
 *
 */
int SoftapController::stopSoftap() {
    int ret = 0;

    ALOGD("%s: ENTER", __func__);

    if (mPid == 0) {
        ALOGE("%s: Already stopped ret=%d EXIT", __func__, ret);
    } else {

        ret = sendCmd(DISABLE_NET_CMD, NULL, NULL);

        ret |= sendCmd(REMOVE_NET_CMD, NULL, NULL);

        ret |= sendCmd(TERMINATE_CMD, NULL, NULL);

        // cleanup dummy reader
        if ((ret = pthread_join(mDummyReader, 0)))
            ALOGE("%s: failed to join dummyReader (%d - %s)",
                 __func__, ret, strerror(ret));

        //stop wpa_supplicant
        ret |= wifi_stop_supplicant();

        if (ret) {
            ALOGE("%s: Unable to stop supplicant ret=%d", __func__, ret);
        }

        //close supplicant connection
        wifi_close_supplicant_connection(NULL);

        //restore STA mode settings
        ret |= restoreSettings();

        mPid = 0;

        /* Notify MAD of new Softap state */
        sendSoftapStateReport(false);
    }

    ALOGD("%s: ret=%d", __func__, ret);
    return ret;
}

/*
 * Function: isSoftapStarted()
 * Description:
 *
 */
bool SoftapController::isSoftapStarted() {
    return (mPid != 0 ? true : false);
}

/*
 * Function: setSoftap()
 * Description:
 *  Start with add_network to fetch network_id.
 *  Then send rest of the set_network commands to configure wpa_supplicant.
 *
 * Arguments from Android App:
 *    argv[2] - wlan interface
 *    argv[3] - softap interface
 *    argv[4] - SSID
 *    argv[5] - Security
 *    argv[6] - Key
 *    argv[7] - Channel
 *    argv[8] - Preamble
 *    argv[9] - Max SCB
 */
int SoftapController::setSoftap(int argc, char *argv[]) {
    int   ret = -1;
    char *buf;

    ALOGD("%s: ENTER", __func__);

    if(startDriver(INTERFACE_NAME))
	ALOGD("%s: Supplicant start failed", __func__);

    if (argc < 4) {
        ALOGE("%s: missing arguments", __func__);
        return ret;
    }

    if (!wifi_command(INTERFACE_NAME,ADD_NET_CMD, reply, &reply_len)) {
        if ((networkID = atoi(reply)) < 0) {
            goto fail;
        }
    } else {
        ALOGD("%s failed, exit", ADD_NET_CMD);
        return ret;
    }

    //Send SSID as a first nertwork parameter
    if (argc > 4) {
        buf = argv[4];
    } else {
        buf = (char *)"AndroidAP";
    }

    if (sendCmd(SET_NET_CMD, "ssid", buf)) {
        goto fail;
    }

    //Send AP-mode
    if (sendCmd(SET_NET_CMD, "mode", "2")) {
        goto fail;
    }

    //Check if security has been set in Android AP
    if (argc > 5) {
        if (strcmp(argv[5], "open") == 0) {
            buf = (char *)"NONE";
        } else {
            buf = (char *)"WPA-PSK";
        }
    } else {
        buf = (char *)"NONE";
    }

    if (strcmp(buf, "NONE") == 0) {
        if (sendCmd(SET_NET_CMD, "key_mgmt", buf)) {
            goto fail;
        }
    }
    else {
        if (sendCmd(SET_NET_CMD, "key_mgmt", "WPA-PSK")) {
            goto fail;
        }
        if (sendCmd(SET_NET_CMD, "pairwise", "CCMP")) {
            goto fail;
        }
        if (sendCmd(SET_NET_CMD, "group", "CCMP")) {
            goto fail;
        }
        if (sendCmd(SET_NET_CMD, "proto", "WPA2")) {
            goto fail;
        }

        //psk key
        if (argc > 6) {
            buf = argv[6];
        } else {
            buf = (char *)"12345678";
        }

        if (sendCmd(SET_NET_CMD, "psk", buf)) {
            goto fail;
        }
    }

    if (argc > 7) {
        buf = (char *)freq_list[(atoi(argv[7]) - 1)];
    } else {
        //use channel 6 by default
        buf = (char *)"2437";
    }

    if (sendCmd(SET_NET_CMD, "frequency", buf)) {
        goto fail;
    }

    ret = 0;
fail:
    ALOGD("%s ret=%d", __func__, ret);
    if (ret)
        stopDriver(NULL);
    return ret;
}

/*
 * Function: fwReloadSoftap()
 * Description:
 *  Make sure that driver is loaded
 *  When starting WiFi Direct driver is not loaded by framework.
 */
int SoftapController::fwReloadSoftap(int argc, char *argv[])
{
    return wifi_load_driver();
}

/*
 * Function: clientsSoftap()
 * Description:
 *  Return connected stations list
 */
int SoftapController::clientsSoftap(char **retbuf)
{
    //TODO: implement client list
    return 1; //error getting client list
}
