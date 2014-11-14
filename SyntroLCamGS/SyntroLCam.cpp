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

#include <QMessageBox>
#include <qboxlayout.h>
#include "SyntroLCam.h"
#include "SyntroAboutDlg.h"
#include "BasicSetupDlg.h"
#include "StreamsDlg.h"
#include "CameraDlg.h"
#include "MotionDlg.h"
#include "AudioDlg.h"

#define FRAME_RATE_TIMER_INTERVAL 2

SyntroLCam::SyntroLCam()
    : QMainWindow()
{
	ui.setupUi(this);

	m_frameCount = 0;
	m_frameRateTimer = 0;
	m_frameRefreshTimer = 0;
	m_camera = NULL;
    m_audio = NULL;

	layoutStatusBar();

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
	connect(ui.actionConfigureCamera, SIGNAL(triggered()), this, SLOT(onConfigureCamera()));
	connect(ui.actionConfigureMotion, SIGNAL(triggered()), this, SLOT(onConfigureMotion()));
	connect(ui.actionConfigureAudio, SIGNAL(triggered()), this, SLOT(onConfigureAudio()));

    SyntroUtils::syntroAppInit();

    m_client = new CamClient(this);
    connect(this, SIGNAL(newStream()), m_client, SLOT(newStream()), Qt::QueuedConnection);
	m_client->resumeThread();
	
	restoreWindowState();

	setWindowTitle(QString("%1 - %2")
                   .arg(SyntroUtils::getAppType())
                   .arg(SyntroUtils::getAppName()));

	m_frameRateTimer = startTimer(FRAME_RATE_TIMER_INTERVAL * 1000);

	startVideo();
    startAudio();
}

SyntroLCam::~SyntroLCam()
{
	if (m_camera) {
		delete m_camera;
		m_camera = NULL;
	}

	clearQueue();
}

void SyntroLCam::closeEvent(QCloseEvent *)
{
	stopVideo();

	clearQueue();

	if (m_frameRateTimer) {
		killTimer(m_frameRateTimer);
		m_frameRateTimer = 0;
	}

	if (m_frameRefreshTimer) {
		killTimer(m_frameRefreshTimer);
		m_frameRefreshTimer = 0;
	}

    m_client->exitThread();

	saveWindowState();
    SyntroUtils::syntroAppExit();
}

bool SyntroLCam::createCamera()
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

void SyntroLCam::startVideo()
{
	if (!m_camera) {
		if (!createCamera()) {
            QMessageBox::warning(this, "SyntroLCam", "Error allocating camera", QMessageBox::Ok);
			return;
		}
	}

	clearQueue();

    connect(this, SIGNAL(newCamera()), m_camera, SLOT(newCamera()), Qt::QueuedConnection);
    connect(m_camera, SIGNAL(cameraState(QString)), this, SLOT(cameraState(QString)), Qt::QueuedConnection);
    connect(m_camera, SIGNAL(videoFormat(int,int,int)), this, SLOT(videoFormat(int,int,int)));
    connect(m_camera, SIGNAL(videoFormat(int,int,int)), m_client, SLOT(videoFormat(int,int,int)));

	connect(m_camera, SIGNAL(newJPEG(QByteArray)), this, SLOT(newJPEG(QByteArray)), Qt::DirectConnection);
	connect(m_camera, SIGNAL(newJPEG(QByteArray)), m_client, SLOT(newJPEG(QByteArray)), Qt::DirectConnection);

    m_camera->resumeThread();
	m_frameCount = 0;
	m_frameRefreshTimer = startTimer(10);
}

void SyntroLCam::stopVideo()
{
	if (m_camera) {
        disconnect(this, SIGNAL(newCamera()), m_camera, SLOT(newCamera()));
		disconnect(m_camera, SIGNAL(newJPEG(QByteArray)), this, SLOT(newJPEG(QByteArray)));
		disconnect(m_camera, SIGNAL(newJPEG(QByteArray)), m_client, SLOT(newJPEG(QByteArray)));

		disconnect(m_camera, SIGNAL(cameraState(QString)), this, SLOT(cameraState(QString)));
        disconnect(m_camera, SIGNAL(videoFormat(int,int,int)), this, SLOT(videoFormat(int,int,int)));
        disconnect(m_camera, SIGNAL(videoFormat(int,int,int)), m_client, SLOT(videoFormat(int,int,int)));

        m_camera->exitThread();
		m_camera = NULL;
	}

	if (m_frameRefreshTimer) {
		killTimer(m_frameRefreshTimer);
		m_frameRefreshTimer = 0;
	}
}

void SyntroLCam::startAudio()
{
    if (!m_audio) {
        m_audio = new AudioDriver();
        connect(this, SIGNAL(newAudioSrc()), m_audio, SLOT(newAudioSrc()), Qt::QueuedConnection);
        connect(m_audio, SIGNAL(newAudio(QByteArray)), m_client, SLOT(newAudio(QByteArray)), Qt::DirectConnection);
        connect(m_audio, SIGNAL(audioFormat(int, int, int)), m_client, SLOT(audioFormat(int, int, int)), Qt::QueuedConnection);
        connect(m_audio, SIGNAL(audioState(QString)), this, SLOT(audioState(QString)), Qt::QueuedConnection);
        m_audio->resumeThread();
    }
}

void SyntroLCam::stopAudio()
{
    if (m_audio) {
        disconnect(this, SIGNAL(newAudioSrc()), m_audio, SLOT(newAudioSrc()));
        disconnect(m_audio, SIGNAL(newAudio(QByteArray)), m_client, SLOT(newAudio(QByteArray)));
        disconnect(m_audio, SIGNAL(audioFormat(int, int, int)), m_client, SLOT(audioFormat(int, int, int)));
		disconnect(m_audio, SIGNAL(audioState(QString)), this, SLOT(audioState(QString)));
        m_audio->exitThread();
        m_audio = NULL;
    }
}

void SyntroLCam::cameraState(QString state)
{
	m_cameraState = state;
}

void SyntroLCam::audioState(QString state)
{
	m_audioState = state;
}

void SyntroLCam::videoFormat(int width, int height, int /* frameRate */)
{
	m_imgSize.setWidth(width);
	m_imgSize.setHeight(height);
}

void SyntroLCam::newJPEG(QByteArray frame)
{
	m_frameCount++;

	if (m_frameQMutex.tryLock()) {
		if (m_jpegFrameQ.empty())
			m_jpegFrameQ.enqueue(frame);

		m_frameQMutex.unlock();
	}
}

void SyntroLCam::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_frameRateTimer) {
		m_controlStatus->setText(m_client->getLinkState());
		m_audioStatus->setText(QString("Audio: ") + m_audioState);
		if (m_cameraState == "Running") {
			m_frameRateStatus->setText(QString().sprintf("Video: %0.1lf fps",
				(double)m_frameCount / FRAME_RATE_TIMER_INTERVAL));

			m_frameCount = 0;
        } else {
			m_frameRateStatus->setText(QString("Video: ") + m_cameraState);
		}
    } else {
		processFrameQueue();
	}
}

void SyntroLCam::processFrameQueue()
{
	QByteArray frame;
	QImage img;

	m_frameQMutex.lock();

	if (!m_jpegFrameQ.empty()) {
		frame = m_jpegFrameQ.dequeue();
	}

	m_frameQMutex.unlock();

	if (isMinimized())
		return;

    showJPEG(frame);
}

void SyntroLCam::showJPEG(QByteArray frame)
{  
	if (frame.size() > 0) {
		QImage img;
		img.loadFromData(frame, "JPEG");
		showImage(img);
	}
}

void SyntroLCam::showImage(QImage img)
{
	if (img.isNull())
		return;

	QImage scaledImg = img.scaled(m_cameraView->size(), Qt::KeepAspectRatio);

	if (!scaledImg.isNull())
		m_cameraView->setPixmap(QPixmap::fromImage(scaledImg));
}

void SyntroLCam::clearQueue()
{
	m_frameQMutex.lock();
	m_jpegFrameQ.clear();
	m_frameQMutex.unlock();
}

void SyntroLCam::layoutStatusBar()
{
	m_controlStatus = new QLabel(this);
	m_controlStatus->setAlignment(Qt::AlignLeft);
	ui.statusBar->addWidget(m_controlStatus, 1);

	m_audioStatus = new QLabel(this);
	m_audioStatus->setAlignment(Qt::AlignLeft);
	ui.statusBar->addWidget(m_audioStatus);

	m_frameRateStatus = new QLabel(this);
	m_frameRateStatus->setAlignment(Qt::AlignCenter | Qt::AlignLeft);
	ui.statusBar->addWidget(m_frameRateStatus);
}

void SyntroLCam::saveWindowState()
{
    QSettings *settings = SyntroUtils::getSettings();

    settings->beginGroup("Window");
    settings->setValue("Geometry", saveGeometry());
    settings->setValue("State", saveState());
    settings->endGroup();

    delete settings;
}

void SyntroLCam::restoreWindowState()
{
    QSettings *settings = SyntroUtils::getSettings();

    settings->beginGroup("Window");
    restoreGeometry(settings->value("Geometry").toByteArray());
    restoreState(settings->value("State").toByteArray());
    settings->endGroup();

    delete settings;
}

void SyntroLCam::onAbout()
{
    SyntroAbout dlg(this);
    dlg.exec();
}

void SyntroLCam::onBasicSetup()
{
    BasicSetupDlg dlg(this);
    dlg.exec();
}

void SyntroLCam::onConfigureStreams()
{
    StreamsDlg dlg(this);

    if (dlg.exec() == QDialog::Accepted)
        emit newStream();
}

void SyntroLCam::onConfigureMotion()
{
    MotionDlg dlg(this);

    if (dlg.exec() == QDialog::Accepted)
        emit newStream();
}

void SyntroLCam::onConfigureCamera()
{
    CameraDlg dlg(this);

    if (dlg.exec() == QDialog::Accepted)
        emit newCamera();
}

void SyntroLCam::onConfigureAudio()
{
    AudioDlg dlg(this);

    if (dlg.exec() == QDialog::Accepted)
        emit newAudioSrc();
}
