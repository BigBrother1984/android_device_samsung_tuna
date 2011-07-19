/*
 * Copyright (C) 2011 The Android Open Source Project
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
 */

#define LOG_TAG "audio_hw_primary"
/*#define LOG_NDEBUG 0*/

#include <dlfcn.h>

#include <utils/Log.h>

#include "ril_interface.h"

/* Function pointers */
void *(*_ril_open_client)(void);
int (*_ril_close_client)(void *);
int (*_ril_connect)(void *);
int (*_ril_is_connected)(void *);
int (*_ril_disconnect)(void *);
int (*_ril_set_call_volume)(void *, enum ril_sound_type, int);
int (*_ril_set_call_audio_path)(void *, enum ril_audio_path);
int (*_ril_set_call_clock_sync)(void *, enum ril_clock_state);

static int ril_connect_if_required(struct ril_handle *ril)
{
    if (_ril_is_connected(ril->client))
        return 0;

    if (_ril_connect(ril->client) != RIL_CLIENT_ERR_SUCCESS) {
        LOGE("ril_connect() failed");
        return -1;
    }
    return 0;
}

int ril_open(struct ril_handle *ril)
{
    if (!ril)
        return -1;

    ril->handle = dlopen(RIL_CLIENT_LIBPATH, RTLD_NOW);

    if (!ril->handle) {
        LOGE("Cannot open '%s'", RIL_CLIENT_LIBPATH);
        return -1;
    }

    _ril_open_client = dlsym(ril->handle, "OpenClient_RILD");
    _ril_close_client = dlsym(ril->handle, "CloseClient_RILD");
    _ril_connect = dlsym(ril->handle, "Connect_RILD");
    _ril_is_connected = dlsym(ril->handle, "isConnected_RILD");
    _ril_disconnect = dlsym(ril->handle, "Disconnect_RILD");
    _ril_set_call_volume = dlsym(ril->handle, "SetCallVolume");
    _ril_set_call_audio_path = dlsym(ril->handle, "SetCallAudioPath");
    _ril_set_call_clock_sync = dlsym(ril->handle, "SetCallClockSync");

    if (!_ril_open_client || !_ril_close_client || !_ril_connect ||
        !_ril_is_connected || !_ril_disconnect || !_ril_set_call_volume ||
        !_ril_set_call_audio_path || !_ril_set_call_clock_sync) {
        LOGE("Cannot get symbols from '%s'", RIL_CLIENT_LIBPATH);
        dlclose(ril->handle);
        return -1;
    }

    ril->client = _ril_open_client();
    if (!ril->client) {
        LOGE("ril_open_client() failed");
        dlclose(ril->handle);
        return -1;
    }

    return 0;
}

int ril_close(struct ril_handle *ril)
{
    if (!ril || !ril->handle || !ril->client)
        return -1;

    if ((_ril_disconnect(ril->client) != RIL_CLIENT_ERR_SUCCESS) ||
        (_ril_close_client(ril->client) != RIL_CLIENT_ERR_SUCCESS)) {
        LOGE("ril_disconnect() or ril_close_client() failed");
        return -1;
    }

    dlclose(ril->handle);
    return 0;
}

int ril_set_call_volume(struct ril_handle *ril, enum ril_sound_type sound_type,
                        int volume)
{
    if (ril_connect_if_required(ril))
        return 0;

    return _ril_set_call_volume(ril->client, sound_type, volume);
}

int ril_set_call_audio_path(struct ril_handle *ril, enum ril_audio_path path)
{
    if (ril_connect_if_required(ril))
        return 0;

    return _ril_set_call_audio_path(ril->client, path);
}

int ril_set_call_clock_sync(struct ril_handle *ril, enum ril_clock_state state)
{
    if (ril_connect_if_required(ril))
        return 0;

    return _ril_set_call_clock_sync(ril->client, state);
}
