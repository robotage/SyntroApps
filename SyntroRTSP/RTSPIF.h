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


#ifndef RTSPIF_H
#define RTSPIF_H

#include "Camera.h"
#include "SyntroLib.h"
#include "SyntroAV/SyntroAVDefs.h"
#include <gst/gst.h>
#include <gst/app/gstappsink.h>

#define	RTSP_BGND_INTERVAL		(SYNTRO_CLOCKS_PER_SEC/50)	
#define RTSP_NOFRAME_TIMEOUT    (SYNTRO_CLOCKS_PER_SEC * 10)
#define RTSP_PIPELINE_RESET     (SYNTRO_CLOCKS_PER_SEC * 60 * 5)

class RTSPIF : public SyntroThread
{
	Q_OBJECT

public:
	RTSPIF();
	virtual ~RTSPIF();

	QSize getImageSize();

    // gstreamer callbacks

    void processVideoSinkData();
    void processVideoPrerollData();

	GstElement *m_pipeline;
 
public slots:
	void setPTZ(SYNTRO_PTZ *PTZ);
    void newCamera();

signals:
	void newImage(QImage frame);
	void netcamStatus(QString status);
	void setVideoFormat(int width, int height, int frameRate);

protected:
	 void timerEvent(QTimerEvent *event);
	 void initThread();
	 void finishThread();

private:
	bool open();
	void close();
    bool newPipeline();
    void deletePipeline();
	int getCapsValue(const QString &caps, const QString& key, const QString& termChar);

	bool m_pipelineActive;
    GstElement *m_appVideoSink;
    GstPad *m_appVideoSinkPad;
    QMutex m_videoLock;
    gint m_videoBusWatch;
	QString m_caps;
	QString m_widthPrefix;
	QString m_heightPrefix;
	QString m_frameratePrefix;
 
	QString m_IPAddress;
	int m_port;
	QString m_user;
	QString m_pw;

	int m_controlState;										// control connection state
	int m_width;
	int m_height;
	int m_frameRate;
    int m_frameSize;

	QImage m_lastImage;										// used to store last image for when required
	int m_frameCount;										// number of frames received

	QMutex m_netCamLock;
	int m_timer;											// ID for background timer

    qint64 m_lastFrameTime;
    bool m_firstFrame;
    qint64 m_startTime;
};

#endif // RTSPIF_H
