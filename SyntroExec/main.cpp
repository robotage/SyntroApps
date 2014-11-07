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

#include <QApplication>

#include "SyntroExec.h"
#include "ExecConsole.h"
#include "SyntroUtils.h"


int runGuiApp(int argc, char *argv[]);
int runConsoleApp(int argc, char *argv[]);
void loadSettings(QStringList arglist);


int main(int argc, char *argv[])
{
	if (SyntroUtils::checkConsoleModeFlag(argc, argv))
		return runConsoleApp(argc, argv);
	else
		return runGuiApp(argc, argv);
}

// look but do not modify argv

int runGuiApp(int argc, char *argv[])
{
	QApplication a(argc, argv);

	loadSettings(a.arguments());

	SyntroExec *w = new SyntroExec();

	w->show();

	return a.exec();
}

int runConsoleApp(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);

	loadSettings(a.arguments());

	ExecConsole cc(&a);

	return a.exec();
	return 1;
}

void loadSettings(QStringList arglist)
{
	SyntroUtils::loadStandardSettings(APPTYPE_EXEC, arglist);

	// app-specific part

	QSettings *settings = SyntroUtils::getSettings();

	int	size = settings->beginReadArray(SYNTROEXEC_PARAMS_COMPONENTS);
	settings->endArray();

	if (size == 0) {
		settings->beginWriteArray(SYNTROEXEC_PARAMS_COMPONENTS);
		settings->setArrayIndex(0);

	// by default, just configure a SyntroControl with default runtime arguments

		settings->setValue(SYNTROEXEC_PARAMS_INUSE, SYNTRO_PARAMS_TRUE);
		settings->setValue(SYNTROEXEC_PARAMS_APP_NAME, APPTYPE_CONTROL);
		settings->setValue(SYNTROEXEC_PARAMS_EXECUTABLE_DIRECTORY, "/usr/bin");
		settings->setValue(SYNTROEXEC_PARAMS_WORKING_DIRECTORY, "");
		settings->setValue(SYNTROEXEC_PARAMS_ADAPTOR, "");
		settings->setValue(SYNTROEXEC_PARAMS_INI_PATH, "");
		settings->setValue(SYNTROEXEC_PARAMS_CONSOLE_MODE, SYNTRO_PARAMS_FALSE);

	// and something for SyntroExec

		settings->setArrayIndex(1);
		settings->setValue(SYNTROEXEC_PARAMS_APP_NAME, APPTYPE_EXEC);

		settings->endArray();
	}

	delete settings;

	return;
}

