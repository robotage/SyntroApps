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

#include "AudioDlg.h"

#include <qlist.h>
#include <qboxlayout.h>
#include <qformlayout.h>
#include <qfile.h>
#include <qgroupbox.h>
#include "AudioDriver.h"

AudioDlg::AudioDlg(QWidget *parent)
    : QDialog(parent, Qt::WindowCloseButtonHint | Qt::WindowTitleHint)
{
	setWindowTitle("Audio configuration");

	m_channelMap[0] = 1;
	m_channelMap[1] = 2;

	m_rateMap[0] = 8000;
    m_rateMap[1] = 48000;

	layoutWindow();

	connect(m_buttons, SIGNAL(accepted()), this, SLOT(onOk()));
    connect(m_buttons, SIGNAL(rejected()), this, SLOT(reject()));
    connect(m_audioSource, SIGNAL(currentIndexChanged(int)), this, SLOT(sourceIndexChanged(int)));
}

void AudioDlg::onOk()
{
	bool changed = false;

	QSettings *settings = SyntroUtils::getSettings();

	settings->beginGroup(AUDIO_GROUP);

	if ((m_enable->checkState() == Qt::Checked) != settings->value(AUDIO_ENABLE).toBool()) {
		settings->setValue(AUDIO_ENABLE, m_enable->checkState() == Qt::Checked);
		changed = true;
	}

    if (m_inputCard->text() != settings->value(AUDIO_INPUT_CARD).toString()) {
        settings->setValue(AUDIO_INPUT_CARD, m_inputCard->text());
        changed = true;
    }

    if (m_inputDevice->text() != settings->value(AUDIO_INPUT_DEVICE).toString()) {
		settings->setValue(AUDIO_INPUT_DEVICE, m_inputDevice->text());
		changed = true;
	}

	if (m_channels->currentIndex() != -1) {
		if (m_channelMap[m_channels->currentIndex()] != settings->value(AUDIO_CHANNELS).toInt()) {
			settings->setValue(AUDIO_CHANNELS, m_channelMap[m_channels->currentIndex()]);
			changed = true;
		}
	}

	if (m_sampleRate->currentIndex() != -1) {
		if (m_rateMap[m_sampleRate->currentIndex()] != settings->value(AUDIO_SAMPLERATE).toInt()) {
			settings->setValue(AUDIO_SAMPLERATE, m_rateMap[m_sampleRate->currentIndex()]);
			changed = true;
		}
	}

	settings->endGroup();

	delete settings;

    if (changed)
		accept();
    else
        reject();
}

void AudioDlg::sourceIndexChanged(int index)
{
    if (index < 0 || index >= m_deviceList.count()) {
        m_inputCard->setText("0");
        m_inputDevice->setText("0");
    }
    else {
        QString entry = m_deviceList[index];

        int card = getCard(entry);
        m_inputCard->setText(QString::number(card));

        int device = getDevice(entry);
        m_inputDevice->setText(QString::number(device));
    }
}

void AudioDlg::selectCurrentDevice(QSettings *settings)
{
    int card = settings->value(AUDIO_INPUT_CARD, 0).toInt();
    int device = settings->value(AUDIO_INPUT_DEVICE, 0).toInt();

    for (int i = 0; i < m_deviceList.count(); i++) {
        QString entry = m_deviceList[i];

        if (getCard(entry) == card && getDevice(entry) == device) {
            m_inputCard->setText(QString::number(card));
            m_inputDevice->setText(QString::number(device));
            m_audioSource->setCurrentIndex(i);
        }
    }
}

void AudioDlg::layoutWindow()
{
	QSettings *settings = SyntroUtils::getSettings();

	settings->beginGroup(AUDIO_GROUP);

	QVBoxLayout *centralLayout = new QVBoxLayout(this);
	
    QFormLayout *formLayout = new QFormLayout;

	m_enable = new QCheckBox(this);
    m_enable->setMinimumWidth(100);
	formLayout->addRow(tr("Enable audio"), m_enable);
	m_enable->setCheckState(settings->value(AUDIO_ENABLE).toBool() ? Qt::Checked : Qt::Unchecked);
    centralLayout->addLayout(formLayout);


    formLayout = new QFormLayout;

    getDeviceList();

    m_audioSource = new QComboBox;
    m_audioSource->setMinimumWidth(160);

    for (int i = 0; i < m_deviceList.count(); i++)
        m_audioSource->addItem(m_deviceList[i]);

    formLayout->addRow(tr("Audio Sources"), m_audioSource);

    m_inputCard = new QLabel;
    m_inputCard->setFrameShape(QFrame::StyledPanel);
    m_inputCard->setMaximumWidth(60);
    formLayout->addRow(tr("Audio card"), m_inputCard);

    m_inputDevice = new QLabel;
    m_inputDevice->setFrameShape(QFrame::StyledPanel);
	m_inputDevice->setMaximumWidth(60);
	formLayout->addRow(tr("Audio device"), m_inputDevice);

    selectCurrentDevice(settings);

    QGroupBox *group = new QGroupBox("Source");
    group->setLayout(formLayout);
    centralLayout->addWidget(group);

    formLayout = new QFormLayout();

	m_channels = new QComboBox(this);
	m_channels->setMaximumWidth(60);
	
	for (int i = 0; i < 2; i++) {
		m_channels->addItem(QString::number(m_channelMap[i]));

		if (m_channelMap[i] == settings->value(AUDIO_CHANNELS).toInt())
			m_channels->setCurrentIndex(i);
	}

	formLayout->addRow(tr("Channels"), m_channels);

	m_sampleRate = new QComboBox(this);
    m_sampleRate->setMaximumWidth(80);
	
    for (int i = 0; i < 2; i++) {
		m_sampleRate->addItem(QString::number(m_rateMap[i]));

        if (m_rateMap[i] == settings->value(AUDIO_SAMPLERATE).toInt())
			m_sampleRate->setCurrentIndex(i);
	}

    formLayout->addRow(tr("Sample rate (sps)"), m_sampleRate);

    group = new QGroupBox("Parameters");
    group->setLayout(formLayout);
    centralLayout->addWidget(group);

    centralLayout->addSpacerItem(new QSpacerItem(20, 20));

	m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
	m_buttons->setCenterButtons(true);

	centralLayout->addWidget(m_buttons);

	settings->endGroup();

	delete settings;
}

void AudioDlg::getDeviceList()
{
    m_deviceList.clear();

    QFile file("/proc/asound/pcm");

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QTextStream in(&file);

    QString line = in.readLine();

    while (!line.isNull()) {
        processDeviceLine(line);
        line = in.readLine();
    }
}

void AudioDlg::processDeviceLine(QString line)
{
    if (!line.contains("capture"))
        return;

    QStringList fields = line.split(':');

    if (fields.length() < 2)
        return;

    QString entry = fields[0].trimmed() + ": " + fields[1].trimmed();

    // now test that we can parse it later
    if (getCard(entry) < 0 || getDevice(entry) < 0)
        return;

    m_deviceList.append(entry);
}

int AudioDlg::getCard(QString audioDevice)
{
    QStringList fields = audioDevice.split(':');

    if (fields.count() != 2)
        return -1;

    QStringList cardDevice = fields[0].split('-');

    if (cardDevice.count() != 2)
        return -2;

    return cardDevice[0].toInt();
}

int AudioDlg::getDevice(QString audioDevice)
{
    QStringList fields = audioDevice.split(':');

    if (fields.count() != 2)
        return -1;

    QStringList cardDevice = fields[0].split('-');

    if (cardDevice.count() != 2)
        return -2;

    return cardDevice[1].toInt();
}
