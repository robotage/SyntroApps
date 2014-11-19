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

#include "VRFloorWidget.h"
#include <qdebug.h>

VRFloorWidget::VRFloorWidget(QObject *parent, VRWIDGET_TYPE widgetType)
    : VRWidget(parent, widgetType)
{
    m_image = NULL;
    m_repeatX = 1;
    m_repeatY = 1;
}

VRFloorWidget::~VRFloorWidget()
{
    if (m_image != NULL)
        delete m_image;
}

void VRFloorWidget::setTextureRepeat(float repeatX, float repeatY)
{
    m_repeatX = repeatX;
    m_repeatY = repeatY;
}

void VRFloorWidget::VRWidgetInit()
{
    m_plane.generate(m_width, m_height, m_repeatX, m_repeatY);
    m_plane.setShader(globalShader[QTGLSHADER_TEXTURE]);

    m_image = new QImage(QSize(1280, 720), QImage::Format_RGB888);
    m_image->fill(QColor(40, 80, 20, 255));
}

void VRFloorWidget::VRWidgetRender()
{
    startWidgetRender();

    startComponentRender();
    m_plane.draw();
    endComponentRender(m_plane.getBoundMinus(), m_plane.getBoundPlus());

    endWidgetRender();
}

void VRFloorWidget::paintFileImage(const char *fileName)
{
    m_image->load(fileName);
    m_image->convertToFormat(QImage::Format_RGB888);
    m_plane.setTexture(*m_image);
}

void VRFloorWidget::paintImage(const QImage& image)
{
    *m_image = image;

    m_plane.setTexture(*m_image);
}
