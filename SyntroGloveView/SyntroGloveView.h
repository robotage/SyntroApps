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

#ifndef SYNTROGLOVEVIEW_H
#define SYNTROGLOVEVIEW_H

#include <QMainWindow>
#include <QLabel>
#include <QMutex>
#include <QCheckBox>
#include <QComboBox>

#include "ui_SyntroGloveView.h"

#include "RTIMULib.h"

#define PRODUCT_TYPE "SyntroGloveView"

//  Settings keys

#define SYNTROGLOVEVIEW_LAST_SOURCE               "lastSource"

class GloveView;
class SyntroServer;
class ViewClient;
class RTIMUNull;
class RTIMUSettings;

class SyntroGloveView : public QMainWindow
{
    Q_OBJECT

public:
    SyntroGloveView(QWidget *parent = 0);

public slots:
    void newIMUData(const RTQuaternion& palmQuat, const RTQuaternion& thumbQuat, const RTQuaternion& fingerQuat);
    void onAbout();
    void onBasicSetup();
    void onSelectSource();
    void dirResponse(QStringList directory);

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

    Ui::SyntroGloveViewClass ui;

    SyntroServer *m_controlServer;
    ViewClient *m_client;
    QStringList m_directory;

    int m_refreshTimer;
    int m_statusTimer;
    int m_directoryTimer;

    GloveView *m_view;

    QLabel *m_controlStatus;
    QLabel *m_rateStatus;

    RTQuaternion m_palmQuat;
    RTQuaternion m_thumbQuat;
    RTQuaternion m_fingerQuat;

    bool m_newIMUData;

    RTIMUNull *m_imu;
    RTIMUSettings *m_imuSettings;
    int m_sampleCount;

    QMutex m_lock;
};

#endif // SyntroNavView_H
