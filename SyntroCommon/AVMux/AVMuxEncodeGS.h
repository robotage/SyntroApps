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
#ifndef _AVMUXENCODE_H_
#define _AVMUXENCODE_H_

#ifdef USING_GSTREAMER

#include "SyntroLib.h"
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>

#include <qmutex.h>

#define AVMUXENCODE_AV_TYPE_MJPPCM  0               // MJPEG + PCM
#define AVMUXENCODE_AV_TYPE_RTPMP4  1               // RTP framed MP4V + MP4A
#define AVMUXENCODE_AV_TYPE_RTPH264 2               // RTP framed H264 + MP4A


#define AVMUXENCODE_INTERVAL  (SYNTRO_CLOCKS_PER_SEC / 50)

typedef struct
{
    QByteArray data;
    qint64 timestamp;
    int param;
} AVMUX_QUEUEDATA;

class AVMuxEncode : public SyntroThread
{
    Q_OBJECT

public:
    AVMuxEncode(int AVMode);
    void setCompressionRates(int videoCompressionRate, int audioCompressionRate);

    bool newPipelines(SYNTRO_AVPARAMS *avParams);
    void deletePipelines();
    gchar *getVideoCaps() { return m_videoCaps; }
    gchar *getAudioCaps() { return m_audioCaps; }
    void newVideoData(QByteArray videoData, qint64 timestamp, int param);
    void newAudioData(QByteArray audioData, qint64 timestamp, int param);
    bool getCompressedVideo(QByteArray& videoData, qint64& timestamp, int& param);
    bool getCompressedAudio(QByteArray& audioData, qint64& timestamp, int& param);

    void newVideoData(QByteArray videoData);
    void newAudioData(QByteArray audioData);
    bool getCompressedVideo(QByteArray& videoData);
    bool getCompressedAudio(QByteArray& audioData);

    // gstreamer callbacks

    void processVideoSinkData();
    void processAudioSinkData();
    void needAudioData();
    GstElement *m_videoPipeline;
    GstElement *m_audioPipeline;

public slots:

signals:

protected:
    void initThread();
    void timerEvent(QTimerEvent *event);
    void finishThread();

private:
    int m_AVMode;

    int m_timer;

    qint64 m_lastQueuedVideoTimestamp;
    qint64 m_lastQueuedAudioTimestamp;

    int m_lastQueuedVideoParam;
    int m_lastQueuedAudioParam;

    void needVideoData();

    SYNTRO_AVPARAMS m_avParams;

    GstElement *m_appVideoSink;
    GstElement *m_appAudioSink;
    GstElement *m_appVideoSrc;
    GstElement *m_appAudioSrc;

    QMutex m_videoSrcLock;
    QQueue <AVMUX_QUEUEDATA *> m_videoSrcQ;

    QMutex m_audioSrcLock;
    QQueue <AVMUX_QUEUEDATA *> m_audioSrcQ;

    QMutex m_videoSinkLock;
    QQueue <AVMUX_QUEUEDATA *> m_videoSinkQ;

    QMutex m_audioSinkLock;
    QQueue <AVMUX_QUEUEDATA *> m_audioSinkQ;

    gchar *m_videoCaps;
    gchar *m_audioCaps;

    gint m_videoBusWatch;
    gint m_audioBusWatch;

    qint64 m_audioTimestamp;

    qint64 m_videoInterval;
    qint64 m_nextVideoTime;

    int m_videoCompressionRate;
    int m_audioCompressionRate;

    bool m_pipelinesActive;

};

#endif

#endif // _AVMUXENCODE_H_
