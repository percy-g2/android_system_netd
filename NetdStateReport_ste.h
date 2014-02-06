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


#ifndef NETDSTATEREPORT_STE_H
#define NETDSTATEREPORT_STE_H (1)

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SOFTAP_STATE_INACTIVE = 0,
    SOFTAP_STATE_ACTIVE = 1
} softap_states_t;

 typedef enum {
     TETHER_STATE_INACTIVE = 0,
     TETHER_STATE_ACTIVE = 1
     } tether_states_t;

void callMadUsbTether(tether_states_t state);

void callMadTxBo(softap_states_t state);

#ifdef __cplusplus
}
#endif

/* Only send the report to modem access daemon if it is available */
#ifdef MAD_AVAILABLE

#define sendSoftapStateReport(active) \
        callMadTxBo(active ? SOFTAP_STATE_ACTIVE : SOFTAP_STATE_INACTIVE)

#define sendTetheringStateReport(active) \
        callMadUsbTether(active ? TETHER_STATE_ACTIVE : TETHER_STATE_INACTIVE)

#else

#define sendSoftapStateReport(active)

#define sendTetheringStateReport(active)

#endif /* MAD_AVAILABLE */

#endif  /* #ifndef NETDSTATEREPORT_STE_H */
