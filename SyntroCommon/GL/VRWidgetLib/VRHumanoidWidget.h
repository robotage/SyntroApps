//
//  Copyright (c) 2014 richards-tech
//
//  This file is part of VRWidgetLib
//
//  VRWidgetLib is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  VRWidgetLib is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with VRWidgetLib.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef VRHUMANOIDWIDGET_H
#define VRHUMANOIDWIDGET_H

#include "VRWidget.h"

class VRHumanoidWidget : public VRWidget
{
    Q_OBJECT

public:
    VRHumanoidWidget(QObject *parent, VRWIDGET_TYPE = VRWIDGET_HUMANOID);
    ~VRHumanoidWidget();

    void setHeadTexture(const QString& path);
    void setHeadTopTexture(const QString& path);
    void setHeadBottomTexture(const QString& path);
    void setNeckTexture(const QString& path);
    void setHandTexture(const QString& path);

    void setTorsoPose(qreal x, qreal y, qreal z);
    void setArmPose(int arm, qreal x, qreal y, qreal z);
    void setElbowPose(int arm, qreal x, qreal y, qreal z);
    void setHeadPose(qreal x, qreal y, qreal z);

    virtual void VRWidgetInit();
    virtual void VRWidgetRender();

private:
    void buildArm(int arm, qreal x, qreal y, qreal z);

    QtGLCylinderComponent m_leg;
    QtGLDiskComponent m_legBottom;

    QtGLCylinderComponent m_torso;
    QtGLDiskComponent m_torsoTop;
    QtGLCylinderComponent m_collar;
    QtGLCylinderComponent m_neck;
    QtGLCylinderComponent m_head;
    QtGLDiskComponent m_headTop;
    QtGLDiskComponent m_headBottom;

    QtGLCylinderComponent m_shoulder;
    QtGLSphereComponent m_joint;
    QtGLCylinderComponent m_armSeg;
    QtGLSphereComponent m_hand;

    qreal m_legRadius;
    qreal m_legHeight;
    qreal m_legYOffset;
    qreal m_legXOffset;

    qreal m_torsoRadius;
    qreal m_torsoHeight;

    qreal m_collarRadius;
    qreal m_collarHeight;

    qreal m_neckRadius;
    qreal m_neckHeight;

    qreal m_headRadius;
    qreal m_headHeight;

    qreal m_shoulderRadius;
    qreal m_shoulderLength;

    qreal m_armSegRadius;
    qreal m_armSegLength;

    qreal m_jointRadius;
    qreal m_handRadius;

    qreal m_armAngles[2][3];
    qreal m_elbowAngles[2][3];
    qreal m_torsoAngles[3];
    qreal m_headAngles[3];
};

#endif // VRHUMANOIDWIDGET_H
