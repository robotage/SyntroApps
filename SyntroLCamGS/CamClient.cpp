//
//  Copyright (c) 2014 Scott Ellis and Richard Barnett
//
//  This file is part of SyntroNet
//
//  SyntroNet is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  SyntroNet is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with SyntroNet.  If not, see <http://www.gnu.org/licenses/>.
//

#include "SyntroLib.h"
#include "CamClient.h"
#include "SyntroUtils.h"

#include <qbuffer.h>
#include <qdebug.h>

#include "AVMuxEncodeGS.h"


//#define STATE_DEBUG_ENABLE

#ifdef STATE_DEBUG_ENABLE
#define STATE_DEBUG(x) qDebug() << x
#else
#define STATE_DEBUG(x)
#endif


CamClient::CamClient(QObject *)
    : Endpoint(CAMERA_IMAGE_INTERVAL, "CamClient")
{
    m_avmuxPortHighRate = -1;
    m_avmuxPortLowRate = -1;
    m_controlPort = -1;
    m_sequenceState = CAMCLIENT_STATE_IDLE;
    m_frameCount = 0;
    m_audioSampleCount = 0;
    m_recordIndex = 0;

    m_highRateEncoder = NULL;
    m_lowRateEncoder = NULL;
    m_lastCapsSend = 0;

    m_gotAudioFormat = false;
    m_gotVideoFormat = false;


    QSettings *settings = SyntroUtils::getSettings();

    settings->beginGroup(CAMCLIENT_STREAM_GROUP);

    if (!settings->contains(CAMCLIENT_HIGHRATEVIDEO_MININTERVAL))
        settings->setValue(CAMCLIENT_HIGHRATEVIDEO_MININTERVAL, "30");

    if (!settings->contains(CAMCLIENT_HIGHRATEVIDEO_MAXINTERVAL))
        settings->setValue(CAMCLIENT_HIGHRATEVIDEO_MAXINTERVAL, "10000");

    if (!settings->contains(CAMCLIENT_HIGHRATEVIDEO_NULLINTERVAL))
        settings->setValue(CAMCLIENT_HIGHRATEVIDEO_NULLINTERVAL, "2000");


    if (!settings->contains(CAMCLIENT_AV_TYPE))
        settings->setValue(CAMCLIENT_AV_TYPE, AVMUXENCODE_AV_TYPE_RTPMP4);

    if (!settings->contains(CAMCLIENT_GENERATE_LOWRATE))
        settings->setValue(CAMCLIENT_GENERATE_LOWRATE, 1);

    if (!settings->contains(CAMCLIENT_GS_VIDEO_HIGHRATE))
        settings->setValue(CAMCLIENT_GS_VIDEO_HIGHRATE, 1000000);

    if (!settings->contains(CAMCLIENT_GS_AUDIO_HIGHRATE))
        settings->setValue(CAMCLIENT_GS_AUDIO_HIGHRATE, 64000);

    if (!settings->contains(CAMCLIENT_GS_VIDEO_LOWRATE))
        settings->setValue(CAMCLIENT_GS_VIDEO_LOWRATE, 250000);

    if (!settings->contains(CAMCLIENT_GS_AUDIO_LOWRATE))
        settings->setValue(CAMCLIENT_GS_AUDIO_LOWRATE, 16000);

    if (!settings->contains(CAMCLIENT_MJPEG_LOWFRAMERATE))
        settings->setValue(CAMCLIENT_MJPEG_LOWFRAMERATE, 2);

    settings->endGroup();

    settings->beginGroup(CAMCLIENT_MOTION_GROUP);

    if (!settings->contains(CAMCLIENT_MOTION_TILESTOSKIP))
        settings->setValue(CAMCLIENT_MOTION_TILESTOSKIP, "0");

    if (!settings->contains(CAMCLIENT_MOTION_INTERVALSTOSKIP))
        settings->setValue(CAMCLIENT_MOTION_INTERVALSTOSKIP, "0");

    if (!settings->contains(CAMCLIENT_MOTION_MIN_DELTA))
        settings->setValue(CAMCLIENT_MOTION_MIN_DELTA, "400");

    if (!settings->contains(CAMCLIENT_MOTION_MIN_NOISE))
        settings->setValue(CAMCLIENT_MOTION_MIN_NOISE, "40");

    if (!settings->contains(CAMCLIENT_MOTION_DELTA_INTERVAL))
        settings->setValue(CAMCLIENT_MOTION_DELTA_INTERVAL, "200");

    if (!settings->contains(CAMCLIENT_MOTION_PREROLL))
        settings->setValue(CAMCLIENT_MOTION_PREROLL, "4000");

    if (!settings->contains(CAMCLIENT_MOTION_POSTROLL))
        settings->setValue(CAMCLIENT_MOTION_POSTROLL, "2000");

    settings->endGroup();

    delete settings;
}

CamClient::~CamClient()
{
}

int CamClient::getFrameCount()
{
    int count;

    QMutexLocker lock(&m_frameCountLock);

    count = m_frameCount;
    m_frameCount = 0;
    return count;
}

int CamClient::getAudioSampleCount()
{
    int count;

    QMutexLocker lock(&m_audioSampleLock);

    count = m_audioSampleCount;
    m_audioSampleCount = 0;
    return count;
}

void CamClient::ageOutPrerollQueues(qint64 now)
{
	while (!m_videoPrerollQueue.empty()) {
		if ((now - m_videoPrerollQueue.head()->timestamp) < m_preroll)
			break;

		delete m_videoPrerollQueue.dequeue();
	}

	while (!m_audioPrerollQueue.empty()) {
		if ((now - m_audioPrerollQueue.head()->timestamp) < m_preroll)
			break;

		delete m_audioPrerollQueue.dequeue();
	}

	while (!m_videoLowRatePrerollQueue.empty()) {
		if ((now - m_videoLowRatePrerollQueue.head()->timestamp) < m_preroll)
			break;

		delete m_videoLowRatePrerollQueue.dequeue();
	}

	while (!m_audioLowRatePrerollQueue.empty()) {
		if ((now - m_audioLowRatePrerollQueue.head()->timestamp) < m_preroll)
			break;

		delete m_audioLowRatePrerollQueue.dequeue();
	}
}

//----------------------------------------------------------
//
//  MJPPCM processing

void CamClient::processAVQueueMJPPCM()
{
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    QByteArray jpeg;
    QByteArray audioFrame;
    PREROLL *preroll;
    QString stateString;
    qint64 timestamp;

    switch (m_sequenceState) {
        // waiting for a motion event
        case CAMCLIENT_STATE_IDLE:

        ageOutPrerollQueues(now);

        // if there is a frame, put on preroll queue and check for motion


        if (dequeueVideoFrame(jpeg, timestamp) && SyntroUtils::syntroTimerExpired(now, m_lastPrerollFrameTime, m_minInterval)) {
            m_lastPrerollFrameTime = now;
            preroll = new PREROLL;
            preroll->data = jpeg;
            preroll->param = SYNTRO_RECORDHEADER_PARAM_PREROLL;
            preroll->timestamp = timestamp;
            m_videoPrerollQueue.enqueue(preroll);

            if (m_generateLowRate && SyntroUtils::syntroTimerExpired(now, m_lastLowRatePrerollFrameTime, m_mjpegLowFrameInterval)) {
                m_lastLowRatePrerollFrameTime = now;
                preroll = new PREROLL;
                preroll->data = jpeg;
                preroll->param = SYNTRO_RECORDHEADER_PARAM_PREROLL;
                preroll->timestamp = timestamp;
                m_videoLowRatePrerollQueue.enqueue(preroll);
            }

            // now check for motion if it's time

            if ((now - m_lastDeltaTime) > m_deltaInterval)
                checkForMotion(now, jpeg);
            if (m_imageChanged) {
                m_sequenceState = CAMCLIENT_STATE_PREROLL; // send the preroll frames
                stateString = QString("STATE_PREROLL: queue size %1").arg(m_videoPrerollQueue.size());
                STATE_DEBUG(stateString);
            } else {
                sendHeartbeatFrameMJPPCM(now, jpeg);
            }
        }
        if (dequeueAudioFrame(audioFrame, timestamp)) {
            preroll = new PREROLL;
            preroll->data = audioFrame;
            preroll->param = SYNTRO_RECORDHEADER_PARAM_PREROLL;
            preroll->timestamp = timestamp;
            m_audioPrerollQueue.enqueue(preroll);

            if (m_generateLowRate) {
                preroll = new PREROLL;
                preroll->data = audioFrame;
                preroll->param = SYNTRO_RECORDHEADER_PARAM_PREROLL;
                preroll->timestamp = timestamp;
                m_audioLowRatePrerollQueue.enqueue(preroll);
            }
        }
        break;

        // sending the preroll queue
        case CAMCLIENT_STATE_PREROLL:
        if (clientIsServiceActive(m_avmuxPortHighRate)) {
            if (clientClearToSend(m_avmuxPortHighRate) && (!m_videoPrerollQueue.empty() || !m_audioPrerollQueue.empty())) {
                if (SyntroUtils::syntroTimerExpired(now, m_lastFrameTime, m_minInterval / 4 + 1)) {
                    sendPrerollMJPPCM(true);
                }
            }
        } else {
            while (!m_videoPrerollQueue.empty())             // clear queue if connection not active
                delete m_videoPrerollQueue.dequeue();
            while (!m_audioPrerollQueue.empty())             // clear queue if connection not active
                delete m_audioPrerollQueue.dequeue();
        }
        if (m_generateLowRate && clientIsServiceActive(m_avmuxPortLowRate) && clientClearToSend(m_avmuxPortLowRate)) {
            if ((!m_videoLowRatePrerollQueue.empty() || !m_audioLowRatePrerollQueue.empty())) {
                if (SyntroUtils::syntroTimerExpired(now, m_lastLowRateFrameTime, m_minInterval / 4 + 1)) {
                    sendPrerollMJPPCM(false);
                }
            }
        } else {
            while (!m_videoLowRatePrerollQueue.empty())       // clear queue if connection not active
                delete m_videoLowRatePrerollQueue.dequeue();
            while (!m_audioLowRatePrerollQueue.empty())       // clear queue if connection not active
                delete m_audioLowRatePrerollQueue.dequeue();
        }
        if (m_videoPrerollQueue.empty() && m_audioPrerollQueue.empty() &&
                m_videoLowRatePrerollQueue.empty() && (m_audioLowRatePrerollQueue.empty())) {
            m_sequenceState = CAMCLIENT_STATE_INSEQUENCE;
            STATE_DEBUG("STATE_INSEQUENCE");
            m_lastChangeTime = now;                             // in case pre-roll sending took a while
        }

        // keep putting frames on preroll queue while sending real preroll

        if (dequeueVideoFrame(jpeg, timestamp) && SyntroUtils::syntroTimerExpired(now, m_lastPrerollFrameTime, m_minInterval)) {
                m_lastPrerollFrameTime = now;
                preroll = new PREROLL;
                preroll->data = jpeg;
                preroll->param = SYNTRO_RECORDHEADER_PARAM_NORMAL;
                preroll->timestamp = timestamp;
                m_videoPrerollQueue.enqueue(preroll);
                if (m_generateLowRate && SyntroUtils::syntroTimerExpired(now, m_lastLowRatePrerollFrameTime, m_mjpegLowFrameInterval)) {
                    m_lastLowRatePrerollFrameTime = now;
                    preroll = new PREROLL;
                    preroll->data = jpeg;
                    preroll->param = SYNTRO_RECORDHEADER_PARAM_PREROLL;
                    preroll->timestamp = timestamp;
                    m_videoLowRatePrerollQueue.enqueue(preroll);
                }
            }

            if (dequeueAudioFrame(audioFrame, timestamp)) {
                preroll = new PREROLL;
                preroll->data = audioFrame;
                preroll->param = SYNTRO_RECORDHEADER_PARAM_NORMAL;
                preroll->timestamp = timestamp;
                m_audioPrerollQueue.enqueue(preroll);

                if (m_generateLowRate) {
                    preroll = new PREROLL;
                    preroll->data = audioFrame;
                    preroll->param = SYNTRO_RECORDHEADER_PARAM_PREROLL;
                    preroll->timestamp = timestamp;
                    m_audioLowRatePrerollQueue.enqueue(preroll);
                }
            }
            break;

        // in the motion sequence
        case CAMCLIENT_STATE_INSEQUENCE:
            if (!sendAVMJPPCM(now, SYNTRO_RECORDHEADER_PARAM_NORMAL, true)) {
                // no motion detected
                m_sequenceState = CAMCLIENT_STATE_POSTROLL; // no motion so go into postroll state
                m_lastChangeTime = now;                        // this sets the start tiem for the postroll
                STATE_DEBUG("STATE_POSTROLL");
            }
            break;

        // handle the post roll stage
        case CAMCLIENT_STATE_POSTROLL:

            if (SyntroUtils::syntroTimerExpired(now, m_lastChangeTime, m_postroll)) {
                // postroll complete
                m_sequenceState = CAMCLIENT_STATE_IDLE;
                m_lastPrerollFrameTime = m_lastFrameTime;
                STATE_DEBUG("STATE_IDLE");
                break;
            }

            // see if anything to send

            if (sendAVMJPPCM(now, SYNTRO_RECORDHEADER_PARAM_POSTROLL, true)) {
                // motion detected again
                m_sequenceState = CAMCLIENT_STATE_INSEQUENCE;
                STATE_DEBUG("Returning to STATE_INSEQUENCE");
            }
            break;

        // in the motion sequence
        case CAMCLIENT_STATE_CONTINUOUS:
            sendAVMJPPCM(now, SYNTRO_RECORDHEADER_PARAM_NORMAL, false);
            break;
    }
}

void CamClient::sendPrerollMJPPCM(bool highRate)
{
    PREROLL *videoPreroll = NULL;
    PREROLL *audioPreroll = NULL;
    int videoSize = 0;
    int audioSize = 0;

    if (highRate) {
        if (!m_videoPrerollQueue.empty()) {
            videoPreroll = m_videoPrerollQueue.dequeue();
            videoSize = videoPreroll->data.size();
            m_lastFrameTime = QDateTime::currentMSecsSinceEpoch();
        }
        if (!m_audioPrerollQueue.empty()) {
            audioPreroll = m_audioPrerollQueue.dequeue();
            audioSize = audioPreroll->data.size();
        }

        if ((videoPreroll != NULL) || (audioPreroll != NULL)) {
            SYNTRO_EHEAD *multiCast = clientBuildMessage(m_avmuxPortHighRate, sizeof(SYNTRO_RECORD_AVMUX) + videoSize + audioSize);
            SYNTRO_RECORD_AVMUX *avHead = (SYNTRO_RECORD_AVMUX *)(multiCast + 1);
            SyntroUtils::avmuxHeaderInit(avHead, &m_avParams, SYNTRO_RECORDHEADER_PARAM_PREROLL, m_recordIndex++, 0, videoSize, audioSize);

            if (audioPreroll != NULL)
                SyntroUtils::convertInt64ToUC8(audioPreroll->timestamp, avHead->recordHeader.timestamp);
            if (videoPreroll != NULL)
                SyntroUtils::convertInt64ToUC8(videoPreroll->timestamp, avHead->recordHeader.timestamp);

            unsigned char *ptr = (unsigned char *)(avHead + 1);

            if (videoSize > 0) {
                memcpy(ptr, videoPreroll->data.data(), videoSize);
                ptr += videoSize;
            }

            if (audioSize > 0)
                memcpy(ptr, audioPreroll->data.data(), audioSize);

            int length = sizeof(SYNTRO_RECORD_AVMUX) + videoSize + audioSize;
            clientSendMessage(m_avmuxPortHighRate, multiCast, length, SYNTROLINK_MEDPRI);
        }
    } else {
        if (!m_videoLowRatePrerollQueue.empty()) {
            videoPreroll = m_videoLowRatePrerollQueue.dequeue();
            videoSize = videoPreroll->data.size();
            m_lastLowRateFrameTime = SyntroClock();
        }
        if (!m_audioLowRatePrerollQueue.empty()) {
            audioPreroll = m_audioLowRatePrerollQueue.dequeue();
            audioSize = audioPreroll->data.size();
        }

        if ((videoPreroll != NULL) || (audioPreroll == NULL)) {
            SYNTRO_EHEAD *multiCast = clientBuildMessage(m_avmuxPortLowRate, sizeof(SYNTRO_RECORD_AVMUX) + videoSize + audioSize);
            SYNTRO_RECORD_AVMUX *avHead = (SYNTRO_RECORD_AVMUX *)(multiCast + 1);
            SyntroUtils::avmuxHeaderInit(avHead, &m_avParams, SYNTRO_RECORDHEADER_PARAM_PREROLL, m_recordIndex++, 0, videoSize, audioSize);

            if (audioPreroll != NULL)
                SyntroUtils::convertInt64ToUC8(audioPreroll->timestamp, avHead->recordHeader.timestamp);
            if (videoPreroll != NULL)
                SyntroUtils::convertInt64ToUC8(videoPreroll->timestamp, avHead->recordHeader.timestamp);

            unsigned char *ptr = (unsigned char *)(avHead + 1);

            if (videoSize > 0) {
                memcpy(ptr, videoPreroll->data.data(), videoSize);
                ptr += videoSize;
            }

            if (audioSize > 0)
                memcpy(ptr, audioPreroll->data.data(), audioSize);

            int length = sizeof(SYNTRO_RECORD_AVMUX) + videoSize + audioSize;
            clientSendMessage(m_avmuxPortLowRate, multiCast, length, SYNTROLINK_MEDPRI);
        }
    }

    if (videoPreroll != NULL)
        delete videoPreroll;
    if (audioPreroll != NULL)
        delete audioPreroll;
}

bool CamClient::sendAVMJPPCM(qint64 now, int param, bool checkMotion)
{
    qint64 videoTimestamp;
    qint64 audioTimestamp;
    QByteArray jpeg;
    QByteArray audioFrame;
    bool lowRateExpired;

    if (clientIsServiceActive(m_avmuxPortHighRate) && !clientClearToSend(m_avmuxPortHighRate))
        return false;                                       // make sure high rate gets every frame if active

    // see if anything to send

    bool videoValid = dequeueVideoFrame(jpeg, videoTimestamp);
    bool audioValid = dequeueAudioFrame(audioFrame, audioTimestamp);

    if (clientIsServiceActive(m_avmuxPortHighRate)) {
        if (!SyntroUtils::syntroTimerExpired(now, m_lastFullFrameTime, m_minInterval)) {
            jpeg.clear();                                       // just discard video as too soon
            if (SyntroUtils::syntroTimerExpired(now, m_lastFrameTime, m_nullFrameInterval)) {
                sendNullFrameMJPPCM(now, true);                 // in case very long time
            }
        }
    }
    if (m_generateLowRate && clientIsServiceActive(m_avmuxPortLowRate)) {
        if (!SyntroUtils::syntroTimerExpired(now, m_lastLowRateFullFrameTime, m_minInterval)) {
            jpeg.clear();                                       // just discard video as too soon
            if (SyntroUtils::syntroTimerExpired(now, m_lastLowRateFrameTime, m_nullFrameInterval)) {
                sendNullFrameMJPPCM(now, false);                // in case very long time
            }
        }
    }
    if (videoValid || audioValid) {
        if (clientIsServiceActive(m_avmuxPortHighRate) && clientClearToSend(m_avmuxPortHighRate) ) {
            SYNTRO_EHEAD *multiCast = clientBuildMessage(m_avmuxPortHighRate, sizeof(SYNTRO_RECORD_AVMUX) + jpeg.size() + audioFrame.size());
            SYNTRO_RECORD_AVMUX *avHead = (SYNTRO_RECORD_AVMUX *)(multiCast + 1);
            SyntroUtils::avmuxHeaderInit(avHead, &m_avParams, param, m_recordIndex++, 0, jpeg.size(), audioFrame.size());
            if (audioValid)
                SyntroUtils::convertInt64ToUC8(audioTimestamp, avHead->recordHeader.timestamp);
            if (videoValid)
                SyntroUtils::convertInt64ToUC8(videoTimestamp, avHead->recordHeader.timestamp);

            unsigned char *ptr = (unsigned char *)(avHead + 1);

            if (jpeg.size() > 0) {
                memcpy(ptr, jpeg.data(), jpeg.size());
                m_lastFullFrameTime = m_lastFrameTime = now;
                ptr += jpeg.size();
            }

            if (audioFrame.size() > 0)
                memcpy(ptr, audioFrame.data(), audioFrame.size());

            int length = sizeof(SYNTRO_RECORD_AVMUX) + jpeg.size() + audioFrame.size();
            clientSendMessage(m_avmuxPortHighRate, multiCast, length, SYNTROLINK_MEDPRI);
        }

        lowRateExpired = SyntroUtils::syntroTimerExpired(now, m_lastLowRateFrameTime, m_mjpegLowFrameInterval);
        if (m_generateLowRate && clientIsServiceActive(m_avmuxPortLowRate) && clientClearToSend(m_avmuxPortLowRate) &&
                (lowRateExpired || audioValid)) {
            if (lowRateExpired)
                m_lastLowRateFrameTime = now;
            SYNTRO_EHEAD *multiCast = clientBuildMessage(m_avmuxPortLowRate, sizeof(SYNTRO_RECORD_AVMUX) + jpeg.size() + audioFrame.size());
            SYNTRO_RECORD_AVMUX *avHead = (SYNTRO_RECORD_AVMUX *)(multiCast + 1);
            SyntroUtils::avmuxHeaderInit(avHead, &m_avParams, param, m_recordIndex++, 0, jpeg.size(), audioFrame.size());
            if (audioValid)
                SyntroUtils::convertInt64ToUC8(audioTimestamp, avHead->recordHeader.timestamp);
            if (videoValid)
                SyntroUtils::convertInt64ToUC8(videoTimestamp, avHead->recordHeader.timestamp);

            unsigned char *ptr = (unsigned char *)(avHead + 1);

            if (jpeg.size() > 0) {
                memcpy(ptr, jpeg.data(), jpeg.size());
                m_lastLowRateFullFrameTime = m_lastLowRateFrameTime = now;
                ptr += jpeg.size();
            }

            if (audioFrame.size() > 0)
                memcpy(ptr, audioFrame.data(), audioFrame.size());

            int length = sizeof(SYNTRO_RECORD_AVMUX) + jpeg.size() + audioFrame.size();
            clientSendMessage(m_avmuxPortLowRate, multiCast, length, SYNTROLINK_MEDPRI);
        }
    }

    if ((jpeg.size() > 0) && checkMotion) {
        if ((now - m_lastDeltaTime) > m_deltaInterval)
            checkForMotion(now, jpeg);
        return m_imageChanged;                              // image may have changed
    }
    return false;                                           // change not processed
}


void CamClient::sendNullFrameMJPPCM(qint64 now, bool highRate)
{
    if (highRate) {
        if (clientIsServiceActive(m_avmuxPortHighRate) && clientClearToSend(m_avmuxPortHighRate)) {
            SYNTRO_EHEAD *multiCast = clientBuildMessage(m_avmuxPortHighRate, sizeof(SYNTRO_RECORD_AVMUX));
            SYNTRO_RECORD_AVMUX *videoHead = (SYNTRO_RECORD_AVMUX *)(multiCast + 1);
            SyntroUtils::avmuxHeaderInit(videoHead, &m_avParams, SYNTRO_RECORDHEADER_PARAM_NOOP, m_recordIndex++, 0, 0, 0);
            int length = sizeof(SYNTRO_RECORD_AVMUX);
            clientSendMessage(m_avmuxPortHighRate, multiCast, length, SYNTROLINK_LOWPRI);
            m_lastFrameTime = now;
        }
    } else {

        if (m_generateLowRate && clientIsServiceActive(m_avmuxPortLowRate) && clientClearToSend(m_avmuxPortLowRate)) {
            SYNTRO_EHEAD *multiCast = clientBuildMessage(m_avmuxPortLowRate, sizeof(SYNTRO_RECORD_AVMUX));
            SYNTRO_RECORD_AVMUX *videoHead = (SYNTRO_RECORD_AVMUX *)(multiCast + 1);
            SyntroUtils::avmuxHeaderInit(videoHead, &m_avParams, SYNTRO_RECORDHEADER_PARAM_NOOP, m_recordIndex++, 0, 0, 0);
            int length = sizeof(SYNTRO_RECORD_AVMUX);
            clientSendMessage(m_avmuxPortLowRate, multiCast, length, SYNTROLINK_LOWPRI);
            m_lastLowRateFrameTime = now;
        }
    }
}

void CamClient::sendHeartbeatFrameMJPPCM(qint64 now, const QByteArray& jpeg)
{
    if (clientIsServiceActive(m_avmuxPortHighRate) && clientClearToSend(m_avmuxPortHighRate) &&
            SyntroUtils::syntroTimerExpired(now, m_lastFullFrameTime, m_fullFrameMaxInterval)) {
        SYNTRO_EHEAD *multiCast = clientBuildMessage(m_avmuxPortHighRate, sizeof(SYNTRO_RECORD_AVMUX) + jpeg.size());
        SYNTRO_RECORD_AVMUX *videoHead = (SYNTRO_RECORD_AVMUX *)(multiCast + 1);
        SyntroUtils::avmuxHeaderInit(videoHead, &m_avParams, SYNTRO_RECORDHEADER_PARAM_REFRESH, m_recordIndex++, 0, jpeg.size(), 0);
        memcpy((unsigned char *)(videoHead + 1), jpeg.data(), jpeg.size());
        int length = sizeof(SYNTRO_RECORD_AVMUX) + jpeg.size();
        clientSendMessage(m_avmuxPortHighRate, multiCast, length, SYNTROLINK_LOWPRI);
        m_lastFrameTime = m_lastFullFrameTime = now;
    }

    if (m_generateLowRate && clientIsServiceActive(m_avmuxPortLowRate) && clientClearToSend(m_avmuxPortLowRate) &&
            SyntroUtils::syntroTimerExpired(now, m_lastLowRateFullFrameTime, m_fullFrameMaxInterval)) {
        SYNTRO_EHEAD *multiCast = clientBuildMessage(m_avmuxPortLowRate, sizeof(SYNTRO_RECORD_AVMUX) + jpeg.size());
        SYNTRO_RECORD_AVMUX *videoHead = (SYNTRO_RECORD_AVMUX *)(multiCast + 1);
        SyntroUtils::avmuxHeaderInit(videoHead, &m_avParams, SYNTRO_RECORDHEADER_PARAM_REFRESH, m_recordIndex++, 0, jpeg.size(), 0);
        memcpy((unsigned char *)(videoHead + 1), jpeg.data(), jpeg.size());
        int length = sizeof(SYNTRO_RECORD_AVMUX) + jpeg.size();
        clientSendMessage(m_avmuxPortLowRate, multiCast, length, SYNTROLINK_LOWPRI);
        m_lastLowRateFrameTime = m_lastLowRateFullFrameTime = now;
    }

    if (SyntroUtils::syntroTimerExpired(now, m_lastFrameTime, m_nullFrameInterval))
        sendNullFrameMJPPCM(now, true);
    if (SyntroUtils::syntroTimerExpired(now, m_lastLowRateFrameTime, m_nullFrameInterval))
        sendNullFrameMJPPCM(now, false);
}
//----------------------------------------------------------


//----------------------------------------------------------
//
//  RTP processing


void CamClient::processAVQueueRTP()
{
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    QByteArray jpeg;
    QByteArray audioFrame;
    PREROLL *preroll;
    QString stateString;
    qint64 timestamp;

    switch (m_sequenceState) {
        // waiting for a motion event
        case CAMCLIENT_STATE_IDLE:
		ageOutPrerollQueues(now);

        // if there is a frame, put on preroll queue and check for motion

        if (dequeueVideoFrame(jpeg, timestamp)) {
            m_lastPrerollFrameTime = now;
            preroll = new PREROLL;
            preroll->data = jpeg;
            preroll->param = SYNTRO_RECORDHEADER_PARAM_PREROLL;
            preroll->timestamp = timestamp;
            m_videoPrerollQueue.enqueue(preroll);

            // now check for motion if it's time

            if ((now - m_lastDeltaTime) > m_deltaInterval)
                checkForMotion(now, jpeg);
            if (m_imageChanged) {
                m_sequenceState = CAMCLIENT_STATE_PREROLL; // send the preroll frames
                stateString = QString("STATE_PREROLL: queue size %1").arg(m_videoPrerollQueue.size());
                STATE_DEBUG(stateString);
            } else {
                sendHeartbeatFrameRTP(now, jpeg);
            }
        }

        if (dequeueAudioFrame(audioFrame, timestamp)) {
            preroll = new PREROLL;
            preroll->data = audioFrame;
            preroll->param = SYNTRO_RECORDHEADER_PARAM_PREROLL;
            preroll->timestamp = timestamp;
            m_audioPrerollQueue.enqueue(preroll);
        }
        break;

        // sending the preroll queue
        case CAMCLIENT_STATE_PREROLL:
            if (clientIsServiceActive(m_avmuxPortHighRate)) {
                if (clientClearToSend(m_avmuxPortHighRate) && (!m_videoPrerollQueue.empty() || !m_audioPrerollQueue.empty())) {
                    sendPrerollRTP();
                }
            } else {
                while (!m_videoPrerollQueue.empty())             // clear queue if connection not active
                    delete m_videoPrerollQueue.dequeue();
                while (!m_audioPrerollQueue.empty())             // clear queue if connection not active
                    delete m_audioPrerollQueue.dequeue();
            }
            if (m_videoPrerollQueue.empty() && (m_audioPrerollQueue.empty())) {
                 m_sequenceState = CAMCLIENT_STATE_INSEQUENCE;
                 STATE_DEBUG("STATE_INSEQUENCE");
                 m_lastChangeTime = now;                    // in case pre-roll sending took a while
            }

            // keep putting frames on preroll queue while sending real preroll

            if (dequeueVideoFrame(jpeg, timestamp)) {
                m_lastPrerollFrameTime = now;
                preroll = new PREROLL;
                preroll->data = jpeg;
                preroll->param = SYNTRO_RECORDHEADER_PARAM_NORMAL;
                preroll->timestamp = timestamp;
                m_videoPrerollQueue.enqueue(preroll);
            }

            if (dequeueAudioFrame(audioFrame, timestamp)) {
                preroll = new PREROLL;
                preroll->data = audioFrame;
                preroll->param = SYNTRO_RECORDHEADER_PARAM_NORMAL;
                preroll->timestamp = timestamp;
                m_audioPrerollQueue.enqueue(preroll);

            }
            break;

        // in the motion sequence
        case CAMCLIENT_STATE_INSEQUENCE:
            if (!sendAVRTP(now, SYNTRO_RECORDHEADER_PARAM_NORMAL, true)) {
                // no motion detected
                m_sequenceState = CAMCLIENT_STATE_POSTROLL; // no motion so go into postroll state
                m_lastChangeTime = now;                        // this sets the start tiem for the postroll
                STATE_DEBUG("STATE_POSTROLL");
            }
            break;

        // handle the post roll stage
        case CAMCLIENT_STATE_POSTROLL:

            if (SyntroUtils::syntroTimerExpired(now, m_lastChangeTime, m_postroll)) {
                // postroll complete
                m_sequenceState = CAMCLIENT_STATE_IDLE;
                m_lastPrerollFrameTime = m_lastFrameTime;
                STATE_DEBUG("STATE_IDLE");
                break;
            }

            // see if anything to send

            if (sendAVRTP(now, SYNTRO_RECORDHEADER_PARAM_POSTROLL, true)) {
                // motion detected again
                m_sequenceState = CAMCLIENT_STATE_INSEQUENCE;
                STATE_DEBUG("Returning to STATE_INSEQUENCE");
            }
            break;

        // in the motion sequence
        case CAMCLIENT_STATE_CONTINUOUS:
            sendAVRTP(now, SYNTRO_RECORDHEADER_PARAM_NORMAL, false);
            break;
    }
}

void CamClient::sendPrerollRTP()
{
    PREROLL *videoPreroll = NULL;
    PREROLL *audioPreroll = NULL;

    if (!m_videoPrerollQueue.empty()) {
        videoPreroll = m_videoPrerollQueue.dequeue();
        m_highRateEncoder->newVideoData(videoPreroll->data, videoPreroll->timestamp, videoPreroll->param);
        if (m_generateLowRate)
            m_lowRateEncoder->newVideoData(videoPreroll->data, videoPreroll->timestamp, videoPreroll->param);
        m_lastFrameTime = QDateTime::currentMSecsSinceEpoch();
        delete videoPreroll;
    }

    if (!m_audioPrerollQueue.empty()) {
        audioPreroll = m_audioPrerollQueue.dequeue();
        m_highRateEncoder->newAudioData(audioPreroll->data, audioPreroll->timestamp, audioPreroll->param);
        if (m_generateLowRate)
            m_lowRateEncoder->newAudioData(audioPreroll->data, audioPreroll->timestamp, audioPreroll->param);
        delete audioPreroll;
    }
}

bool CamClient::sendAVRTP(qint64 now, int param, bool checkMotion)
{
    QByteArray jpeg;
    QByteArray audioFrame;
    qint64 timestamp;

    if (!(clientIsServiceActive(m_avmuxPortHighRate) || (m_generateLowRate && clientIsServiceActive(m_avmuxPortLowRate)))) {
        clearVideoQueue();
        clearAudioQueue();
        return false;
    }

    if (dequeueVideoFrame(jpeg, timestamp)) {
        if (SyntroUtils::syntroTimerExpired(now, m_lastFrameTime, m_minInterval)) {
            if (clientIsServiceActive(m_avmuxPortHighRate))
                m_highRateEncoder->newVideoData(jpeg, timestamp, param);
            if ((m_generateLowRate) && clientIsServiceActive(m_avmuxPortLowRate))
                m_lowRateEncoder->newVideoData(jpeg, timestamp, param);
            m_lastFrameTime = m_lastFullFrameTime = now;
        }
        sendHeartbeatFrameRTP(now, jpeg);                   // do this to ensure refreshes even in motion events
    }

    if (dequeueAudioFrame(audioFrame, timestamp)) {
        if (clientIsServiceActive(m_avmuxPortHighRate))
            m_highRateEncoder->newAudioData(audioFrame, timestamp, param);
        if ((m_generateLowRate) && clientIsServiceActive(m_avmuxPortLowRate))
            m_lowRateEncoder->newAudioData(audioFrame, timestamp, param);
    }

    if ((jpeg.size() > 0) && checkMotion) {
        if ((now - m_lastDeltaTime) > m_deltaInterval)
            checkForMotion(now, jpeg);
        return m_imageChanged;                              // image may have changed
    }
    return false;                                           // change not processed
}


void CamClient::sendNullFrameRTP(qint64 now)
{
    if (clientIsServiceActive(m_avmuxPortHighRate) && clientClearToSend(m_avmuxPortHighRate)) {
        SYNTRO_EHEAD *multiCast = clientBuildMessage(m_avmuxPortHighRate, sizeof(SYNTRO_RECORD_AVMUX));
        SYNTRO_RECORD_AVMUX *videoHead = (SYNTRO_RECORD_AVMUX *)(multiCast + 1);
        SyntroUtils::avmuxHeaderInit(videoHead, &m_avParams, SYNTRO_RECORDHEADER_PARAM_NOOP, m_recordIndex++, 0, 0, 0);
        int length = sizeof(SYNTRO_RECORD_AVMUX);
        clientSendMessage(m_avmuxPortHighRate, multiCast, length, SYNTROLINK_LOWPRI);
    }

    if (m_generateLowRate && clientIsServiceActive(m_avmuxPortLowRate) && clientClearToSend(m_avmuxPortLowRate)) {
        SYNTRO_EHEAD *multiCast = clientBuildMessage(m_avmuxPortLowRate, sizeof(SYNTRO_RECORD_AVMUX));
        SYNTRO_RECORD_AVMUX *videoHead = (SYNTRO_RECORD_AVMUX *)(multiCast + 1);
        SyntroUtils::avmuxHeaderInit(videoHead, &m_avParams, SYNTRO_RECORDHEADER_PARAM_NOOP, m_recordIndex++, 0, 0, 0);
        int length = sizeof(SYNTRO_RECORD_AVMUX);
        clientSendMessage(m_avmuxPortLowRate, multiCast, length, SYNTROLINK_LOWPRI);
    }
    m_lastFrameTime = now;
}

void CamClient::sendHeartbeatFrameRTP(qint64 now, const QByteArray& jpeg)
{

    if (SyntroUtils::syntroTimerExpired(now, m_lastRefreshTime, m_fullFrameMaxInterval)) {
        if (clientIsServiceActive(m_avmuxPortHighRate) && clientClearToSend(m_avmuxPortHighRate)) {
            SYNTRO_EHEAD *multiCast = clientBuildMessage(m_avmuxPortHighRate, sizeof(SYNTRO_RECORD_AVMUX) + jpeg.size());
            SYNTRO_RECORD_AVMUX *videoHead = (SYNTRO_RECORD_AVMUX *)(multiCast + 1);
            SyntroUtils::avmuxHeaderInit(videoHead, &m_avParams, SYNTRO_RECORDHEADER_PARAM_REFRESH, m_recordIndex++, 0, jpeg.size(), 0);
            memcpy((unsigned char *)(videoHead + 1), jpeg.data(), jpeg.size());
            int length = sizeof(SYNTRO_RECORD_AVMUX) + jpeg.size();
            clientSendMessage(m_avmuxPortHighRate, multiCast, length, SYNTROLINK_LOWPRI);
        }
        if (m_generateLowRate && clientIsServiceActive(m_avmuxPortLowRate) && clientClearToSend(m_avmuxPortLowRate)) {
            SYNTRO_EHEAD *multiCast = clientBuildMessage(m_avmuxPortLowRate, sizeof(SYNTRO_RECORD_AVMUX) + jpeg.size());
            SYNTRO_RECORD_AVMUX *videoHead = (SYNTRO_RECORD_AVMUX *)(multiCast + 1);
            SyntroUtils::avmuxHeaderInit(videoHead, &m_avParams, SYNTRO_RECORDHEADER_PARAM_REFRESH, m_recordIndex++, 0, jpeg.size(), 0);
            memcpy((unsigned char *)(videoHead + 1), jpeg.data(), jpeg.size());
            int length = sizeof(SYNTRO_RECORD_AVMUX) + jpeg.size();
            clientSendMessage(m_avmuxPortLowRate, multiCast, length, SYNTROLINK_LOWPRI);
        }
        m_lastFrameTime = m_lastRefreshTime = now;
    }
    if (SyntroUtils::syntroTimerExpired(now, m_lastFrameTime, m_nullFrameInterval)) {
        // send null frame as no changes
        sendNullFrameRTP(now);
    }
}
//----------------------------------------------------------


void CamClient::checkForMotion(qint64 now, QByteArray& jpeg)
{
    // these five lines are a hack because my jpeg decoder doesn't work for webcam jpegs
    // Qt produces a jpeg that my code likes

 /*   QImage frame;
    frame.loadFromData(jpeg, "JPEG");
    QBuffer buffer(&jpeg);
    buffer.open(QIODevice::WriteOnly);
    frame.save(&buffer, "JPEG");
*/
    m_imageChanged = m_cd.imageChanged(jpeg);
    if (m_imageChanged)
        m_lastChangeTime = now;
    m_lastDeltaTime = now;
}


void CamClient::processEncoderQueue()
{
    QByteArray data;

    int videoLength, audioLength;
    SYNTRO_UC4 lengthData;
    QByteArray videoArray;
    QByteArray audioArray;
    int totalLength;
    unsigned char *ptr;
    qint64 videoTimestamp;
    qint64 audioTimestamp;
    int videoParam;
    int audioParam;

    if (!clientIsServiceActive(m_avmuxPortHighRate)) {             // just discard encoder queue
        while (m_highRateEncoder->getCompressedVideo(data, videoTimestamp, videoParam))
            ;
        while (m_highRateEncoder->getCompressedAudio(data, audioTimestamp, audioParam))
            ;
    }
    if (m_generateLowRate && !clientIsServiceActive(m_avmuxPortLowRate)) {
        while (m_lowRateEncoder->getCompressedVideo(data, videoTimestamp, videoParam))
            ;
        while (m_lowRateEncoder->getCompressedAudio(data, audioTimestamp, audioParam))
            ;
    }

    if (SyntroUtils::syntroTimerExpired(QDateTime::currentMSecsSinceEpoch(), m_lastCapsSend, CAMCLIENT_CAPS_INTERVAL)) {
        qDebug() << "caps";
        sendHighRateCaps();
        sendLowRateCaps();
        m_lastCapsSend = QDateTime::currentMSecsSinceEpoch();
    }

    if (clientIsServiceActive(m_avmuxPortHighRate) && clientClearToSend(m_avmuxPortHighRate)) {
        videoArray.clear();
        audioArray.clear();

        while (m_highRateEncoder->getCompressedVideo(data, videoTimestamp, videoParam)) {
            SyntroUtils::convertIntToUC4(data.length(), lengthData);
            videoArray.append((const char *)lengthData, 4);
            videoArray.append(data);
        }
        videoLength = videoArray.length();

        while (m_highRateEncoder->getCompressedAudio(data, audioTimestamp, audioParam)) {
            SyntroUtils::convertIntToUC4(data.length(), lengthData);
            audioArray.append((const char *)lengthData, 4);
            audioArray.append(data);
        }
        audioLength = audioArray.length();

        totalLength = videoLength + audioLength;

        if (totalLength > 0) {
            SYNTRO_EHEAD *multiCast = clientBuildMessage(m_avmuxPortHighRate, sizeof(SYNTRO_RECORD_AVMUX) + totalLength);
            SYNTRO_RECORD_AVMUX *avmux = (SYNTRO_RECORD_AVMUX *)(multiCast + 1);
            SyntroUtils::avmuxHeaderInit(avmux, &m_avParams, SYNTRO_RECORDHEADER_PARAM_NORMAL, m_recordIndex++, 0, videoLength, audioLength);

            if ((audioLength > 0) && (videoLength == 0)) {
                SyntroUtils::convertInt64ToUC8(audioTimestamp, avmux->recordHeader.timestamp);
            SyntroUtils::convertIntToUC2(audioParam, avmux->recordHeader.param);
            } else {
                SyntroUtils::convertInt64ToUC8(videoTimestamp, avmux->recordHeader.timestamp);
                SyntroUtils::convertIntToUC2(videoParam, avmux->recordHeader.param);
            }

            ptr = (unsigned char *)(avmux + 1);

            if (videoLength > 0) {
                memcpy(ptr, videoArray.constData(), videoLength);
                ptr += videoLength;
            }

            if (audioLength > 0) {
                memcpy(ptr, audioArray.constData(), audioLength);
                ptr += audioLength;
            }
            clientSendMessage(m_avmuxPortHighRate, multiCast, sizeof(SYNTRO_RECORD_AVMUX) + totalLength, SYNTROLINK_MEDPRI);
        }
    }

    if (m_generateLowRate && clientIsServiceActive(m_avmuxPortLowRate) && clientClearToSend(m_avmuxPortLowRate)) {
        videoArray.clear();
        audioArray.clear();

        while (m_lowRateEncoder->getCompressedVideo(data, videoTimestamp, videoParam)) {
            SyntroUtils::convertIntToUC4(data.length(), lengthData);
            videoArray.append((const char *)lengthData, 4);
            videoArray.append(data);
        }
        videoLength = videoArray.length();

        while (m_lowRateEncoder->getCompressedAudio(data, audioTimestamp, audioParam)) {
            SyntroUtils::convertIntToUC4(data.length(), lengthData);
            audioArray.append((const char *)lengthData, 4);
            audioArray.append(data);
        }
        audioLength = audioArray.length();

        totalLength = videoLength + audioLength;

        if (totalLength > 0) {
            SYNTRO_EHEAD *multiCast = clientBuildMessage(m_avmuxPortLowRate, sizeof(SYNTRO_RECORD_AVMUX) + totalLength);
            SYNTRO_RECORD_AVMUX *avmux = (SYNTRO_RECORD_AVMUX *)(multiCast + 1);
            SyntroUtils::avmuxHeaderInit(avmux, &m_avParams, SYNTRO_RECORDHEADER_PARAM_NORMAL, m_recordIndex++, 0, videoLength, audioLength);

            if ((audioLength > 0) && (videoLength == 0)) {
                SyntroUtils::convertInt64ToUC8(audioTimestamp, avmux->recordHeader.timestamp);
            SyntroUtils::convertIntToUC2(audioParam, avmux->recordHeader.param);
            } else {
                SyntroUtils::convertInt64ToUC8(videoTimestamp, avmux->recordHeader.timestamp);
                SyntroUtils::convertIntToUC2(videoParam, avmux->recordHeader.param);
            }

            ptr = (unsigned char *)(avmux + 1);

            if (videoLength > 0) {
                memcpy(ptr, videoArray.constData(), videoLength);
                ptr += videoLength;
            }

            if (audioLength > 0) {
                memcpy(ptr, audioArray.constData(), audioLength);
                ptr += audioLength;
            }
            clientSendMessage(m_avmuxPortLowRate, multiCast, sizeof(SYNTRO_RECORD_AVMUX) + totalLength, SYNTROLINK_MEDPRI);
        }
    }
}

void CamClient::appClientInit()
{
	newStream();
}

void CamClient::appClientExit()
{
    clearQueues();
    if (m_avParams.avmuxSubtype == SYNTRO_RECORD_TYPE_AVMUX_RTP) {
        m_highRateEncoder->exitThread();
        if (m_generateLowRate)
            m_lowRateEncoder->exitThread();
    }
}

void CamClient::appClientBackground()
{
    if (m_avParams.avmuxSubtype == SYNTRO_RECORD_TYPE_AVMUX_RTP) {
        processAVQueueRTP();
        processEncoderQueue();
    } else {
        processAVQueueMJPPCM();
    }
}

void CamClient::appClientConnected()
{
    clearQueues();
}

void CamClient::appClientReceiveE2E(int servicePort, SYNTRO_EHEAD *header, int /*length*/)
{
    if (servicePort != m_controlPort) {
        logWarn(QString("Received E2E on incorrect port %1").arg(m_controlPort));
		free(header);
		return;
	}

	free(header);
}

bool CamClient::dequeueVideoFrame(QByteArray& videoData, qint64& timestamp)
{
    QMutexLocker lock(&m_videoQMutex);

    if (m_videoFrameQ.empty())
        return false;

    CLIENT_QUEUEDATA *qd = m_videoFrameQ.dequeue();
    videoData = qd->data;
    timestamp = qd->timestamp;
    delete qd;
    return true;
}

bool CamClient::dequeueAudioFrame(QByteArray& audioData, qint64& timestamp)
{
    QMutexLocker lock(&m_audioQMutex);

    if (m_audioFrameQ.empty())
        return false;

    CLIENT_QUEUEDATA *qd = m_audioFrameQ.dequeue();
    audioData = qd->data;
    timestamp = qd->timestamp;
    delete qd;
    return true;
}

void CamClient::clearVideoQueue(){
    QMutexLocker lock(&m_videoQMutex);

    while (!m_videoFrameQ.empty())
        delete m_videoFrameQ.dequeue();
}

void CamClient::clearAudioQueue(){
    QMutexLocker lock(&m_audioQMutex);

    while (!m_audioFrameQ.empty())
        delete m_audioFrameQ.dequeue();
}

void CamClient::clearQueues()
{
    clearVideoQueue();
    clearAudioQueue();

    while (!m_videoPrerollQueue.empty())
        delete m_videoPrerollQueue.dequeue();

    while (!m_videoLowRatePrerollQueue.empty())
        delete m_videoLowRatePrerollQueue.dequeue();

    while (!m_audioPrerollQueue.empty())
        delete m_audioPrerollQueue.dequeue();

    while (!m_audioLowRatePrerollQueue.empty())
        delete m_audioLowRatePrerollQueue.dequeue();

     if (m_deltaInterval == 0) {
        m_sequenceState = CAMCLIENT_STATE_CONTINUOUS;    // motion detection inactive
        STATE_DEBUG("STATE_CONTINUOUS");
    } else {
        m_sequenceState = CAMCLIENT_STATE_IDLE;          // use motion detect state machine
        STATE_DEBUG("STATE_IDLE");
    }
}



void CamClient::newJPEG(QByteArray frame)
{
    m_videoQMutex.lock();

    if (m_videoFrameQ.count() > 5)
        delete m_videoFrameQ.dequeue();

    CLIENT_QUEUEDATA *qd = new CLIENT_QUEUEDATA;
    qd->data = frame;
    qd->timestamp = QDateTime::currentMSecsSinceEpoch();
    m_videoFrameQ.enqueue(qd);

    m_videoQMutex.unlock();

    QMutexLocker lock(&m_frameCountLock);
    m_frameCount++;
}

void CamClient::newAudio(QByteArray audioFrame)
{
    m_audioQMutex.lock();

    if (m_audioFrameQ.count() > 5)
        delete m_audioFrameQ.dequeue();

    CLIENT_QUEUEDATA *qd = new CLIENT_QUEUEDATA;
    qd->data = audioFrame;
    qd->timestamp = QDateTime::currentMSecsSinceEpoch();
    m_audioFrameQ.enqueue(qd);

    m_audioQMutex.unlock();

    QMutexLocker lock(&m_audioSampleLock);
    m_audioSampleCount += audioFrame.length();
}


void CamClient::newStream()
{
    int avtype;
	// remove the old streams
	
    if (m_avmuxPortHighRate != -1)
        clientRemoveService(m_avmuxPortHighRate);
    m_avmuxPortHighRate = -1;

    if (m_avmuxPortLowRate != -1)
        clientRemoveService(m_avmuxPortLowRate);
    m_avmuxPortLowRate = -1;

    if (m_controlPort != -1)
        clientRemoveService(m_controlPort);

    if (m_highRateEncoder != NULL)
        m_highRateEncoder->exitThread();
    if (m_generateLowRate)
        m_lowRateEncoder->exitThread();

    m_highRateEncoder = NULL;
    m_lowRateEncoder = NULL;

    // and start the new streams

    QSettings *settings = SyntroUtils::getSettings();

    settings->beginGroup(CAMCLIENT_STREAM_GROUP);

    avtype = settings->value(CAMCLIENT_AV_TYPE).toInt();

    m_minInterval = settings->value(CAMCLIENT_HIGHRATEVIDEO_MININTERVAL).toInt();
    m_fullFrameMaxInterval = settings->value(CAMCLIENT_HIGHRATEVIDEO_MAXINTERVAL).toInt();
    m_nullFrameInterval = settings->value(CAMCLIENT_HIGHRATEVIDEO_NULLINTERVAL).toInt();

    m_generateLowRate = settings->value(CAMCLIENT_GENERATE_LOWRATE).toBool();
    m_videoCompressionRateHigh = settings->value(CAMCLIENT_GS_VIDEO_HIGHRATE).toInt();
    m_audioCompressionRateHigh = settings->value(CAMCLIENT_GS_AUDIO_HIGHRATE).toInt();
    m_videoCompressionRateLow = settings->value(CAMCLIENT_GS_VIDEO_LOWRATE).toInt();
    m_audioCompressionRateLow = settings->value(CAMCLIENT_GS_AUDIO_LOWRATE).toInt();

    int lowFrameRate = settings->value(CAMCLIENT_MJPEG_LOWFRAMERATE).toInt();

    if (lowFrameRate <= 0)
        m_mjpegLowFrameInterval = 200;
    else
        m_mjpegLowFrameInterval = 1000 / lowFrameRate;

    m_avmuxPortHighRate = clientAddService(SYNTRO_STREAMNAME_AVMUX, SERVICETYPE_MULTICAST, true);
    if (m_generateLowRate)
        m_avmuxPortLowRate = clientAddService(SYNTRO_STREAMNAME_AVMUXLR, SERVICETYPE_MULTICAST, true);

    m_controlPort = clientAddService(SYNTRO_STREAMNAME_CONTROL, SERVICETYPE_E2E, true);


    settings->endGroup();

    settings->beginGroup(CAMCLIENT_MOTION_GROUP);

    m_tilesToSkip = settings->value(CAMCLIENT_MOTION_TILESTOSKIP).toInt();
    m_intervalsToSkip = settings->value(CAMCLIENT_MOTION_INTERVALSTOSKIP).toInt();
    m_minDelta = settings->value(CAMCLIENT_MOTION_MIN_DELTA).toInt();
    m_minNoise = settings->value(CAMCLIENT_MOTION_MIN_NOISE).toInt();
    m_deltaInterval = settings->value(CAMCLIENT_MOTION_DELTA_INTERVAL).toInt();
    m_preroll = settings->value(CAMCLIENT_MOTION_PREROLL).toInt();
    m_postroll = settings->value(CAMCLIENT_MOTION_POSTROLL).toInt();

    settings->endGroup();

    delete settings;

    m_cd.setDeltaThreshold(m_minDelta);
    m_cd.setNoiseThreshold(m_minNoise);
    m_cd.setTilesToSkip(m_tilesToSkip);
    m_cd.setIntervalsToSkip(m_intervalsToSkip);

    qint64 now = QDateTime::currentMSecsSinceEpoch();

    m_lastFrameTime = now;
    m_lastLowRateFrameTime = now;
    m_lastFullFrameTime = now;
    m_lastLowRateFullFrameTime = now;
    m_lastRefreshTime = now;
    m_lastPrerollFrameTime = now;
    m_lastLowRatePrerollFrameTime = now;
    m_lastChangeTime = now;
    m_imageChanged = false;

    m_cd.setUninitialized();

    clearQueues();

    switch (avtype) {
    case AVMUXENCODE_AV_TYPE_MJPPCM:
        m_avParams.avmuxSubtype = SYNTRO_RECORD_TYPE_AVMUX_MJPPCM;
        m_avParams.videoSubtype = SYNTRO_RECORD_TYPE_VIDEO_MJPEG;
        m_avParams.audioSubtype = SYNTRO_RECORD_TYPE_AUDIO_PCM;
        m_highRateEncoder = NULL;
        m_lowRateEncoder = NULL;
        break;

    case AVMUXENCODE_AV_TYPE_RTPMP4:
        m_avParams.avmuxSubtype = SYNTRO_RECORD_TYPE_AVMUX_RTP;
        m_avParams.videoSubtype = SYNTRO_RECORD_TYPE_VIDEO_RTPMPEG4;
        m_avParams.audioSubtype = SYNTRO_RECORD_TYPE_AUDIO_RTPAAC;
        m_highRateEncoder = new AVMuxEncode(AVMUXENCODE_AV_TYPE_RTPMP4);
        m_highRateEncoder->resumeThread();
        if (m_generateLowRate) {
            m_lowRateEncoder = new AVMuxEncode(AVMUXENCODE_AV_TYPE_RTPMP4);
            m_lowRateEncoder->resumeThread();
        } else {
            m_lowRateEncoder = NULL;
        }
        break;

    case AVMUXENCODE_AV_TYPE_RTPH264:
        m_avParams.avmuxSubtype = SYNTRO_RECORD_TYPE_AVMUX_RTP;
        m_avParams.videoSubtype = SYNTRO_RECORD_TYPE_VIDEO_RTPH264;
        m_avParams.audioSubtype = SYNTRO_RECORD_TYPE_AUDIO_RTPAAC;
        m_highRateEncoder = new AVMuxEncode(AVMUXENCODE_AV_TYPE_RTPH264);
        m_highRateEncoder->resumeThread();
        if (m_generateLowRate) {
            m_lowRateEncoder = new AVMuxEncode(AVMUXENCODE_AV_TYPE_RTPH264);
            m_lowRateEncoder->resumeThread();
        } else {
            m_lowRateEncoder = NULL;
        }
        break;
    }
    establishPipelines();
}

void CamClient::videoFormat(int width, int height, int framerate)
{
    m_avParams.videoWidth = width;
    m_avParams.videoHeight = height;
    m_avParams.videoFramerate = framerate;
    m_gotVideoFormat = true;
    establishPipelines();
}

void CamClient::audioFormat(int sampleRate, int channels, int sampleSize)
{
    m_avParams.audioSampleRate = sampleRate;
    m_avParams.audioChannels = channels;
    m_avParams.audioSampleSize = sampleSize;
    m_gotAudioFormat = true;
    establishPipelines();
}

void CamClient::establishPipelines()
{
    if (!m_gotVideoFormat || !m_gotAudioFormat)
        return;
    if (m_avParams.avmuxSubtype == SYNTRO_RECORD_TYPE_AVMUX_RTP) {
        m_highRateEncoder->setCompressionRates(m_videoCompressionRateHigh, m_audioCompressionRateHigh);
        m_highRateEncoder->newPipelines(&m_avParams);
        if (m_generateLowRate) {
            m_lowRateEncoder->setCompressionRates(m_videoCompressionRateLow, m_audioCompressionRateLow);
            m_lowRateEncoder->newPipelines(&m_avParams);
        }
    }
}


void CamClient::sendHighRateCaps()
{
    if (!clientIsServiceActive(m_avmuxPortHighRate) || !clientClearToSend(m_avmuxPortHighRate))
        return;

    gchar *videoCaps = m_highRateEncoder->getVideoCaps();
    gchar *audioCaps = m_highRateEncoder->getAudioCaps();

    int videoLength = 0;
    int audioLength = 0;
    int totalLength = 0;

    if (videoCaps != NULL)
        videoLength = strlen(videoCaps) + 1;

    if (audioCaps != NULL)
        audioLength = strlen(audioCaps) + 1;

    totalLength = videoLength + audioLength;

    if (totalLength == 0)
        return;

    SYNTRO_EHEAD *multiCast = clientBuildMessage(m_avmuxPortHighRate, sizeof(SYNTRO_RECORD_AVMUX) + totalLength);
    SYNTRO_RECORD_AVMUX *avmux = (SYNTRO_RECORD_AVMUX *)(multiCast + 1);
    SyntroUtils::avmuxHeaderInit(avmux, &m_avParams, SYNTRO_RECORDHEADER_PARAM_NORMAL, m_recordIndex++, 0, videoLength, audioLength);
    avmux->videoSubtype = SYNTRO_RECORD_TYPE_VIDEO_RTPCAPS;
    avmux->audioSubtype = SYNTRO_RECORD_TYPE_AUDIO_RTPCAPS;

    unsigned char *ptr = (unsigned char *)(avmux + 1);

    if (videoLength > 0) {
        memcpy(ptr, videoCaps, videoLength);
        ptr += videoLength;
 //       qDebug() << "sending vcaps " << m_videoCaps;
    }

    if (audioLength > 0) {
        memcpy(ptr, audioCaps, audioLength);
        ptr += audioLength;
 //       qDebug() << "sending acaps " << m_audioCaps;
    }

    clientSendMessage(m_avmuxPortHighRate, multiCast, sizeof(SYNTRO_RECORD_AVMUX) + totalLength, SYNTROLINK_MEDPRI);
}


void CamClient::sendLowRateCaps()
{
    if ((m_lowRateEncoder == NULL) || !clientIsServiceActive(m_avmuxPortLowRate) ||
            !clientClearToSend(m_avmuxPortLowRate))
        return;

    gchar *videoCaps = m_lowRateEncoder->getVideoCaps();
    gchar *audioCaps = m_lowRateEncoder->getAudioCaps();

    int videoLength = 0;
    int audioLength = 0;
    int totalLength = 0;

    if (videoCaps != NULL)
        videoLength = strlen(videoCaps) + 1;

    if (audioCaps != NULL)
        audioLength = strlen(audioCaps) + 1;

    totalLength = videoLength + audioLength;

    if (totalLength == 0)
        return;

    SYNTRO_EHEAD *multiCast = clientBuildMessage(m_avmuxPortLowRate, sizeof(SYNTRO_RECORD_AVMUX) + totalLength);
    SYNTRO_RECORD_AVMUX *avmux = (SYNTRO_RECORD_AVMUX *)(multiCast + 1);
    SyntroUtils::avmuxHeaderInit(avmux, &m_avParams, SYNTRO_RECORDHEADER_PARAM_NORMAL, m_recordIndex++, 0, videoLength, audioLength);
    avmux->videoSubtype = SYNTRO_RECORD_TYPE_VIDEO_RTPCAPS;
    avmux->audioSubtype = SYNTRO_RECORD_TYPE_AUDIO_RTPCAPS;

    unsigned char *ptr = (unsigned char *)(avmux + 1);

    if (videoLength > 0) {
        memcpy(ptr, videoCaps, videoLength);
        ptr += videoLength;
 //       qDebug() << "sending vcaps " << m_videoCaps;
    }

    if (audioLength > 0) {
        memcpy(ptr, audioCaps, audioLength);
        ptr += audioLength;
 //       qDebug() << "sending acaps " << m_audioCaps;
    }

    clientSendMessage(m_avmuxPortLowRate, multiCast, sizeof(SYNTRO_RECORD_AVMUX) + totalLength, SYNTROLINK_MEDPRI);
}

