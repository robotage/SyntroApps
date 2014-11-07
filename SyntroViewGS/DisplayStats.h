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

#ifndef DISPLAYSTATS_H
#define DISPLAYSTATS_H

#include <qdialog.h>
#include <qevent.h>
#include "ui_DisplayStats.h"
#include "SyntroLib.h"
#include "AVSource.h"

#define DEFAULT_ROW_HEIGHT 20

class DisplayStats : public QDialog
{
	Q_OBJECT

public:
	DisplayStats(QWidget *parent);
	~DisplayStats();

	void addSource(AVSource *avSource);
	void removeSource(QString name);

	void deleteAllServices();

protected:
	void showEvent(QShowEvent *event);
	void closeEvent(QCloseEvent *event);
	void timerEvent(QTimerEvent *event);

private:
	void saveDialogState();
	void restoreDialogState();
	QString formatByteTotalForDisplay(qint64 bytes);

	int m_timer;
	QList<AVSource *> m_avSources;

	Ui::CDisplayStats ui;

	QString m_logTag;
};

#endif // DISPLAYSTATS_H
