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

#ifndef VRHANDWIDGET_H
#define VRHANDWIDGET_H

#include "VRWidget.h"

#define VRHANDWIDGET_FINGERS	5

class VRHandWidget : public VRWidget
{
    Q_OBJECT

public:
    VRHandWidget(QObject *parent, VRWIDGET_TYPE = VRWIDGET_HAND);
    ~VRHandWidget();

    void setFingerAngles(int finger, qreal x, qreal y, qreal z);

    virtual void VRWidgetInit();
    virtual void VRWidgetRender();

private:
    QtGLCylinderComponent m_palmCylinder;
    QtGLDiskComponent m_palmTop;
    QtGLCylinderComponent m_fingerCylinder;
    QtGLDiskComponent m_fingerTop;
    QtGLCylinderComponent m_thumbCylinder;

    QtGLCylinderComponent m_stubFingerCylinder;
    QtGLCylinderComponent m_stubThumbCylinder;

    qreal m_palmRadius;
    qreal m_palmHeight;

    qreal m_fingerLength;
    qreal m_fingerRadius;
    qreal m_fingerOffsetX;
    qreal m_fingerOffsetY;

    qreal m_thumbLength;
    qreal m_thumbOffsetX;
    qreal m_thumbOffsetY;
    qreal m_thumbOffsetAngle;

    qreal m_fingerAngles[VRHANDWIDGET_FINGERS][3];
};

#endif // VRHANDWIDGET_H
