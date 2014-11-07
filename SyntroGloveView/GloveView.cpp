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

#include "GloveView.h"

#include <qevent.h>
#include <qdir.h>
#include <qdebug.h>
#include <qapplication.h>

GloveView::GloveView(QWidget *parent)
    : QGLWidget(QGLFormat(QGL::SampleBuffers), parent)
{
    globalGLWidget = this;

    QGLFormat fmt = context()->format();

    m_handWidget = NULL;

    globalTransforms.viewportFieldOfView = 45.0;
    globalTransforms.tanViewportFOV = tan(globalTransforms.viewportFieldOfView * QTGL_DEGREE_TO_RAD / 2.0);

    setAutoFillBackground(false);
}

void GloveView::closeEvent(QCloseEvent *)
{
    if (m_handWidget) {
        delete m_handWidget;
        m_handWidget = NULL;
    }

    for (int index = 0; index < QTGLSHADER_COUNT; index++)
        delete globalShader[index];
}

void GloveView::setPose(const RTQuaternion& palmQuat, const RTQuaternion& thumbQuat, const RTQuaternion& fingerQuat)
{

    int finger;
    RTQuaternion adjustedPalm = palmQuat;
    RTQuaternion adjustedThumb = thumbQuat;
    RTQuaternion adjustedFinger = fingerQuat;
    RTVector3 palmPose;
    RTVector3 thumbPose;
    RTVector3 fingerPose;

    adjustFingerAngles(adjustedPalm, adjustedThumb, adjustedFinger, palmPose, thumbPose, fingerPose);

    m_handWidget->setFingerAngles(0,
        (thumbPose.y()) * QTGL_RAD_TO_DEGREE,
        (thumbPose.z()) * -QTGL_RAD_TO_DEGREE,
        (thumbPose.x()) * QTGL_RAD_TO_DEGREE * 0);

    for (finger = 1; finger < VRHANDWIDGET_FINGERS; finger++)
            m_handWidget->setFingerAngles(finger,
                    (fingerPose.y()) * QTGL_RAD_TO_DEGREE,
                    (fingerPose.z()) * QTGL_RAD_TO_DEGREE,
                    (fingerPose.x()) * QTGL_RAD_TO_DEGREE * 0);

    m_handWidget->setRotation(
            QTGL_RAD_TO_DEGREE * palmPose.y(),
            -QTGL_RAD_TO_DEGREE * palmPose.z() + 90,
            -QTGL_RAD_TO_DEGREE * palmPose.x());
    updateGL();
}

void GloveView::adjustFingerAngles(RTQuaternion& palmQuat, RTQuaternion& thumbQuat, RTQuaternion& fingerQuat,
                        RTVector3& palmPose, RTVector3& thumbPose, RTVector3& fingerPose)

{
    RTQuaternion palmQuatConjugate = palmQuat.conjugate();

    palmQuat.toEuler(palmPose);

     //  adjust thumb pose relative to palm

    thumbQuat = palmQuatConjugate * thumbQuat;
    thumbQuat.toEuler(thumbPose);

    //  adjust finger pose relative to palm

    fingerQuat = palmQuatConjugate * fingerQuat;
    fingerQuat.toEuler(fingerPose);
}

void GloveView::initializeGL()
{

    initializeGLFunctions();
    QtGLInit(this);

    QtGLAddLight(QVector4D(20.0f, 5.0f, 0.0f, 1.0f), QVector3D(0.2f, 0.2f, 0.2f),
                    QVector3D(0.2f, 0.2f, 0.2f), QVector3D(0.8f, 0.0f, 0.0f));

    QtGLAddLight(QVector4D(-20.0f, 5.0f, 0.0f, 1.0f), QVector3D(0.0f, 0.0f, 0.0f),
                    QVector3D(0.2f, 0.2f, 0.2f), QVector3D(0.0f, 0.0f, 0.8f));

    QtGLAddLight(QVector4D(0.0f, 0.0f, 0.0f, 1.0f), QVector3D(0.0f, 0.0f, 0.0f),
                    QVector3D(0.2f, 0.2f, 0.2f), QVector3D(0.5f, 0.5f, 0.5f));

    m_handWidget = new VRHandWidget(globalGLWidget);

    if (m_handWidget) {
        m_handWidget->setRotation(QVector3D());
        m_handWidget->VRWidgetInit();
    }
}

void GloveView::paintGL()
{
    QMutexLocker lock(&m_lock);

    qglClearColor(QColor(20, 40, 80));
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    m_handWidget->VRWidgetRender();
}

void GloveView::resizeGL(int width, int height)
{
    setupViewport(width, height);
}

void GloveView::setupViewport(int width, int height)
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

    m_handWidget->setCenter(0, 0, GLOVEVIEW_HAND_DEPTH);
}

