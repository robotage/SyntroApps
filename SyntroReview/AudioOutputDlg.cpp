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

#include "AudioOutputDlg.h"

#include <qlist.h>
#include <qboxlayout.h>
#include <qformlayout.h>
#if defined(Q_OS_OSX) || defined(Q_OS_WIN)
#include <QAudioOutput>
#endif

AudioOutputDlg::AudioOutputDlg(QWidget *parent)
	: QDialog(parent, Qt::WindowCloseButtonHint | Qt::WindowTitleHint)
{
	setWindowTitle("Audio output configuration");

	layoutWindow();

	connect(m_buttons, SIGNAL(accepted()), this, SLOT(onOk()));
    connect(m_buttons, SIGNAL(rejected()), this, SLOT(reject()));
}

void AudioOutputDlg::onOk()
{
	bool changed = false;

	QSettings *settings = SyntroUtils::getSettings();

	settings->beginGroup(AUDIO_GROUP);

	if ((m_enable->checkState() == Qt::Checked) != settings->value(AUDIO_ENABLE).toBool()) {
		settings->setValue(AUDIO_ENABLE, m_enable->checkState() == Qt::Checked);
		changed = true;
	}

#if defined(Q_OS_OSX) || defined(Q_OS_WIN)
	if (m_outputDevice->currentText() != settings->value(AUDIO_OUTPUT_DEVICE).toString()) {
		settings->setValue(AUDIO_OUTPUT_DEVICE, m_outputDevice->currentText());
		changed = true;
	}
#else
    if (m_outputCard->text() != settings->value(AUDIO_OUTPUT_CARD).toString()) {
        settings->setValue(AUDIO_OUTPUT_CARD, m_outputCard->text());
        changed = true;
    }
    if (m_outputDevice->text() != settings->value(AUDIO_OUTPUT_DEVICE).toString()) {
        settings->setValue(AUDIO_OUTPUT_DEVICE, m_outputDevice->text());
		changed = true;
	}
#endif

	settings->endGroup();

	delete settings;

	if (changed)
		accept();
	else
		reject();
}

void AudioOutputDlg::layoutWindow()
{

	QSettings *settings = SyntroUtils::getSettings();

    setModal(true);
	settings->beginGroup(AUDIO_GROUP);

	QVBoxLayout *centralLayout = new QVBoxLayout(this);
	centralLayout->setSpacing(20);
	centralLayout->setContentsMargins(11, 11, 11, 11);
	
	QFormLayout *formLayout = new QFormLayout();
	formLayout->setSpacing(16);
	formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

	m_enable = new QCheckBox(this);
    m_enable->setMinimumWidth(100);
	formLayout->addRow(tr("Enable audio"), m_enable);
	m_enable->setCheckState(settings->value(AUDIO_ENABLE).toBool() ? Qt::Checked : Qt::Unchecked);

#if defined(Q_OS_OSX) || defined(Q_OS_WIN32)
	m_outputDevice = new QComboBox(this);
	m_outputDevice->setMaximumWidth(200);
	m_outputDevice->setMinimumWidth(200);
	formLayout->addRow(tr("Audio output device"), m_outputDevice);
		
	m_outputDevice->addItem(AUDIO_DEFAULT_DEVICE);

	QList<QAudioDeviceInfo> devices = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);

	for (int i = 0; i < devices.size(); i++) {
		m_outputDevice->addItem(devices.at(i).deviceName());
		if (devices.at(i).deviceName() == settings->value(AUDIO_OUTPUT_DEVICE).toString())
			m_outputDevice->setCurrentIndex(i+1);
	}

#else
    m_outputCard = new QLineEdit(this);
    m_outputCard->setMaximumWidth(60);
    formLayout->addRow(tr("Audio output card"), m_outputCard);
    m_outputCard->setText(settings->value(AUDIO_OUTPUT_CARD).toString());
    m_outputCard->setValidator(new QIntValidator(0, 64));

	m_outputDevice = new QLineEdit(this);
	m_outputDevice->setMaximumWidth(60);
	formLayout->addRow(tr("Audio output device"), m_outputDevice);
	m_outputDevice->setText(settings->value(AUDIO_OUTPUT_DEVICE).toString());
    m_outputDevice->setValidator(new QIntValidator(0, 64));

#endif

	centralLayout->addLayout(formLayout);

	m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
	m_buttons->setCenterButtons(true);

	centralLayout->addWidget(m_buttons);

	settings->endGroup();
	delete settings;
}

