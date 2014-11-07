//
//  Copyright (c) 2014 Richard Barnett
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

#include "RTSPDlg.h"
#include <qboxlayout.h>
#include <qformlayout.h>
#include "Camera.h"

RTSPDlg::RTSPDlg(QWidget *parent)
	: QDialog(parent)
{
	layoutWindow();
	setWindowTitle("Camera configuration");
	connect(m_buttons, SIGNAL(accepted()), this, SLOT(onOk()));
    connect(m_buttons, SIGNAL(rejected()), this, SLOT(onCancel()));
}

RTSPDlg::~RTSPDlg()
{

}

void RTSPDlg::onOk()
{
	bool changed = false;

	QSettings *settings = SyntroUtils::getSettings();

    settings->beginGroup(SYNTRORTSP_CAMERA_GROUP);

    if (m_IPAddress->text() != settings->value(SYNTRORTSP_CAMERA_IPADDRESS).toString()) {
        settings->setValue(SYNTRORTSP_CAMERA_IPADDRESS, m_IPAddress->text());
		changed = true;
	}
    if (m_username->text() != settings->value(SYNTRORTSP_CAMERA_USERNAME).toString()) {
        settings->setValue(SYNTRORTSP_CAMERA_USERNAME, m_username->text());
		changed = true;
	}
    if (m_password->text() != settings->value(SYNTRORTSP_CAMERA_PASSWORD).toString()) {
        settings->setValue(SYNTRORTSP_CAMERA_PASSWORD, m_password->text());
		changed = true;
	}
    if (m_TCPPort->text() != settings->value(SYNTRORTSP_CAMERA_TCPPORT).toString()) {
        settings->setValue(SYNTRORTSP_CAMERA_TCPPORT, m_TCPPort->text());
		changed = true;
	}

	settings->endGroup();
	delete settings;
    if (changed)
		accept();
    else
        reject();
}

void RTSPDlg::onCancel()
{
	reject();
}

void RTSPDlg::layoutWindow()
{

    setModal(true);

	QSettings *settings = SyntroUtils::getSettings();

    settings->beginGroup(SYNTRORTSP_CAMERA_GROUP);

	QVBoxLayout *centralLayout = new QVBoxLayout(this);
	centralLayout->setSpacing(20);
	centralLayout->setContentsMargins(11, 11, 11, 11);
	
	QFormLayout *formLayout = new QFormLayout();
	formLayout->setSpacing(16);
	formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

	m_IPAddress = new QLineEdit(this);
	m_IPAddress->setMinimumWidth(300);
	formLayout->addRow(tr("IP address of camera"), m_IPAddress);
    m_IPAddress->setText(settings->value(SYNTRORTSP_CAMERA_IPADDRESS).toString());

	m_username = new QLineEdit(this);
	m_username->setMinimumWidth(300);
	formLayout->addRow(tr("Username"), m_username);
    m_username->setText(settings->value(SYNTRORTSP_CAMERA_USERNAME).toString());

	m_password = new QLineEdit(this);
	m_password->setMinimumWidth(300);
    m_password->setEchoMode(QLineEdit::Password);
	formLayout->addRow(tr("Password"), m_password);
    m_password->setText(settings->value(SYNTRORTSP_CAMERA_PASSWORD).toString());

	m_TCPPort = new QLineEdit(this);
	m_TCPPort->setMinimumWidth(300);
	formLayout->addRow(tr("TCP port"), m_TCPPort);
    m_TCPPort->setText(settings->value(SYNTRORTSP_CAMERA_TCPPORT).toString());
	m_TCPPort->setValidator(new QIntValidator(0, 65535));

	centralLayout->addLayout(formLayout);

	m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
	m_buttons->setCenterButtons(true);

	centralLayout->addWidget(m_buttons);

	settings->endGroup();
	delete settings;
}

