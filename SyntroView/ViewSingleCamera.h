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

#ifndef VIEWSINGLECAMERA_H
#define VIEWSINGLECAMERA_H

#include <qdialog.h>
#include <qlabel.h>

#include "ui_ViewSingleCamera.h"
#include "SyntroLib.h"
#include "AVSource.h"

class ViewSingleCamera : public QDialog
{
	Q_OBJECT

public:
	ViewSingleCamera(QWidget *parent, AVSource *avSource);

	void setSource(AVSource *avSource);
	QString sourceName();

signals:
	void closed();

protected:
	void timerEvent(QTimerEvent *event);
	void closeEvent(QCloseEvent *event);

private:
	void newImage(QImage image);
	void saveWindowState();
	void restoreWindowState();

	Ui::ViewSingleCamera ui;

	int m_timer;
	AVSource *m_avSource;
	qint64 m_lastFrame;
	QLabel *m_cameraView;
};

#endif // VIEWSINGLECAMERA_H
