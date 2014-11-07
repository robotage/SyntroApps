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


#ifndef RTSPCONSOLE_H
#define RTSPCONSOLE_H

#include <QThread>
#include "SyntroLib.h"
#include "Camera.h"

class	CameraClient;
class	RTSPIF;

class RTSPConsole : public QThread
{
	Q_OBJECT

public:
    RTSPConsole(bool daemonMode, QObject *parent);
	~RTSPConsole();

public slots:
	void newImage(QImage);
	void clientRunning();

protected:
	void run();
    void runConsole();
    void runDaemon();
	void showHelp();
	void showStatus();

#ifndef WIN32
    void registerSigHandler();
    static void sigHandler(int sig);
    static volatile bool sigIntReceived;
#endif

	void timerEvent(QTimerEvent *event);

	QObject *m_parent;
	CameraClient *m_client;
	RTSPIF *m_camera;

	int m_frameCount;
	int m_frameRateTimer;
	double m_nfps;

	void startCapture();
	void stopCapture();
	bool createCamera();

	QString m_logTag;
    bool m_daemonMode;

};

#endif // RTSPCONSOLE_H
