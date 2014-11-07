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

#ifndef SYNTROEXEC_H
#define SYNTROEXEC_H

#include <QPushButton>
#include <QCheckBox>
#include <qtablewidget.h>

#include "ui_SyntroExec.h"
#include "SyntroLib.h"
#include "ComponentManager.h"

#define APPTYPE_EXEC	"SyntroExec"
#define COMPTYPE_EXEC	"Exec"
#define INSTANCE_EXEC	1

//		PARAMS keys

//	Note: The appName is something like "SyntroControl". Extensions like ".exe" for Windows must not be added
//	as SyntroExec will automatically add them.

#define	SYNTROEXEC_PARAMS_COMPONENTS		"components"	// the component settings array
#define	SYNTROEXEC_PARAMS_INUSE				"inUse"			// true/false in use flag
#define	SYNTROEXEC_PARAMS_APP_NAME			"appName"		// the Syntro app name
#define	SYNTROEXEC_PARAMS_EXECUTABLE_DIRECTORY "executableDirectory" // the directory containing the Syntro app executable
#define	SYNTROEXEC_PARAMS_WORKING_DIRECTORY	"workingDirectory" // the execution directory
#define	SYNTROEXEC_PARAMS_ADAPTOR			"adaptor"		// the network adaptor
#define	SYNTROEXEC_PARAMS_CONSOLE_MODE		"consoleMode"	// true for console mode
#define SYNTROEXEC_PARAMS_INI_PATH			"iniPath"		// the path name of the .ini file

//	Display columns

#define	SYNTROEXEC_COL_CONFIG			0					// configure entry
#define	SYNTROEXEC_COL_INUSE			1					// entry in use
#define	SYNTROEXEC_COL_APPNAME			2					// application name
#define	SYNTROEXEC_COL_UID				3					// UID
#define	SYNTROEXEC_COL_STATE			4					// execution state

#define SYNTROEXEC_COL_COUNT			5					// number of columns

//		Thread Messages

#define	SYNTROSERVER_ONCONNECT_MESSAGE		(SYNTRO_MSTART+0)
#define	SYNTROSERVER_ONACCEPT_MESSAGE		(SYNTRO_MSTART+1)
#define	SYNTROSERVER_ONCLOSE_MESSAGE		(SYNTRO_MSTART+2)
#define	SYNTROSERVER_ONRECEIVE_MESSAGE		(SYNTRO_MSTART+3)
#define	SYNTROSERVER_ONSEND_MESSAGE			(SYNTRO_MSTART+4)

//		Timer intervals

#define	SYNTROEXEC_INTERVAL					(SYNTRO_CLOCKS_PER_SEC / 2)	// SyntroExec run frequency

#define SYNTROEXEC_STARTUP_DELAY			(HELLO_INTERVAL * 2)		// enough time to find any apps that shouldn't be running
#define	SYNTROEXEC_HELLO_STARTUP_DELAY		(SYNTRO_CLOCKS_PER_SEC * 5) // how long a component has to send its first hello
#define	SYNTROEXEC_RESTART_INTERVAL			(SYNTRO_CLOCKS_PER_SEC * 5) // at most an attempt every five seconds

class ComponentManager;

class ExecButton : public QPushButton
{
	Q_OBJECT

public:
	ExecButton(const QString& text, QWidget *parent, int id);

public slots:
	void originalClicked(bool);

signals:
	void buttonClicked(int);

private:
	int m_id;
};

class ExecCheckBox : public QCheckBox
{
	Q_OBJECT

public:
	ExecCheckBox(QWidget *parent, int id);

public slots:
	void originalClicked(bool);

signals:
	void boxClicked(bool, int);

private:
	int m_id;
};



class SyntroExec : public QMainWindow
{
	Q_OBJECT

public:
	SyntroExec();
	~SyntroExec() {}

public slots:
	void onAbout();
	void updateExecStatus(int index, bool inUse, QStringList list);
	void boxClicked(bool, int);
	void buttonClicked(int);
	void managerRunning();

signals:
	void loadComponent(int);							// load specified component

private:
	void layoutTable();
	void closeEvent(QCloseEvent * event);
	void saveWindowState();
	void restoreWindowState();

	bool m_suppressSignals;
	ComponentManager *m_manager;

	QTableWidget *m_table;
	ExecCheckBox *m_useBox[SYNTRO_MAX_COMPONENTSPERDEVICE];

	Ui::SyntroExecClass ui;
};

#endif // SYNTROEXEC_H
