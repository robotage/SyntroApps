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

#include "VRPlaneWidget.h"
#include <qdebug.h>

VRPlaneWidget::VRPlaneWidget(QObject *parent, VRWIDGET_TYPE widgetType)
    : VRWidget(parent, widgetType)
{
    m_image = NULL;
}

VRPlaneWidget::~VRPlaneWidget()
{
    if (m_image != NULL)
        delete m_image;
}

void VRPlaneWidget::VRWidgetInit()
{
    m_plane.generate(m_width, m_height);
    m_plane.setShader(globalShader[QTGLSHADER_TEXTURE]);

    m_image = new QImage(QSize(1280, 720), QImage::Format_RGB888);
    m_image->fill(QColor(40, 80, 20, 255));

    m_font.setFamily("Arial");
    m_font.setPixelSize(8);
    m_font.setFixedPitch(true);

}

void VRPlaneWidget::VRWidgetRender()
{
    startWidgetRender();

    startComponentRender();
    m_plane.draw();
    endComponentRender(m_plane.getBoundMinus(), m_plane.getBoundPlus());

    endWidgetRender();
}

void VRPlaneWidget::paintText(const QString& text, int x, int y, int width)
{
    if (m_image == NULL)
        return;

    QPainter painter(m_image);

    QFontMetrics metrics = QFontMetrics(m_font);
    int border = qMax(4, metrics.leading());

    QRect rect;

    rect = metrics.boundingRect(text);
    painter.setRenderHint(QPainter::TextAntialiasing);
    painter.fillRect(QRect(x, y, width + 2 * border, rect.height() + 2 * border),
                      QColor(0, 0, 0, 128));
    painter.setPen(Qt::white);
    painter.drawText(x + border, y + border,
                      width + border, rect.height() + border,
                      Qt::AlignLeft | Qt::TextWordWrap, text);

    painter.end();

    m_plane.setTexture(*m_image);
}

void VRPlaneWidget::paintFileImage(const char *fileName)
{
    m_image->load(fileName);
    m_image->convertToFormat(QImage::Format_RGB888);
    m_plane.setTexture(*m_image);
}

void VRPlaneWidget::paintImage(const QImage& image)
{
    *m_image = image;

    m_plane.setTexture(*m_image);
}
