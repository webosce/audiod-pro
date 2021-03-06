// Copyright (c) 2018 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0


#ifndef UMIAUDIOMIXER_H_
#define UMIAUDIOMIXER_H_
#include "AudioMixer.h"
#include <vector>
#include <string>
#include "main.h"
#include "messageUtils.h"
/*Connect and disconnect status of streams to inform scenario modules*/
typedef enum ConnStatus
{
    eUnknown,
    eConnect,
    eDisConnect
}E_CONNSTATUS;

class umiaudiomixer
{

    private :

    /*To store umi mixer ready status*/
    bool mIsReadyToProgram;

    static umiaudiomixer *mObjUmiMixer;

    /*call back pointer to call onsink changed*/
    AudiodCallbacksInterface * mCallbacks;

    /*To store the status if the starem is currently active*/

    std::vector<EVirtualSink> mVectActiveStreams;

    /*Constructor*/
    umiaudiomixer();
    umiaudiomixer(const umiaudiomixer &) = delete;
    umiaudiomixer& operator=(const umiaudiomixer &) = delete;

    /*Destructor*/
    ~umiaudiomixer();

    public:

    /*Initialize umi mixer*/
    void initUmiMixer(GMainLoop * loop, LSHandle * handle, AudiodCallbacksInterface* interface);

    /*Connect hardware stream*/
    bool connectAudio(std::string strSourceName, std::string strPhysicalSinkName, LSFilterFunc cb, envelopeRef *message);

    /*Disconnect hardware stream*/
    bool disconnectAudio(std::string strSourceName, std::string strPhysicalSinkName, LSFilterFunc cb, envelopeRef *message);

    /*Set sound out of TV/ALSA eg:Headphone, TV internal speaker, Soundbar*/
    bool setSoundOut(std::string strOutputMode, LSFilterFunc cb, envelopeRef *message);

    /*Set the volume of speaker */
    bool setMasterVolume(std::string strSoundOutPut, int iVolume, LSFilterFunc cb, envelopeRef *message);

    /*Get the volume of speaker */
    bool getMasterVolume(LSFilterFunc cb, envelopeRef *message);

    /*Volume Up for speaker */
    bool masterVolumeUp(std::string strSoundOutPut, LSFilterFunc cb, envelopeRef *message);

    /*Volume down for speaker */
    bool masterVolumeDown(std::string strSoundOutPut, LSFilterFunc cb, envelopeRef *message);

    /*Volume mute for speaker */
    bool masterVolumeMute(std::string strSoundOutPut, bool bIsMute, LSFilterFunc cb, envelopeRef *message);

    /*Volume unmute for speaker */
    bool inputVolumeMute(std::string strPhysicalSink, std::string strSource, bool bIsMute, LSFilterFunc cb, envelopeRef *message);

    /*To get all the info of connected UMI audio streams*/
    bool getConnectionStatus(LSFilterFunc cb, envelopeRef *message);

    /*To send on sink changed status to all the pulse audiod and umi scenario modules*/
    void onSinkChangedReply(EVirtualSink eVirtualSink, E_CONNSTATUS eConnStatus, ESinkType eSinkType);

    /*To inform scenario modules if the mixer interafce is ready to use, in tis case it audiooutputd luna service status*/
    bool readyToProgram ();

    /*To keep track of active stream types*/
    void updateStreamStatus(EVirtualSink eVirtualSink, E_CONNSTATUS eConnStatus);

    /*To get the status if the starem is currently active*/
    bool isStreamActive(EVirtualSink eVirtualSink);

    /*To update IsReadyToProgram*/
    void setMixerReadyStatus(bool eStatus);

    static umiaudiomixer* getUmiMixerInstance();

    static void createUmiMixerInstance();

    /*To know audiooutputd server status*/
    static bool audiodOutputdServiceStatusCallBack(LSHandle *sh,
    const char *serviceName,
    bool connected,
    void *ctx);
};
#endif /* AUDIOMIXER_H_ */
