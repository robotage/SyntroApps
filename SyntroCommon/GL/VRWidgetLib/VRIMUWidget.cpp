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

#include "VRIMUWidget.h"

VRIMUWidget::VRIMUWidget(QObject *parent, VRWIDGET_TYPE widgetType)
	: VRWidget(parent, widgetType)
{
}

VRIMUWidget::~VRIMUWidget()
{

}

void VRIMUWidget::VRWidgetInit()
{
    m_IMULength = m_width;
    m_IMURadius = m_IMULength / 10.0;

    m_IMUXCylinder.generate(m_IMURadius / 100.0, m_IMURadius, m_IMULength, 50, 1);
    m_IMUXCylinder.setShader(globalShader[QTGLSHADER_ADSTEXTURE]);
    m_IMUXCylinder.setMaterial(QVector3D(1.0, 1.0, 1.0), QVector3D(0.8, 0.8, 0.8),
                        QVector3D(1.0, 1.0, 1.0), 3.0);
    m_IMUXCylinder.setTexture(QPixmap(":/Images/RedShade.png").toImage());

    m_IMUXTop.generate(0, m_IMURadius, 50, 1);
    m_IMUXTop.setShader(globalShader[QTGLSHADER_ADSTEXTURE]);
    m_IMUXTop.setMaterial(QVector3D(1.0, 1.0, 1.0), QVector3D(0.8, 0.8, 0.8),
                        QVector3D(1.0, 1.0, 1.0), 3.0);
    m_IMUXTop.setTexture(QPixmap(":/Images/RedShade.png").toImage());

    m_IMUYCylinder.generate(m_IMURadius / 100.0, m_IMURadius, m_IMULength, 50, 1);
    m_IMUYCylinder.setShader(globalShader[QTGLSHADER_ADSTEXTURE]);
    m_IMUYCylinder.setMaterial(QVector3D(1.0, 1.0, 1.0), QVector3D(0.8, 0.8, 0.8),
                        QVector3D(1.0, 1.0, 1.0), 3.0);
    m_IMUYCylinder.setTexture(QPixmap(":/Images/GreenShade.png").toImage());

    m_IMUYTop.generate(0, m_IMURadius, 50, 1);
    m_IMUYTop.setShader(globalShader[QTGLSHADER_ADSTEXTURE]);
    m_IMUYTop.setMaterial(QVector3D(1.0, 1.0, 1.0), QVector3D(0.8, 0.8, 0.8),
                        QVector3D(1.0, 1.0, 1.0), 3.0);
    m_IMUYTop.setTexture(QPixmap(":/Images/GreenShade.png").toImage());

    m_IMUZCylinder.generate(m_IMURadius / 100.0, m_IMURadius, m_IMULength, 50, 1);
    m_IMUZCylinder.setShader(globalShader[QTGLSHADER_ADSTEXTURE]);
    m_IMUZCylinder.setMaterial(QVector3D(1.0, 1.0, 1.0), QVector3D(0.8, 0.8, 0.8),
                        QVector3D(1.0, 1.0, 1.0), 3.0);
    m_IMUZCylinder.setTexture(QPixmap(":/Images/BlueShade.png").toImage());

    m_IMUZTop.generate(0, m_IMURadius, 50, 1);
    m_IMUZTop.setShader(globalShader[QTGLSHADER_ADSTEXTURE]);
    m_IMUZTop.setMaterial(QVector3D(1.0, 1.0, 1.0), QVector3D(0.8, 0.8, 0.8),
                        QVector3D(1.0, 1.0, 1.0), 3.0);
    m_IMUZTop.setTexture(QPixmap(":/Images/BlueShade.png").toImage());

    m_cube.generate(m_IMULength / 2.0, m_IMULength / 2.0, m_IMULength / 2.0);
    m_cube.setShader(globalShader[QTGLSHADER_FLAT]);
    m_cube.setMaterial(QVector3D(1.0, 1.0, 1.0), QVector3D(0.8, 0.8, 0.8),
                        QVector3D(1.0, 1.0, 1.0), 6.0);
    m_cube.setColor(QColor(255, 255, 0));

    setRotationOrder(VRWidgetRotationXZY);					// so that roll/pitch/yaw works
}

void VRIMUWidget::VRWidgetRender()
{
	startWidgetRender();

    //  do cube

    startComponentRender(0, 0, 0, 0, 0, 0);
    m_cube.draw();
    endComponentRender(m_IMUXCylinder.getBoundMinus(), m_IMUXCylinder.getBoundPlus());

    // do X IMU cylinder

    startComponentRender(0, 0, 0, 0, 90, 0);
    m_IMUXCylinder.draw();
    endComponentRender(m_IMUXCylinder.getBoundMinus(), m_IMUXCylinder.getBoundPlus());

    // do X IMU top

    startComponentRender(m_IMULength, 0, 0, 0, 90, 0);
    m_IMUXTop.draw();
    endComponentRender(m_IMUXTop.getBoundMinus(), m_IMUXTop.getBoundPlus());

    // do Y IMU cylinder

    startComponentRender(0, 0, 0, 0, 0, 0);
    m_IMUYCylinder.draw();
    endComponentRender(m_IMUYCylinder.getBoundMinus(), m_IMUYCylinder.getBoundPlus());

    // do Y IMU top

    startComponentRender(0, 0, m_IMULength, 0, 0, -90);
    m_IMUYTop.draw();
    endComponentRender(m_IMUYTop.getBoundMinus(), m_IMUYTop.getBoundPlus());

    // do Z IMU cylinder

    startComponentRender(0, 0, 0, 90, 0, 0);
    m_IMUZCylinder.draw();
    endComponentRender(m_IMUZCylinder.getBoundMinus(), m_IMUZCylinder.getBoundPlus());

    // do Z IMU top

    startComponentRender(0, -m_IMULength, 0, 90, 0, 0);
    m_IMUZTop.draw();
    endComponentRender(m_IMUZTop.getBoundMinus(), m_IMUZTop.getBoundPlus());

    endWidgetRender();
}

