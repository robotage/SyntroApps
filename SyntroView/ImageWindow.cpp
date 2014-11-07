//
//  Copyright (c) 2014 Scott Ellis and Richard Barnett
//	
//  This file is part of SyntroNet
//
//  SyntroNet is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  SyntroNet is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with SyntroNet.  If not, see <http://www.gnu.org/licenses/>.
//

#include <QtGui>
#include "ImageWindow.h"

#define	SPACESVIEW_CAMERA_DEADTIME		(15 * SYNTRO_CLOCKS_PER_SEC)


ImageWindow::ImageWindow(AVSource *avSource, bool showName, bool showDate, 
						bool showTime, QColor textColor, QWidget *parent)
	: QLabel(parent)
{
	m_avSource = avSource;
	m_showName = showName;
	m_showDate = showDate;
	m_showTime = showTime;
	m_textColor = textColor;

	m_selected = false;
	m_idle = true;

	m_lastFrame = SyntroClock();
    m_displayDate = QDate::currentDate();
    m_displayTime = QTime::currentTime();

	setAlignment(Qt::AlignCenter);

	setMinimumWidth(120);
	setMinimumHeight(90);

	setMaximumWidth(640);
	setMaximumHeight(480);

	if (m_avSource)
		m_timer = startTimer(30);
}

ImageWindow::~ImageWindow()
{
	killTimer(m_timer);
	m_avSource = NULL;
}

QString ImageWindow::sourceName()
{
	if (m_avSource)
		return m_avSource->name();

	return QString();
}

void ImageWindow::setShowName(bool enable)
{
	m_showName = enable;
	update();
}

void ImageWindow::setShowDate(bool enable)
{
	m_showDate = enable;
	update();
}

void ImageWindow::setShowTime(bool enable)
{
	m_showTime = enable;
	update();
}

void ImageWindow::setTextColor(QColor color)
{
	m_textColor = color;
	update();
}

void ImageWindow::mousePressEvent(QMouseEvent *)
{
	if (m_avSource)
		emit imageMousePress(m_avSource->name());
}

void ImageWindow::mouseDoubleClickEvent(QMouseEvent *)
{
	if (m_avSource)
		emit imageDoubleClick(m_avSource->name());
}

bool ImageWindow::selected()
{
	return m_selected;
}

void ImageWindow::setSelected(bool select)
{
	m_selected = select;
	update();
}

void ImageWindow::newImage(QImage image, qint64 timestamp)
{
    m_lastFrame = SyntroClock();
    if (image.width() == 0)
        return;

	m_idle = false;
	m_image = image;
 
    QPixmap pixmap = QPixmap::fromImage(image.scaled(size(), Qt::KeepAspectRatio));
    setPixmap(pixmap);

    QDateTime dt = QDateTime::fromMSecsSinceEpoch(timestamp);
    m_displayDate = dt.date();
    m_displayTime = dt.time();

    update();
}

void ImageWindow::timerEvent(QTimerEvent *)
{
	if (m_avSource && m_lastFrame < m_avSource->lastUpdate()) {
		newImage(m_avSource->image(), m_avSource->imageTimestamp());
	}
	else if (SyntroUtils::syntroTimerExpired(SyntroClock(), m_lastFrame, SPACESVIEW_CAMERA_DEADTIME)) {
		m_idle = true;
		update();
	}
}

void ImageWindow::paintEvent(QPaintEvent *event)
 {
	QString timestamp;
	QLabel::paintEvent(event);
    QString timeFormat("hh:mm:ss:zzz");
    QString dateFormat("ddd MMMM d yyyy");

	QPainter painter(this);

	QRect dr = drawingRect();

	if (m_idle)
		painter.fillRect(dr, Qt::Dense2Pattern);
	
	if (m_selected) {
		QPen pen(Qt::green, 3);
		painter.setPen(pen);
		painter.drawRect(dr);
	}

	if (!m_showDate && !m_showTime && !m_showName)
		return;

	painter.setPen(m_textColor);

	int fontHeight = dr.height() / 20;

	if (fontHeight < 8)
		fontHeight = 8;
	else if (fontHeight > 12)
		fontHeight = 12;

	painter.setFont(QFont("Arial", fontHeight));

	if (m_showName)
		painter.drawText(dr.left() + 4, dr.top() + fontHeight + 2, m_avSource->name());

	if (m_showTime || m_showDate) {
		if (dr.width() < 160) {
			// only room for one, choose time over date
			if (m_showDate && m_showTime)
                timestamp = m_displayTime.toString(timeFormat);
			else if (m_showDate)
                timestamp = m_displayDate.toString(dateFormat);
			else
                timestamp = m_displayTime.toString(timeFormat);
		}
		else if (!m_showDate) {
            timestamp = m_displayTime.toString(timeFormat);
		}
		else if (!m_showTime) {
            timestamp = m_displayDate.toString(dateFormat);
		}
		else {
            timestamp = m_displayDate.toString(dateFormat) + " " + m_displayTime.toString(timeFormat);
		}

		painter.drawText(dr.left() + 4, dr.bottom() - 2, timestamp);
	}
}

// Assumes horizontal and vertical center alignment
QRect ImageWindow::drawingRect()
{
	QRect dr = rect();

	const QPixmap *pm = pixmap();

	if (pm) {
		QRect pmRect = pm->rect();

		int x = (dr.width() - pmRect.width()) / 2;
		int y = (dr.height() - pmRect.height()) / 2;

		dr.adjust(x, y, -x, -y);
	}

	return dr;
}
