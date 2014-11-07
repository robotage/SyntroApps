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


#ifndef CONFIGUREDLG_H
#define CONFIGUREDLG_H

#include <qlabel.h>
#include <qdialog.h>
#include <qvalidator.h>
#include <qsettings.h>
#include <qlineedit.h>
#include <qcheckbox.h>
#include <qcombobox.h>

class ExecNameValidator : public QValidator
{
	Q_OBJECT;

	QValidator::State validate(QString &input, int &pos) const;
};


class ExecLabel : public QLabel
{
public:
	ExecLabel(const QString& title, const QString& value, bool directory);

protected:
	void mousePressEvent (QMouseEvent *ev);

private:
	bool m_directory;
	QString m_title;
};

class ConfigureDlg : public QDialog
{
	Q_OBJECT

friend class StoreLabel;

public:
	ConfigureDlg(QWidget *parent, int index);

public slots:
	void cancelButtonClick();
	void okButtonClick();

protected:
	QLabel *m_storePath;

private:
	void layoutWidgets();
	void loadCurrentValues();
	void populateAdaptors();

	int m_index;

	QPushButton *m_okButton;
	QPushButton *m_cancelButton;

	QLineEdit *m_appName;
	ExecLabel *m_executableDirectory;
	ExecLabel *m_workingDirectory;
	ExecLabel *m_iniPath;
	QComboBox *m_adaptor;
	QCheckBox *m_consoleMode;

	ExecNameValidator m_validator;
};

#endif // CONFIGUREDLG_H
