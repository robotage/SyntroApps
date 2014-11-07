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


#ifndef STREAMSDLG_H
#define STREAMSDLG_H

#include <QDialog>
#include <qvalidator.h>
#include <qsettings.h>
#include <qlineedit.h>
#include <qdialogbuttonbox.h>
#include <qmessagebox.h>
#include <qcheckbox.h>
#include "syntrogui_global.h"

#include "SyntroLib.h"
#include "SyntroValidators.h"

class StreamsDlg : public QDialog
{
	Q_OBJECT

public:
	StreamsDlg(QWidget *parent);

public slots:
	void onOk();
    void lowRateStateChange(int state);

private:
	void layoutWindow();
    void enableLowRate(bool enable);

	QLineEdit *m_highRateMinInterval;
	QLineEdit *m_highRateMaxInterval;
	QLineEdit *m_highRateNullInterval;
    QLineEdit *m_highRateVideoCompression;
    QLineEdit *m_highRateAudioCompression;
	QCheckBox *m_generateLowRate;
    QLineEdit *m_lowRateVideoCompression;
    QLineEdit *m_lowRateAudioCompression;

	QDialogButtonBox *m_buttons;

};

#endif // STREAMSDLG_H
