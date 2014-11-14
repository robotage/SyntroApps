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

#include "StreamsDlg.h"
#include <qboxlayout.h>
#include <qformlayout.h>
#include <qgroupbox.h>

#include "CamClient.h"

StreamsDlg::StreamsDlg(QWidget *parent)
    : QDialog(parent, Qt::WindowCloseButtonHint | Qt::WindowTitleHint)
{
	layoutWindow();
	setWindowTitle("Streams configuration");
	connect(m_buttons, SIGNAL(accepted()), this, SLOT(onOk()));
    connect(m_buttons, SIGNAL(rejected()), this, SLOT(reject()));
}

void StreamsDlg::onOk()
{
	bool changed = false;

	QSettings *settings = SyntroUtils::getSettings();

	settings->beginGroup(CAMCLIENT_STREAM_GROUP);

	if (m_highRateMinInterval->text() != settings->value(CAMCLIENT_HIGHRATEVIDEO_MININTERVAL).toString()) {
        settings->setValue(CAMCLIENT_HIGHRATEVIDEO_MININTERVAL, m_highRateMinInterval->text());
		changed = true;
	}

	if (m_highRateMaxInterval->text() != settings->value(CAMCLIENT_HIGHRATEVIDEO_MAXINTERVAL).toString()) {
        settings->setValue(CAMCLIENT_HIGHRATEVIDEO_MAXINTERVAL, m_highRateMaxInterval->text());
		changed = true;
	}

	if (m_highRateNullInterval->text() != settings->value(CAMCLIENT_HIGHRATEVIDEO_NULLINTERVAL).toString()) {
        settings->setValue(CAMCLIENT_HIGHRATEVIDEO_NULLINTERVAL, m_highRateNullInterval->text());
		changed = true;
	}

	if ((m_generateLowRate->checkState() == Qt::Checked) != settings->value(CAMCLIENT_GENERATE_LOWRATE).toBool()) {
		settings->setValue(CAMCLIENT_GENERATE_LOWRATE, m_generateLowRate->checkState() == Qt::Checked);
		changed = true;
	}

    if (m_highRateVideoCompression->text() != settings->value(CAMCLIENT_GS_VIDEO_HIGHRATE).toString()) {
        settings->setValue(CAMCLIENT_GS_VIDEO_HIGHRATE, m_highRateVideoCompression->text());
        changed = true;
    }

    if (m_highRateAudioCompression->text() != settings->value(CAMCLIENT_GS_AUDIO_HIGHRATE).toString()) {
        settings->setValue(CAMCLIENT_GS_AUDIO_HIGHRATE, m_highRateAudioCompression->text());
        changed = true;
    }


    if (m_lowRateVideoCompression->text() != settings->value(CAMCLIENT_GS_VIDEO_LOWRATE).toString()) {
        settings->setValue(CAMCLIENT_GS_VIDEO_LOWRATE, m_lowRateVideoCompression->text());
        changed = true;
    }

    if (m_lowRateAudioCompression->text() != settings->value(CAMCLIENT_GS_AUDIO_LOWRATE).toString()) {
        settings->setValue(CAMCLIENT_GS_AUDIO_LOWRATE, m_lowRateAudioCompression->text());
        changed = true;
    }

    settings->endGroup();

    delete settings;

    if (changed)
		accept();
    else
        reject();
}

void StreamsDlg::lowRateStateChange(int state)
{
    enableLowRate(state == Qt::Checked);
}

void StreamsDlg::enableLowRate(bool enable)
{
    m_lowRateVideoCompression->setDisabled(!enable);
    m_lowRateAudioCompression->setDisabled(!enable);
}

void StreamsDlg::layoutWindow()
{
	QSettings *settings = SyntroUtils::getSettings();

	settings->beginGroup(CAMCLIENT_STREAM_GROUP);

	QVBoxLayout *centralLayout = new QVBoxLayout(this);
	
    QFormLayout *formLayout = new QFormLayout;

	m_highRateMinInterval = new QLineEdit(this);
    m_highRateMinInterval->setMaximumWidth(80);
	formLayout->addRow(tr("High rate min interval (mS)"), m_highRateMinInterval);
	m_highRateMinInterval->setText(settings->value(CAMCLIENT_HIGHRATEVIDEO_MININTERVAL).toString());
	m_highRateMinInterval->setValidator(new QIntValidator(33, 10000));

	m_highRateMaxInterval = new QLineEdit(this);
    m_highRateMaxInterval->setMaximumWidth(80);
	formLayout->addRow(tr("High rate max interval (mS)"), m_highRateMaxInterval);
	m_highRateMaxInterval->setText(settings->value(CAMCLIENT_HIGHRATEVIDEO_MAXINTERVAL).toString());
	m_highRateMaxInterval->setValidator(new QIntValidator(10000, 60000));

    m_highRateNullInterval = new QLineEdit(this);
    m_highRateNullInterval->setMaximumWidth(80);
    formLayout->addRow(tr("High rate null interval (mS)"), m_highRateNullInterval);
    m_highRateNullInterval->setText(settings->value(CAMCLIENT_HIGHRATEVIDEO_NULLINTERVAL).toString());
    m_highRateNullInterval->setValidator(new QIntValidator(1000, 10000));

    m_highRateVideoCompression = new QLineEdit(this);
    m_highRateVideoCompression->setMaximumWidth(80);
    formLayout->addRow(tr("High rate video compression rate (bps)"), m_highRateVideoCompression);
    m_highRateVideoCompression->setText(settings->value(CAMCLIENT_GS_VIDEO_HIGHRATE).toString());
    m_highRateVideoCompression->setValidator(new QIntValidator(10000, 20000000));

    m_highRateAudioCompression = new QLineEdit(this);
    m_highRateAudioCompression->setMaximumWidth(80);
    formLayout->addRow(tr("High rate audio compression rate (bps)"), m_highRateAudioCompression);
    m_highRateAudioCompression->setText(settings->value(CAMCLIENT_GS_AUDIO_HIGHRATE).toString());
    m_highRateAudioCompression->setValidator(new QIntValidator(8000, 100000));

    QGroupBox *group = new QGroupBox("High Rate Parameters");
    group->setLayout(formLayout);
    centralLayout->addWidget(group);

    formLayout = new QFormLayout;

	m_generateLowRate = new QCheckBox(this);
	formLayout->addRow(tr("Generate low rate"), m_generateLowRate);
	m_generateLowRate->setCheckState(settings->value(CAMCLIENT_GENERATE_LOWRATE).toBool() ? Qt::Checked : Qt::Unchecked);
    centralLayout->addLayout(formLayout);    

    formLayout = new QFormLayout;

    group = new QGroupBox("Low Rate Parameters");
    group->setLayout(formLayout);
    centralLayout->addWidget(group);

    m_lowRateVideoCompression = new QLineEdit(this);
    m_lowRateVideoCompression->setMaximumWidth(60);
    formLayout->addRow(tr("Low rate video compression rate (bps)"), m_lowRateVideoCompression);
    m_lowRateVideoCompression->setText(settings->value(CAMCLIENT_GS_VIDEO_LOWRATE).toString());
    m_lowRateVideoCompression->setValidator(new QIntValidator(10000, 20000000));

    m_lowRateAudioCompression = new QLineEdit(this);
    m_lowRateAudioCompression->setMaximumWidth(60);
    formLayout->addRow(tr("Low rate audio compression rate (bps)"), m_lowRateAudioCompression);
    m_lowRateAudioCompression->setText(settings->value(CAMCLIENT_GS_AUDIO_LOWRATE).toString());
    m_lowRateAudioCompression->setValidator(new QIntValidator(8000, 100000));



	m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
	m_buttons->setCenterButtons(true);

    centralLayout->addSpacerItem(new QSpacerItem(20, 20));

	centralLayout->addWidget(m_buttons);

	connect(m_generateLowRate, SIGNAL(stateChanged(int)), this, SLOT(lowRateStateChange(int)));

    enableLowRate(m_generateLowRate->checkState() == Qt::Checked);

	settings->endGroup();

	delete settings;
}
