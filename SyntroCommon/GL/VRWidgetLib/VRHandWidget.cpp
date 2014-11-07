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

#include "VRHandWidget.h"

VRHandWidget::VRHandWidget(QObject *parent, VRWIDGET_TYPE widgetType)
    : VRWidget(parent, widgetType)
{
    for (int i = 0; i < VRHANDWIDGET_FINGERS; i++) {
        m_fingerAngles[i][0] = 0;
        m_fingerAngles[i][1] = 0;
        m_fingerAngles[i][2] = 0;
    }
}

VRHandWidget::~VRHandWidget()
{

}

void VRHandWidget::VRWidgetInit()
{
    m_palmRadius = m_width / 2.5;
    m_palmHeight = m_palmRadius / 5.0;

    m_fingerLength = m_palmRadius * 1.5;
    m_fingerRadius = m_palmHeight / 2.0;
    m_fingerOffsetX = 0;
    m_fingerOffsetY = m_palmRadius * 0.9;

    m_thumbLength = m_fingerLength * 0.8;
    m_thumbOffsetX = -m_palmRadius * 0.9;
    m_thumbOffsetY = m_palmRadius * 0.2;
//    m_thumbOffsetAngle = -30.0;
    m_thumbOffsetAngle = 0;

    m_palmCylinder.generate(m_palmRadius, m_palmRadius, m_palmHeight, 50, 1);
    m_palmCylinder.setShader(globalShader[QTGLSHADER_ADSTEXTURE]);
    m_palmCylinder.setMaterial(QVector3D(1.0, 1.0, 1.0), QVector3D(0.5, 0.5, 0.5),
                        QVector3D(1.0, 1.0, 1.0), 3.0);
    m_palmCylinder.setTexture(QPixmap(":/Images/RedShade.png").toImage());

    m_palmTop.generate(0, m_palmRadius, 50, 1);
    m_palmTop.setShader(globalShader[QTGLSHADER_ADSTEXTURE]);
    m_palmTop.setMaterial(QVector3D(1.0, 1.0, 1.0), QVector3D(0.5, 0.5, 0.5),
                        QVector3D(1.0, 1.0, 1.0), 3.0);
    m_palmTop.setTexture(QPixmap(":/Images/GreenShade.png").toImage());

    m_fingerCylinder.generate(m_fingerRadius, m_fingerRadius, m_fingerLength, 50, 1);
    m_fingerCylinder.setShader(globalShader[QTGLSHADER_ADS]);
    m_fingerCylinder.setMaterial(QVector3D(0.0, 1.0, 0.0), QVector3D(0.0, 0.2, 0.0),
                        QVector3D(1.0, 1.0, 1.0), 3.0);

    m_fingerTop.generate(0, m_fingerRadius, 50, 1);
    m_fingerTop.setShader(globalShader[QTGLSHADER_ADS]);
    m_fingerTop.setMaterial(QVector3D(0.0, 1.0, 0.0), QVector3D(0.0, 0.2, 0.0),
                        QVector3D(1.0, 1.0, 1.0), 3.0);

    m_thumbCylinder.generate(m_fingerRadius, m_fingerRadius, m_thumbLength, 50, 1);
    m_thumbCylinder.setShader(globalShader[QTGLSHADER_ADS]);
    m_thumbCylinder.setMaterial(QVector3D(0.0, 1.0, 0.0), QVector3D(0.0, 0.2, 0.0),
                        QVector3D(1.0, 1.0, 1.0), 3.0);

    setRotationOrder(VRWidgetRotationZXY);					// so that roll/pitch/yaw works
}

void VRHandWidget::VRWidgetRender()
{
    startWidgetRender();

    // do palm cylinder

    startComponentRender(0, -m_palmHeight / 2.0, 0, -90, 0, 0);
    m_palmCylinder.draw();
    endComponentRender(m_palmCylinder.getBoundMinus(), m_palmCylinder.getBoundPlus());

    // do palm top

    startComponentRender(0, m_palmHeight / 2.0, 0, -90, 0, 0);
    m_palmTop.draw();
    endComponentRender(m_palmTop.getBoundMinus(), m_palmTop.getBoundPlus());

    // do palm bottom

    startComponentRender(0, -m_palmHeight / 2.0, 0, 90, 0, 0);
    m_palmTop.draw();
    endComponentRender(m_palmTop.getBoundMinus(), m_palmTop.getBoundPlus());

    // do fingers

    for (int i = 1; i < VRHANDWIDGET_FINGERS; i++) {
        startCompositeRender(
                m_palmRadius * (qreal(i) - 2.5) * 0.4 + m_fingerOffsetX,
                0,
                -(m_fingerOffsetY - abs(i - 2) * m_fingerRadius * 2.0),
                m_fingerAngles[i][0],
                m_fingerAngles[i][1],
                m_fingerAngles[i][2]);

        startComponentRender();
        m_fingerCylinder.draw();
        endComponentRender(m_fingerCylinder.getBoundMinus(), m_fingerCylinder.getBoundPlus());

        startComponentRender(0, 0, m_fingerLength, 0, 0, 0);
        m_fingerTop.draw();
        endComponentRender(m_fingerTop.getBoundMinus(), m_fingerTop.getBoundPlus());

        endCompositeRender();
    }

    // do thumb

    startCompositeRender(
        m_thumbOffsetX,
        0,
        -m_thumbOffsetY,
        m_fingerAngles[0][0],
        m_fingerAngles[0][1],
        m_fingerAngles[0][2]);

    startComponentRender();
    m_thumbCylinder.draw();
    endComponentRender(m_thumbCylinder.getBoundMinus(), m_thumbCylinder.getBoundPlus());

    startComponentRender(0, 0, m_thumbLength, 0, 0, 0);
    m_fingerTop.draw();
    endComponentRender(m_fingerTop.getBoundMinus(), m_fingerTop.getBoundPlus());

    endCompositeRender();

    endWidgetRender();
}

void VRHandWidget::setFingerAngles(int finger, qreal x, qreal y, qreal z)
{
    if ((finger < 0) || (finger >= VRHANDWIDGET_FINGERS)) {
        qDebug() << "Illegal finger number " << finger;
        return;
    }
    m_fingerAngles[finger][0] = x;
    m_fingerAngles[finger][1] = y;
    m_fingerAngles[finger][2] = z;
}
