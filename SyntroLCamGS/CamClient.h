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

#ifndef CAMCLIENT_H
#define CAMCLIENT_H

#include "SyntroLib.h"
#include "ChangeDetector.h"

#ifdef USING_GSTREAMER
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>
#endif


#include <qimage.h>
#include <qmutex.h>

//  Settings keys

//----------------------------------------------------------
//	Stream group

// group name for stream-related entries

#define	CAMCLIENT_STREAM_GROUP           "StreamGroup"

#define CAMCLIENT_AV_TYPE                "AvType"
#define	CAMCLIENT_HIGHRATEVIDEO_MININTERVAL		"HighRateMinInterval"
#define	CAMCLIENT_HIGHRATEVIDEO_MAXINTERVAL     "HighRateMaxInterval"
#define	CAMCLIENT_HIGHRATEVIDEO_NULLINTERVAL    "HighRateNullInterval"
#define CAMCLIENT_GENERATE_LOWRATE				"GenerateLowRate"

#define CAMCLIENT_GS_VIDEO_HIGHRATE             "GSVideoHighRate"
#define CAMCLIENT_GS_VIDEO_LOWRATE              "GSVideoLowRate"
#define CAMCLIENT_GS_AUDIO_HIGHRATE             "GSAudioHighRate"
#define CAMCLIENT_GS_AUDIO_LOWRATE              "GSAudioLowRate"
#define CAMCLIENT_MJPEG_LOWFRAMERATE            "MJPEGLowFrameRate"

//----------------------------------------------------------
//	Motion group

// group name for motion detection entries

#define	CAMCLIENT_MOTION_GROUP           "MotionGroup"

#define CAMCLIENT_MOTION_TILESTOSKIP     "MotionTilesToSkip"
#define CAMCLIENT_MOTION_INTERVALSTOSKIP "MotionIntervalsToSkip"
#define	CAMCLIENT_MOTION_MIN_DELTA       "MotionMinDelta"
#define	CAMCLIENT_MOTION_MIN_NOISE       "MotionMinNoise"

// interval between frames checked for deltas in mS. 0 means never check - always send image

#define CAMCLIENT_MOTION_DELTA_INTERVAL  "MotionDeltaInterval"

//  length of preroll. 0 turns off the feature

#define CAMCLIENT_MOTION_PREROLL         "MotionPreroll"

// length of postroll. 0 turns off the feature

#define CAMCLIENT_MOTION_POSTROLL        "MotionPostroll"

// maximum rate - 120 per second (allows for 4x rate during preroll send)

#define	CAMERA_IMAGE_INTERVAL	((qint64)SYNTRO_CLOCKS_PER_SEC/120)

typedef struct
{
    QByteArray data;                                        // the data
    qint64 timestamp;                                       // the timestamp
    int param;                                              // param for frame
} PREROLL;

typedef struct
{
    QByteArray data;                                        // the data
    qint64 timestamp;                                       // the timestamp
} CLIENT_QUEUEDATA;

#ifdef USING_GSTREAMER
class AVMuxEncode;
#endif

#define CAMERA_GROUP					"CameraGroup"

#define	CAMERA_CAMERA					"Device"
#define	CAMERA_WIDTH					"Width"
#define	CAMERA_HEIGHT					"Height"
#define	CAMERA_FRAMERATE				"FrameRate"
#define CAMERA_FORMAT					"Format"

#define CAMERA_DEFAULT_DEVICE			"<default device>"

// These defines are for the motion sequence state machine

#define CAMCLIENT_STATE_IDLE         0               // waiting for a motion event
#define CAMCLIENT_STATE_PREROLL      1               // sending preroll saved frames from the queue
#define CAMCLIENT_STATE_INSEQUENCE   2               // sending frames as normal during motion sequence
#define CAMCLIENT_STATE_POSTROLL     3               // sending the postroll frames
#define CAMCLIENT_STATE_CONTINUOUS   4               // no motion detect so continuous sending

#define CAMCLIENT_CAPS_INTERVAL 5000                 // interval between caps sends

class CamClient : public Endpoint
{
    Q_OBJECT

public:
    CamClient(QObject *parent);
    virtual ~CamClient();
    int getFrameCount();
    int getAudioSampleCount();

public slots:
    void newStream();
    void newJPEG(QByteArray);
    void newAudio(QByteArray);
    void videoFormat(int width, int height, int framerate);
    void audioFormat(int sampleRate, int channels, int sampleSize);

protected:
    void appClientInit();
    void appClientExit();
    void appClientReceiveE2E(int servicePort, SYNTRO_EHEAD *header, int length); // process an E2E message
    void appClientConnected();								// called when endpoint is connected to SyntroControl
    void appClientBackground();
    void processAudioQueue();  								// processes the audio queue
    void processSensorQueue();  							// processes the sensor queue
    void processEncoderQueue();                             // processes the encoder output

    int m_avmuxPortHighRate;								// the local port assigned to the high rate avmux service
    int m_avmuxPortLowRate;                                 // the local port assigned to the low rate avmux service
    int m_controlPort;										// the local port assigned to the control E2E service

private:
    void processAVQueueMJPPCM();                            // processes the video and audio data in MJPPCM mode
    void sendHeartbeatFrameMJPPCM(qint64 now, const QByteArray& jpeg);  // see if need to send null or full frame
    bool sendAVMJPPCM(qint64 now, int param, bool checkMotion); // sends a audio and video if there is any. Returns true if motion
    void sendNullFrameMJPPCM(qint64 now, bool highRate);    // sends a null frame
    void sendPrerollMJPPCM(bool highRate);                  // sends a preroll audio and/or video frame

    bool m_generateLowRate;
    int m_videoCompressionRateHigh;
    int m_audioCompressionRateHigh;
    int m_videoCompressionRateLow;
    int m_audioCompressionRateLow;

#ifdef USING_GSTREAMER
    void processAVQueueRTP();                            // processes the video and audio data in MJPPCM mode
    void sendHeartbeatFrameRTP(qint64 now, const QByteArray& jpeg);  // see if need to send null or full frame
    bool sendAVRTP(qint64 now, int param, bool checkMotion); // sends a audio and video if there is any. Returns true if motion
    void sendNullFrameRTP(qint64 now);                   // sends a null frame
    void sendPrerollRTP();                               // sends a preroll audio and/or video frame

    AVMuxEncode *m_highRateEncoder;
    AVMuxEncode *m_lowRateEncoder;

    void establishPipelines();
    void sendHighRateCaps();
    void sendLowRateCaps();

    qint64 m_lastCapsSend;

#endif

    void checkForMotion(qint64 now, QByteArray& jpeg);      // checks to see if a motion event has occured
    bool dequeueVideoFrame(QByteArray& videoData, qint64& timestamp);
    bool dequeueAudioFrame(QByteArray& audioData, qint64& timestamp);
    bool dequeueSensorData(QByteArray& sensorData);
    void clearVideoQueue();
    void clearAudioQueue();
    void clearQueues();
    void ageOutPrerollQueues(qint64 now);

    qint64 m_lastChangeTime;                                // time last frame change was detected
    bool m_imageChanged;                                    // if image has changed

    qint64 m_lastFrameTime;                                 // last time any frame was sent - null or full
    qint64 m_lastLowRateFrameTime;                          // last time any frame was sent - null or full - on low rate
    qint64 m_lastPrerollFrameTime;                          // last time a frame was added to the preroll
    qint64 m_lastLowRatePrerollFrameTime;                   // last time a low rate frame was added to the preroll
    qint64 m_lastFullFrameTime;                             // last time a full frame was sent
    qint64 m_lastLowRateFullFrameTime;                      // last time a full frame was sent on low rate
    qint64 m_lastRefreshTime;                               // last refresh time (only for RTP)
    qint64 m_minInterval;                                   // min interval between frames
    qint64 m_fullFrameMaxInterval;                          // max interval between full frame refreshes
    qint64 m_nullFrameInterval;                             // max interval between null or real frames
    qint64 m_mjpegLowFrameInterval;                         // min interval between low rate mjpeg frames

    int m_minDelta;											// min image change required
    int m_minNoise;											// min chunk change for its delta to be counted
    qint64 m_deltaInterval;                                 // interval between frames checked for motion
    qint64 m_preroll;                                       // length in mS of preroll
    qint64 m_postroll;                                      // length in mS of postroll

    int m_tilesToSkip;                                      // number of tiles in an interval to skip
    int m_intervalsToSkip;                                  // number of intervals to skip

    int m_sequenceState;                                    // the state of the motion sequence state machine
    qint64 m_postrollStart;                                 // time that the postroll started

    QQueue<PREROLL *> m_videoPrerollQueue;                  // queue of PREROLL structures for the video preroll queue
    QQueue<PREROLL *> m_videoLowRatePrerollQueue;           // queue of PREROLL structures for the low rate video preroll queue
    QQueue<PREROLL *> m_audioPrerollQueue;                  // queue of PREROLL structures for the audio preroll queue
    QQueue<PREROLL *> m_audioLowRatePrerollQueue;           // queue of PREROLL structures for the low rate audio preroll queue

    qint64 m_lastDeltaTime;                                 // when the delta was last checked

    QQueue <CLIENT_QUEUEDATA *> m_videoFrameQ;
    QMutex m_videoQMutex;

    QQueue <CLIENT_QUEUEDATA *> m_audioFrameQ;
    QMutex m_audioQMutex;

    ChangeDetector m_cd;                                    // the change detector instance

    int m_frameCount;
    QMutex m_frameCountLock;

    int m_audioSampleCount;
    QMutex m_audioSampleLock;

    int m_recordIndex;                                      // increments for every avmux record constructed

    SYNTRO_AVPARAMS m_avParams;                             // used to hold stream parameters

    bool m_gotVideoFormat;
    bool m_gotAudioFormat;
};

#endif // CAMCLIENT_H

