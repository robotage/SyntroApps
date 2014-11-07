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

#ifndef SYNTROLCAM_H
#define SYNTROLCAM_H

#include <QMainWindow>
#include "ui_SyntroLCam.h"

#include "VideoDriver.h"
#include "AudioDriver.h"
#include "CamClient.h"

#define PRODUCT_TYPE "SyntroLCamGS"

class SyntroLCam : public QMainWindow
{
	Q_OBJECT

public:
    SyntroLCam();
    ~SyntroLCam();

signals:
    void newStream();
    void newCamera();
    void newAudioSrc();

public slots:
	void onAbout();
	void onBasicSetup();
	void onConfigureCamera();
	void onConfigureStreams();
	void onConfigureMotion();
	void onConfigureAudio();

	void cameraState(QString state);
 	void audioState(QString state);
    void videoFormat(int width, int height, int frameRate);
	void newJPEG(QByteArray);

protected:
	void timerEvent(QTimerEvent *event);
	void closeEvent(QCloseEvent *event);

private:
	void startVideo();
	void stopVideo();
    void startAudio();
    void stopAudio();
	void processFrameQueue();
	void showJPEG(QByteArray frame);
	void showImage(QImage img);
	bool createCamera();
	void clearQueue();
	void layoutStatusBar();
	void saveWindowState();
	void restoreWindowState();

    Ui::SyntroCamClass ui;

	QLabel *m_cameraView;
	QLabel *m_audioStatus;
	QLabel *m_frameRateStatus;
	QLabel *m_controlStatus;

	CamClient *m_client;
    VideoDriver *m_camera;
    AudioDriver *m_audio;
	QString m_cameraState;
	QString m_audioState;
	QMutex m_frameQMutex;
	QQueue <QByteArray> m_jpegFrameQ;

	int m_frameRateTimer;
	int m_frameRefreshTimer;
	int m_frameCount;
	QSize m_imgSize;
};

#endif // SYNTROLCAM_H

