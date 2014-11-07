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

#include "SyntroNavView.h"
#include "IMUView.h"
#include "ViewClient.h"
#include "StreamDlg.h"

#include "SyntroLib.h"
#include "SyntroServer.h"
#include "SyntroAboutDlg.h"
#include "BasicSetupDlg.h"
#include "SyntroNavDefs.h"
#include "SelectFusionDlg.h"

#define RATE_INTERVAL   2                                   // two seconds between sample rate calculations

//  Display type codes

#define DISPLAY_FUSION      0                               // displays fusion algorithm output
#define DISPLAY_MEASURED    1                               // measured from accels and compass
#define DISPLAY_ACCELONLY   2                               // just the accel data
#define DISPLAY_COMPASSONLY 3                               // just the compass data

SyntroNavView::SyntroNavView(QWidget *parent)
    : QMainWindow(parent)
{
    m_logTag = "SyntroNavView";
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
    connect(m_client, SIGNAL(newIMUData(const RTIMU_DATA&)),
            this, SLOT(newIMUData(const RTIMU_DATA&)), Qt::DirectConnection);

    connect(ui.actionExit, SIGNAL(triggered()), this, SLOT(close()));

    m_client->resumeThread();

    m_imuSettings = new RTIMUSettings("SyntroNavView");
    m_imuSettings->m_imuType = RTIMU_TYPE_NULL;
    m_imuSettings->saveSettings();
    m_imu = (RTIMUNull *)RTIMU::createIMU(m_imuSettings);
    m_localFusion = false;

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

    QString source = settings->value(SYNTRONAVVIEW_LAST_SOURCE).toString();
    if (source != "")
        emit newSource(source);

    delete settings;
}

void SyntroNavView::startControlServer()
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

void SyntroNavView::onAbout()
{
    SyntroAbout dlg(this);
    dlg.exec();
}

void SyntroNavView::onBasicSetup()
{
    BasicSetupDlg dlg(this);
    dlg.exec();
}

void SyntroNavView::closeEvent(QCloseEvent *)
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

void SyntroNavView::newIMUData(const RTIMU_DATA& data)
{
    QMutexLocker locker(&m_lock);

    m_sampleCount++;

    if (m_localFusion) {
        m_imu->setIMUData(data);
        m_imu->IMURead();
        m_imuData = m_imu->getIMUData();
        m_measuredPose = m_imu->getMeasuredPose();
        m_measuredQPose = m_imu->getMeasuredQPose();
    } else {
        m_imuData = data;
        m_measuredPose = RTMath::poseFromAccelMag(data.accel, data.compass);
        m_measuredQPose.fromEuler(m_measuredPose);
    }
    m_newIMUData = true;
}

void SyntroNavView::dirResponse(QStringList directory)
{
    m_directory = directory;

    if (m_directory.length() > 0) {
        if (!ui.actionSelectSource->isEnabled())
            ui.actionSelectSource->setEnabled(true);
    }
}

void SyntroNavView::onSelectSource()
{
    StreamDlg dlg(this, m_directory);

    if (dlg.exec() != QDialog::Accepted)
        return;

    emit newSource(dlg.getSelection());

    QSettings *settings = SyntroUtils::getSettings();
    settings->setValue(SYNTRONAVVIEW_LAST_SOURCE, dlg.getSelection());
    delete settings;

}

void SyntroNavView::onSelectFusionAlgorithm()
{
    SelectFusionDlg dlg(m_imuSettings, this);

    if (dlg.exec() == QDialog::Accepted) {
        delete m_imu;
        m_imu = (RTIMUNull *)RTIMU::createIMU(m_imuSettings);
        if (m_localFusion)
            m_imu->IMUInit();

        m_fusionType->setText(RTFusion::fusionName(m_imuSettings->m_fusionType));
    }
}

void SyntroNavView::onEnableLocalFusion(int state)
{
    if (state == Qt::Checked) {
        m_enableGyro->setEnabled(true);
        m_enableAccel->setEnabled(true);
        m_enableCompass->setEnabled(true);
        m_enableDebug->setEnabled(true);
        m_localFusion = true;
        m_imu->IMUInit();
        m_imu->resetFusion();
    } else {
        m_enableGyro->setEnabled(false);
        m_enableAccel->setEnabled(false);
        m_enableCompass->setEnabled(false);
        m_enableDebug->setEnabled(false);
        m_localFusion = false;
    }
}

void SyntroNavView::onEnableGyro(int state)
{
    m_imu->setGyroEnable(state == Qt::Checked);
}

void SyntroNavView::onEnableAccel(int state)
{
    m_imu->setAccelEnable(state == Qt::Checked);
}

void SyntroNavView::onEnableCompass(int state)
{
    m_imu->setCompassEnable(state == Qt::Checked);
}

void SyntroNavView::onEnableDebug(int state)
{
    m_imu->setDebugEnable(state == Qt::Checked);
}

void SyntroNavView::timerEvent(QTimerEvent *event)
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

        m_gyroX->setText(QString::number(m_imuData.gyro.x(), 'f', 6));
        m_gyroY->setText(QString::number(m_imuData.gyro.y(), 'f', 6));
        m_gyroZ->setText(QString::number(m_imuData.gyro.z(), 'f', 6));
        m_accelX->setText(QString::number(m_imuData.accel.x(), 'f', 6));
        m_accelY->setText(QString::number(m_imuData.accel.y(), 'f', 6));
        m_accelZ->setText(QString::number(m_imuData.accel.z(), 'f', 6));
        m_compassX->setText(QString::number(m_imuData.compass.x(), 'f', 6));
        m_compassY->setText(QString::number(m_imuData.compass.y(), 'f', 6));
        m_compassZ->setText(QString::number(m_imuData.compass.z(), 'f', 6));

        m_measuredPoseX->setText(QString::number(m_measuredPose.x() * RTMATH_RAD_TO_DEGREE, 'f', 6));
        m_measuredPoseY->setText(QString::number(m_measuredPose.y() * RTMATH_RAD_TO_DEGREE, 'f', 6));
        m_measuredPoseZ->setText(QString::number(m_measuredPose.z() * RTMATH_RAD_TO_DEGREE, 'f', 6));

        m_measuredQPoseScalar->setText(QString::number(m_measuredQPose.scalar(), 'f', 6));
        m_measuredQPoseX->setText(QString::number(m_measuredQPose.x(), 'f', 6));
        m_measuredQPoseY->setText(QString::number(m_measuredQPose.y(), 'f', 6));
        m_measuredQPoseZ->setText(QString::number(m_measuredQPose.z(), 'f', 6));

        m_fusionPoseX->setText(QString::number(m_imuData.fusionPose.x() * RTMATH_RAD_TO_DEGREE, 'f', 6));
        m_fusionPoseY->setText(QString::number(m_imuData.fusionPose.y() * RTMATH_RAD_TO_DEGREE, 'f', 6));
        m_fusionPoseZ->setText(QString::number(m_imuData.fusionPose.z() * RTMATH_RAD_TO_DEGREE, 'f', 6));

        m_fusionScalar->setText(QString::number(m_imuData.fusionQPose.scalar(), 'f', 6));
        m_fusionX->setText(QString::number(m_imuData.fusionQPose.x(), 'f', 6));
        m_fusionY->setText(QString::number(m_imuData.fusionQPose.y(), 'f', 6));
        m_fusionZ->setText(QString::number(m_imuData.fusionQPose.z(), 'f', 6));

        int index;
        RTVector3 vec;

        if ((index = m_displaySelect->currentIndex()) == -1) {
            m_view->updateIMU(m_imuData.fusionPose);
        } else {
            switch (m_displaySelect->itemData(index).toInt()) {
            case DISPLAY_MEASURED:
                m_view->updateIMU(m_measuredPose);
                break;

            case DISPLAY_ACCELONLY:
                m_imuData.accel.accelToEuler(vec);
                m_view->updateIMU(vec);
                break;

            case DISPLAY_COMPASSONLY:
                vec = RTMath::poseFromAccelMag(m_imuData.accel, m_imuData.compass);
                vec.setX(0);
                vec.setY(0);
                m_view->updateIMU(vec);
                break;

            default:
                m_view->updateIMU(m_imuData.fusionPose);
                break;
            }
        }
    }
}


void SyntroNavView::initStatusBar()
{
    m_controlStatus = new QLabel(this);
    m_controlStatus->setAlignment(Qt::AlignLeft);
    ui.statusBar->addWidget(m_controlStatus, 1);

    m_rateStatus = new QLabel(this);
    m_rateStatus->setAlignment(Qt::AlignCenter | Qt::AlignLeft);
    ui.statusBar->addWidget(m_rateStatus);
}

void SyntroNavView::layoutWindow()
{
    QHBoxLayout *mainLayout = new QHBoxLayout();
    mainLayout->setContentsMargins(3, 3, 3, 3);
    mainLayout->setSpacing(3);

    QVBoxLayout *vLayout = new QVBoxLayout();

    vLayout->addWidget(new QLabel("Fusion state (quaternion): "));

    QHBoxLayout *dataLayout = new QHBoxLayout();
    dataLayout->addSpacing(30);
    m_fusionScalar = new QLabel("1");
    m_fusionScalar->setFrameStyle(QFrame::Panel);
    m_fusionX = new QLabel("0");
    m_fusionX->setFrameStyle(QFrame::Panel);
    m_fusionY = new QLabel("0");
    m_fusionY->setFrameStyle(QFrame::Panel);
    m_fusionZ = new QLabel("0");
    m_fusionZ->setFrameStyle(QFrame::Panel);
    dataLayout->addWidget(m_fusionScalar);
    dataLayout->addWidget(m_fusionX);
    dataLayout->addWidget(m_fusionY);
    dataLayout->addWidget(m_fusionZ);
    vLayout->addLayout(dataLayout);

    vLayout->addWidget(new QLabel("Measured pose (quaternion): "));

    dataLayout = new QHBoxLayout();
    dataLayout->addSpacing(30);
    m_measuredQPoseScalar = new QLabel("1");
    m_measuredQPoseScalar->setFrameStyle(QFrame::Panel);
    m_measuredQPoseX = new QLabel("0");
    m_measuredQPoseX->setFrameStyle(QFrame::Panel);
    m_measuredQPoseY = new QLabel("0");
    m_measuredQPoseY->setFrameStyle(QFrame::Panel);
    m_measuredQPoseZ = new QLabel("0");
    m_measuredQPoseZ->setFrameStyle(QFrame::Panel);
    dataLayout->addWidget(m_measuredQPoseScalar);
    dataLayout->addWidget(m_measuredQPoseX);
    dataLayout->addWidget(m_measuredQPoseY);
    dataLayout->addWidget(m_measuredQPoseZ);
    vLayout->addLayout(dataLayout);

    vLayout->addWidget(new QLabel("Fusion pose (degrees): "));

    m_fusionPoseX = new QLabel("0");
    m_fusionPoseX->setFrameStyle(QFrame::Panel);
    m_fusionPoseY = new QLabel("0");
    m_fusionPoseY->setFrameStyle(QFrame::Panel);
    m_fusionPoseZ = new QLabel("0");
    m_fusionPoseZ->setFrameStyle(QFrame::Panel);
    dataLayout = new QHBoxLayout();
    dataLayout->addSpacing(30);
    dataLayout->addWidget(m_fusionPoseX);
    dataLayout->addWidget(m_fusionPoseY);
    dataLayout->addWidget(m_fusionPoseZ);
    vLayout->addLayout(dataLayout);

    vLayout->addWidget(new QLabel("Measured pose (degrees): "));

    m_measuredPoseX = new QLabel("0");
    m_measuredPoseX->setFrameStyle(QFrame::Panel);
    m_measuredPoseY = new QLabel("0");
    m_measuredPoseY->setFrameStyle(QFrame::Panel);
    m_measuredPoseZ = new QLabel("0");
    m_measuredPoseZ->setFrameStyle(QFrame::Panel);
    dataLayout = new QHBoxLayout();
    dataLayout->addSpacing(30);
    dataLayout->addWidget(m_measuredPoseX);
    dataLayout->addWidget(m_measuredPoseY);
    dataLayout->addWidget(m_measuredPoseZ);
    vLayout->addLayout(dataLayout);

    vLayout->addWidget(new QLabel("Gyros (radians/s): "));

    m_gyroX = new QLabel("0");
    m_gyroX->setFrameStyle(QFrame::Panel);
    m_gyroY = new QLabel("0");
    m_gyroY->setFrameStyle(QFrame::Panel);
    m_gyroZ = new QLabel("0");
    m_gyroZ->setFrameStyle(QFrame::Panel);
    dataLayout = new QHBoxLayout();
    dataLayout->addSpacing(30);
    dataLayout->addWidget(m_gyroX);
    dataLayout->addWidget(m_gyroY);
    dataLayout->addWidget(m_gyroZ);
    vLayout->addLayout(dataLayout);

    vLayout->addWidget(new QLabel("Accelerometers (g): "));

    m_accelX = new QLabel("0");
    m_accelX->setFrameStyle(QFrame::Panel);
    m_accelY = new QLabel("0");
    m_accelY->setFrameStyle(QFrame::Panel);
    m_accelZ = new QLabel("0");
    m_accelZ->setFrameStyle(QFrame::Panel);
    dataLayout = new QHBoxLayout();
    dataLayout->addSpacing(30);
    dataLayout->addWidget(m_accelX);
    dataLayout->addWidget(m_accelY);
    dataLayout->addWidget(m_accelZ);
    vLayout->addLayout(dataLayout);

    vLayout->addWidget(new QLabel("Magnetometers (uT): "));

    m_compassX = new QLabel("0");
    m_compassX->setFrameStyle(QFrame::Panel);
    m_compassY = new QLabel("0");
    m_compassY->setFrameStyle(QFrame::Panel);
    m_compassZ = new QLabel("0");
    m_compassZ->setFrameStyle(QFrame::Panel);
    dataLayout = new QHBoxLayout();
    dataLayout->addSpacing(30);
    dataLayout->addWidget(m_compassX);
    dataLayout->addWidget(m_compassY);
    dataLayout->addWidget(m_compassZ);
    vLayout->addLayout(dataLayout);

    vLayout->addSpacing(10);

    QHBoxLayout *fusionBox = new QHBoxLayout();
    QLabel *fusionTypeLabel = new QLabel("Fusion algorithm: ");
    fusionBox->addWidget(fusionTypeLabel);
    fusionTypeLabel->setMaximumWidth(150);
    m_fusionType = new QLabel();
    fusionBox->addWidget(m_fusionType);
    vLayout->addLayout(fusionBox);
    m_fusionType->setText(RTFusion::fusionName(m_imuSettings->m_fusionType));

    vLayout->addSpacing(10);

    vLayout->addWidget(new QLabel("Local fusion controls: "));

    m_enableLocalFusion = new QCheckBox("Enable local fusion");
    m_enableLocalFusion->setChecked(false);
    vLayout->addWidget(m_enableLocalFusion);

    m_enableGyro = new QCheckBox("Enable gyros");
    m_enableGyro->setChecked(true);
    m_enableGyro->setEnabled(false);
    vLayout->addWidget(m_enableGyro);

    m_enableAccel = new QCheckBox("Enable accels");
    m_enableAccel->setEnabled(false);
    m_enableAccel->setChecked(true);
    vLayout->addWidget(m_enableAccel);

    m_enableCompass = new QCheckBox("Enable compass");
    m_enableCompass->setEnabled(false);
    m_enableCompass->setChecked(true);
    vLayout->addWidget(m_enableCompass);

    m_enableDebug = new QCheckBox("Enable debug messages");
    m_enableDebug->setEnabled(false);
    m_enableDebug->setChecked(false);
    vLayout->addWidget(m_enableDebug);

    vLayout->addStretch(1);
    mainLayout->addLayout(vLayout);

    vLayout = new QVBoxLayout();
    vLayout->setContentsMargins(3, 3, 3, 3);
    vLayout->setSpacing(3);

    QHBoxLayout *displayLayout = new QHBoxLayout();
    QLabel *displayLabel = new QLabel("Display type:  ");
    displayLayout->addWidget(displayLabel);
    displayLayout->setAlignment(displayLabel, Qt::AlignRight);

    m_displaySelect = new QComboBox();
    m_displaySelect->addItem("Fusion pose", DISPLAY_FUSION);
    m_displaySelect->addItem("Measured pose", DISPLAY_MEASURED);
    m_displaySelect->addItem("Accels only", DISPLAY_ACCELONLY);
    m_displaySelect->addItem("Compass only", DISPLAY_COMPASSONLY);

    displayLayout->addWidget(m_displaySelect);
    vLayout->addLayout(displayLayout);

    m_view = new IMUView(this);
    vLayout->addWidget(m_view);

    mainLayout->addLayout(vLayout);

    centralWidget()->setLayout(mainLayout);

    setMinimumWidth(800);
    setMinimumHeight(500);

    connect(m_enableLocalFusion, SIGNAL(stateChanged(int)), this, SLOT(onEnableLocalFusion(int)));
    connect(m_enableGyro, SIGNAL(stateChanged(int)), this, SLOT(onEnableGyro(int)));
    connect(m_enableAccel, SIGNAL(stateChanged(int)), this, SLOT(onEnableAccel(int)));
    connect(m_enableCompass, SIGNAL(stateChanged(int)), this, SLOT(onEnableCompass(int)));
    connect(m_enableDebug, SIGNAL(stateChanged(int)), this, SLOT(onEnableDebug(int)));

}

void SyntroNavView::saveWindowState()
{
    QSettings *settings = SyntroUtils::getSettings();

    settings->beginGroup("Window");
    settings->setValue("Geometry", saveGeometry());
    settings->setValue("State", saveState());
    settings->endGroup();

    delete settings;
}

void SyntroNavView::restoreWindowState()
{
    QSettings *settings = SyntroUtils::getSettings();

    settings->beginGroup("Window");
    restoreGeometry(settings->value("Geometry").toByteArray());
    restoreState(settings->value("State").toByteArray());
    settings->endGroup();

    delete settings;
}
