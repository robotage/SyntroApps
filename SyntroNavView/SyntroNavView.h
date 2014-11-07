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

#ifndef SYNTRONAVVIEW_H
#define SYNTRONAVVIEW_H

#include <QMainWindow>
#include <QLabel>
#include <QMutex>
#include <QCheckBox>
#include <QComboBox>

#include "ui_SyntroNavView.h"

#include "RTIMULib.h"

#define PRODUCT_TYPE "SyntroNavView"

//  Settings keys

#define SYNTRONAVVIEW_LAST_SOURCE               "lastSource"

class IMUView;
class SyntroServer;
class ViewClient;
class RTIMUNull;
class RTIMUSettings;

class SyntroNavView : public QMainWindow
{
	Q_OBJECT

public:
    SyntroNavView(QWidget *parent = 0);

public slots:
    void newIMUData(const RTIMU_DATA& data);
    void onAbout();
    void onBasicSetup();
    void onSelectSource();
    void onSelectFusionAlgorithm();
    void dirResponse(QStringList directory);
    void onEnableLocalFusion(int);
    void onEnableGyro(int);
    void onEnableAccel(int);
    void onEnableCompass(int);
    void onEnableDebug(int);

signals:
    void requestDir();
    void newSource(const QString source);

protected:
    void closeEvent(QCloseEvent *);
    void timerEvent(QTimerEvent *);

private:
    void startControlServer();
    void restoreWindowState();
    void saveWindowState();

    void initStatusBar();
    void layoutWindow();

    Ui::SyntroNavViewClass ui;

    SyntroServer *m_controlServer;
    ViewClient *m_client;
    QStringList m_directory;

    int m_refreshTimer;
    int m_statusTimer;
    int m_directoryTimer;

    IMUView *m_view;

    QComboBox *m_displaySelect;

    QLabel *m_fusionScalar;
    QLabel *m_fusionX;
    QLabel *m_fusionY;
    QLabel *m_fusionZ;

    QLabel *m_fusionPoseX;
    QLabel *m_fusionPoseY;
    QLabel *m_fusionPoseZ;

    QLabel *m_measuredPoseX;
    QLabel *m_measuredPoseY;
    QLabel *m_measuredPoseZ;

    QLabel *m_measuredQPoseScalar;
    QLabel *m_measuredQPoseX;
    QLabel *m_measuredQPoseY;
    QLabel *m_measuredQPoseZ;

    QLabel *m_gyroX;
    QLabel *m_gyroY;
    QLabel *m_gyroZ;

    QLabel *m_accelX;
    QLabel *m_accelY;
    QLabel *m_accelZ;

    QLabel *m_compassX;
    QLabel *m_compassY;
    QLabel *m_compassZ;

    QLabel *m_fusionType;
    QCheckBox *m_enableLocalFusion;
    QCheckBox *m_enableGyro;
    QCheckBox *m_enableAccel;
    QCheckBox *m_enableCompass;
    QCheckBox *m_enableDebug;

    QLabel *m_controlStatus;
    QLabel *m_rateStatus;

    RTVector3 m_measuredPose;
    RTQuaternion m_measuredQPose;

    RTIMU_DATA m_imuData;

    bool m_newIMUData;

    RTIMUNull *m_imu;
    RTIMUSettings *m_imuSettings;
    bool m_localFusion;
    int m_sampleCount;

    QMutex m_lock;

    QString m_logTag;

};

#endif // SyntroNavView_H
