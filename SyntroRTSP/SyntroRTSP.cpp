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


#include "SyntroRTSP.h"
#include <QtGlobal>
#include <qnetworkinterface.h>
#include <qboxlayout.h>

#include "CameraClient.h"
#include "RTSPIF.h"
#include "SyntroAboutDlg.h"
#include "BasicSetupDlg.h"
#include "StreamsDlg.h"
#include "MotionDlg.h"
#include "RTSPDlg.h"

#define STATUS_REFRESH_INTERVAL_SECONDS 2

SyntroRTSP::SyntroRTSP()
	: QMainWindow()
{
    m_logTag = "SyntroRTSP";
	ui.setupUi(this);

	m_frameCount = 0;
	m_frameRateTimer = 0;
	m_frameRefreshTimer = 0;
	m_camera = NULL;
	m_cameraClient = NULL;

	QWidget *centralWidget = new QWidget(this);
	QVBoxLayout *verticalLayout = new QVBoxLayout(centralWidget);
	verticalLayout->setSpacing(6);
	verticalLayout->setContentsMargins(0, 0, 0, 0);
	m_cameraView = new QLabel(centralWidget);
	
	QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	sizePolicy.setHorizontalStretch(0);
	sizePolicy.setVerticalStretch(0);
	sizePolicy.setHeightForWidth(m_cameraView->sizePolicy().hasHeightForWidth());
	m_cameraView->setSizePolicy(sizePolicy);
	m_cameraView->setMinimumSize(QSize(320, 240));
	m_cameraView->setAlignment(Qt::AlignCenter);

	verticalLayout->addWidget(m_cameraView);

	setCentralWidget(centralWidget);

	connect(ui.actionExit, SIGNAL(triggered()), this, SLOT(close()));
	connect(ui.actionAbout, SIGNAL(triggered()), this, SLOT(onAbout()));
	connect(ui.actionBasicSetup, SIGNAL(triggered()), this, SLOT(onBasicSetup()));
    connect(ui.actionConfigureStreams, SIGNAL(triggered()), this, SLOT(onConfigureStreams()));
    connect(ui.actionConfigureMotionDetection, SIGNAL(triggered()), this, SLOT(onConfigureMotionDetection()));
    connect(ui.actionConfigureCamera, SIGNAL(triggered()), this, SLOT(onConfigureCamera()));

	m_controlStatus = new QLabel(this);
	m_controlStatus->setAlignment(Qt::AlignLeft);
	m_controlStatus->setText("");
	ui.statusBar->addWidget(m_controlStatus, 1);

 	m_netcamStatus = new QLabel(this);
	m_netcamStatus->setAlignment(Qt::AlignLeft);
	m_netcamStatus->setText("");
	ui.statusBar->addWidget(m_netcamStatus, 1);

	m_frameRateStatus = new QLabel(this);
	m_frameRateStatus->setAlignment(Qt::AlignCenter | Qt::AlignLeft);
	m_frameRateStatus->setText("0.0 fps  ");
	ui.statusBar->addPermanentWidget(m_frameRateStatus);

	restoreWindowState();

	SyntroUtils::syntroAppInit();
		
	clearQueue();

	m_cameraClient = new CameraClient(this);
	m_cameraClient->resumeThread();

	connect(m_cameraClient, SIGNAL(running()), this, SLOT(clientRunning()), Qt::QueuedConnection);

	m_frameCount = 0;
	m_frameRateTimer = startTimer(STATUS_REFRESH_INTERVAL_SECONDS * 1000);
	m_frameRefreshTimer = startTimer(20);
	m_frameRateStatus->setText("");
	
	setWindowTitle(QString("%1 - %2")
		.arg(SyntroUtils::getAppType())
		.arg(SyntroUtils::getAppName()));
}

SyntroRTSP::~SyntroRTSP()
{
}

void SyntroRTSP::clientRunning()
{
	m_camera = new RTSPIF();
	connect(m_camera, SIGNAL(newImage(QImage)), this, SLOT(newImage(QImage)), Qt::DirectConnection);
	connect(m_camera, SIGNAL(newImage(QImage)), m_cameraClient, SLOT(newImage(QImage)), Qt::DirectConnection);
    connect(m_camera, SIGNAL(netcamStatus(QString)), this, SLOT(netcamStatus(QString)), Qt::QueuedConnection);
	connect(m_camera, SIGNAL(setVideoFormat(int, int, int)), m_cameraClient, SLOT(setVideoFormat(int, int, int)), Qt::DirectConnection);
    connect(m_cameraClient, SIGNAL(setPTZ(SYNTRO_PTZ *)), m_camera, SLOT(setPTZ(SYNTRO_PTZ *)), Qt::DirectConnection);
    connect(this, SIGNAL(newStream()), m_cameraClient, SLOT(newStream()), Qt::QueuedConnection);
    connect(this, SIGNAL(newCamera()), m_camera, SLOT(newCamera()), Qt::QueuedConnection);
    m_cameraClient->setVideoFormat(640,480, 10);
    m_camera->resumeThread();
}

void SyntroRTSP::onAbout()
{
	SyntroAbout *dlg = new SyntroAbout();
	dlg->show();
}

void SyntroRTSP::onBasicSetup()
{
	BasicSetupDlg *dlg = new BasicSetupDlg(this);
	dlg->show();
}

void SyntroRTSP::onConfigureStreams()
{
    StreamsDlg dlg(this);
    if (dlg.exec() == QDialog::Accepted)
        emit newStream();
}

void SyntroRTSP::onConfigureMotionDetection()
{
    MotionDlg dlg(this);
    if (dlg.exec() == QDialog::Accepted)
        emit newStream();
}

void SyntroRTSP::onConfigureCamera()
{
    RTSPDlg dlg(this);
    if (dlg.exec() == QDialog::Accepted)
        emit newCamera();
}

void SyntroRTSP::saveWindowState()
{
	QSettings *settings = SyntroUtils::getSettings();

	settings->beginGroup("Window");
	settings->setValue("Geometry", saveGeometry());
	settings->setValue("State", saveState());
	settings->endGroup();
	
	delete settings;
}

void SyntroRTSP::restoreWindowState()
{
	QSettings *settings = SyntroUtils::getSettings();
		
	settings->beginGroup("Window");
	restoreGeometry(settings->value("Geometry").toByteArray());
	restoreState(settings->value("State").toByteArray());
	settings->endGroup();
	
	delete settings;
}

void SyntroRTSP::closeEvent(QCloseEvent *)
{
	saveWindowState();

	if (m_frameRateTimer) {
		killTimer(m_frameRateTimer);
		m_frameRateTimer = 0;
	}

	if (m_camera != NULL)
		m_camera->exitThread();
	m_cameraClient->exitThread();
	SyntroUtils::syntroAppExit();
}

void SyntroRTSP::newImage(QImage frame)
{
	m_frameCount++;

	if (m_frameQMutex.tryLock()) {
		if (m_frameQ.empty())
			m_frameQ.enqueue(frame);

		m_frameQMutex.unlock();
	}
}

void SyntroRTSP::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_frameRateTimer) {
		QString fps;
		double count = m_frameCount;
		m_frameCount = 0;
		fps.sprintf("%0.1lf fps  ", count / 3.0);
		m_frameRateStatus->setText(fps);
		m_controlStatus->setText(m_cameraClient->getLinkState());
	}
	else {
		QImage frame;

		m_frameQMutex.lock();

		if (!m_frameQ.empty()) {
			frame = m_frameQ.dequeue();
			m_frameQMutex.unlock();
			showImage(frame);
		} else {
			m_frameQMutex.unlock();
		}
	}
}

void SyntroRTSP::showImage(const QImage& frame)
{	
	if (isMinimized()) 
		return;
	
//	QImage swappedImg = img.rgbSwapped();

    QImage scaledImg = frame.scaled(m_cameraView->size(), Qt::KeepAspectRatio);
	m_cameraView->setPixmap(QPixmap::fromImage(scaledImg));
}

void SyntroRTSP::clearQueue()
{
	m_frameQMutex.lock();
	m_frameQ.clear();
	m_frameQMutex.unlock();
}

void SyntroRTSP::netcamStatus(QString status)
{
	m_netcamStatus->setText(status);
    logInfo(status);
}
