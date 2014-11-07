////////////////////////////////////////////////////////////
//
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

#ifndef GLOVEVIEW_H
#define GLOVEVIEW_H

#include <QGLWidget>
#include <QtOpenGL>
#include <QtGL.h>

#include "RTIMULib.h"
#include "VRHandWidget.h"

#define GLOVEVIEW_HAND_DEPTH			-15						// normal hand position
#define	GLOVEVIEW_RESTART_INTERVAL		1000					// length of time in restart interval

class VRHandWidget;

class GloveView : public QGLWidget, protected QGLFunctions
{
    Q_OBJECT

public:
    GloveView(QWidget *parent = NULL);

    void setPose(const RTQuaternion& palmQuat, const RTQuaternion& thumbQuat, const RTQuaternion& fingerQuat);

protected:
    void initializeGL();
    void resizeGL(int width, int height);
    void paintGL();
    void closeEvent(QCloseEvent *);

private:
    void adjustFingerAngles(RTQuaternion& palmQuat, RTQuaternion& thumbQuat, RTQuaternion& fingerQuat,
                            RTVector3& palmPose, RTVector3& thumbPose, RTVector3& fingerPose);
    void setupViewport(int width, int height);

    QMutex m_lock;
    VRHandWidget *m_handWidget;
};

#endif // GLOVEVIEW_H
