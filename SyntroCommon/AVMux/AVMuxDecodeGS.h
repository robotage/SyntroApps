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

#ifndef _AVMUXDECODE_H_
#define _AVMUXDECODE_H_

#include "SyntroLib.h"
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>

#include <qmutex.h>

#define AVMUXDECODE_INTERVAL  (SYNTRO_CLOCKS_PER_SEC / 100)

class AVMuxDecode : public SyntroThread
{
    Q_OBJECT

public:
    AVMuxDecode();

    // gstreamer callbacks

    void processVideoSinkData();
    void processAudioSinkData();

    GstElement *m_videoPipeline;
    GstElement *m_audioPipeline;


public slots:
    void newAVMuxData(QByteArray avmuxArray);

signals:
    void newImage(QImage image, qint64 timestamp);
    void newAudioSamples(QByteArray dataArray, qint64 timestamp, int rate, int channels, int size);

protected:
    void initThread();
    void timerEvent(QTimerEvent *event);
    void finishThread();

private:
    void processMJPPCM(SYNTRO_RECORD_AVMUX *avmux);
    void processRTP(SYNTRO_RECORD_AVMUX *avmux);
    void processAVData(QByteArray avmuxArray);

    bool newPipelines(SYNTRO_AVPARAMS *avParams);
    void putRTPVideoCaps(unsigned char *data, int length);
    void putRTPAudioCaps(unsigned char *data, int length);
    void putVideoData(const unsigned char *data, int length);
    void putAudioData(const unsigned char *data, int length);

    void deletePipelines();

    int m_timer;

    QMutex m_avmuxLock;
    QQueue <QByteArray> m_avmuxQ;

    QMutex m_videoLock;
    QQueue <GstBuffer *> m_videoSinkQ;

    QMutex m_audioLock;
    QQueue <GstBuffer *> m_audioSinkQ;

    qint64 m_lastVideoTimestamp;
    qint64 m_lastAudioTimestamp;

    // gstreamer stuff

    SYNTRO_AVPARAMS m_avParams;
    SYNTRO_AVPARAMS m_oldAvParams;

    QMutex m_lock;

    GstElement *m_appVideoSink;
    GstElement *m_appAudioSink;
    GstElement *m_appVideoSrc;
    GstElement *m_appAudioSrc;

    bool m_waitingForVideoCaps;
    bool m_waitingForAudioCaps;

    qint64 m_videoOffset;
    qint64 m_audioOffset;
    qint64 m_nextVideoTime;
    qint64 m_nextAudioTime;
    qint64 m_videoInterval;
    qint64 m_audioInterval;

	bool m_pipelinesActive;

    gint m_videoBusWatch;
    gint m_audioBusWatch;

};

#endif // _AVMUXDECODE_H_
