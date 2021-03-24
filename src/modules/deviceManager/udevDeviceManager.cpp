/* @@@LICENSE
*
*      Copyright (c) 2020-2021 LG Electronics Company.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* LICENSE@@@ */
#include "udevDeviceManager.h"

#define SYSFS_HEADSET_MIC "/sys/devices/virtual/switch/h2w/state"

bool UdevDeviceManager::mIsObjRegistered = UdevDeviceManager::CreateInstance();

UdevDeviceManager* UdevDeviceManager::getInstance()
{
    static UdevDeviceManager objUdevDeviceManager;
    return &objUdevDeviceManager;
}

bool UdevDeviceManager::event(LSHandle *lshandle, LSMessage *message, void *ctx)
{
    bool returnValue = false;
    LSMessageJsonParser    msg(message, SCHEMA_3(REQUIRED(event, string),\
        OPTIONAL(soundcard_no, integer),OPTIONAL(device_no, integer)));
    if (!msg.parse(__FUNCTION__, lshandle))
        return true;
    const gchar * reply = STANDARD_JSON_SUCCESS;
    //read optional parameters with appropriate default values
    int soundcard_no = 0;
    int device_no = 0;
    if (!msg.get("soundcard_no", soundcard_no))
        soundcard_no = 0;
    if (!msg.get("device_no", device_no))
        device_no = 0;
    std::string event;
    UdevDeviceManager* objUdevDeviceManager = UdevDeviceManager::getInstance();
    if (objUdevDeviceManager)
    {
        if (!msg.get("event", event)) {
            reply = MISSING_PARAMETER_ERROR(event, string);
        }
        else
        {
            mObjAudioMixer = AudioMixer::getAudioMixerInstance();
            if (mObjAudioMixer)
            {
                if (event == "headset-removed") {
                    returnValue = mObjAudioMixer->programHeadsetRoute(0);
                    if (false == returnValue)
                        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INTERNAL_ERROR, "Audiod internal error");
                }
                else if (event == "headset-inserted") {
                    returnValue = mObjAudioMixer->programHeadsetRoute(1);
                    if (false == returnValue)
                        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INTERNAL_ERROR, "Audiod internal error");
                }
                else if (event == "usb-mic-inserted") {
                    returnValue = mObjAudioMixer->loadUSBSinkSource('j', soundcard_no, device_no, 1);
                    if (false == returnValue)
                        reply = INVALID_PARAMETER_ERROR(soundcard_no, integer);
                }
                else if (event == "usb-mic-removed") {
                    returnValue = mObjAudioMixer->loadUSBSinkSource('j', soundcard_no, device_no, 0);
                    if (false == returnValue)
                        reply = INVALID_PARAMETER_ERROR(soundcard_no, integer);
                }
                else if (event == "usb-headset-inserted") {
                    returnValue = mObjAudioMixer->loadUSBSinkSource('z', soundcard_no, device_no, 1);
                    if (false == returnValue)
                        reply = INVALID_PARAMETER_ERROR(soundcard_no, integer);
                }
                else if (event == "usb-headset-removed") {
                    returnValue = mObjAudioMixer->loadUSBSinkSource('z', soundcard_no, device_no, 0);
                    if (false == returnValue)
                        reply = INVALID_PARAMETER_ERROR(soundcard_no, integer);
                }
                else
                    reply = INVALID_PARAMETER_ERROR(event, string);
            }
            else
            {
                PM_LOG_ERROR(MSGID_UDEV_MANAGER, INIT_KVCOUNT, \
                    "loadUdevDeviceManager: mObjAudioMixer is null");
                reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INTERNAL_ERROR, "Audiod internal error");
            }
        }
    }
    else
    {
        PM_LOG_ERROR(MSGID_UDEV_MANAGER, INIT_KVCOUNT, \
            "loadUdevDeviceManager: mObjUdevDeviceManager is null");
        reply = STANDARD_JSON_ERROR(AUDIOD_ERRORCODE_INTERNAL_ERROR, "Audiod internal error");
    }
    CLSError lserror;
    if (!LSMessageReply(lshandle, message, reply, &lserror))
        lserror.Print(__FUNCTION__, __LINE__);
    return true;
}

UdevDeviceManager::UdevDeviceManager() : mObjAudioMixer(nullptr)
{
    PM_LOG_INFO(MSGID_UDEV_MANAGER, INIT_KVCOUNT, \
        "UdevDeviceManager constructor");
}

UdevDeviceManager::~UdevDeviceManager()
{
    PM_LOG_INFO(MSGID_UDEV_MANAGER, INIT_KVCOUNT, \
        "UdevDeviceManager destructor");
}