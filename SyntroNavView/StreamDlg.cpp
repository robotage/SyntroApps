//
//  Copyright (c) 2014 richards-tech.
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

#include <qlabel.h>
#include <qboxlayout.h>
#include <qmessagebox.h>
#include <qdebug.h>

#include "SyntroDefs.h"
#include "DirectoryEntry.h"
#include "StreamDlg.h"
#include "SyntroUtils.h"

StreamDlg::StreamDlg(QWidget *parent, QStringList directory)
    : QDialog(parent, Qt::WindowCloseButtonHint | Qt::WindowTitleHint)
{
    parseAvailableServices(directory);

    layoutWindow();

    connect(m_buttons, SIGNAL(accepted()), this, SLOT(onOk()));
    connect(m_buttons, SIGNAL(rejected()), this, SLOT(reject()));
    setWindowTitle("Source Selection");
}

void StreamDlg::onOk()
{
    if (m_availableList->currentRow() != -1) {
        m_selection = m_availableList->currentItem()->text();
        accept();
    } else {
        m_selection = "";
        reject();
    }
 }

void StreamDlg::parseAvailableServices(QStringList directory)
{
    DirectoryEntry de;
    QString servicePath;
    QString streamName;
    QString streamSource;

    for (int i = 0; i < directory.count(); i++) {
        de.setLine(directory.at(i));

        if (!de.isValid())
            continue;

        QStringList services = de.multicastServices();

        for (int i = 0; i < services.count(); i++) {
            servicePath = de.appName() + SYNTRO_SERVICEPATH_SEP + services.at(i);

            SyntroUtils::removeStreamNameFromPath(servicePath, streamSource, streamName);

            if (streamName == SYNTRO_STREAMNAME_NAV)
                m_availableStreams.append(streamSource);
        }
    }
}

void StreamDlg::layoutWindow()
{
    QVBoxLayout *vLayout = new QVBoxLayout();

    m_availableList = new QListWidget();
    m_availableList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_availableList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_availableStreams.sort();

    vLayout->addWidget(new QLabel("Available Streams"));

    for (int i = 0; i < m_availableStreams.count(); i++)
        m_availableList->addItem(new QListWidgetItem(m_availableStreams.at(i)));

    vLayout->addWidget(m_availableList, 1);

    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    m_buttons->setCenterButtons(true);

    vLayout->addWidget(m_buttons);

    setLayout(vLayout);
}
