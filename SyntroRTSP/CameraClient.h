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


#ifndef CAMERACLIENT_H
#define CAMERACLIENT_H

#include "SyntroLib.h"

#include "SyntroAV/SyntroAVDefs.h"

#define	CAMERA_IMAGE_INTERVAL	((qint64)SYNTRO_CLOCKS_PER_SEC/120)		

typedef struct
{
    QImage frame;											// the frame
    qint64 timestamp;										// the timestamp
	int marker;												// motion marker for frame
} PREROLL;

// These defines are for the motion sequence state machine

#define CAMERACLIENT_STATE_IDLE         0                   // waiting for a motion event
#define CAMERACLIENT_STATE_PREROLL      1                   // sending preroll saved frames from the queue
#define CAMERACLIENT_STATE_INSEQUENCE   2                   // sending frames as normal during motion sequence
#define CAMERACLIENT_STATE_POSTROLL     3                   // sending the postroll frames
#define CAMERACLIENT_STATE_CONTINUOUS   4                   // no motion detect so continuous sending


class CameraClient : public Endpoint
{
	Q_OBJECT

public:
	CameraClient(QObject *parent);
	virtual ~CameraClient();

public slots:
	void newStream();
	void newImage(QImage image);
	void setVideoFormat(int width, int height, int frameRate);

signals:
	void setPTZ(SYNTRO_PTZ *PTZ);

protected:
	void appClientInit();
	void appClientExit();
	void appClientReceiveE2E(int servicePort, SYNTRO_EHEAD *header, int length); // process an E2E message
	void appClientConnected();								// called when endpoint is connected to SyntroControl
	void appClientBackground();

	void processVideoQueue();								// the main video frame processing state machine
	int m_highRateVideoPort;								// the local port assigned to the high rate service
	int m_lowRateVideoPort;									// the local port assigned to the low rate service
	int m_PTZPort;											// the local port assigned to the PTZ service

private:
	void checkForMotion(qint64 now, const QImage& frame);
	void getAndSendFrame(qint64 now);
	void sendFrame(qint64 now, const QImage& frame, int param, bool highRate);
	void sendNullFrame(qint64 now, bool highRate);
	void sendPreroll(PREROLL *preroll, bool highRate);
	void sendHeartbeatFrame(qint64 now, const QImage& frame);  // see if need to send null or full frame
	void computeAbsoluteDifferences(const QImage& newFrame, QImage& oldFrame);
	bool processDifferences(const QImage& diff);
	bool dequeueVideoFrame(QImage& frame);
	void clearQueue();

	QImage m_oldSmallFrame;									// previous old small frame

	qint64 m_highRateLastFrameTime;							// time of last high rate frame
	qint64 m_highRateMinInterval;							// min interval between high rate frames
	qint64 m_highRateFullFrameMaxInterval;					// max interval between full frame refreshes
	qint64 m_highRateNullFrameInterval;						// max interval between null or real frames
	bool m_highRateWaitingForFirst;							// if waiting for first of a sequence

	qint64 m_lowRateLastFrameTime;							// time of last low rate frame
	qint64 m_lowRateMinInterval;							// min interval between low rate frames
	qint64 m_lowRateFullFrameMaxInterval;					// max interval between full frame refreshes
	qint64 m_lowRateNullFrameInterval;						// max interval between null or real frames
	bool m_lowRateWaitingForFirst;							// if waiting for first of a sequence

	int m_minDelta;											// min image change required

	QQueue <QImage> m_frameQ;
	QMutex m_frameQMutex;

    int m_sequenceState;                                    // the state of the motion sequence state machine
    qint64 m_postrollStart;                                 // time that the postroll started

    QQueue<PREROLL *> m_highRatePrerollQueue;               // queue of PREROLL structures for the preroll queue
    QQueue<PREROLL *> m_lowRatePrerollQueue;				// queue of PREROLL structures for the preroll queue

    qint64 m_preroll;                                       // length in mS of preroll
    qint64 m_postroll;                                      // length in mS of postroll
    qint64 m_highRateLastPrerollFrameTime;                  // last time a frame was added to the high rate preroll
    qint64 m_lowRateLastPrerollFrameTime;                   // last time a frame was added to the low rate preroll
    qint64 m_deltaInterval;                                 // interval between frames checked for motion
    qint64 m_lastDeltaTime;                                 // when the delta was last checked

	qint64 m_lastChangeTime;                                // time last frame change was detected
    bool m_imageChanged;                                    // if image has changed

	qint64 m_highRateLastFullFrameTime;
	qint64 m_lowRateLastFullFrameTime;

	int m_recordIndex;

	SYNTRO_AVPARAMS m_avHighParams;
	SYNTRO_AVPARAMS m_avLowParams;

};

#endif // CAMERACLIENT_H

