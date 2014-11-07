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

#include "ViewSingleCamera.h"

#define	SYNTROVIEW_CAMERA_DEADTIME		(10 * SYNTRO_CLOCKS_PER_SEC)

ViewSingleCamera::ViewSingleCamera(QWidget *parent, AVSource *avSource)
	: QDialog(parent, Qt::WindowCloseButtonHint | Qt::WindowTitleHint)
{
	ui.setupUi(this);

	m_avSource = avSource;
	m_lastFrame = -1;

	restoreWindowState();
		
	if (m_avSource) {
		setWindowTitle(m_avSource->name());
		m_timer = startTimer(30);
	}
	else {
		m_timer = 0;
	}
}

void ViewSingleCamera::setSource(AVSource *avSource)
{
	m_avSource = avSource;

	if (m_avSource) {
		setWindowTitle(m_avSource->name());

		newImage(m_avSource->image());

		if (!m_timer)
			m_timer = startTimer(30);
	}
	else {
		m_lastFrame = -1;
		ui.cameraView->setText("No Image");

		if (m_timer) {
			killTimer(m_timer);
			m_timer = 0;
		}

		// is this what we want?
		setWindowTitle("");
	}
}

QString ViewSingleCamera::sourceName()
{
	if (m_avSource)
		return m_avSource->name();

	return QString();
}

void ViewSingleCamera::closeEvent(QCloseEvent *)
{
	saveWindowState();

	killTimer(m_timer);
	m_timer = 0;

	emit closed();
}

void ViewSingleCamera::newImage(QImage image)
{
	m_lastFrame = SyntroClock();

	if (image.width() == 0)
        return;
	else
		ui.cameraView->setPixmap(QPixmap::fromImage(image.scaled(size(), Qt::KeepAspectRatio)));

    update();
}

void ViewSingleCamera::timerEvent(QTimerEvent *)
{
	if (m_avSource && m_lastFrame < m_avSource->lastUpdate()) {
		newImage(m_avSource->image());
	}
	else if (SyntroUtils::syntroTimerExpired(SyntroClock(), m_lastFrame, SYNTROVIEW_CAMERA_DEADTIME)) {
		ui.cameraView->setText("No Image");
	}
}

void ViewSingleCamera::saveWindowState()
{
	QSettings *settings = SyntroUtils::getSettings();

	settings->beginGroup("SingleCameraView");
	settings->setValue("Geometry", saveGeometry());
	settings->endGroup();
	
	delete settings;
}

void ViewSingleCamera::restoreWindowState()
{
	QSettings *settings = SyntroUtils::getSettings();

	settings->beginGroup("SingleCameraView");
	restoreGeometry(settings->value("Geometry").toByteArray());
	settings->endGroup();
	
	delete settings;
}
