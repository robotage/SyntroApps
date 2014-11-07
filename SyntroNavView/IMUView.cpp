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

#include "IMUView.h"

#include <qevent.h>
#include <qdir.h>
#include <qdebug.h>
#include <qapplication.h>

IMUView::IMUView(QWidget *parent)
	: QGLWidget(QGLFormat(QGL::SampleBuffers), parent)
{
	globalGLWidget = this;

	QGLFormat fmt = context()->format();

    m_IMUWidget = NULL;

	globalTransforms.viewportFieldOfView = 45.0;
	globalTransforms.tanViewportFOV = tan(globalTransforms.viewportFieldOfView * QTGL_DEGREE_TO_RAD / 2.0);

    setAutoFillBackground(false);
}

void IMUView::closeEvent(QCloseEvent *)
{
    if (m_IMUWidget) {
        delete m_IMUWidget;
        m_IMUWidget = NULL;
	}

	for (int index = 0; index < QTGLSHADER_COUNT; index++)
		delete globalShader[index];
}

void IMUView::updateIMU(RTVector3& pose)
{
    m_IMUWidget->setRotation(
            QTGL_RAD_TO_DEGREE * pose.x(),
            -QTGL_RAD_TO_DEGREE * pose.z(),
            QTGL_RAD_TO_DEGREE * pose.y());
	updateGL();
}

void IMUView::initializeGL()
{

	initializeGLFunctions();
	QtGLInit(this);
  
	QtGLAddLight(QVector4D(20.0f, 5.0f, 0.0f, 1.0f), QVector3D(0.2f, 0.2f, 0.2f),
                    QVector3D(0.2f, 0.2f, 0.2f), QVector3D(0.4f, 0.4f, 0.4f));

	QtGLAddLight(QVector4D(-20.0f, 5.0f, 0.0f, 1.0f), QVector3D(0.0f, 0.0f, 0.0f),
                    QVector3D(0.2f, 0.2f, 0.2f), QVector3D(0.8f, 0.8f, 0.8f));

	QtGLAddLight(QVector4D(0.0f, 0.0f, 0.0f, 1.0f), QVector3D(0.0f, 0.0f, 0.0f),
					QVector3D(0.2f, 0.2f, 0.2f), QVector3D(0.5f, 0.5f, 0.5f));

    m_IMUWidget = new VRIMUWidget(globalGLWidget);

    if (m_IMUWidget) {
        m_IMUWidget->setRotation(QVector3D());
        m_IMUWidget->VRWidgetInit();
	}
}

void IMUView::paintGL()
{
	QMutexLocker lock(&m_lock);

	qglClearColor(QColor(20, 40, 80));
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

    m_IMUWidget->VRWidgetRender();
}

void IMUView::resizeGL(int width, int height)
{
	setupViewport(width, height);
}

void IMUView::setupViewport(int width, int height)
{
	float w2 = width / 2;
	float tanFOV = globalTransforms.tanViewportFOV;

	globalTransforms.width = width;
	globalTransforms.height = height;
	globalTransforms.nearPlane = 1.0f;
	globalTransforms.farPlane = w2 / tanFOV;
	globalTransforms.halfWidth = width / 2.0f;
	globalTransforms.halfHeight = height / 2.0f;
	globalTransforms.viewportAspect = (float)width / (float)height;
    glViewport(0, 0, width, height);
	globalTransforms.projectionMatrix.setToIdentity();
	globalTransforms.modelViewMatrix.setToIdentity();

	globalTransforms.projectionMatrix.frustum(-tanFOV, +tanFOV, -tanFOV/globalTransforms.viewportAspect,
		tanFOV/globalTransforms.viewportAspect, globalTransforms.nearPlane, globalTransforms.farPlane);

    m_IMUWidget->setCenter(0, 0, IMUVIEW_DEPTH);
}

