//
//  Copyright (c) 2014 richards-tech.
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

#include "SyntroGloveView.h"
#include "GloveView.h"
#include "ViewClient.h"
#include "StreamDlg.h"

#include "SyntroLib.h"
#include "SyntroServer.h"
#include "SyntroAboutDlg.h"
#include "BasicSetupDlg.h"

#define RATE_INTERVAL   2                                   // two seconds between sample rate calculations

SyntroGloveView::SyntroGloveView(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    SyntroUtils::syntroAppInit();
    startControlServer();

    m_client = new ViewClient();

    connect(m_client, SIGNAL(dirResponse(QStringList)), this, SLOT(dirResponse(QStringList)));
    connect(this, SIGNAL(requestDir()), m_client, SLOT(requestDir()));

    connect(ui.actionAbout, SIGNAL(triggered()), this, SLOT(onAbout()));
    connect(ui.actionBasicSetup, SIGNAL(triggered()), this, SLOT(onBasicSetup()));

    connect(ui.actionSelectSource, SIGNAL(triggered()), this, SLOT(onSelectSource()));
    connect(ui.actionSelectFusionAlgorithm, SIGNAL(triggered()), this, SLOT(onSelectFusionAlgorithm()));

    connect(this, SIGNAL(newSource(const QString)), m_client, SLOT(newSource(QString)));

    connect(m_client, SIGNAL(newIMUData(const RTQuaternion&, const RTQuaternion&, const RTQuaternion&)),
            this, SLOT(newIMUData(const RTQuaternion&, const RTQuaternion&, const RTQuaternion&)), Qt::DirectConnection);

    connect(ui.actionExit, SIGNAL(triggered()), this, SLOT(close()));

    m_client->resumeThread();

    m_imuSettings = new RTIMUSettings("SyntroGloveView");
    m_imuSettings->m_imuType = RTIMU_TYPE_NULL;
    m_imuSettings->saveSettings();
    m_imu = (RTIMUNull *)RTIMU::createIMU(m_imuSettings);

    initStatusBar();
    layoutWindow();
    restoreWindowState();
    setWindowTitle(QString("%1 - %2")
        .arg(SyntroUtils::getAppType())
        .arg(SyntroUtils::getAppName()));

    m_sampleCount = 0;

    m_statusTimer = startTimer(RATE_INTERVAL * 1000);
    m_refreshTimer = startTimer(20);
    m_directoryTimer = startTimer(10000);

    ui.actionSelectSource->setEnabled(false);
    m_newIMUData = false;

    QSettings *settings = SyntroUtils::getSettings();

    QString source = settings->value(SYNTROGLOVEVIEW_LAST_SOURCE).toString();
    if (source != "")
        emit newSource(source);

    delete settings;
}

void SyntroGloveView::startControlServer()
{
    QSettings *settings = SyntroUtils::getSettings();

    if (settings->value(SYNTRO_PARAMS_LOCALCONTROL).toBool()) {
        m_controlServer = new SyntroServer();
        m_controlServer->resumeThread();
    }
    else {
        m_controlServer = NULL;
    }

    delete settings;
}

void SyntroGloveView::onAbout()
{
    SyntroAbout dlg(this);
    dlg.exec();
}

void SyntroGloveView::onBasicSetup()
{
    BasicSetupDlg dlg(this);
    dlg.exec();
}

void SyntroGloveView::closeEvent(QCloseEvent *)
{
    killTimer(m_statusTimer);
    killTimer(m_directoryTimer);
    killTimer(m_refreshTimer);

    if (m_client) {
        m_client->exitThread();
        m_client = NULL;
    }

    if (m_controlServer) {
        m_controlServer->exitThread();
        m_controlServer = NULL;
    }

    delete m_imuSettings;

    saveWindowState();

    SyntroUtils::syntroAppExit();
}

void SyntroGloveView::newIMUData(const RTQuaternion& palmQuat, const RTQuaternion& thumbQuat, const RTQuaternion& fingerQuat)
{
    QMutexLocker locker(&m_lock);

    m_sampleCount++;

    m_palmQuat = palmQuat;
    m_thumbQuat = thumbQuat;
    m_fingerQuat = fingerQuat;
    m_newIMUData = true;
}

void SyntroGloveView::dirResponse(QStringList directory)
{
    m_directory = directory;

    if (m_directory.length() > 0) {
        if (!ui.actionSelectSource->isEnabled())
            ui.actionSelectSource->setEnabled(true);
    }
}

void SyntroGloveView::onSelectSource()
{
    StreamDlg dlg(this, m_directory);

    if (dlg.exec() != QDialog::Accepted)
        return;

    emit newSource(dlg.getSelection());

    QSettings *settings = SyntroUtils::getSettings();
    settings->setValue(SYNTROGLOVEVIEW_LAST_SOURCE, dlg.getSelection());
    delete settings;

}

void SyntroGloveView::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_directoryTimer) {
        emit requestDir();
    } else if (event->timerId() == m_statusTimer) {
        m_controlStatus->setText(m_client->getLinkState());

        qreal rate = (qreal)m_sampleCount / (qreal)RATE_INTERVAL;
        m_sampleCount = 0;
        m_rateStatus->setText(QString("Sample rate: %1 per second").arg(rate));

    } else {
        QMutexLocker locker(&m_lock);

        if (!m_newIMUData)
            return;

        m_view->setPose(m_palmQuat, m_thumbQuat, m_fingerQuat);
        m_newIMUData = false;
    }
}


void SyntroGloveView::initStatusBar()
{
    m_controlStatus = new QLabel(this);
    m_controlStatus->setAlignment(Qt::AlignLeft);
    ui.statusBar->addWidget(m_controlStatus, 1);

    m_rateStatus = new QLabel(this);
    m_rateStatus->setAlignment(Qt::AlignCenter | Qt::AlignLeft);
    ui.statusBar->addWidget(m_rateStatus);
}

void SyntroGloveView::layoutWindow()
{
    QHBoxLayout *mainLayout = new QHBoxLayout();
    mainLayout->setContentsMargins(3, 3, 3, 3);
    mainLayout->setSpacing(3);

    QVBoxLayout *vLayout = new QVBoxLayout();

    m_view = new GloveView(this);
    vLayout->addWidget(m_view);

    mainLayout->addLayout(vLayout);

    centralWidget()->setLayout(mainLayout);

    setMinimumWidth(400);
    setMinimumHeight(400);
}

void SyntroGloveView::saveWindowState()
{
    QSettings *settings = SyntroUtils::getSettings();

    settings->beginGroup("Window");
    settings->setValue("Geometry", saveGeometry());
    settings->setValue("State", saveState());
    settings->endGroup();

    delete settings;
}

void SyntroGloveView::restoreWindowState()
{
    QSettings *settings = SyntroUtils::getSettings();

    settings->beginGroup("Window");
    restoreGeometry(settings->value("Geometry").toByteArray());
    restoreState(settings->value("State").toByteArray());
    settings->endGroup();

    delete settings;
}
