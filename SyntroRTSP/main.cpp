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


#include "SyntroRTSP.h"
#include <qapplication.h>
#include <QtDebug>
#include <QSettings>

#include "SyntroUtils.h"
#include "RTSPConsole.h"

#include <gst/gst.h>

int runGuiApp(int argc, char *argv[]);
int runConsoleApp(int argc, char *argv[]);
void loadSettings();

int main(int argc, char *argv[])
{
    if (SyntroUtils::checkConsoleModeFlag(argc, argv))
		return runConsoleApp(argc, argv);
	else
		return runGuiApp(argc, argv);
}

int runGuiApp(int argc, char *argv[])
{
	QApplication a(argc, argv);

	SyntroUtils::loadStandardSettings(PRODUCT_TYPE, a.arguments());
    gst_init (&argc, &argv);
	loadSettings();

    SyntroRTSP *w = new SyntroRTSP();

	w->show();

	return a.exec();
}

int runConsoleApp(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);

    bool daemonMode = SyntroUtils::checkDaemonModeFlag(argc, argv);

	SyntroUtils::loadStandardSettings(PRODUCT_TYPE, a.arguments());
    gst_init (&argc, &argv);
	loadSettings();

    RTSPConsole wc(daemonMode, &a);

	return a.exec();
	return 1;
}

void loadSettings()
{
	QSettings *settings = SyntroUtils::getSettings();

    settings->beginGroup(SYNTRORTSP_CAMERA_GROUP);

    if (!settings->contains(SYNTRORTSP_CAMERA_IPADDRESS))
        settings->setValue(SYNTRORTSP_CAMERA_IPADDRESS, "192.168.0.253");

    if (!settings->contains(SYNTRORTSP_CAMERA_USERNAME))
        settings->setValue(SYNTRORTSP_CAMERA_USERNAME, "admin");

    if (!settings->contains(SYNTRORTSP_CAMERA_PASSWORD))
        settings->setValue(SYNTRORTSP_CAMERA_PASSWORD, "default");

    if (!settings->contains(SYNTRORTSP_CAMERA_TCPPORT))
        settings->setValue(SYNTRORTSP_CAMERA_TCPPORT, 80);

	settings->endGroup();
	delete settings;
}
