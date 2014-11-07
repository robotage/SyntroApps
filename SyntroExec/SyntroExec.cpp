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

#include "SyntroExec.h"
#include "SyntroAboutDlg.h"
#include "ConfigureDlg.h"

#define DEFAULT_ROW_HEIGHT 20


SyntroExec::SyntroExec()
	: QMainWindow()
{
	m_suppressSignals = true;
	ui.setupUi(this);

	connect(ui.actionExit, SIGNAL(triggered()), this, SLOT(close()));
	connect(ui.actionAbout, SIGNAL(triggered()), this, SLOT(onAbout()));

	layoutTable();
 
	restoreWindowState();

	SyntroUtils::syntroAppInit();

	setWindowTitle(QString("%1 - %2")
		.arg(SyntroUtils::getAppType())
		.arg(SyntroUtils::getAppName()));


	m_manager = new ComponentManager();
	connect(m_manager, SIGNAL(updateExecStatus(int, bool, QStringList)), 
		this, SLOT(updateExecStatus(int, bool, QStringList)), Qt::QueuedConnection);
	connect(this, SIGNAL(loadComponent(int)), m_manager, SLOT(loadComponent(int)), Qt::QueuedConnection);
	connect(m_manager, SIGNAL(running()), this, SLOT(managerRunning()));
	m_manager->resumeThread();

	m_suppressSignals = false;
}

void SyntroExec::managerRunning()
{
	for (int i = 0; i < SYNTRO_MAX_COMPONENTSPERDEVICE; i++)
		emit loadComponent(i);
}

void SyntroExec::layoutTable()
{
	QTableWidgetItem *item;

	int cellHeight = fontMetrics().lineSpacing() + 6;
	
	m_table = new QTableWidget(this);

	m_table->setColumnCount(SYNTROEXEC_COL_COUNT);
	m_table->setColumnWidth(SYNTROEXEC_COL_CONFIG, 80);
	m_table->setColumnWidth(SYNTROEXEC_COL_INUSE, 60);
	m_table->setColumnWidth(SYNTROEXEC_COL_APPNAME, 140);
	m_table->setColumnWidth(SYNTROEXEC_COL_UID, 140);
	m_table->setColumnWidth(SYNTROEXEC_COL_STATE, 140);

	m_table->verticalHeader()->setDefaultSectionSize(cellHeight);

    m_table->setHorizontalHeaderLabels(QStringList() << tr("") << tr("In use") << tr("App name") 
			<< tr("UID") << tr("Execution state"));
  
    setMinimumSize(620, 120);

    m_table->setSelectionMode(QAbstractItemView::NoSelection);
 
	for (int row = 0; row < SYNTRO_MAX_COMPONENTSPERDEVICE; row++) {
		m_table->insertRow(row);
		m_table->setRowHeight(row, cellHeight);
		m_table->setContentsMargins(5, 5, 5, 5);

		ExecButton *button = new ExecButton("Configure", this, row);
		m_table->setCellWidget(row, SYNTROEXEC_COL_CONFIG, button);
		if (row == INSTANCE_EXEC)
			button->setEnabled(false);
		else
			connect(button, SIGNAL(buttonClicked(int)), this, SLOT(buttonClicked(int)));

		m_useBox[row] = new ExecCheckBox(m_table, row);
		m_table->setCellWidget(row, SYNTROEXEC_COL_INUSE, m_useBox[row]);
		connect(m_useBox[row], SIGNAL(boxClicked(bool, int)), this, SLOT(boxClicked(bool, int)));
		
		for (int col = 2; col < SYNTROEXEC_COL_COUNT; col++) {
			item = new QTableWidgetItem();
			item->setTextAlignment(Qt::AlignLeft | Qt::AlignBottom);
			item->setFlags(Qt::ItemIsEnabled);
			item->setText("");
			m_table->setItem(row, col, item);
		}
	}
	setCentralWidget(m_table);
}


void SyntroExec::closeEvent(QCloseEvent *)
{
	m_manager->exitThread();
	saveWindowState();
	SyntroUtils::syntroAppExit();
}

void SyntroExec:: updateExecStatus(int index, bool inUse, QStringList list)
{
	m_suppressSignals = true;

	m_table->item(index, SYNTROEXEC_COL_APPNAME)->setText(list.at(0));
	m_useBox[index]->setCheckState(inUse ? Qt::Checked : Qt::Unchecked);
	m_table->item(index, SYNTROEXEC_COL_UID)->setText(list.at(1));
	m_table->item(index, SYNTROEXEC_COL_STATE)->setText(list.at(2));

	m_suppressSignals = false;
}

void SyntroExec::saveWindowState()
{
	QSettings *settings = SyntroUtils::getSettings();

	settings->beginGroup("ControlWindow");
	settings->setValue("Geometry", saveGeometry());
	settings->setValue("State", saveState());

	settings->beginWriteArray("Grid");
	for (int i = 0; i < m_table->columnCount(); i++) {
		settings->setArrayIndex(i);
		settings->setValue("columnWidth", m_table->columnWidth(i));
	}
	settings->endArray();
	settings->endGroup();
	
	delete settings;
}

void SyntroExec::restoreWindowState()
{
	QSettings *settings = SyntroUtils::getSettings();

	settings->beginGroup("ControlWindow");
	restoreGeometry(settings->value("Geometry").toByteArray());
	restoreState(settings->value("State").toByteArray());

	int count = settings->beginReadArray("Grid");
	for (int i = 0; i < count && i < m_table->columnCount(); i++) {
		settings->setArrayIndex(i);
		int width = settings->value("columnWidth").toInt();

		if (width > 0)
			m_table->setColumnWidth(i, width);
	}
	settings->endArray();
	settings->endGroup();
	
	delete settings;
}

void SyntroExec::onAbout()
{
	SyntroAbout *dlg = new SyntroAbout();
	dlg->show();
}

void SyntroExec::buttonClicked(int buttonId)
{
	if (m_suppressSignals)
		return;												// avoids thrashing when loading config

	if (buttonId == INSTANCE_EXEC)
		return;												// cannot configure Exec
	ConfigureDlg *dlg = new ConfigureDlg(this, buttonId);
	if (dlg->exec()) {										// need to update client with changes
		emit loadComponent(buttonId);
	}
}

void SyntroExec::boxClicked(bool, int boxId)
{
	QSettings *settings = SyntroUtils::getSettings();

	if (m_suppressSignals)
		return;												// avoids thrashing when loading config
	settings->beginWriteArray(SYNTROEXEC_PARAMS_COMPONENTS);
	settings->setArrayIndex(boxId);

	if (m_useBox[boxId]->checkState() == Qt::Checked)
		settings->setValue(SYNTROEXEC_PARAMS_INUSE, SYNTRO_PARAMS_TRUE);
	else
		settings->setValue(SYNTROEXEC_PARAMS_INUSE, SYNTRO_PARAMS_FALSE);
	settings->endArray();
	emit loadComponent(boxId);
	delete settings;
}



//----------------------------------------------------------
//
// ExecButton functions

ExecButton::ExecButton(const QString& text, QWidget *parent, int id)
	: QPushButton(text, parent) 
{
	m_id = id;
	connect(this, SIGNAL(clicked(bool)), this, SLOT(originalClicked(bool)));
};

void ExecButton::originalClicked(bool )
{
	emit buttonClicked(m_id);
}

//----------------------------------------------------------
//
// ExecCheckBox functions

ExecCheckBox::ExecCheckBox(QWidget *parent, int id)
	: QCheckBox(parent) 
{
	m_id = id;
	connect(this, SIGNAL(clicked(bool)), this, SLOT(originalClicked(bool)));
};

void ExecCheckBox::originalClicked(bool state)
{
	emit boxClicked(state, m_id);
}
