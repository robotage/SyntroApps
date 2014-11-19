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

#ifndef VRPLANEWIDGET_H
#define VRPLANEWIDGET_H

#include <VRWidget.h>

class VRPlaneWidget : public VRWidget
{
    Q_OBJECT

public:
    VRPlaneWidget(QObject *parent, VRWIDGET_TYPE = VRWIDGET_PLANE);
    ~VRPlaneWidget();

    // paintImage() paints the image

    void paintImage(const QImage& image);

    // paintText() paints the text on the plane at the specified location within the specified width

    void paintText(const QString& text, int x, int y, int width);

    // paintFileImage() paints an image from a file on the plane

    void paintFileImage(const char *fileName);

    virtual void VRWidgetInit();
    virtual void VRWidgetRender();

private:
    QtGLPlaneComponent m_plane;

    QImage *m_image;										// the image used to hold what's displayed on the plane
    QFont m_font;											// the font used for text
};

#endif // VRPLANEWIDGET_H
