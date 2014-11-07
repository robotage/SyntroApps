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

#include "Camera.h"

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

    settings->beginGroup(SYNTRORTSP_STREAM_GROUP);

    if (m_highRateMinInterval->text() != settings->value(SYNTRORTSP_HIGHRATE_VIDEO_MININTERVAL).toString()) {
        settings->setValue(SYNTRORTSP_HIGHRATE_VIDEO_MININTERVAL, m_highRateMinInterval->text());
		changed = true;
	}

    if (m_highRateMaxInterval->text() != settings->value(SYNTRORTSP_HIGHRATE_VIDEO_MAXINTERVAL).toString()) {
        settings->setValue(SYNTRORTSP_HIGHRATE_VIDEO_MAXINTERVAL, m_highRateMaxInterval->text());
		changed = true;
	}

    if (m_highRateNullInterval->text() != settings->value(SYNTRORTSP_HIGHRATE_VIDEO_NULLINTERVAL).toString()) {
        settings->setValue(SYNTRORTSP_HIGHRATE_VIDEO_NULLINTERVAL, m_highRateNullInterval->text());
		changed = true;
	}

    if (m_lowRateMinInterval->text() != settings->value(SYNTRORTSP_LOWRATE_VIDEO_MININTERVAL).toString()) {
        settings->setValue(SYNTRORTSP_LOWRATE_VIDEO_MININTERVAL, m_lowRateMinInterval->text());
		changed = true;
	}

    if (m_lowRateMaxInterval->text() != settings->value(SYNTRORTSP_LOWRATE_VIDEO_MAXINTERVAL).toString()) {
        settings->setValue(SYNTRORTSP_LOWRATE_VIDEO_MAXINTERVAL, m_lowRateMaxInterval->text());
		changed = true;
	}

    if (m_lowRateNullInterval->text() != settings->value(SYNTRORTSP_LOWRATE_VIDEO_NULLINTERVAL).toString()) {
        settings->setValue(SYNTRORTSP_LOWRATE_VIDEO_NULLINTERVAL, m_lowRateNullInterval->text());
		changed = true;
	}

	settings->endGroup();

    delete settings;

    if (changed)
		accept();
    else
        reject();
}

void StreamsDlg::layoutWindow()
{
	QSettings *settings = SyntroUtils::getSettings();

    settings->beginGroup(SYNTRORTSP_STREAM_GROUP);

	QVBoxLayout *centralLayout = new QVBoxLayout(this);
	
    QFormLayout *formLayout = new QFormLayout;

	m_highRateMinInterval = new QLineEdit(this);
	m_highRateMinInterval->setMaximumWidth(60);
	formLayout->addRow(tr("High rate min interval (mS)"), m_highRateMinInterval);
    m_highRateMinInterval->setText(settings->value(SYNTRORTSP_HIGHRATE_VIDEO_MININTERVAL).toString());
	m_highRateMinInterval->setValidator(new QIntValidator(33, 10000));

	m_highRateMaxInterval = new QLineEdit(this);
	m_highRateMaxInterval->setMaximumWidth(60);
	formLayout->addRow(tr("High rate max interval (mS)"), m_highRateMaxInterval);
    m_highRateMaxInterval->setText(settings->value(SYNTRORTSP_HIGHRATE_VIDEO_MAXINTERVAL).toString());
	m_highRateMaxInterval->setValidator(new QIntValidator(10000, 60000));

	m_highRateNullInterval = new QLineEdit(this);
	m_highRateNullInterval->setMaximumWidth(60);
	formLayout->addRow(tr("High rate null interval (mS)"), m_highRateNullInterval);
    m_highRateNullInterval->setText(settings->value(SYNTRORTSP_HIGHRATE_VIDEO_NULLINTERVAL).toString());
	m_highRateNullInterval->setValidator(new QIntValidator(1000, 10000));

    QGroupBox *group = new QGroupBox("High Rate Parameters");
    group->setLayout(formLayout);
    centralLayout->addWidget(group);

    formLayout = new QFormLayout;

	m_lowRateMinInterval = new QLineEdit(this);
	m_lowRateMinInterval->setMaximumWidth(60);
	formLayout->addRow(tr("Low rate min interval (mS)"), m_lowRateMinInterval);
    m_lowRateMinInterval->setText(settings->value(SYNTRORTSP_LOWRATE_VIDEO_MININTERVAL).toString());
	m_lowRateMinInterval->setValidator(new QIntValidator(33, 10000));

	m_lowRateMaxInterval = new QLineEdit(this);
	m_lowRateMaxInterval->setMaximumWidth(60);
	formLayout->addRow(tr("Low rate max interval (mS)"), m_lowRateMaxInterval);
    m_lowRateMaxInterval->setText(settings->value(SYNTRORTSP_LOWRATE_VIDEO_MAXINTERVAL).toString());
	m_lowRateMaxInterval->setValidator(new QIntValidator(10000, 60000));

	m_lowRateNullInterval = new QLineEdit(this);
	m_lowRateNullInterval->setMaximumWidth(60);
	formLayout->addRow(tr("Low rate null interval (mS)"), m_lowRateNullInterval);
    m_lowRateNullInterval->setText(settings->value(SYNTRORTSP_LOWRATE_VIDEO_NULLINTERVAL).toString());
	m_lowRateNullInterval->setValidator(new QIntValidator(1000, 10000));

    group = new QGroupBox("Low Rate Parameters");
    group->setLayout(formLayout);
    centralLayout->addWidget(group);

	m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
	m_buttons->setCenterButtons(true);

    centralLayout->addSpacerItem(new QSpacerItem(20, 20));

	centralLayout->addWidget(m_buttons);

	settings->endGroup();

	delete settings;
}
