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

#ifndef IMUVIEW_H
#define IMUVIEW_H

#include <QGLWidget>
#include <QtOpenGL>
#include <QtGL.h>

#include "VRIMUWidget.h"
#include "RTMath.h"

#define IMUVIEW_DEPTH                   -15						// normal IMU position
#define	IMUVIEW_RESTART_INTERVAL		1000					// length of time in restart interval

class VRIMUWidget;

class IMUView : public QGLWidget, protected QGLFunctions
{
	Q_OBJECT

public:
    IMUView(QWidget *parent = NULL);
    void updateIMU(RTVector3& pose);

protected:
	void initializeGL();
	void resizeGL(int width, int height);
	void paintGL();
	void closeEvent(QCloseEvent *);

private:
	void setupViewport(int width, int height);

    VRIMUWidget *m_IMUWidget;

    QMutex m_lock;
};

#endif // IMUVIEW_H
