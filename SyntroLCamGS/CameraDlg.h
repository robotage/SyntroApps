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


#ifndef CAMERADLG_H
#define CAMERADLG_H

#include <QDialog>
#include <qvalidator.h>
#include <qsettings.h>
#include <qlineedit.h>
#include <qdialogbuttonbox.h>
#include <qmessagebox.h>
#include <qcombobox.h>
#include <qlist.h>
#include "syntrogui_global.h"

#include "SyntroLib.h"
#include "SyntroValidators.h"

class CameraDlg : public QDialog
{
	Q_OBJECT

public:
	CameraDlg(QWidget *parent);

public slots:
	void onOk();

private:
	void layoutWindow();
    QList<int> getVideoDeviceList(QSettings *settings);

    //QLineEdit *m_index;
    QComboBox *m_index;
	QLineEdit *m_width;
	QLineEdit *m_height;
	QLineEdit *m_rate;
	QDialogButtonBox *m_buttons;
};

#endif // CAMERADLG_H
