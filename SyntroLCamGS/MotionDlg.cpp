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

#include "MotionDlg.h"
#include <qboxlayout.h>
#include <qformlayout.h>
#include "CamClient.h"

MotionDlg::MotionDlg(QWidget *parent)
    : QDialog(parent, Qt::WindowCloseButtonHint | Qt::WindowTitleHint)
{
	layoutWindow();
	setWindowTitle("Motion configuration");
	connect(m_buttons, SIGNAL(accepted()), this, SLOT(onOk()));
    connect(m_buttons, SIGNAL(rejected()), this, SLOT(reject()));
}

void MotionDlg::onOk()
{
	bool changed = false;

	QSettings *settings = SyntroUtils::getSettings();

	settings->beginGroup(CAMCLIENT_MOTION_GROUP);

	if (m_minDelta->sliderPosition() != settings->value(CAMCLIENT_MOTION_MIN_DELTA).toInt()) {
		settings->setValue(CAMCLIENT_MOTION_MIN_DELTA, m_minDelta->sliderPosition());
		changed = true;
	}

	if (m_motionDelta->text() != settings->value(CAMCLIENT_MOTION_DELTA_INTERVAL).toString()) {
		settings->setValue(CAMCLIENT_MOTION_DELTA_INTERVAL, m_motionDelta->text());
		changed = true;
	}

	if (m_preroll->text() != settings->value(CAMCLIENT_MOTION_PREROLL).toString()) {
		settings->setValue(CAMCLIENT_MOTION_PREROLL, m_preroll->text());
		changed = true;
	}

	if (m_postroll->text() != settings->value(CAMCLIENT_MOTION_POSTROLL).toString()) {
		settings->setValue(CAMCLIENT_MOTION_POSTROLL, m_postroll->text());
		changed = true;
	}

	settings->endGroup();

    delete settings;

    if (changed)
		accept();
    else
        reject();
}

void MotionDlg::layoutWindow()
{
	QSettings *settings = SyntroUtils::getSettings();

	settings->beginGroup(CAMCLIENT_MOTION_GROUP);

	QVBoxLayout *centralLayout = new QVBoxLayout(this);
	
	QFormLayout *formLayout = new QFormLayout();

	QHBoxLayout *sliderLayout = new QHBoxLayout();
	m_minDelta = new QSlider(Qt::Horizontal, this);
	m_minDelta->setMinimumWidth(250);
	m_minDelta->setMaximumWidth(250);
	m_minDelta->setRange(0, 4000);
	m_minDelta->setTickInterval(500);
	m_minDelta->setTickPosition(QSlider::TicksBelow);
	m_minDeltaValue = new QLabel(this);
	m_minDeltaValue->setMaximumWidth(70);
	m_minDeltaValue->setFrameStyle(QFrame::StyledPanel);
	sliderLayout->addWidget(m_minDeltaValue);
	sliderLayout->addWidget(m_minDelta);
	m_minDelta->setSliderPosition(settings->value(CAMCLIENT_MOTION_MIN_DELTA).toInt());
	m_minDeltaValue->setText(settings->value(CAMCLIENT_MOTION_MIN_DELTA).toString());
	connect(m_minDelta, SIGNAL(sliderMoved(int)), this, SLOT(sliderMoved(int)));

    formLayout->addRow(tr("Min image change delta"), sliderLayout);

	m_motionDelta = new QLineEdit(this);
	m_motionDelta->setMaximumWidth(60);
    formLayout->addRow(tr("Motion check interval (mS) - 0 disables motion detection"), m_motionDelta);
	m_motionDelta->setText(settings->value(CAMCLIENT_MOTION_DELTA_INTERVAL).toString());
	m_motionDelta->setValidator(new QIntValidator(33, 10000));

	m_preroll = new QLineEdit(this);
	m_preroll->setMaximumWidth(60);
	formLayout->addRow(tr("Preroll time (mS)"), m_preroll);
	m_preroll->setText(settings->value(CAMCLIENT_MOTION_PREROLL).toString());
	m_preroll->setValidator(new QIntValidator(200, 10000));

	m_postroll = new QLineEdit(this);
	m_postroll->setMaximumWidth(60);
	formLayout->addRow(tr("Postroll time (mS)"), m_postroll);
	m_postroll->setText(settings->value(CAMCLIENT_MOTION_POSTROLL).toString());
	m_postroll->setValidator(new QIntValidator(200, 10000));

	centralLayout->addLayout(formLayout);
    centralLayout->addSpacerItem(new QSpacerItem(20, 20));

	m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
	m_buttons->setCenterButtons(true);

	centralLayout->addWidget(m_buttons);
	settings->endGroup();

	delete settings;
}

void MotionDlg::sliderMoved(int pos)
{
	m_minDeltaValue->setText(QString::number(pos));
}
