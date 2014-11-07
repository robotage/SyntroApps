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

#include <qlabel.h>
#include <qboxlayout.h>
#include <qmessagebox.h>
#include <qfiledialog.h>
#include <qdebug.h>

#include "SyntroDefs.h"
#include "DirectoryEntry.h"
#include "CFSDialog.h"
#include "SyntroUtils.h"

CFSDialog::CFSDialog(QWidget *parent, QStringList directory) 
	: QDialog(parent, Qt::WindowCloseButtonHint | Qt::WindowTitleHint)
{
	QSettings *settings = SyntroUtils::getSettings();

	int count = settings->beginReadArray(SYNTRO_PARAMS_CFS_STORES);

	for (int i = 0; i < count; i++) {
		settings->setArrayIndex(i);
		m_currentCFSList.append(settings->value(SYNTRO_PARAMS_CFS_STORE).toString());
	}

	settings->endArray();

	delete settings;

	parseAvailableServices(directory);

	layoutWindow();

	connect(m_addButton, SIGNAL(clicked()), this, SLOT(onAddCFS()));
	connect(m_removeButton, SIGNAL(clicked()), this, SLOT(onRemoveCFS()));

	connect(m_okButton, SIGNAL(clicked()), this, SLOT(onOK()));
	connect(m_cancelButton, SIGNAL(clicked()), this, SLOT(reject()));

	connect(m_upButton, SIGNAL(clicked()), this, SLOT(onMoveUp()));
	connect(m_downButton, SIGNAL(clicked()), this, SLOT(onMoveDown()));

	connect(m_currentList, SIGNAL(itemSelectionChanged()), this, SLOT(onCurrentCFSSelectionChanged()));
 
	m_upButton->setEnabled(false);
	m_downButton->setEnabled(false);

	setWindowTitle("Cloud File System Selection");
} 

void CFSDialog::onOK()
{
	int settingsRow;
	bool changed = false;

	QSettings *settings = SyntroUtils::getSettings();

	if (!changed) {
		int	size = settings->beginReadArray(SYNTRO_PARAMS_CFS_STORES);

		changed = size != m_currentList->count();
		if (!changed) {
			for (int row = 0; row < size; row++) {
				settings->setArrayIndex(row);
				if (m_currentList->item(row)->text() != settings->value(SYNTRO_PARAMS_CFS_STORE).toString()) {
					changed = true;
					break;
				}
			}
		}
		settings->endArray();
	}
	if (changed) {
		settings->remove(SYNTRO_PARAMS_CFS_STORES);	// clear old entries
		settingsRow = 0;
		settings->beginWriteArray(SYNTRO_PARAMS_CFS_STORES);
		for (int row = 0; row < m_currentList->count(); row++) {
			settings->setArrayIndex(settingsRow);
			if (m_currentList->item(row)->text().length() > 0) {
				settings->setValue(SYNTRO_PARAMS_CFS_STORE, m_currentList->item(row)->text());
				settingsRow++;
			}
		}
		settings->endArray();
		delete settings;
		accept();
		return;
	}
	delete settings;
	reject();
}

void CFSDialog::parseAvailableServices(QStringList directory)
{
	DirectoryEntry de;
	QString servicePath;
	QString CFSName;
	QString CFSSource;

	for (int i = 0; i < directory.count(); i++) {
		de.setLine(directory.at(i));

		if (!de.isValid())
			continue;
	
		QStringList services = de.e2eServices();

		for (int i = 0; i < services.count(); i++) {
			servicePath = de.appName() + SYNTRO_SERVICEPATH_SEP + services.at(i);

			SyntroUtils::removeStreamNameFromPath(servicePath, CFSSource, CFSName);
 
			if (m_currentCFSList.contains(CFSSource))
				continue;

			if (CFSName == SYNTRO_STREAMNAME_CFS)
				m_availableCFSList.append(CFSSource);
		}
	}		
}

QStringList CFSDialog::newCFSList()
{
	QStringList list;

	for (int i = 0; i < m_currentList->count(); i++) 
		list << m_currentList->item(i)->text();

	return list;
}

void CFSDialog::onAddCFS()
{
	QList<QListWidgetItem *> selection = m_availableList->selectedItems();

	for (int i = 0; i < selection.count(); i++)
		m_currentList->addItem(new QListWidgetItem(selection.at(i)->text()));

	for (int i = m_availableList->count() - 1; i > -1; i--) {
		if (m_availableList->item(i)->isSelected()) {
			QListWidgetItem *item = m_availableList->takeItem(i);

			if (item)
				delete item;
		}
	}
}

void CFSDialog::onRemoveCFS()
{
	QList<QListWidgetItem *> selection = m_currentList->selectedItems();

	for (int i = 0; i < selection.count(); i++)
		m_availableList->addItem(new QListWidgetItem(selection.at(i)->text()));

	for (int i = m_currentList->count() - 1; i > -1; i--) {
		if (m_currentList->item(i)->isSelected()) {
			QListWidgetItem *item = m_currentList->takeItem(i);

			if (item)
				delete item;
		}
	}
}

void CFSDialog::onMoveUp()
{
	int count = m_currentList->count();

	for (int i = 1; i < count; i++) {
		if (m_currentList->item(i)->isSelected()) {
			QListWidgetItem *item = m_currentList->takeItem(i);

			if (item) {
				i--;

				m_currentList->insertItem(i, item);

				if (i > 0)
					m_currentList->setCurrentRow(i);
			}

			break;
		}
	}
}

void CFSDialog::onMoveDown()
{
	int count = m_currentList->count() - 1;

	for (int i = 0; i < count; i++) {
		if (m_currentList->item(i)->isSelected()) {
			QListWidgetItem *item = m_currentList->takeItem(i);

			if (item) {
				i++;

				m_currentList->insertItem(i, item);

				if (i < count)
					m_currentList->setCurrentRow(i);
			}

			break;
		}
	}
}

void CFSDialog::onCurrentCFSSelectionChanged()
{
	int row = -1;
	int count = m_currentList->count();

	if (count > 1) {
		for (int i = 0; i < count; i++) {
			if (m_currentList->item(i)->isSelected()) {
				if (row != -1) {
					// multiple selections
					row = -1;
					break;
				}

				row = i;
			}
		}
	}

	if (row == -1) {
		m_upButton->setEnabled(false);
		m_downButton->setEnabled(false);
	}
	else if (row == 0) {
		m_upButton->setEnabled(false);
		m_downButton->setEnabled(true);
	}
	else if (row == (count - 1)) {
		m_upButton->setEnabled(true);
		m_downButton->setEnabled(false);
	}
	else {
		m_upButton->setEnabled(true);
		m_downButton->setEnabled(true);
	}
}

void CFSDialog::layoutWindow()
{
	QVBoxLayout *vLayout = new QVBoxLayout();
	QHBoxLayout *hLayout = new QHBoxLayout();


	// current list
	QVBoxLayout *currentLayout = new QVBoxLayout;

	m_currentList = new QListWidget();
	m_currentList->setSelectionMode(QAbstractItemView::MultiSelection);
	m_currentList->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_currentList->setMaximumWidth(160);
	
	for (int i = 0; i < m_currentCFSList.count(); i++)
		m_currentList->addItem(new QListWidgetItem(m_currentCFSList.at(i)));

	m_currentList->setMovement(QListView::Snap);

	currentLayout->addStretch();	
	currentLayout->addWidget(new QLabel("Current CFS List"));
	currentLayout->addWidget(m_currentList, 1);
	currentLayout->addStretch();

	hLayout->addLayout(currentLayout);

	// add remove buttons
	QVBoxLayout *addRemoveLayout = new QVBoxLayout;

	m_addButton = new QPushButton("Add");
	m_removeButton = new QPushButton("Remove");
	m_upButton = new QPushButton("Up");
	m_downButton = new QPushButton("Down");
	
	addRemoveLayout->addStretch();
	addRemoveLayout->addWidget(m_addButton);
	addRemoveLayout->addWidget(m_removeButton);
	addRemoveLayout->addSpacerItem(new QSpacerItem(20, 20));
	addRemoveLayout->addWidget(m_upButton);
	addRemoveLayout->addWidget(m_downButton);
	addRemoveLayout->addStretch();

	hLayout->addLayout(addRemoveLayout);

	// available list
	QVBoxLayout *availableLayout = new QVBoxLayout;

	m_availableList = new QListWidget();
	m_availableList->setSelectionMode(QAbstractItemView::MultiSelection);
	m_availableList->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_availableList->setMaximumWidth(160);

	m_availableCFSList.sort();

	for (int i = 0; i < m_availableCFSList.count(); i++)
		m_availableList->addItem(new QListWidgetItem(m_availableCFSList.at(i)));

	availableLayout->addStretch();	
	availableLayout->addWidget(new QLabel("Available Cloud File Systems"));
	availableLayout->addWidget(m_availableList, 1);
	availableLayout->addStretch();

	hLayout->addLayout(availableLayout);

	vLayout->addLayout(hLayout, 1);
	vLayout->addStretch();

	QHBoxLayout *buttonLayout = new QHBoxLayout();

	m_okButton = new QPushButton("OK");
	m_cancelButton = new QPushButton("Cancel");

	buttonLayout->addStretch();
	buttonLayout->addWidget(m_okButton);
	buttonLayout->addWidget(m_cancelButton);
	buttonLayout->addStretch();

	vLayout->addSpacing(20);
	vLayout->addLayout(buttonLayout);
	setLayout(vLayout);
};
