//
//  Copyright (c) 2014 Richard Barnett
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


#include <qbuffer.h>

#include "SyntroLib.h"
#include "CameraClient.h"
#include "SyntroUtils.h"
#include "Camera.h"

// #define STATE_DEBUG_ENABLE

#ifdef STATE_DEBUG_ENABLE
#define STATE_DEBUG(x) qDebug() << x
#else
#define STATE_DEBUG(x)
#endif


CameraClient::CameraClient(QObject *)
		: Endpoint(CAMERA_IMAGE_INTERVAL, COMPTYPE_CAMERA)
{
	m_highRateVideoPort = -1;
	m_lowRateVideoPort = -1;
	m_PTZPort = -1;
    m_sequenceState = CAMERACLIENT_STATE_IDLE;
	memset(&m_avHighParams, 0, sizeof(SYNTRO_AVPARAMS));
	memset(&m_avLowParams, 0, sizeof(SYNTRO_AVPARAMS));
}

CameraClient::~CameraClient()
{
}

void CameraClient::processVideoQueue()
{
	qint64 now = QDateTime::currentMSecsSinceEpoch();
	QImage frame;
    PREROLL *preroll;
    QString stateString;

    switch (m_sequenceState) {
        // waiting for a motion event
        case CAMERACLIENT_STATE_IDLE:
            // age preroll queue

            while (!m_highRatePrerollQueue.empty()) {
                if ((now - m_highRatePrerollQueue.head()->timestamp) < m_preroll)
                    break;

                // head is too old to keep

                delete m_highRatePrerollQueue.dequeue();
            }
            while (!m_lowRatePrerollQueue.empty()) {
                if ((now - m_lowRatePrerollQueue.head()->timestamp) < m_preroll)
                    break;

                // head is too old to keep

                delete m_lowRatePrerollQueue.dequeue();
            }

            // if there is a frame, put on preroll queue and check for motion

            if (!dequeueVideoFrame(frame))
                break;

            // see if enough time yet for high rate frame rate control. Just discard frame if not
            if (!SyntroUtils::syntroTimerExpired(now, m_highRateLastPrerollFrameTime, m_highRateMinInterval))
               break;

            m_highRateLastPrerollFrameTime = now;
            preroll = new PREROLL;
            preroll->frame = frame;
            preroll->timestamp = now;
			preroll->marker = SYNTRO_RECORDHEADER_PARAM_PREROLL;
            m_highRatePrerollQueue.enqueue(preroll);

			// see if enough time yet for low rate frame rate control. 
			if (SyntroUtils::syntroTimerExpired(now, m_lowRateLastPrerollFrameTime, m_lowRateMinInterval)) {
				preroll = new PREROLL;
				preroll->frame = frame;
				preroll->timestamp = now;
				preroll->marker = SYNTRO_RECORDHEADER_PARAM_PREROLL;
				m_lowRatePrerollQueue.enqueue(preroll);
				m_lowRateLastPrerollFrameTime = now;
			}

            // now check for motion if it's time

            if ((now - m_lastDeltaTime) > m_deltaInterval)
                checkForMotion(now, frame);
            if (m_imageChanged) {
                m_sequenceState = CAMERACLIENT_STATE_PREROLL; // send the preroll frames
                stateString = QString("STATE_PREROLL: queue size %1 (high rate) and %2 (low rate)")
						.arg(m_highRatePrerollQueue.size()).arg(m_lowRatePrerollQueue.size());
                STATE_DEBUG(stateString);

				// mark start and end of preroll sequences

				if (!m_highRatePrerollQueue.isEmpty()) {
					m_highRatePrerollQueue.first()->marker = SYNTRO_RECORDHEADER_PARAM_PREROLL;
				}
				if (!m_lowRatePrerollQueue.isEmpty()) {
					m_lowRatePrerollQueue.first()->marker = SYNTRO_RECORDHEADER_PARAM_PREROLL;
				}
				m_highRateWaitingForFirst = true;
				m_lowRateWaitingForFirst = true;
            } else {
                sendHeartbeatFrame(now, frame);
			}
            break;

        // sending the preroll queue
        case CAMERACLIENT_STATE_PREROLL:
            if (clientIsServiceActive(m_highRateVideoPort)) {
				if (clientClearToSend(m_highRateVideoPort) && !m_highRatePrerollQueue.empty()) {
					if (SyntroUtils::syntroTimerExpired(now, m_highRateLastFrameTime, m_highRateMinInterval / 4 + 1))
						sendPreroll(m_highRatePrerollQueue.dequeue(), true);
				}
            } else {
				while (!m_highRatePrerollQueue.empty())
					delete m_highRatePrerollQueue.dequeue();
			}
            if (clientIsServiceActive(m_lowRateVideoPort)) {
				if (clientClearToSend(m_lowRateVideoPort) && !m_lowRatePrerollQueue.empty()) {
 					if (SyntroUtils::syntroTimerExpired(now, m_lowRateLastFrameTime, m_lowRateMinInterval / 4 + 1))
						sendPreroll(m_lowRatePrerollQueue.dequeue(), false);
				}
            } else {
				while (!m_lowRatePrerollQueue.empty())
					delete m_lowRatePrerollQueue.dequeue();
			}
            if (m_highRatePrerollQueue.empty()) {
                 m_sequenceState = CAMERACLIENT_STATE_INSEQUENCE;
                 STATE_DEBUG("STATE_INSEQUENCE");
                 m_lastChangeTime = now;                    // in case pre-roll sending took a while
            }

            // continue to put new frames on preroll queue so nothing is lost while preroll is being sent

            if (!dequeueVideoFrame(frame))
                break;

            // see if enough time yet for high rate frame rate control. Just discard frame if not
            if (!SyntroUtils::syntroTimerExpired(now, m_highRateLastPrerollFrameTime, m_highRateMinInterval))
               break;

            m_highRateLastPrerollFrameTime = now;
            preroll = new PREROLL;
            preroll->frame = frame;
            preroll->timestamp = now;
			preroll->marker = SYNTRO_RECORDHEADER_PARAM_NORMAL;
			if (m_highRateWaitingForFirst) {
				preroll->marker = SYNTRO_RECORDHEADER_PARAM_NORMAL;
				m_highRateWaitingForFirst = false;
			}
            m_highRatePrerollQueue.enqueue(preroll);

			// see if enough time yet for low rate frame rate control. 
			if (!SyntroUtils::syntroTimerExpired(now, m_lowRateLastPrerollFrameTime, m_lowRateMinInterval)) {
				preroll = new PREROLL;
				preroll->frame = frame;
				preroll->timestamp = now;
				preroll->marker = SYNTRO_RECORDHEADER_PARAM_NORMAL;
				if (m_lowRateWaitingForFirst) {
					preroll->marker = SYNTRO_RECORDHEADER_PARAM_NORMAL;
					m_lowRateWaitingForFirst = false;
				}
				m_lowRatePrerollQueue.enqueue(preroll);
				m_lowRateLastPrerollFrameTime = now;
			}
			break;

        // in the motion sequence
        case CAMERACLIENT_STATE_INSEQUENCE:

            // see if anything to send

            if (!dequeueVideoFrame(frame))
                break;

            // see if enough time yet for frame rate control. Just discard frame if not
            if (!SyntroUtils::syntroTimerExpired(now, m_highRateLastFrameTime, m_highRateMinInterval))
                break;

            // check for motion

            if ((now - m_lastDeltaTime) > m_deltaInterval)
                checkForMotion(now, frame);

			if (clientIsServiceActive(m_highRateVideoPort)) {

				// see if allowed to send high rate frame and discard frame if not
				if (clientClearToSend(m_highRateVideoPort)) {

			        // ok - send frame

					if (m_highRateWaitingForFirst)
						sendFrame(now, frame, SYNTRO_RECORDHEADER_PARAM_NORMAL, true);
					else
						sendFrame(now, frame, SYNTRO_RECORDHEADER_PARAM_NORMAL, true);
					m_highRateWaitingForFirst = false;
				}
			} else {
				m_highRateLastFrameTime = now;					// need to do this to keep low rate frames going
			}

            // see if allowed to send anything and discard frame if not
            if (clientIsServiceActive(m_lowRateVideoPort) && clientClearToSend(m_lowRateVideoPort)) {
		        if (SyntroUtils::syntroTimerExpired(now, m_lowRateLastFrameTime, m_lowRateMinInterval)) {
					if (m_lowRateWaitingForFirst)
						sendFrame(now, frame, SYNTRO_RECORDHEADER_PARAM_NORMAL, false);
					else
						sendFrame(now, frame, SYNTRO_RECORDHEADER_PARAM_NORMAL, false);
					m_lowRateWaitingForFirst = false;
				}
			}

            // check for motion

            if ((now - m_lastDeltaTime) > m_deltaInterval)
                checkForMotion(now, frame);

            if (!m_imageChanged) {
                m_sequenceState = CAMERACLIENT_STATE_POSTROLL; // no motion so go into postroll state
                m_lastChangeTime = now;                        // this sets the start tiem for the postroll
                STATE_DEBUG("STATE_POSTROLL");
				m_highRateWaitingForFirst = true;
				m_lowRateWaitingForFirst = true;
            }
			break;

        // handle the post roll stage
        case CAMERACLIENT_STATE_POSTROLL:

            if (SyntroUtils::syntroTimerExpired(now, m_lastChangeTime, m_postroll)) {
                // postroll complete
                m_sequenceState = CAMERACLIENT_STATE_IDLE;
                m_highRateLastPrerollFrameTime = m_highRateLastFrameTime;
                m_lowRateLastPrerollFrameTime = m_lowRateLastFrameTime;
                STATE_DEBUG("STATE_IDLE");
                break;
            }

            // see if anything to send

            if (!dequeueVideoFrame(frame))
                break;

            // check for motion - will extend postroll if necessary

            if ((now - m_lastDeltaTime) > m_deltaInterval)
                checkForMotion(now, frame);
            if (m_imageChanged) {
                m_sequenceState = CAMERACLIENT_STATE_INSEQUENCE;
                STATE_DEBUG("Returning to STATE_INSEQUENCE");
            }

            // see if enough time yet for frame rate control. Just discard frame if not
            if (!SyntroUtils::syntroTimerExpired(now, m_highRateLastFrameTime, m_highRateMinInterval))
                break;

			if (clientIsServiceActive(m_highRateVideoPort)) {

				// see if allowed to send high rate frame and discard frame if not
				if (clientClearToSend(m_highRateVideoPort)) {

			        // ok - send frame

					if (m_highRateWaitingForFirst)
						sendFrame(now, frame, SYNTRO_RECORDHEADER_PARAM_POSTROLL, true);
					else
						sendFrame(now, frame, SYNTRO_RECORDHEADER_PARAM_POSTROLL, true);
					m_highRateWaitingForFirst = false;
				}
			} else {
				m_highRateLastFrameTime = now;					// need to do this to keep low rate frames going
			}

            // see if allowed to send anything and discard frame if not
            if (clientIsServiceActive(m_lowRateVideoPort) && clientClearToSend(m_lowRateVideoPort)) {
				if (SyntroUtils::syntroTimerExpired(now, m_lowRateLastFrameTime, m_lowRateMinInterval)) {
					if (m_lowRateWaitingForFirst)
						sendFrame(now, frame, SYNTRO_RECORDHEADER_PARAM_POSTROLL, false);
					else
						sendFrame(now, frame, SYNTRO_RECORDHEADER_PARAM_POSTROLL, false);
					m_lowRateWaitingForFirst = false;
				}
			}
			break;

        // in the motion sequence
        case CAMERACLIENT_STATE_CONTINUOUS:
            getAndSendFrame(now);
            break;
    }
}

void CameraClient::checkForMotion(qint64 now, const QImage& frame)
{
	bool diff;
	QImage smallFrame;

	smallFrame = frame.scaled(frame.width() / 4, frame.height() / 4);
	if (m_oldSmallFrame.width() == 0) {
		m_oldSmallFrame = smallFrame;
	}

	computeAbsoluteDifferences(smallFrame, m_oldSmallFrame);
	diff = processDifferences(m_oldSmallFrame);
	m_oldSmallFrame = smallFrame;					// update the old frame

	if (diff) {
//		TRACE0("Difference");
			m_lastChangeTime = now;
	}
	m_imageChanged = diff;
	m_lastDeltaTime = now;
}

void CameraClient::computeAbsoluteDifferences(const QImage& newFrame, QImage& oldFrame)
{
	int		i;
	unsigned char *ni;
	unsigned char *oi;
	int size = newFrame.byteCount();

	ni = (unsigned char *)newFrame.bits();
	oi = (unsigned char *)oldFrame.bits();
	for (i = 0; i < size; i++, ni++, oi++)	{
		*oi = abs(*oi - *ni);
	}
}

bool CameraClient::processDifferences(const QImage& diff)
{
	int		i;
	unsigned char *di;
	int		delta;
	int size = diff.byteCount();

	delta = 0;
	di = (unsigned char *)diff.bits();
	for (i = 0; i < size; i++)
	{
		if (*di++ > 32)
			delta++;
	}	
	return delta >= m_minDelta;
}

void CameraClient::sendPreroll(PREROLL *preroll, bool highRate)
{
	QByteArray jpeg;
	QImage smallFrame;

	if (highRate) {
		QBuffer buffer(&jpeg);
		buffer.open(QIODevice::WriteOnly);
		preroll->frame.save(&buffer, "JPG");
		SYNTRO_EHEAD *multiCast = clientBuildMessage(m_highRateVideoPort, sizeof(SYNTRO_RECORD_AVMUX) + jpeg.size());
		SYNTRO_RECORD_AVMUX *avmuxHead = (SYNTRO_RECORD_AVMUX *)(multiCast + 1);
        SyntroUtils::avmuxHeaderInit(avmuxHead, &m_avHighParams, SYNTRO_RECORDHEADER_PARAM_PREROLL, m_recordIndex++, 0, jpeg.size(), 0);
		SyntroUtils::convertInt64ToUC8(preroll->timestamp, avmuxHead->recordHeader.timestamp);
		memcpy((unsigned char *)(avmuxHead + 1), jpeg.data(), jpeg.size());
		int length = sizeof(SYNTRO_RECORD_AVMUX) + jpeg.size();
		clientSendMessage(m_highRateVideoPort, multiCast, length, SYNTROLINK_LOWPRI);
		delete preroll;
		m_highRateLastFrameTime = QDateTime::currentMSecsSinceEpoch();
	} else {
		smallFrame = preroll->frame.scaled(preroll->frame.width() / 2, preroll->frame.height() / 2);
		QBuffer buffer(&jpeg);
		buffer.open(QIODevice::WriteOnly);
		smallFrame.save(&buffer, "JPG");
		SYNTRO_EHEAD *multiCast = clientBuildMessage(m_lowRateVideoPort, sizeof(SYNTRO_RECORD_AVMUX) + jpeg.size());
		SYNTRO_RECORD_AVMUX *avmuxHead = (SYNTRO_RECORD_AVMUX *)(multiCast + 1);
        SyntroUtils::avmuxHeaderInit(avmuxHead, &m_avLowParams, SYNTRO_RECORDHEADER_PARAM_PREROLL, m_recordIndex++, 0, jpeg.size(), 0);
		SyntroUtils::convertInt64ToUC8(preroll->timestamp, avmuxHead->recordHeader.timestamp);
		memcpy((unsigned char *)(avmuxHead + 1), jpeg.data(), jpeg.size());
		int length = sizeof(SYNTRO_RECORD_AVMUX) + jpeg.size();
		clientSendMessage(m_lowRateVideoPort, multiCast, length, SYNTROLINK_LOWPRI);
		delete preroll;
		m_lowRateLastFrameTime = QDateTime::currentMSecsSinceEpoch();
	}
}

void CameraClient::sendFrame(qint64 now, const QImage& frame, int param, bool highRate)
{
	int length;
	SYNTRO_EHEAD *multiCast;
	SYNTRO_RECORD_AVMUX *avmuxHead;
	QByteArray jpeg;
	QImage smallFrame;

	if (highRate) {
		QBuffer buffer(&jpeg);
		buffer.open(QIODevice::WriteOnly);
		frame.save(&buffer, "JPG");
		multiCast = clientBuildMessage(m_highRateVideoPort, sizeof(SYNTRO_RECORD_AVMUX) + jpeg.size());
		avmuxHead = (SYNTRO_RECORD_AVMUX *)(multiCast + 1);
        SyntroUtils::avmuxHeaderInit(avmuxHead, &m_avHighParams, param, m_recordIndex++, 0, jpeg.size(), 0);
		memcpy((unsigned char *)(avmuxHead + 1), jpeg.data(), jpeg.size());
		length = sizeof(SYNTRO_RECORD_AVMUX) + jpeg.size();
		clientSendMessage(m_highRateVideoPort, multiCast, length, SYNTROLINK_LOWPRI);
		m_highRateLastFrameTime = m_highRateLastFullFrameTime = now;
	} else {
		smallFrame = frame.scaled(frame.width() / 2, frame.height() / 2);
		QBuffer buffer(&jpeg);
		buffer.open(QIODevice::WriteOnly);
		smallFrame.save(&buffer, "JPG");
		multiCast = clientBuildMessage(m_lowRateVideoPort, sizeof(SYNTRO_RECORD_AVMUX) + jpeg.size());
		avmuxHead = (SYNTRO_RECORD_AVMUX *)(multiCast + 1);
        SyntroUtils::avmuxHeaderInit(avmuxHead, &m_avLowParams, param, m_recordIndex++, 0, jpeg.size(), 0);
		memcpy((unsigned char *)(avmuxHead + 1), jpeg.data(), jpeg.size());
		length = sizeof(SYNTRO_RECORD_AVMUX) + jpeg.size();
		clientSendMessage(m_lowRateVideoPort, multiCast, length, SYNTROLINK_LOWPRI);
		m_lowRateLastFrameTime = m_lowRateLastFullFrameTime = now;
	}
}

void CameraClient::sendNullFrame(qint64 now, bool highRate)
{
	int length;
	SYNTRO_EHEAD *multiCast;
	SYNTRO_RECORD_AVMUX *avmuxHead;

	if (highRate) {
		multiCast = clientBuildMessage(m_highRateVideoPort, sizeof(SYNTRO_RECORD_AVMUX));
		avmuxHead = (SYNTRO_RECORD_AVMUX *)(multiCast + 1);
        SyntroUtils::avmuxHeaderInit(avmuxHead, &m_avHighParams, SYNTRO_RECORDHEADER_PARAM_NOOP, m_recordIndex++, 0, 0, 0);
		length = sizeof(SYNTRO_RECORD_AVMUX);
		clientSendMessage(m_highRateVideoPort, multiCast, length, SYNTROLINK_LOWPRI);
		m_highRateLastFrameTime = now;
	} else {
		multiCast = clientBuildMessage(m_lowRateVideoPort, sizeof(SYNTRO_RECORD_AVMUX));
		avmuxHead = (SYNTRO_RECORD_AVMUX *)(multiCast + 1);
        SyntroUtils::avmuxHeaderInit(avmuxHead, &m_avLowParams, SYNTRO_RECORDHEADER_PARAM_NOOP, m_recordIndex++, 0, 0, 0);
		length = sizeof(SYNTRO_RECORD_AVMUX);
		clientSendMessage(m_lowRateVideoPort, multiCast, length, SYNTROLINK_LOWPRI);
		m_lowRateLastFrameTime = now;
	}
}


void CameraClient::sendHeartbeatFrame(qint64 now, const QImage& frame)
{
    // see if can send anything anyway
    if (clientIsServiceActive(m_highRateVideoPort)) {
		if (clientClearToSend(m_highRateVideoPort)) {
		    if (SyntroUtils::syntroTimerExpired(now, m_highRateLastFullFrameTime, m_highRateFullFrameMaxInterval)) {
				sendFrame(now, frame, SYNTRO_RECORDHEADER_PARAM_REFRESH, true);
			} else {
				if (SyntroUtils::syntroTimerExpired(now, m_highRateLastFrameTime, m_highRateNullFrameInterval)) {
					// send null frame as no changes
					sendNullFrame(now, true);
				}
			}
		}
	} else {
		m_highRateLastFrameTime = now;
	}

    if (clientIsServiceActive(m_lowRateVideoPort)) {
		if (clientClearToSend(m_lowRateVideoPort)) {
		    if (SyntroUtils::syntroTimerExpired(now, m_lowRateLastFullFrameTime, m_lowRateFullFrameMaxInterval)) {
				sendFrame(now, frame, SYNTRO_RECORDHEADER_PARAM_REFRESH, false);
			} else {
				if (SyntroUtils::syntroTimerExpired(now, m_lowRateLastFrameTime, m_lowRateNullFrameInterval)) {
					// send null frame as no changes
					sendNullFrame(now, false);
				}
			}
		}
	}
}

void CameraClient::getAndSendFrame(qint64 now)
{
	QImage frame;

    // see if anything to send

    if (!dequeueVideoFrame(frame))
        return;

 	if (clientIsServiceActive(m_highRateVideoPort)) {

		// see if allowed to send high rate frame and discard frame if not
		if (clientClearToSend(m_highRateVideoPort)) {

	        // ok - send frame

			sendFrame(now, frame, SYNTRO_RECORDHEADER_PARAM_NORMAL, true);
		}
	} else {
		m_highRateLastFrameTime = now;					// need to do this to keep low rate frames going
	}

    // see if allowed to send anything and discard frame if not
    if (clientIsServiceActive(m_lowRateVideoPort) && clientClearToSend(m_lowRateVideoPort)) {
		QImage smallFrame = frame.scaled(frame.width() / 2, frame.height() / 2);
		sendFrame(now, smallFrame, SYNTRO_RECORDHEADER_PARAM_NORMAL, false);
	}
}

void CameraClient::appClientInit()
{
	newStream();
	m_recordIndex = 0;
}

void CameraClient::appClientExit()
{
	clearQueue();
}

void CameraClient::appClientBackground()
{
	processVideoQueue();
}

void CameraClient::appClientConnected()
{
	clearQueue();
}

void CameraClient::appClientReceiveE2E(int servicePort, SYNTRO_EHEAD *header, int length)
{
	if (servicePort != m_PTZPort) {
		logWarn(QString("Received E2E on incorrect port %1").arg(m_PTZPort));
		free(header);
		return;
	}
	if (length != sizeof(SYNTRO_PTZ)) {
		logWarn(QString("Received PTZ message with incorrect length %1").arg(length));
		free(header);
		return;
	}
	emit setPTZ((SYNTRO_PTZ *)(header + 1));
	free(header);
}

bool CameraClient::dequeueVideoFrame(QImage& frame)
{
	QMutexLocker lock(&m_frameQMutex);

	if (m_frameQ.empty())
		return false;

	frame = m_frameQ.dequeue();
	return true;
}

void CameraClient::clearQueue()
{
	QMutexLocker lock(&m_frameQMutex);
	m_frameQ.clear();
}

void CameraClient::newImage(QImage frame)
{
	if (m_frameQMutex.tryLock()) {
		m_frameQ.enqueue(frame);

		if (m_frameQ.count() > 3)
			m_frameQ.dequeue();

		m_frameQMutex.unlock();
	}
}


void CameraClient::newStream()
{
	// check defaults present

	QSettings *settings = SyntroUtils::getSettings();

    settings->beginGroup(SYNTRORTSP_STREAM_GROUP);

    if (!settings->contains(SYNTRORTSP_HIGHRATE_VIDEO_MININTERVAL))
        settings->setValue(SYNTRORTSP_HIGHRATE_VIDEO_MININTERVAL, "100");

    if (!settings->contains(SYNTRORTSP_HIGHRATE_VIDEO_MAXINTERVAL))
        settings->setValue(SYNTRORTSP_HIGHRATE_VIDEO_MAXINTERVAL, "10000");

    if (!settings->contains(SYNTRORTSP_HIGHRATE_VIDEO_NULLINTERVAL))
        settings->setValue(SYNTRORTSP_HIGHRATE_VIDEO_NULLINTERVAL, "2000");

    if (!settings->contains(SYNTRORTSP_LOWRATE_VIDEO_MININTERVAL))
        settings->setValue(SYNTRORTSP_LOWRATE_VIDEO_MININTERVAL, "6000");

    if (!settings->contains(SYNTRORTSP_LOWRATE_VIDEO_MAXINTERVAL))
        settings->setValue(SYNTRORTSP_LOWRATE_VIDEO_MAXINTERVAL, "60000");

    if (!settings->contains(SYNTRORTSP_LOWRATE_VIDEO_NULLINTERVAL))
        settings->setValue(SYNTRORTSP_LOWRATE_VIDEO_NULLINTERVAL, "6000");

    settings->endGroup();
    settings->beginGroup(SYNTRORTSP_MOTION_GROUP);

    if (!settings->contains(SYNTRORTSP_MOTION_MIN_DELTA))
        settings->setValue(SYNTRORTSP_MOTION_MIN_DELTA, "500");

       if (!settings->contains(SYNTRORTSP_MOTION_DELTA_INTERVAL))
        settings->setValue(SYNTRORTSP_MOTION_DELTA_INTERVAL, "200");

    if (!settings->contains(SYNTRORTSP_MOTION_PREROLL))
        settings->setValue(SYNTRORTSP_MOTION_PREROLL, "2000");

    if (!settings->contains(SYNTRORTSP_MOTION_POSTROLL))
        settings->setValue(SYNTRORTSP_MOTION_POSTROLL, "2000");

	settings->endGroup();

	// remove the old streams
	
	if (m_highRateVideoPort != -1)
		clientRemoveService(m_highRateVideoPort);
	if (m_lowRateVideoPort != -1)
		clientRemoveService(m_lowRateVideoPort);
	if (m_PTZPort != -1)
		clientRemoveService(m_PTZPort);

	// and start the new streams

    settings->beginGroup(SYNTRORTSP_STREAM_GROUP);

	m_highRateVideoPort = clientAddService(SYNTRO_STREAMNAME_AVMUX, SERVICETYPE_MULTICAST, true);
	m_lowRateVideoPort = clientAddService(SYNTRO_STREAMNAME_AVMUXLR, SERVICETYPE_MULTICAST, true);

	m_PTZPort = clientAddService(SYNTRO_STREAMNAME_PTZ, SERVICETYPE_E2E, true);

    m_highRateMinInterval = settings->value(SYNTRORTSP_HIGHRATE_VIDEO_MININTERVAL).toInt();
    m_highRateFullFrameMaxInterval = settings->value(SYNTRORTSP_HIGHRATE_VIDEO_MAXINTERVAL).toInt();
    m_highRateNullFrameInterval = settings->value(SYNTRORTSP_HIGHRATE_VIDEO_NULLINTERVAL).toInt();

    m_lowRateMinInterval = settings->value(SYNTRORTSP_LOWRATE_VIDEO_MININTERVAL).toInt();
    m_lowRateFullFrameMaxInterval = settings->value(SYNTRORTSP_LOWRATE_VIDEO_MAXINTERVAL).toInt();
    m_lowRateNullFrameInterval = settings->value(SYNTRORTSP_LOWRATE_VIDEO_NULLINTERVAL).toInt();

    settings->endGroup();
    settings->beginGroup(SYNTRORTSP_MOTION_GROUP);

    m_minDelta = settings->value(SYNTRORTSP_MOTION_MIN_DELTA).toInt();
    m_deltaInterval = settings->value(SYNTRORTSP_MOTION_DELTA_INTERVAL).toInt();
    m_preroll = settings->value(SYNTRORTSP_MOTION_PREROLL).toInt();
    m_postroll = settings->value(SYNTRORTSP_MOTION_POSTROLL).toInt();

    settings->endGroup();

    m_highRateLastFrameTime = QDateTime::currentMSecsSinceEpoch();
    m_lowRateLastFrameTime = QDateTime::currentMSecsSinceEpoch();
    m_highRateLastPrerollFrameTime = QDateTime::currentMSecsSinceEpoch();
    m_lowRateLastPrerollFrameTime = QDateTime::currentMSecsSinceEpoch();
    m_lastDeltaTime = QDateTime::currentMSecsSinceEpoch();

	if (m_deltaInterval == 0) {
        m_sequenceState = CAMERACLIENT_STATE_CONTINUOUS;    // motion detection inactive
        STATE_DEBUG("STATE_CONTINUOUS");
    } else {
        m_sequenceState = CAMERACLIENT_STATE_IDLE;          // use motion detect state machine
        STATE_DEBUG("STATE_IDLE");
    }

    delete settings;
	m_lastChangeTime = QDateTime::currentMSecsSinceEpoch();
	m_imageChanged = false;
}

void CameraClient::setVideoFormat(int width, int height, int frameRate)
{
	m_avHighParams.videoFramerate = frameRate;
	m_avHighParams.videoHeight = height;
	m_avHighParams.videoWidth = width;
	m_avLowParams.videoFramerate = frameRate;
	m_avLowParams.videoHeight = height/2;
	m_avLowParams.videoWidth = width/2;
}

