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


#include <QFormLayout>
#include <QFileDialog>
#include <QMessageBox>
#include "ConfigureDlg.h"
#include "SyntroExec.h"


ConfigureDlg::ConfigureDlg(QWidget *parent, int index)
	: QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint)
{
	m_index = index;
	layoutWidgets();
	connect(m_cancelButton, SIGNAL(clicked()), this, SLOT(cancelButtonClick()));
	connect(m_okButton, SIGNAL(clicked()), this, SLOT(okButtonClick()));
	loadCurrentValues();
}


void ConfigureDlg::okButtonClick()
{
	QSettings *settings = SyntroUtils::getSettings();

	settings->beginWriteArray(SYNTROEXEC_PARAMS_COMPONENTS);
	settings->setArrayIndex(m_index);

	settings->setValue(SYNTROEXEC_PARAMS_APP_NAME, m_appName->text());
	settings->setValue(SYNTROEXEC_PARAMS_EXECUTABLE_DIRECTORY, m_executableDirectory->text());
	settings->setValue(SYNTROEXEC_PARAMS_WORKING_DIRECTORY, m_workingDirectory->text());

	if (m_adaptor->currentText() == "<any>")
		settings->setValue(SYNTROEXEC_PARAMS_ADAPTOR, "");
	else
		settings->setValue(SYNTROEXEC_PARAMS_ADAPTOR, m_adaptor->currentText());
	
	settings->setValue(SYNTROEXEC_PARAMS_INI_PATH, m_iniPath->text());

	settings->setValue(SYNTROEXEC_PARAMS_CONSOLE_MODE, 
		m_consoleMode->checkState() == Qt::Checked ? SYNTRO_PARAMS_TRUE : SYNTRO_PARAMS_FALSE);

	settings->endArray();
	
	delete settings;
	
	accept();
}

void ConfigureDlg::cancelButtonClick()
{
	reject();
}

void ConfigureDlg::loadCurrentValues()
{
	QSettings *settings = SyntroUtils::getSettings();

	settings->beginReadArray(SYNTROEXEC_PARAMS_COMPONENTS);
	settings->setArrayIndex(m_index);

	m_appName->setText(settings->value(SYNTROEXEC_PARAMS_APP_NAME).toString());
	m_executableDirectory->setText(settings->value(SYNTROEXEC_PARAMS_EXECUTABLE_DIRECTORY).toString());
	m_workingDirectory->setText(settings->value(SYNTROEXEC_PARAMS_WORKING_DIRECTORY).toString());
	m_iniPath->setText(settings->value(SYNTROEXEC_PARAMS_INI_PATH).toString());

	m_consoleMode->setCheckState(settings->value(SYNTROEXEC_PARAMS_CONSOLE_MODE).toString() == 
					SYNTRO_PARAMS_TRUE ? Qt::Checked : Qt::Unchecked);

	int findIndex = m_adaptor->findText(settings->value(SYNTROEXEC_PARAMS_ADAPTOR).toString());
	if (findIndex != -1)
		m_adaptor->setCurrentIndex(findIndex);
	else
		m_adaptor->setCurrentIndex(0);

	settings->endArray();

	delete settings;
}

void ConfigureDlg::layoutWidgets()
{
	QFormLayout *formLayout = new QFormLayout;

	QHBoxLayout *e = new QHBoxLayout;
	e->addStretch();
	formLayout->addRow(new QLabel("Application start configuration"), e);

	m_appName = new QLineEdit;
	m_appName->setMinimumWidth(200);
	m_appName->setValidator(&m_validator);
	if (m_index == INSTANCE_CONTROL)
		m_appName->setReadOnly(true);
	formLayout->addRow(new QLabel("App name"), m_appName);
	m_appName->setToolTip("The name of the app. This is the executable name \n(without the .exe extension in the case of Windows).");

	m_executableDirectory = new ExecLabel("Executable directory", "", true);
	formLayout->addRow(new QLabel("Executable directory"), m_executableDirectory);
	m_executableDirectory->setToolTip("The directory that contains the executable.\nIf empty, use SyntroExec's working directory.");

	m_workingDirectory = new ExecLabel("Working directory", "", true);
	formLayout->addRow(new QLabel("Working directory"), m_workingDirectory);
	m_workingDirectory->setToolTip("The directory where the app should execute.\nIf empty, use the executable directory.");

	m_iniPath = new ExecLabel("Settings file", "", false);
	formLayout->addRow(new QLabel("Settings file"), m_iniPath);
	m_iniPath->setToolTip("The path to the settings file (including the filename).\nIf empty, use the app's default .ini in the working directory.");

	m_adaptor = new QComboBox;
	m_adaptor->setEditable(false);
	populateAdaptors();
	QHBoxLayout *a = new QHBoxLayout;
	a->addWidget(m_adaptor);
	a->addStretch();
	formLayout->addRow(new QLabel("Ethernet adaptor"), a);

	m_consoleMode = new QCheckBox;
	formLayout->addRow(new QLabel("Console mode"), m_consoleMode);

	QHBoxLayout *buttonLayout = new QHBoxLayout;

	m_okButton = new QPushButton("Ok");
	m_cancelButton = new QPushButton("Cancel");

	buttonLayout->addStretch();
	buttonLayout->addWidget(m_okButton);
	buttonLayout->addWidget(m_cancelButton);
	buttonLayout->addStretch();

	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addLayout(formLayout);
	mainLayout->addSpacing(20);
	mainLayout->addLayout(buttonLayout);
	setLayout(mainLayout);

	setWindowTitle("Configure application entry");
}

void ConfigureDlg::populateAdaptors()
{
	QNetworkInterface		cInterface;
	int index;

	m_adaptor->insertItem(0, "<any>");
	index = 1;
	QList<QNetworkInterface> ani = QNetworkInterface::allInterfaces();
	foreach (cInterface, ani)
		m_adaptor->insertItem(index++, cInterface.humanReadableName());
}


QValidator::State ExecNameValidator::validate(QString &input, int &pos) const
{
	if (pos == 0)
		return QValidator::Acceptable;						// empty string ok

	if (SyntroUtils::isReservedNameCharacter(input.at(pos-1).toLatin1()))
		return QValidator::Invalid;

	if (pos >= SYNTRO_MAX_NAME)
		return QValidator::Invalid;

	if (input == COMPTYPE_EXEC)
		return QValidator::Invalid;

	if (input == COMPTYPE_CONTROL)
		return QValidator::Invalid;

	return QValidator::Acceptable;
}


//----------------------------------------------------------
//
//	ExecLabel functions

ExecLabel::ExecLabel(const QString& title, const QString& value, bool directory) : QLabel(value)
{
	m_directory = directory;
	m_title = title;
	setFrameStyle(QFrame::Sunken | QFrame::Panel);
	setMinimumWidth(200);
}

void ExecLabel::mousePressEvent (QMouseEvent *)
{
	QFileDialog *fileDialog;
	QMessageBox messageBox;

	fileDialog = new QFileDialog(this, m_title + " - press Cancel for option to clear field");
	if (m_directory)
		fileDialog->setFileMode(QFileDialog::DirectoryOnly);
	fileDialog->selectFile(text());
	if (fileDialog->exec()) {
		setText(fileDialog->selectedFiles().at(0));
	} else {
		messageBox.setText("Do you want to clear this field?");
		messageBox.setIcon(QMessageBox::Question);
		messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
		if (messageBox.exec() == QMessageBox::Yes)
			setText("");
	}
}
