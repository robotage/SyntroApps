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

#ifndef VRFLOORWIDGET_H
#define VRFLOORWIDGET_H

#include <VRWidget.h>

class VRFloorWidget : public VRWidget
{
    Q_OBJECT

public:
    VRFloorWidget(QObject *parent, VRWIDGET_TYPE = VRWIDGET_FLOOR);
    ~VRFloorWidget();

    // setTextureRepeat sets the texture repeat counts

    void setTextureRepeat(float repeatX, float repeatY);

    // paintImage() paints the image

    void paintImage(const QImage& image);

    // paintFileImage() paints an image from a file on the plane

    void paintFileImage(const char *fileName);

    virtual void VRWidgetInit();
    virtual void VRWidgetRender();

private:
    QtGLPlaneComponent m_plane;

    QImage *m_image;										// the image used to hold what's displayed on the plane
    float m_repeatX;
    float m_repeatY;
};

#endif // VRFLOORWIDGET_H
