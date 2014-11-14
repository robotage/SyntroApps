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

#include "CameraDlg.h"
#include <qboxlayout.h>
#include <qformlayout.h>
#include <qdir.h>
#include <qalgorithms.h>

#include "CamClient.h"

CameraDlg::CameraDlg(QWidget *parent)
    : QDialog(parent, Qt::WindowCloseButtonHint | Qt::WindowTitleHint)
{
	layoutWindow();
	setWindowTitle("Camera configuration");
	connect(m_buttons, SIGNAL(accepted()), this, SLOT(onOk()));
    connect(m_buttons, SIGNAL(rejected()), this, SLOT(reject()));
}

void CameraDlg::onOk()
{
	bool changed = false;

	QSettings *settings = SyntroUtils::getSettings();

	settings->beginGroup(CAMERA_GROUP);

    int index = m_index->currentIndex();

    if (index != settings->value(CAMERA_CAMERA).toInt()) {
        settings->setValue(CAMERA_CAMERA, index);
        changed = true;
     }

	if (m_width->text() != settings->value(CAMERA_WIDTH).toString()) {
		settings->setValue(CAMERA_WIDTH, m_width->text());
		changed = true;
	}

	if (m_height->text() != settings->value(CAMERA_HEIGHT).toString()) {
		settings->setValue(CAMERA_HEIGHT, m_height->text());
		changed = true;
	}

	if (m_rate->text() != settings->value(CAMERA_FRAMERATE).toString()) {
		settings->setValue(CAMERA_FRAMERATE, m_rate->text());
		changed = true;
	}

	settings->endGroup();

	delete settings;

    if (changed)
		accept();
    else
        reject();
}

void CameraDlg::layoutWindow()
{
	QSettings *settings = SyntroUtils::getSettings();

    settings->beginGroup(CAMERA_GROUP);

	QVBoxLayout *centralLayout = new QVBoxLayout(this);

	QFormLayout *formLayout = new QFormLayout();

    m_index = new QComboBox;
    m_index->setMaximumWidth(60);

    QList<int> deviceList = getVideoDeviceList(settings);
    int currentIndex = settings->value(CAMERA_CAMERA).toInt();

    for (int i = 0; i < deviceList.count(); i++) {
        m_index->addItem(QString::number(deviceList.at(i)));

        if (i == currentIndex)
            m_index->setCurrentIndex(i);
    }

    formLayout->addRow(tr("Camera"), m_index);

	m_width = new QLineEdit(this);
	m_width->setMaximumWidth(60);
	formLayout->addRow(tr("Frame width"), m_width);
	m_width->setText(settings->value(CAMERA_WIDTH).toString());
	m_width->setValidator(new QIntValidator(120, 1920));

	m_height = new QLineEdit(this);
	m_height->setMaximumWidth(60);
	formLayout->addRow(tr("Frame height"), m_height);
	m_height->setText(settings->value(CAMERA_HEIGHT).toString());
	m_height->setValidator(new QIntValidator(80, 1200));

	m_rate = new QLineEdit(this);
	m_rate->setMaximumWidth(60);
	formLayout->addRow(tr("Frame rate"), m_rate);
	m_rate->setText(settings->value(CAMERA_FRAMERATE).toString());
	m_rate->setValidator(new QIntValidator(1, 100));

	centralLayout->addLayout(formLayout);

    centralLayout->addSpacerItem(new QSpacerItem(20, 20));

	m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
	m_buttons->setCenterButtons(true);

	centralLayout->addWidget(m_buttons);

	settings->endGroup();
	delete settings;
}

QList<int> CameraDlg::getVideoDeviceList(QSettings *settings)
{
    QList<int> list;
    QDir dir;
    QStringList nameFilter;

    dir.setPath("/dev");

    nameFilter << "video*";

    QStringList sList = dir.entryList(nameFilter, QDir::System | QDir::Readable | QDir::Writable, QDir::Name);

    for (int i = 0; i < sList.count(); i++) {
        QString file = sList.at(i);

        // handle /dev/video0 first since zero is the error value
        if (file == "video0") {
            list.append(0);
        }
        else if (file.length() > 5) {
            int val = file.mid(5).toInt();

            if (val > 0)
                list.append(val);
        }
    }

    int userIndex = settings->value(CAMERA_CAMERA).toInt();

    if (!list.contains(userIndex))
        list.append(userIndex);

    qSort(list.begin(), list.end());

    return list;
}
