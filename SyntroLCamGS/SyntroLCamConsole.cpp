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

#include "SyntroLCamConsole.h"
#include "SyntroLCam.h"

#ifdef WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#endif

#include "VideoDriver.h"
#include "AudioDriver.h"

#define FRAME_RATE_TIMER_INTERVAL 3

volatile bool SyntroLCamConsole::sigIntReceived = false;

SyntroLCamConsole::SyntroLCamConsole(bool daemonMode, QObject *parent)
	: QThread(parent)
{
	m_daemonMode = daemonMode;

    m_computedFrameRate = 0.0;
	m_frameCount = 0;
	m_frameRateTimer = 0;
	m_camera = NULL;
    m_audio = NULL;

	m_client = NULL;

    m_width = 0;
    m_height = 0;
    m_framerate = 0;

#ifndef WIN32
	if (m_daemonMode) {
		registerSigHandler();
		
		if (daemon(1, 1)) {
			perror("daemon");
			return;
		}
	}
#endif

	connect((QCoreApplication *)parent, SIGNAL(aboutToQuit()), this, SLOT(aboutToQuit()));

    SyntroUtils::syntroAppInit();

    m_client = new CamClient(this);
	m_client->resumeThread();

    if (!m_daemonMode) {
        m_frameCount = 0;
        m_frameRateTimer = startTimer(FRAME_RATE_TIMER_INTERVAL * 1000);
    }

	start();
}

void SyntroLCamConsole::aboutToQuit()
{
    if (m_frameRateTimer) {
        killTimer(m_frameRateTimer);
        m_frameRateTimer = 0;
    }

	for (int i = 0; i < 5; i++) {
		if (wait(1000))
			break;

		if (!m_daemonMode)
			printf("Waiting for console thread to finish...\n");
	}
}

bool SyntroLCamConsole::createCamera()
{
	if (m_camera) {
		delete m_camera;
		m_camera = NULL;
	}

    m_camera = new VideoDriver();

	if (!m_camera)
		return false;

	return true;
}

bool SyntroLCamConsole::startVideo()
{
	if (!m_camera) {
		if (!createCamera()) {
            appLogError("Error allocating camera");
			return false;
		}
	}

	if (!m_daemonMode) {
		connect(m_camera, SIGNAL(cameraState(QString)), this, SLOT(cameraState(QString)), Qt::DirectConnection);
		connect(m_camera, SIGNAL(newFrame()), this, SLOT(newFrame()), Qt::DirectConnection);
    }

    connect(m_camera, SIGNAL(videoFormat(int,int,int)), this, SLOT(videoFormat(int,int,int)));
    connect(m_camera, SIGNAL(videoFormat(int,int,int)), m_client, SLOT(videoFormat(int,int,int)));

    connect(m_camera, SIGNAL(newJPEG(QByteArray)), m_client, SLOT(newJPEG(QByteArray)), Qt::DirectConnection);

    m_camera->resumeThread();

	return true;
}

void SyntroLCamConsole::stopVideo()
{
    if (m_camera) {
        disconnect(m_camera, SIGNAL(newJPEG(QByteArray)), m_client, SLOT(newJPEG(QByteArray)));

        disconnect(m_camera, SIGNAL(cameraState(QString)), this, SLOT(cameraState(QString)));
        disconnect(m_camera, SIGNAL(videoFormat(int,int,int)), this, SLOT(videoFormat(int,int,int)));
        disconnect(m_camera, SIGNAL(videoFormat(int,int,int)), m_client, SLOT(videoFormat(int,int,int)));

        m_camera->exitThread();
        m_camera = NULL;
    }
}

void SyntroLCamConsole::startAudio()
{
    m_audio = new AudioDriver();
    connect(m_audio, SIGNAL(newAudio(QByteArray)), m_client, SLOT(newAudio(QByteArray)), Qt::DirectConnection);
    connect(m_audio, SIGNAL(audioFormat(int, int, int)), m_client, SLOT(audioFormat(int, int, int)), Qt::QueuedConnection);
    m_audio->resumeThread();
}

void SyntroLCamConsole::stopAudio()
{
    disconnect(m_audio, SIGNAL(newAudio(QByteArray)), m_client, SLOT(newAudio(QByteArray)));
    disconnect(m_audio, SIGNAL(audioFormat(int, int, int)), m_client, SLOT(audioFormat(int, int, int)));

    m_audio->exitThread();
    m_audio = NULL;
}

void SyntroLCamConsole::videoFormat(int width, int height, int framerate)
{
    m_width = width;
    m_height = height;
    m_framerate = framerate;
}

void SyntroLCamConsole::cameraState(QString state)
{
	m_cameraState = state;
}

void SyntroLCamConsole::newFrame()
{
    m_frameCount++;
}

void SyntroLCamConsole::timerEvent(QTimerEvent *)
{
    m_computedFrameRate =  (double)m_frameCount / (double)FRAME_RATE_TIMER_INTERVAL;
	m_frameCount = 0;

    m_audioSamplesPerSecond = (double)m_client->getAudioSampleCount() / (double)FRAME_RATE_TIMER_INTERVAL;;
}

void SyntroLCamConsole::showHelp()
{
	printf("\nOptions are:\n\n");
	printf("  h - Show help\n");
	printf("  s - Show status\n");
	printf("  x - Exit\n");
}

void SyntroLCamConsole::showStatus()
{    
	printf("\nStatus: %s\n", qPrintable(m_client->getLinkState()));

	if (m_cameraState == "Running")
        printf("Measured frame rate is    : %f fps\n", m_computedFrameRate);
    else
		printf("Camera state: %s\n", qPrintable(m_cameraState));

    printf("Frame format is  : %s\n", qPrintable(m_videoFormat));
    printf("Frame size is    : %d x %d\n", m_width, m_height);
    printf("Frame rate is    : %d\n", m_framerate);
    printf("Audio byte rate is: %f\n", m_audioSamplesPerSecond);
}

void SyntroLCamConsole::run()
{
#ifndef WIN32
	if (m_daemonMode)
		runDaemon();
	else
#endif
		runConsole();

    stopVideo();
    stopAudio();

    m_client->exitThread();
    SyntroUtils::syntroAppExit();
	QCoreApplication::exit();
}

void SyntroLCamConsole::runConsole()
{
#ifndef WIN32
	struct termios	ctty;

	tcgetattr(fileno(stdout), &ctty);
	ctty.c_lflag &= ~(ICANON);
	tcsetattr(fileno(stdout), TCSANOW, &ctty);
#endif

	bool grabbing = startVideo();
    startAudio();

	while (grabbing) {
		printf("\nEnter option: ");

#ifdef WIN32
		switch (tolower(_getch()))
#else
        switch (tolower(getchar()))
#endif		
		{
		case 'h':
			showHelp();
			break;

		case 's':
			showStatus();
			break;

		case 'x':
			printf("\nExiting\n");
			grabbing = false;		
			break;

		case '\n':
			continue;
		}
	}
}

#ifndef WIN32
void SyntroLCamConsole::runDaemon()
{
	startVideo();
    startAudio();

    while (!SyntroLCamConsole::sigIntReceived)
		msleep(100); 
}

void SyntroLCamConsole::registerSigHandler()
{
	struct sigaction sia;

	bzero(&sia, sizeof sia);
    sia.sa_handler = SyntroLCamConsole::sigHandler;

	if (sigaction(SIGINT, &sia, NULL) < 0)
		perror("sigaction(SIGINT)");
}

void SyntroLCamConsole::sigHandler(int)
{
    SyntroLCamConsole::sigIntReceived = true;
}
#endif
