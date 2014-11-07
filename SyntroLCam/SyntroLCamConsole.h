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

#ifndef SYNTROLCAMCONSOLE_H
#define SYNTROLCAMCONSOLE_H

#include <QThread>

class CamClient;
class VideoDriver;
class AudioDriver;

class SyntroLCamConsole : public QThread
{
	Q_OBJECT

public:
    SyntroLCamConsole(bool daemonMode, QObject *parent);

public slots:
	void cameraState(QString);
	void newFrame();
	void aboutToQuit();
    void videoFormat(int width, int height, int frameRate);

protected:
	void run();
	void timerEvent(QTimerEvent *event);

private:
	bool createCamera();
	bool startVideo();
	void stopVideo();
    void startAudio();
    void stopAudio();
	void showHelp();
	void showStatus();

	void runConsole();
	void runDaemon();

	void registerSigHandler();
	static void sigHandler(int sig);

	CamClient *m_client;
    VideoDriver *m_camera;
    AudioDriver *m_audio;

	QString m_cameraState;
	int m_frameCount;
	int m_frameRateTimer;
    double m_computedFrameRate;
    double m_audioSamplesPerSecond;
	bool m_daemonMode;
	static volatile bool sigIntReceived;

    QString m_videoFormat;
    int m_width;
    int m_height;
    int m_framerate;

};

#endif // SYNTROLCAMCONSOLE_H

