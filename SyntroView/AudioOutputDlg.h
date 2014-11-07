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


#ifndef AUDIOOUTPUTDLG_H
#define AUDIOOUTPUTDLG_H

#include <QDialog>
#include <qvalidator.h>
#include <qsettings.h>
#include <qlineedit.h>
#include <qdialogbuttonbox.h>
#include <qcheckbox.h>
#include <qmessagebox.h>
#include <qcombobox.h>
#include "syntrogui_global.h"

#include "SyntroLib.h"
#include "SyntroValidators.h"

//  group for audio related entries

#define	AUDIO_GROUP                   "AudioGroup"

// service enable flag

#define	AUDIO_ENABLE                  "AudioEnable"

// audio source to use

#define	AUDIO_OUTPUT_CARD               "AudioOutputCard"
#define	AUDIO_OUTPUT_DEVICE             "AudioOutputDevice"

#define AUDIO_DEFAULT_DEVICE			"<default device>"
#define AUDIO_DEFAULT_DEVICE_MAC		"Built-in Output"

class AudioOutputDlg : public QDialog
{
	Q_OBJECT

public:
	AudioOutputDlg(QWidget *parent);
	~AudioOutputDlg();

public slots:
	void onOk();

private:
	void layoutWindow();

	QCheckBox *m_enable;
#if defined(Q_OS_WIN) || defined(Q_OS_OSX)
	QComboBox *m_outputDevice;
#else
    QLineEdit *m_outputDevice;
	QLineEdit *m_outputCard;
#endif
	QDialogButtonBox *m_buttons;
};

#endif // AUDIOOUTPUTDLG_H
