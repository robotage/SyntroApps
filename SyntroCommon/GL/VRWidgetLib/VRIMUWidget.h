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

#ifndef VRIMUWIDGET_H
#define VRIMUWIDGET_H

#include "VRWidget.h"

class VRIMUWidget : public VRWidget
{
	Q_OBJECT

public:
    VRIMUWidget(QObject *parent, VRWIDGET_TYPE = VRWIDGET_IMU);
    ~VRIMUWidget();

	virtual void VRWidgetInit();
	virtual void VRWidgetRender();

private:
    QtGLCylinderComponent m_IMUXCylinder;
    QtGLCylinderComponent m_IMUYCylinder;
    QtGLCylinderComponent m_IMUZCylinder;
    QtGLDiskComponent m_IMUXTop;
    QtGLDiskComponent m_IMUYTop;
    QtGLDiskComponent m_IMUZTop;

    QtGLWireCubeComponent m_cube;

    qreal m_IMURadius;
    qreal m_IMULength;
};

#endif // VRIMUWIDGET_H
