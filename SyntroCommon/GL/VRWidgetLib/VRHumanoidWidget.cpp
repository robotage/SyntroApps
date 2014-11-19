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

#include "VRHumanoidWidget.h"

#define FLESH_COLOR     QVector3D(0.75 / 1.3, 0.515 / 1.3, 0.4 / 1.3)

#define TORSO_COLOR_AMBIENT     QVector3D(0.1, 0.1, 0.1)
#define TORSO_COLOR_DIFFUSE     QVector3D(0.1, 0.1, 0.1)
#define TORSO_COLOR_SPECULAR    QVector3D(1.0, 1.0, 1.0)

#define LEG_COLOR_AMBIENT       QVector3D(0.1, 0.1, 0.7)
#define LEG_COLOR_DIFFUSE       QVector3D(0.1, 0.1, 0.7)
#define LEG_COLOR_SPECULAR      QVector3D(1.0, 1.0, 1.0)

VRHumanoidWidget::VRHumanoidWidget(QObject *parent, VRWIDGET_TYPE widgetType)
    : VRWidget(parent, widgetType)
{
}

VRHumanoidWidget::~VRHumanoidWidget()
{

}

void VRHumanoidWidget::VRWidgetInit()
{
    m_legRadius = m_width / 20;
    m_legHeight = m_legRadius * 11.5;
    m_legYOffset = 0;
    m_legXOffset = m_legRadius * 1.2;

    m_torsoRadius = m_legRadius * 3;
    m_torsoHeight = m_legRadius * 8;

    m_neckRadius = m_legRadius * 1.2;
    m_neckHeight = m_legRadius * 2;

    m_headRadius = m_legRadius * 2;
    m_headHeight = m_headRadius * 2;

    m_shoulderRadius = m_legRadius;
    m_shoulderLength = m_torsoRadius * 2 + m_legRadius * 2;

    m_jointRadius = m_shoulderRadius * 1.0;

    m_armSegLength = m_torsoHeight / 1.8;
    m_armSegRadius = m_shoulderRadius * 0.7;

    m_handRadius = m_armSegRadius;

    m_leg.generate(m_legRadius, m_legRadius, m_legHeight * 1.1, 50, 1);
    m_leg.setShader(globalShader[QTGLSHADER_ADS]);
    m_leg.setMaterial(LEG_COLOR_AMBIENT, LEG_COLOR_DIFFUSE, LEG_COLOR_SPECULAR, 1.0);

    m_legBottom.generate(0, m_legRadius, 50, 1);
    m_legBottom.setShader(globalShader[QTGLSHADER_ADS]);
    m_legBottom.setMaterial(LEG_COLOR_AMBIENT, LEG_COLOR_DIFFUSE, LEG_COLOR_SPECULAR, 1.0);

    m_torso.generate(m_torsoRadius, m_torsoRadius, m_torsoHeight, 50, 1);
    m_torso.setShader(globalShader[QTGLSHADER_ADS]);
    m_torso.setMaterial(TORSO_COLOR_AMBIENT, TORSO_COLOR_DIFFUSE, TORSO_COLOR_SPECULAR, 3.0);

    m_torsoTop.generate(0, m_torsoRadius, 50, 1);
    m_torsoTop.setShader(globalShader[QTGLSHADER_ADS]);
    m_torsoTop.setMaterial(TORSO_COLOR_AMBIENT, TORSO_COLOR_DIFFUSE, TORSO_COLOR_SPECULAR, 3.0);

    m_neck.generate(m_neckRadius, m_neckRadius, m_neckHeight, 50, 1);
    m_neck.setShader(globalShader[QTGLSHADER_ADSTEXTURE]);
    m_neck.setMaterial(QVector3D(1.0, 1.0, 1.0), QVector3D(0.5, 0.5, 0.5),
                        QVector3D(1.0, 1.0, 1.0), 1.0);

    m_collar.generate(m_neckRadius * 1.1, m_neckRadius * 1.1, m_neckHeight * 0.4, 50, 1);
    m_collar.setShader(globalShader[QTGLSHADER_ADS]);
    m_collar.setMaterial(TORSO_COLOR_AMBIENT, TORSO_COLOR_DIFFUSE, TORSO_COLOR_SPECULAR, 1.0);

    m_head.generate(m_headRadius, m_headRadius, m_headHeight, 50, 1);
    m_head.setShader(globalShader[QTGLSHADER_ADSTEXTURE]);
    m_head.setMaterial(QVector3D(1.0, 1.0, 1.0), QVector3D(0.5, 0.5, 0.5),
                        QVector3D(1.0, 1.0, 1.0), 1.0);

    m_headTop.generate(0, m_headRadius, 50, 1);
    m_headTop.setShader(globalShader[QTGLSHADER_ADSTEXTURE]);
    m_headTop.setMaterial(QVector3D(1.0, 1.0, 1.0), QVector3D(0.5, 0.5, 0.5),
                        QVector3D(1.0, 1.0, 1.0), 1.0);

    m_headBottom.generate(0, m_headRadius, 50, 1);
    m_headBottom.setShader(globalShader[QTGLSHADER_ADSTEXTURE]);
    m_headBottom.setMaterial(QVector3D(1.0, 1.0, 1.0), QVector3D(0.5, 0.5, 0.5),
                        QVector3D(1.0, 1.0, 1.0), 1.0);

    m_shoulder.generate(m_shoulderRadius, m_shoulderRadius, m_shoulderLength, 50, 1);
    m_shoulder.setShader(globalShader[QTGLSHADER_ADS]);
    m_shoulder.setMaterial(TORSO_COLOR_AMBIENT, TORSO_COLOR_DIFFUSE, TORSO_COLOR_SPECULAR, 3.0);

    m_joint.generate(m_jointRadius, 50, 50);
    m_joint.setShader(globalShader[QTGLSHADER_ADS]);
    m_joint.setMaterial(TORSO_COLOR_AMBIENT, TORSO_COLOR_DIFFUSE, TORSO_COLOR_SPECULAR, 3.0);

    m_armSeg.generate(m_armSegRadius, m_armSegRadius, m_armSegLength, 50, 1);
    m_armSeg.setShader(globalShader[QTGLSHADER_ADS]);
    m_armSeg.setMaterial(TORSO_COLOR_AMBIENT, TORSO_COLOR_DIFFUSE, TORSO_COLOR_SPECULAR, 3.0);

    m_hand.generate(m_handRadius, 50, 50);
    m_hand.setShader(globalShader[QTGLSHADER_ADSTEXTURE]);
    m_hand.setMaterial(QVector3D(1.0, 1.0, 1.0), QVector3D(0.5, 0.5, 0.5),
                        QVector3D(1.0, 1.0, 1.0), 2.0);

    setRotationOrder(VRWidgetRotationZXY);					// so that roll/pitch/yaw works
}

void VRHumanoidWidget::VRWidgetRender()
{
    startWidgetRender();

    // do legs

    // do leg pose as torso y rotation

    startCompositeRender(0, m_legYOffset, 0, 0, m_torsoAngles[1], 0);

    // do right leg

    startComponentRender(-m_legXOffset, 0, 0, -90, 0, 0);
    m_leg.draw();
    endComponentRender(m_leg.getBoundMinus(), m_leg.getBoundPlus());

    // do right leg bottom

    startComponentRender(-m_legXOffset, 0, 0, 90, 0, 0);
    m_legBottom.draw();
    endComponentRender(m_legBottom.getBoundMinus(), m_legBottom.getBoundPlus());

    // do left leg bottom

    startComponentRender(m_legXOffset, 0, 0, -90, 0, 0);
    m_leg.draw();
    endComponentRender(m_leg.getBoundMinus(), m_leg.getBoundPlus());

    // do left leg bottom

    startComponentRender(m_legXOffset, 0, 0, 90, 0, 0);
    m_legBottom.draw();
    endComponentRender(m_legBottom.getBoundMinus(), m_legBottom.getBoundPlus());

    // do torso

    // set up pose

    startCompositeRender(0, m_legHeight, 0, m_torsoAngles[0], 0, m_torsoAngles[2]);

    // do torso cylinder

    startComponentRender(0, 0, 0, -90, 0, 0);
    m_torso.draw();
    endComponentRender(m_torso.getBoundMinus(), m_torso.getBoundPlus());

    // do torso top

    startComponentRender(0, m_torsoHeight, 0, -90, 0, 0);
    m_torsoTop.draw();
    endComponentRender(m_torsoTop.getBoundMinus(), m_torsoTop.getBoundPlus());

    // do torso bottom

    startComponentRender(0, 0, 0, 90, 0, 0);
    m_torsoTop.draw();
    endComponentRender(m_torsoTop.getBoundMinus(), m_torsoTop.getBoundPlus());

    // do shoulder

    startComponentRender(-m_shoulderLength / 2, m_torsoHeight - m_shoulderRadius * 1.5, 0, 0, 90, 0);
    m_shoulder.draw();
    endComponentRender(m_shoulder.getBoundMinus(), m_shoulder.getBoundPlus());

    buildArm(0, m_shoulderLength / 2, m_torsoHeight - m_shoulderRadius * 1.5, 0);
    buildArm(1, -m_shoulderLength / 2, m_torsoHeight - m_shoulderRadius * 1.5, 0);

    // do neck

    startComponentRender(0, m_torsoHeight, 0, -90, 0, 0);
    m_neck.draw();
    endComponentRender(m_neck.getBoundMinus(), m_neck.getBoundPlus());

    // do collar

    startComponentRender(0, m_torsoHeight, 0, -90, 0, 0);
    m_collar.draw();
    endComponentRender(m_collar.getBoundMinus(), m_collar.getBoundPlus());

    // do head

    // start head composite

    startCompositeRender(0, m_torsoHeight + m_neckHeight - m_neckHeight /3, 0,
                         m_headAngles[0], m_headAngles[1], m_headAngles[2]);

    startComponentRender(0, m_headHeight, 0, 90, 90, 0);
    m_head.draw();
    endComponentRender(m_head.getBoundMinus(), m_head.getBoundPlus());

    // do head top

    startComponentRender(0, m_headHeight, 0, -90, 0, 0);
    m_headTop.draw();
    endComponentRender(m_headTop.getBoundMinus(), m_headTop.getBoundPlus());

    // do head bottom

    startComponentRender(0, 0, 0, 90, 0, 0);
    m_headBottom.draw();
    endComponentRender(m_headBottom.getBoundMinus(), m_headBottom.getBoundPlus());

    // end head composite

    endCompositeRender();

    // end torso composite

    endCompositeRender();

    // end leg composite

    endCompositeRender();

    endWidgetRender();
}

void VRHumanoidWidget::buildArm(int arm, qreal x, qreal y, qreal z)
{
    // set pose of upper arm

    startCompositeRender(x, y, z,
                         m_armAngles[arm][0], m_armAngles[arm][1], m_armAngles[arm][2]);

    // do shoulder joint

    startComponentRender(0, 0, 0, 0, 0, 0);
    m_joint.draw();
    endComponentRender(m_joint.getBoundMinus(), m_joint.getBoundPlus());

    // do upper segment

    startComponentRender(0, 0, 0, 90, 0, 0);
    m_armSeg.draw();
    endComponentRender(m_armSeg.getBoundMinus(), m_armSeg.getBoundPlus());

    // do elbow

    // set pose of elbow

    startCompositeRender(0, -m_armSegLength, 0,
                         m_elbowAngles[arm][0], m_elbowAngles[arm][1], m_elbowAngles[arm][2]);

    // do elbow joint

    startComponentRender(0, 0, 0, 0, 0, 0);
    m_joint.draw();
    endComponentRender(m_joint.getBoundMinus(), m_joint.getBoundPlus());

    // do lower arm segment

    startComponentRender(0, 0, 0, 90, 0, 0);
    m_armSeg.draw();
    endComponentRender(m_armSeg.getBoundMinus(), m_armSeg.getBoundPlus());

    // do hand

    startComponentRender(0, -m_armSegLength, 0, 0, 0, 0);
    m_hand.draw();
    endComponentRender(m_hand.getBoundMinus(), m_hand.getBoundPlus());

    // end elbow pose

    endCompositeRender();

    // end upper arm pose

    endCompositeRender();
}

void VRHumanoidWidget::setArmPose(int arm, qreal x, qreal y, qreal z)
{
//    qDebug() << "Arm " << x << " " << y << " " << z;
    m_armAngles[arm][0] = x;
    m_armAngles[arm][1] = y;
    m_armAngles[arm][2] = z;
}

void VRHumanoidWidget::setElbowPose(int arm, qreal x, qreal y, qreal z)
{
//    qDebug() << "Elbow " << x << " " << y << " " << z;
    m_elbowAngles[arm][0] = x;
    m_elbowAngles[arm][1] = y;
    m_elbowAngles[arm][2] = z;
}

void VRHumanoidWidget::setTorsoPose(qreal x, qreal y, qreal z)
{
    m_torsoAngles[0] = x;
    m_torsoAngles[1] = y;
    m_torsoAngles[2] = z;
}

void VRHumanoidWidget::setHeadPose(qreal x, qreal y, qreal z)
{
    m_headAngles[0] = x;
    m_headAngles[1] = y;
    m_headAngles[2] = z;
}

void VRHumanoidWidget::setHeadTexture(const QString &path)
{
    m_head.setTexture(QPixmap(path).toImage());
}

void VRHumanoidWidget::setHeadTopTexture(const QString &path)
{
    m_headTop.setTexture(QPixmap(path).toImage());
}

void VRHumanoidWidget::setHeadBottomTexture(const QString &path)
{
    m_headBottom.setTexture(QPixmap(path).toImage());
}

void VRHumanoidWidget::setNeckTexture(const QString &path)
{
    m_neck.setTexture(QPixmap(path).toImage());
}

void VRHumanoidWidget::setHandTexture(const QString &path)
{
    m_hand.setTexture(QPixmap(path).toImage());
}

