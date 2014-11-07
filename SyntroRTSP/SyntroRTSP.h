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


#ifndef SPACESRTSP_H
#define SPACESRTSP_H

#include "ui_SyntroRTSP.h"
#include "Camera.h"
#include <qlabel.h>
#include <qmutex.h>
#include <qqueue.h>

//	Settings keys

#define	PRODUCT_TYPE	"SyntroRTSP"

class CameraClient;
class RTSPIF;

class SyntroRTSP : public QMainWindow
{
    Q_OBJECT

public:
    SyntroRTSP();
    ~SyntroRTSP();

public slots:
    void onAbout();
    void onBasicSetup();
    void onConfigureCamera();
    void onConfigureStreams();
    void onConfigureMotionDetection();
    void newImage(QImage frame);
    void netcamStatus(QString status);
    void clientRunning();

signals:
    void newStream();
    void newCamera();

protected:
    void timerEvent(QTimerEvent *event);
    void closeEvent(QCloseEvent *event);

private:
    void showImage(const QImage& frame);
    bool createCamera();
    void clearQueue();
    void saveWindowState();
    void restoreWindowState();

    Ui::SyntroRTSPClass ui;
    CameraClient *m_cameraClient;
    RTSPIF *m_camera;
    QMutex m_frameQMutex;
    QQueue <QImage> m_frameQ;
    int m_frameRateTimer;
    int m_frameRefreshTimer;
    QLabel *m_frameRateStatus;
    QLabel *m_controlStatus;
    QLabel *m_netcamStatus;
    int m_frameCount;
    bool m_scaling;
    QLabel *m_cameraView;

    QString m_logTag;
};

#endif // SYNTRORTSP_H
