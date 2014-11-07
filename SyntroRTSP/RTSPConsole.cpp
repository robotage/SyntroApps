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


#include "RTSPConsole.h"
#include "SyntroRTSP.h"
#include "CameraClient.h"
#include "RTSPIF.h"

#ifdef WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#include <signal.h>


volatile bool RTSPConsole::sigIntReceived = false;


#endif

RTSPConsole::RTSPConsole(bool daemonMode, QObject *parent)
	: QThread(parent)
{
    m_daemonMode = daemonMode;
    m_logTag = PRODUCT_TYPE;
	m_parent = parent;
	m_nfps = 0;
    m_camera = NULL;

#ifndef WIN32
    if (m_daemonMode) {
        registerSigHandler();

        if (daemon(1, 1)) {
            perror("daemon");
            return;
        }
    }
#endif

	SyntroUtils::syntroAppInit();

	m_client = new CameraClient(this);
	m_client->resumeThread();
	connect(m_client, SIGNAL(running()), this, SLOT(clientRunning()), Qt::QueuedConnection);

    if (!m_daemonMode) {
        m_frameCount = 0;
        m_frameRateTimer = startTimer(3 * SYNTRO_CLOCKS_PER_SEC);
    }

	start();
}

RTSPConsole::~RTSPConsole()
{
}

void RTSPConsole::clientRunning()
{
	m_camera = new RTSPIF();

    if (!m_daemonMode)
        connect(m_camera, SIGNAL(newImage(QImage)), this, SLOT(newImage(QImage)), Qt::DirectConnection);
	connect(m_camera, SIGNAL(newImage(QImage)), m_client, SLOT(newImage(QImage)), Qt::DirectConnection);
	connect(m_camera, SIGNAL(setVideoFormat(int, int, int)), m_client, SLOT(setVideoFormat(int, int, int)), Qt::DirectConnection);
    connect(m_client, SIGNAL(setPTZ(SYNTRO_PTZ *)), m_camera, SLOT(setPTZ(SYNTRO_PTZ *)), Qt::DirectConnection);
 	m_client->setVideoFormat(640,480, 10);
    m_camera->resumeThread();
}

void RTSPConsole::newImage(QImage )
{
	m_frameCount++;
}


void RTSPConsole::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_frameRateTimer) 
	{
		QString fps;
		double count = m_frameCount;
		m_frameCount = 0;
		m_nfps =  count / 3.0;
	}
}

void RTSPConsole::showHelp()
{
	printf("\nOptions are:\n\n");
	printf("  H - Show help\n");
	printf("  S - Show status\n");
	printf("  X - Exit\n");
}

void RTSPConsole::showStatus()
{
	printf("\nSyntroControl status is:%s\n", qPrintable(m_client->getLinkState()));
	printf("Frame rate is    :%f fps\n", m_nfps);
}


void RTSPConsole::run()
{

#ifndef WIN32
    if (m_daemonMode)
        runDaemon();
    else
#endif
        runConsole();

    m_camera->exitThread();
    m_client->exitThread();
    SyntroUtils::syntroAppExit();
    QCoreApplication::exit();
}

void RTSPConsole::runConsole()
{
	bool	mustExit;

#ifndef WIN32
        struct termios	ctty;

        tcgetattr(fileno(stdout), &ctty);
        ctty.c_lflag &= ~(ICANON);
        tcsetattr(fileno(stdout), TCSANOW, &ctty);
#endif
	mustExit = false;
	while(!mustExit)
	{
                printf("\nEnter option: ");
#ifdef WIN32
		switch (toupper(_getch()))
#else
                switch (toupper(getchar()))
#endif
		{
			case 'H':					// help
				showHelp();
				break;

			case 'S':					// show status
				showStatus();
				break;

			case 'X':					// exit program
                printf("\nExiting\n");
				if (m_frameRateTimer) {
					killTimer(m_frameRateTimer);
					m_frameRateTimer = 0;
				}
				mustExit = true;
				break;

			case '\n':
				continue;
		}
	}
}

#ifndef WIN32
void RTSPConsole::runDaemon()
{
    while (!RTSPConsole::sigIntReceived)
        msleep(100);
}

void RTSPConsole::registerSigHandler()
{
    struct sigaction sia;

    bzero(&sia, sizeof sia);
    sia.sa_handler = RTSPConsole::sigHandler;

    if (sigaction(SIGINT, &sia, NULL) < 0)
        perror("sigaction(SIGINT)");
}

void RTSPConsole::sigHandler(int)
{
    RTSPConsole::sigIntReceived = true;
}
#endif
