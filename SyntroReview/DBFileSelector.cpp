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


#include "DBFileSelector.h"
#include "SyntroLib.h"

DBFileSelector::DBFileSelector(QWidget *parent, QStringList fileList, QString filter, QString *filePath)
	: QDialog(parent, Qt::WindowCloseButtonHint | Qt::WindowTitleHint)
{
	QVBoxLayout *vertLayout = new QVBoxLayout(this);

	m_list = new QListWidget();
	vertLayout->addWidget(m_list);

	m_buttons = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Open);
	m_buttons->setCenterButtons(true);
	vertLayout->addWidget(m_buttons);

	m_filePath = filePath;
	if (filter == "") {
		m_filteredList = fileList;
	} else {
		m_filteredList.clear();
		for (int i = 0; i < fileList.size(); i++) {
			if (fileList.at(i).contains(filter)) {
				m_filteredList.append(fileList.at(i));
			}
		}
	}
	m_list->addItems(m_filteredList);
	resize(500, 400);
	connect(m_buttons, SIGNAL(accepted()), this, SLOT(onOpen()));
    connect(m_list, SIGNAL(itemDoubleClicked(QListWidgetItem *)), this, SLOT(onDoubleClick(QListWidgetItem *)));
}

void DBFileSelector::onOpen()
{
	QListWidgetItem *lwi = m_list->currentItem();

	if (lwi == NULL) {
		reject();
		return;
	}
	*m_filePath = lwi->text();
	accept();
}

void DBFileSelector::onDoubleClick(QListWidgetItem *item)
{
	*m_filePath = item->text();
	accept();
}
