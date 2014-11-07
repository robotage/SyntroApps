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

#include "ExecConsole.h"
#include "SyntroLib.h"
#include "ComponentManager.h"
#include "SyntroExec.h"

#ifdef WIN32
#include <conio.h>
#else
#include <termios.h>
#endif


ExecConsole::ExecConsole(QObject *parent)
    : QThread(parent)
{
	SyntroUtils::syntroAppInit();

	m_manager = new ComponentManager();

    connect(m_manager, SIGNAL(running()), this, SLOT(managerRunning()));
    connect(this, SIGNAL(loadComponent(int)), m_manager, SLOT(loadComponent(int)), Qt::QueuedConnection);

	m_manager->resumeThread();
    start();
}

void ExecConsole::managerRunning()
{
    for (int i = 0; i < SYNTRO_MAX_COMPONENTSPERDEVICE; i++)
        emit loadComponent(i);
}


void ExecConsole::displayManagedComponents()
{
	ManagerComponent *component;
	int	index;

	printf("\n%-5s %-18s %-17s %-10s\n", 
			"Inst", "App name", "UID", "State");
	printf("%-5s %-18s %-17s %-10s\n", 
			"----", "--------", "---", "-----");

	index = 0;
	while ((component = m_manager->getComponent(index++)) != NULL) {
		if (component->processState == "unused")
			continue;

        printf("%-5d %-18s %-17s %-10s\n",
			component->instance, 
			qPrintable(component->appName), 
			qPrintable(SyntroUtils::displayUID(&component->UID)), 
            qPrintable(component->processState));
	}
}

void ExecConsole::showHelp()
{
	printf("\nOptions are:\n\n");
	printf("  H - Display this help page\n");
	printf("  M - Display managed components\n");
	printf("  X - Exit\n");
}

void ExecConsole::run()
{
	bool mustExit;

#ifndef WIN32
        struct termios	ctty;

        tcgetattr(fileno(stdout), &ctty);
        ctty.c_lflag &= ~(ICANON);
        tcsetattr(fileno(stdout), TCSANOW, &ctty);
#endif

	mustExit = false;

	while (!mustExit) {

		printf("\nEnter option: ");

#ifdef WIN32
		switch (toupper(_getch()))
#else
        switch (toupper(getchar()))
#endif		
		{
		case 'H':
			showHelp();
			break;

		case 'M':
			displayManagedComponents();
			break;

		case 'X':
			printf("\nExiting\n");
			mustExit = true;
            emit m_manager->exitThread();
			((QCoreApplication *)parent())->exit();
			break;

		default:
			break;
		}
	}
}

