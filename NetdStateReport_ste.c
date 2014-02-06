/*
 * Copyright (C) 2008 The Android Open Source Project
 * Copyright (C) ST-Ericsson SA 2011
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
 */

#include <dbus/dbus.h>

#define LOG_TAG "STE SoftAPCtrl"
#include <cutils/log.h>
#include <cutils/properties.h>

/* Modem access daemon */
#include <ste_mad/mad.h>

#include "NetdStateReport_ste.h"

static void callmad_txbo_tether(const char *state,const char* ste_dbus_interface,
                                                     const char* ste_dbus_method );

/*
 * Function: initDbus()
 * Description:
 *  Initialize the DBus Connection.
 */
static DBusConnection* initDbus() {
    DBusConnection *conn = NULL;
    DBusError err;
    char dbus_address[PROPERTY_VALUE_MAX];
    dbus_error_init(&err);

    /* connect to the bus */
    memset(dbus_address, 0, sizeof(dbus_address));
    (void)property_get("ste.dbus.bus.address", dbus_address, NULL);
    conn = dbus_connection_open(dbus_address, &err);

    if (conn == NULL || dbus_error_is_set(&err)) {
        /* "Connect to DBus failed (%s)", err.message */
        ALOGE("%s: Failed to connect to DBus: (%s)", __func__, err.message);
        goto exit;
    }

    /* Make sure we are registered to the bus */
    if (!dbus_bus_get_unique_name(conn) &&
            !dbus_bus_register(conn, &err)) {
        ALOGE("%s: Failed to register with DBus: (%s)", __func__, err.message);
    }

    /* success */
    dbus_connection_ref(conn);

exit:
    dbus_error_free(&err);
    return conn;
}

/*
 * Function: callMadTxBo()
 * Description:
 *  Sends a Tx Backoff state value to Modem Access Daemon via Dbus.
 */
void callMadTxBo(softap_states_t state) {

    const char *value = (state == SOFTAP_STATE_ACTIVE) ? STE_MAD_FEATURE_ON : STE_MAD_FEATURE_OFF;

    callmad_txbo_tether(value, STE_MAD_DBUS_TXBO_IF_NAME ,STE_MAD_TXBO_WIFI_AP);

    return;
}

/*
 * Function: callMadUsbTether()
 * Description:
 *  Sends a Tether state value to Modem Access Daemon via Dbus.
 */
void callMadUsbTether(tether_states_t state)
{

    const char *value = (state == TETHER_STATE_ACTIVE) ? STE_MAD_FEATURE_ON : STE_MAD_FEATURE_OFF;

    callmad_txbo_tether(value, STE_MAD_DBUS_TETHER_NAME ,STE_MAD_USB_TETHER);

    return;
}

/* This function will be called for Passing either TXBO Back off or USB Tethering Events */
static void callmad_txbo_tether(const char *state,const char* ste_dbus_interface,const char* ste_dbus_method )
{

    DBusMessage *msg = NULL;
    DBusMessageIter args;
    DBusPendingCall *pending;
    DBusConnection *conn = initDbus();

    if (!conn) {
        goto error;
    }

    /* create a signal and check for errors */
    msg = dbus_message_new_method_call(STE_MAD_DBUS_SERVICE,
                                       STE_MAD_DBUS_OBJECT_NAME,
                                       ste_dbus_interface,
                                       ste_dbus_method);
    if (NULL == msg) {
        ALOGE("%s: Failed to create DBus msg!", __func__);
        goto error;
    }

    /* append arguments onto signal */
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &state)) {
        ALOGE("%s: DBus: OUT OF MEMORY!", __func__);
        goto error;
    }

    /* send the message and flush the connection */
    if (!dbus_connection_send_with_reply(conn, msg, &pending, -1)) {
        ALOGE("%s: Failed to DBus message!", __func__);
        goto error;
    }

    dbus_connection_flush(conn);

    /* free the message */
    dbus_message_unref(msg);

    /* block until we receive a reply*/
    dbus_pending_call_block(pending);

    /* Message is now sent */

    // get the reply message
    msg = dbus_pending_call_steal_reply(pending);

    /* free the pending message handle */
    dbus_pending_call_unref(pending);

    if (NULL == msg) {
        ALOGE("%s: Failed to receive reply!", __func__);
        goto error;
    }

    if (DBUS_MESSAGE_TYPE_ERROR == dbus_message_get_type(msg)) {
        DBusError err;
        dbus_error_init(&err);
        dbus_set_error_from_message(&err, msg);
        ALOGE("%s: Received error (%s)",  __func__, err.message);
        dbus_error_free(&err);
        goto error;
    }

    /* We're not expecting any return parameters.
      Just free the message and exit */
    goto exit;

error:
    ALOGE("%s: Unable to send state!", __func__);
exit:

    if (conn) {
        dbus_connection_unref(conn);
    }

    if (msg) {
        dbus_message_unref(msg);
    }

    return;


}


