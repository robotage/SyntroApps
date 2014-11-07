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

#ifndef COMPONENTMANAGER_H
#define COMPONENTMANAGER_H

#include <QThread>
#include <QProcess>
#include "SyntroLib.h"
#include "SyntroComponentData.h"

class ManagerComponent
{
public:

//	params from .ini

	bool inUse;												// true if being used
	QString appName;										// the Syntro app name
	QString executableDirectory;							// the directory that contains the Syntro app executable
	QString workingDirectory;								// the working directory
	QString adaptor;										// the network adaptor name
	bool consoleMode;										// true if console mode
	QString iniPath;										// path to the .ini file

//	internally generated vars

	int instance;											// the components instance
	QProcess process;										// the actual process
	QString processState;									// current state of the process
	qint64 timeStarted;										// last time this component was started
	SYNTRO_UID UID;											// its UID
};

class ComponentManager : public SyntroThread
{
	Q_OBJECT

public:
	ComponentManager();
	~ComponentManager();

	ManagerComponent *getComponent(int index);				// gets a component object or null if out of range

	QMutex m_lock;											// for access to the m_component table

	SyntroComponentData m_componentData;

public slots:
	void loadComponent(int index);							// load specified component

signals:
	void updateExecStatus(int index, bool inUse, QStringList list);

protected:
	void initThread();
	void timerEvent(QTimerEvent *event);

private:
	void updateStatus(ManagerComponent *managerComponent);
	int m_timer;

	ManagerComponent *m_components[SYNTRO_MAX_COMPONENTSPERDEVICE]; // the component array		
	QString m_extension;									// OS-dependent executable extension
	qint64 m_startTime;										// used to time the period looking for old apps
	bool m_startMode;										// if in start mode

	void killComponents();
	void startModeKillAll();								// kill all running components at startup
	void findAndKillProcess(QString processName);			// finds and kills processes at startup
	void managerBackground();
	void startComponent(ManagerComponent *component);
};

#endif // COMPONENTMANAGER_H
