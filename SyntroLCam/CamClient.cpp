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

    QSettings *settings = SyntroUtils::getSettings();

    settings->beginGroup(CAMCLIENT_STREAM_GROUP);

    if (!settings->contains(CAMCLIENT_HIGHRATEVIDEO_MININTERVAL))
        settings->setValue(CAMCLIENT_HIGHRATEVIDEO_MININTERVAL, "100");

    if (!settings->contains(CAMCLIENT_HIGHRATEVIDEO_MAXINTERVAL))
        settings->setValue(CAMCLIENT_HIGHRATEVIDEO_MAXINTERVAL, "10000");

    if (!settings->contains(CAMCLIENT_HIGHRATEVIDEO_NULLINTERVAL))
        settings->setValue(CAMCLIENT_HIGHRATEVIDEO_NULLINTERVAL, "2000");

    if (!settings->contains(CAMCLIENT_GENERATE_LOWRATE))
        settings->setValue(CAMCLIENT_GENERATE_LOWRATE, true);

    if (!settings->contains(CAMCLIENT_LOWRATE_HALFRES))
        settings->setValue(CAMCLIENT_LOWRATE_HALFRES, true);

     if (!settings->contains(CAMCLIENT_LOWRATEVIDEO_MININTERVAL))
        settings->setValue(CAMCLIENT_LOWRATEVIDEO_MININTERVAL, "6000");

    if (!settings->contains(CAMCLIENT_LOWRATEVIDEO_MAXINTERVAL))
        settings->setValue(CAMCLIENT_LOWRATEVIDEO_MAXINTERVAL, "60000");

    if (!settings->contains(CAMCLIENT_LOWRATEVIDEO_NULLINTERVAL))
        settings->setValue(CAMCLIENT_LOWRATEVIDEO_NULLINTERVAL, "8000");


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

            if (m_generateLowRate && SyntroUtils::syntroTimerExpired(now, m_lastLowRatePrerollFrameTime, m_lowRateMinInterval)) {
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
                if (m_generateLowRate && SyntroUtils::syntroTimerExpired(now, m_lastLowRatePrerollFrameTime, m_lowRateMinInterval)) {
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
    QByteArray audioFrame;
    bool highRateExpired;
    bool lowRateExpired;
    QImage frame;
    QImage smallFrame;
    QByteArray jpeg;
    QByteArray largeJpeg;
    QByteArray smallJpeg;
    bool videoValid = false;
    bool audioValid = false;


    highRateExpired = SyntroUtils::syntroTimerExpired(now, m_lastFullFrameTime, m_minInterval);
    lowRateExpired = SyntroUtils::syntroTimerExpired(now, m_lastLowRateFullFrameTime, m_lowRateMinInterval);

    if (m_generateLowRate && clientIsServiceActive(m_avmuxPortLowRate)) {
        if (!lowRateExpired) {
            if (SyntroUtils::syntroTimerExpired(now, m_lastLowRateFrameTime, m_lowRateNullFrameInterval)) {
                sendNullFrameMJPPCM(now, false);            // in case very long time
            }
        }
    }

    if (highRateExpired) {
        videoValid = dequeueVideoFrame(jpeg, videoTimestamp);
        audioValid = dequeueAudioFrame(audioFrame, audioTimestamp);
    } else {
        return false;
    }

    if (videoValid || audioValid) {
        if (clientIsServiceActive(m_avmuxPortHighRate) && clientClearToSend(m_avmuxPortHighRate) && (highRateExpired || audioValid)) {

            if (highRateExpired)
                largeJpeg = jpeg;
            else
                largeJpeg = QByteArray();

            SYNTRO_EHEAD *multiCast = clientBuildMessage(m_avmuxPortHighRate, sizeof(SYNTRO_RECORD_AVMUX)
                                                      + largeJpeg.size() + audioFrame.size());
            SYNTRO_RECORD_AVMUX *avHead = (SYNTRO_RECORD_AVMUX *)(multiCast + 1);
            SyntroUtils::avmuxHeaderInit(avHead, &m_avParams, param, m_recordIndex++, 0, largeJpeg.size(), audioFrame.size());
            if (audioValid)
                SyntroUtils::convertInt64ToUC8(audioTimestamp, avHead->recordHeader.timestamp);
            if (videoValid)
                SyntroUtils::convertInt64ToUC8(videoTimestamp, avHead->recordHeader.timestamp);

            unsigned char *ptr = (unsigned char *)(avHead + 1);

            if (largeJpeg.size() > 0) {
                memcpy(ptr, largeJpeg.data(), largeJpeg.size());
                m_lastFullFrameTime = m_lastFrameTime = now;
                ptr += largeJpeg.size();
            }

            if (audioFrame.size() > 0) {
                memcpy(ptr, audioFrame.data(), audioFrame.size());
            }

            int length = sizeof(SYNTRO_RECORD_AVMUX) + largeJpeg.size() + audioFrame.size();
            clientSendMessage(m_avmuxPortHighRate, multiCast, length, SYNTROLINK_MEDPRI);
        }

        if (m_generateLowRate && clientIsServiceActive(m_avmuxPortLowRate) && clientClearToSend(m_avmuxPortLowRate) &&
                (lowRateExpired || audioValid)) {
            if (lowRateExpired)
                m_lastLowRateFrameTime = now;

            if (lowRateExpired) {
                if (m_lowRateHalfRes) {
                    frame = QImage::fromData(jpeg, "JPEG");
                    smallFrame = frame.scaled(frame.width() / 2, frame.height() / 2);
                    QBuffer buffer(&smallJpeg);
                    buffer.open(QIODevice::WriteOnly);
                    smallFrame.save(&buffer, "JPG");
                } else {
                    smallJpeg = jpeg;
                }
            } else {
                smallJpeg = QByteArray();
            }

            SYNTRO_EHEAD *multiCast = clientBuildMessage(m_avmuxPortLowRate, sizeof(SYNTRO_RECORD_AVMUX)
                                                         + smallJpeg.size() + audioFrame.size());
            SYNTRO_RECORD_AVMUX *avHead = (SYNTRO_RECORD_AVMUX *)(multiCast + 1);
            SyntroUtils::avmuxHeaderInit(avHead, &m_avParams, param, m_recordIndex++, 0, smallJpeg.size(), audioFrame.size());
            if (audioValid)
                SyntroUtils::convertInt64ToUC8(audioTimestamp, avHead->recordHeader.timestamp);
            if (videoValid)
                SyntroUtils::convertInt64ToUC8(videoTimestamp, avHead->recordHeader.timestamp);

            unsigned char *ptr = (unsigned char *)(avHead + 1);

            if (smallJpeg.size() > 0) {
                memcpy(ptr, smallJpeg.data(), smallJpeg.size());
                m_lastLowRateFullFrameTime = m_lastLowRateFrameTime = now;
                ptr += smallJpeg.size();
            }

            if (audioFrame.size() > 0) {
                memcpy(ptr, audioFrame.data(), audioFrame.size());
            }

            int length = sizeof(SYNTRO_RECORD_AVMUX) + smallJpeg.size() + audioFrame.size();
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
    QImage frame;
    QImage smallFrame;
    QByteArray smallJpeg;

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
            SyntroUtils::syntroTimerExpired(now, m_lastLowRateFullFrameTime, m_lowRateFullFrameMaxInterval)) {
        if (m_lowRateHalfRes) {
            frame = QImage::fromData(jpeg, "JPEG");
            smallFrame = frame.scaled(frame.width() / 2, frame.height() / 2);
            QBuffer buffer(&smallJpeg);
            buffer.open(QIODevice::WriteOnly);
            smallFrame.save(&buffer, "JPG");
        } else {
            smallJpeg = jpeg;
        }

        SYNTRO_EHEAD *multiCast = clientBuildMessage(m_avmuxPortLowRate, sizeof(SYNTRO_RECORD_AVMUX) + smallJpeg.size());
        SYNTRO_RECORD_AVMUX *videoHead = (SYNTRO_RECORD_AVMUX *)(multiCast + 1);
        SyntroUtils::avmuxHeaderInit(videoHead, &m_avParams, SYNTRO_RECORDHEADER_PARAM_REFRESH, m_recordIndex++, 0, smallJpeg.size(), 0);
        memcpy((unsigned char *)(videoHead + 1), smallJpeg.data(), smallJpeg.size());
        int length = sizeof(SYNTRO_RECORD_AVMUX) + smallJpeg.size();
        clientSendMessage(m_avmuxPortLowRate, multiCast, length, SYNTROLINK_LOWPRI);
        m_lastLowRateFrameTime = m_lastLowRateFullFrameTime = now;
    }

    if (SyntroUtils::syntroTimerExpired(now, m_lastFrameTime, m_nullFrameInterval))
        sendNullFrameMJPPCM(now, true);
    if (SyntroUtils::syntroTimerExpired(now, m_lastLowRateFrameTime, m_lowRateNullFrameInterval))
        sendNullFrameMJPPCM(now, false);
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


void CamClient::appClientInit()
{
    newStream();
}

void CamClient::appClientExit()
{
    clearQueues();
}

void CamClient::appClientBackground()
{
    processAVQueueMJPPCM();
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
    // remove the old streams

    if (m_avmuxPortHighRate != -1)
        clientRemoveService(m_avmuxPortHighRate);
    m_avmuxPortHighRate = -1;

    if (m_avmuxPortLowRate != -1)
        clientRemoveService(m_avmuxPortLowRate);
    m_avmuxPortLowRate = -1;

    if (m_controlPort != -1)
        clientRemoveService(m_controlPort);

    // and start the new streams

    QSettings *settings = SyntroUtils::getSettings();

    settings->beginGroup(CAMCLIENT_STREAM_GROUP);

    m_minInterval = settings->value(CAMCLIENT_HIGHRATEVIDEO_MININTERVAL).toInt();
    m_fullFrameMaxInterval = settings->value(CAMCLIENT_HIGHRATEVIDEO_MAXINTERVAL).toInt();
    m_nullFrameInterval = settings->value(CAMCLIENT_HIGHRATEVIDEO_NULLINTERVAL).toInt();

    m_generateLowRate = settings->value(CAMCLIENT_GENERATE_LOWRATE).toBool();
    m_lowRateHalfRes = settings->value(CAMCLIENT_LOWRATE_HALFRES).toBool();
    m_lowRateMinInterval = settings->value(CAMCLIENT_LOWRATEVIDEO_MININTERVAL).toInt();
    m_lowRateFullFrameMaxInterval = settings->value(CAMCLIENT_LOWRATEVIDEO_MAXINTERVAL).toInt();
    m_lowRateNullFrameInterval = settings->value(CAMCLIENT_LOWRATEVIDEO_NULLINTERVAL).toInt();

    m_avmuxPortHighRate = clientAddService(SYNTRO_STREAMNAME_AVMUX, SERVICETYPE_MULTICAST, true);
    if (m_generateLowRate)
        m_avmuxPortLowRate = clientAddService(SYNTRO_STREAMNAME_AVMUXLR, SERVICETYPE_MULTICAST, true);

    m_controlPort = clientAddService(SYNTRO_STREAMNAME_CONTROL, SERVICETYPE_E2E, true);


    settings->endGroup();

    m_gotAudioFormat = false;
    m_gotVideoFormat = false;

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

    m_avParams.avmuxSubtype = SYNTRO_RECORD_TYPE_AVMUX_MJPPCM;
    m_avParams.videoSubtype = SYNTRO_RECORD_TYPE_VIDEO_MJPEG;
    m_avParams.audioSubtype = SYNTRO_RECORD_TYPE_AUDIO_PCM;
}

void CamClient::videoFormat(int width, int height, int framerate)
{
    m_avParams.videoWidth = width;
    m_avParams.videoHeight = height;
    m_avParams.videoFramerate = framerate;
    m_gotVideoFormat = true;
}

void CamClient::audioFormat(int sampleRate, int channels, int sampleSize)
{
    m_avParams.audioSampleRate = sampleRate;
    m_avParams.audioChannels = channels;
    m_avParams.audioSampleSize = sampleSize;
    m_gotAudioFormat = true;
}
