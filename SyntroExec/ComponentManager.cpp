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
#include "ComponentManager.h"

ComponentManager::ComponentManager() 
	: SyntroThread(QString("ComponentManager"), QString(APPTYPE_EXEC))
{

#if defined(Q_OS_WIN64) || defined(Q_OS_WIN32)
	m_extension = ".exe";									// Windows needs the extension
#endif

}

ComponentManager::~ComponentManager()
{
	killTimer(m_timer);

	killComponents();

	for (int i = 0; i < SYNTRO_MAX_COMPONENTSPERDEVICE; i++) {
		delete m_components[i];
	}
}

void ComponentManager::initThread()
{
	QSettings *settings = SyntroUtils::getSettings();
	m_componentData.init(COMPTYPE_EXEC, settings->value(SYNTRO_PARAMS_HBINTERVAL).toInt());
	delete settings;

	for (int i = 0; i < SYNTRO_MAX_COMPONENTSPERDEVICE; i++) {
		m_components[i] = new ManagerComponent;
		m_components[i]->inUse = false;
		m_components[i]->instance = i;
		m_components[i]->processState = "Unused";
	}

    m_startMode = true;
	m_startTime = SyntroClock();							// indicate in start mode

	m_timer = startTimer(SYNTROEXEC_INTERVAL);

	m_components[INSTANCE_EXEC]->inUse = true;
	m_components[INSTANCE_EXEC]->appName = APPTYPE_EXEC;
	m_components[INSTANCE_EXEC]->processState = "Execing";
	m_components[INSTANCE_EXEC]->UID = m_componentData.getMyUID();		// form SyntroExec's UID
	SyntroUtils::convertIntToUC2(INSTANCE_EXEC, m_components[INSTANCE_EXEC]->UID.instance);
	updateStatus(m_components[INSTANCE_EXEC]);
}

void ComponentManager::killComponents()
{
	int i;

	//	try just terminating all components

	for (i = 0; i < SYNTRO_MAX_COMPONENTSPERDEVICE; i++) {
		if (i == INSTANCE_EXEC)
			continue;
		if (!m_components[i]->inUse)
			continue;
		logInfo(QString("Terminating ") + m_components[i]->appName);
		m_components[i]->process.terminate();
	}

	qint64 startTime = SyntroClock();

	// wait for up to five seconds to see if they have all died

	while (!SyntroUtils::syntroTimerExpired(SyntroClock(), startTime, 2000)) {
		for (i = 0; i < SYNTRO_MAX_COMPONENTSPERDEVICE; i++) {
			if (i == INSTANCE_EXEC)
				continue;
			if (!m_components[i]->inUse)
				continue;
			TRACE2("Component %s state %d", qPrintable(m_components[i]->appName), 
							m_components[i]->process.waitForFinished(0));
			if (!m_components[i]->process.waitForFinished(0))
				break;									// at least one still running
		}
		if (i == SYNTRO_MAX_COMPONENTSPERDEVICE)
			return;										// none are running so exit
		thread()->msleep(100);							// give the components a chance to terminate and test again
	}

	// kill off anything still running

	for (i = 0; i < SYNTRO_MAX_COMPONENTSPERDEVICE; i++) {
		if (i == INSTANCE_EXEC)
			continue;
		if (!m_components[i]->inUse)
			continue;
		if (!m_components[i]->process.waitForFinished(0)) {
			logInfo(QString("Killing ") + m_components[i]->appName);
			m_components[i]->process.kill();
		}
	}
}

void ComponentManager::loadComponent(int index)
{
	ManagerComponent *component;
	
	QSettings *settings = SyntroUtils::getSettings();

	settings->beginReadArray(SYNTROEXEC_PARAMS_COMPONENTS);
	settings->setArrayIndex(index);
	component = m_components[index];

	//	Instances 0 and 1 are special  so force values if that's what is being dealt with

	if (index == INSTANCE_CONTROL) {
		settings->setValue(SYNTROEXEC_PARAMS_APP_NAME, APPTYPE_CONTROL);
	}

	if (index == INSTANCE_EXEC) {
		settings->setValue(SYNTROEXEC_PARAMS_APP_NAME, COMPTYPE_EXEC);
		settings->endArray();			
		updateStatus(component);				
		delete settings;
		return;
	}

	if (component->inUse) {									// if in use, need to check if something critcal has changed
		if ((settings->value(SYNTROEXEC_PARAMS_INUSE).toString() == SYNTRO_PARAMS_FALSE) ||
			(settings->value(SYNTROEXEC_PARAMS_APP_NAME).toString() != component->appName) ||
			(settings->value(SYNTROEXEC_PARAMS_EXECUTABLE_DIRECTORY).toString() != component->executableDirectory) ||
			(settings->value(SYNTROEXEC_PARAMS_WORKING_DIRECTORY).toString() != component->workingDirectory) ||
			(settings->value(SYNTROEXEC_PARAMS_ADAPTOR).toString() != component->adaptor) ||
			(settings->value(SYNTROEXEC_PARAMS_INI_PATH).toString() != component->iniPath) ||
			((settings->value(SYNTROEXEC_PARAMS_CONSOLE_MODE).toString() == SYNTRO_PARAMS_TRUE) != component->consoleMode)) {

		// something critical changed - must kill existing.

			logInfo(QString("Killing app %1 by user command").arg(component->appName));
			component->process.terminate();
			thread()->msleep(1000);							// ugly but it gives the app a chance
			component->process.kill();
			component->inUse = false;
		} else {											// everything ok - maybe monitor hellos changed?
			settings->endArray();			
			updateStatus(component);				// make sure display updated
			delete settings;
			return;
		}
	}

	// Safe to read new data in now

	memset(&(component->UID), 0, sizeof(SYNTRO_UID));
	component->appName = settings->value(SYNTROEXEC_PARAMS_APP_NAME).toString();

	component->executableDirectory = settings->value(SYNTROEXEC_PARAMS_EXECUTABLE_DIRECTORY, "").toString();
	component->workingDirectory = settings->value(SYNTROEXEC_PARAMS_WORKING_DIRECTORY, "").toString();
	component->adaptor = settings->value(SYNTROEXEC_PARAMS_ADAPTOR, "").toString(); // first adaptor with valid address
	component->iniPath = settings->value(SYNTROEXEC_PARAMS_INI_PATH, "").toString(); // default .ini in working directory
	component->consoleMode =  settings->value(SYNTROEXEC_PARAMS_CONSOLE_MODE, "").toString() == 
								SYNTRO_PARAMS_TRUE;
	component->timeStarted = SyntroClock() - SYNTROEXEC_RESTART_INTERVAL; // force immediate attempt

	if (component->workingDirectory.length() == 0)
		component->workingDirectory = component->executableDirectory;

	component->UID = m_componentData.getMyUID();			// form the component's UID
	SyntroUtils::convertIntToUC2(component->instance, component->UID.instance);

	if ((settings->value(SYNTROEXEC_PARAMS_INUSE).toString() == SYNTRO_PARAMS_FALSE) ||
		(settings->value(SYNTROEXEC_PARAMS_APP_NAME).toString()) == "") {
		settings->endArray();		
		component->processState = "unused";
		updateStatus(component);					// make sure display updated
		delete settings;
		return;
	}

	// If get here, need to start the process

	if (component->iniPath.length() == 0) {
		logWarn(QString("Loaded %1 instance %2 with default .ini file").arg(component->appName).arg(component->instance));
	}
	else {
		logInfo(QString("Loaded %1 instance %2, .ini file %3")
			.arg(component->appName).arg(component->instance).arg(component->iniPath));
	}
	component->processState = "Starting...";
	component->inUse = true;
	settings->endArray();
	updateStatus(component);
	delete settings;
}

ManagerComponent *ComponentManager::getComponent(int index)
{
	if (index >= 0 && index < SYNTRO_MAX_COMPONENTSPERDEVICE)
		return m_components[index];

	return NULL;
}

void ComponentManager::managerBackground()
{
	ManagerComponent *component;
	int i;
	qint64 now;

	now = SyntroClock();

	if (m_startMode) {										// do special processing in start mode
		if (!SyntroUtils::syntroTimerExpired(now, m_startTime, SYNTROEXEC_STARTUP_DELAY))
			return;											// do nothing while waiting for timer
		// now exiting start mode - kill anything from this machine generating hellos
		m_startMode = false;
		startModeKillAll();
		m_components[INSTANCE_EXEC]->processState = "Execing";
		updateStatus(m_components[INSTANCE_EXEC]);
		return;
	}

	for (i = 0; i < SYNTRO_MAX_COMPONENTSPERDEVICE; i++) {
		component = m_components[i];
		if (component->instance == INSTANCE_EXEC)
			continue;										// don't process SyntroExec's dummy entry
		if (!component->inUse)
			continue;
		m_lock.lock();
		switch (component->process.state()) {
			case QProcess::NotRunning:
				component->processState = "Not running";
				if (SyntroUtils::syntroTimerExpired(now, component->timeStarted, SYNTROEXEC_RESTART_INTERVAL)) 
					startComponent(component);
				break;

			case QProcess::Starting:
				component->processState = "Starting";
				break;

			case QProcess::Running:
				component->processState = "Running";
				break;
		}
		m_lock.unlock();
		updateStatus(component);
	}
}

void ComponentManager::startComponent(ManagerComponent *component)
{
	QStringList arguments;
	QString argString;
	int i;

	component->process.setWorkingDirectory(component->workingDirectory);

	if (component->consoleMode) {
		arguments << QString("-c");
	}

	if (component->iniPath.length() != 0) {
		arguments << QString("-s") + component->iniPath;
	}

	if (component->adaptor.length() != 0) {
		arguments << QString("-a") + component->adaptor;
	}

	QString executable = component->executableDirectory + "/" + component->appName + m_extension;

	component->process.start(executable, arguments);

	for (i = 0; i < arguments.size(); i++)
		argString += arguments.at(i) + QString(" ");

	logInfo(QString("Starting %1 with arguments %2").arg(executable).arg(argString));

	component->timeStarted = SyntroClock();
}


void ComponentManager::startModeKillAll()
{
	int	i;

    //	make sure no instances of apps named in the .ini are running

	for (i = 0; i < SYNTRO_MAX_COMPONENTSPERDEVICE; i++) {
		if (!m_components[i]->inUse)
			continue;
		if (i == INSTANCE_EXEC)
			continue;										// don't kill SyntroExec!
		if (m_components[i]->appName.length() == 0)
			continue;
		findAndKillProcess(m_components[i]->appName + m_extension);
	}
}

void ComponentManager::timerEvent(QTimerEvent * /* event */)
{
	managerBackground();
}


void ComponentManager:: updateStatus(ManagerComponent *component)
{
	QMutexLocker locker(&m_lock);

	QStringList list;
	QString appName;
	QString uid;
	QString execState;

	if (component->inUse) {
		appName = component->appName;
		uid = SyntroUtils::displayUID(&component->UID);
		execState = component->processState;
	}
	list.append(appName);
	list.append(uid);
	list.append(execState);
	emit updateExecStatus(component->instance, component->inUse, list);
}

//----------------------------------------------------------
//
//	Platform specific process killing code

#if defined(Q_OS_WIN32)
#define _MSC_VER 1600
#ifndef WINVER				// Allow use of features specific to Windows XP or later.
#define WINVER 0x0501		// Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows XP or later.                   
#define _WIN32_WINNT 0x0501	// Change this to the appropriate value to target other versions of Windows.
#endif						

#ifndef _WIN32_WINDOWS		// Allow use of features specific to Windows 98 or later.
#define _WIN32_WINDOWS 0x0410 // Change this to the appropriate value to target Windows Me or later.
#endif
#define STRICT
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <TlHelp32.h>

void ComponentManager::findAndKillProcess(QString processName) 
{ 
    HANDLE processSnapshot = NULL; 
    PROCESSENTRY32 pe32 = {0}; 
    HANDLE hProcess; 
	char foundName[1000];
 
    //  Take a snapshot of all processes in the system. 

    processSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); 

    if (processSnapshot == INVALID_HANDLE_VALUE) 
	{
		logError("Failed to get process snapshot handle");
        return; 
	}
 
    //  Fill in the size of the structure before using it. 

    pe32.dwSize = sizeof(PROCESSENTRY32); 
 
    //  Walk the snapshot of the processes, and for each process, 
    //  display information. 

    if (Process32First(processSnapshot, &pe32)) 
    { 
        do 
        { 
			hProcess = OpenProcess (PROCESS_ALL_ACCESS, 
                 FALSE, pe32.th32ProcessID); 
			if (hProcess != NULL)
			{
				wcstombs(foundName, pe32.szExeFile, 1000);
				if (strcmp(qPrintable(processName), foundName) == 0) 
				{							// found it
					logInfo(QString("Start mode killing %1").arg(foundName));
					TerminateProcess(hProcess, 2);
				}
				CloseHandle (hProcess); 
			}
        } 
        while (Process32Next(processSnapshot, &pe32)); 
     } 
    CloseHandle (processSnapshot);
} 
#elif defined(Q_OS_LINUX)
#include <signal.h>
#include <sys/types.h>
#include <dirent.h>

void ComponentManager::findAndKillProcess(QString processName) 
{ 
	DIR 	*d = opendir("/proc");
	FILE 	*fp;
	pid_t 	pid;
	struct 	dirent *de;
	char 	*end;
	char 	cmdl[200], *name;

	while ((de = readdir(d)) != NULL)
	{
	    pid = strtoul(de->d_name, &end, 10);
	    if (*end != '\0')
	       continue; // skip this dir.
	    sprintf( cmdl, "/proc/%d/cmdline", pid);
	    fp = fopen(cmdl, "rt");
	    if (fp == NULL)
	    	continue;
	    name = fgets(cmdl, (int)sizeof(cmdl), fp);
	    if (name == NULL)
		continue;	
	    fclose(fp);
	    if (strstr(cmdl, qPrintable(processName)) != NULL)
	    {									// its a known component
			logInfo(QString("Start mode killing  %1").arg(processName));
			kill(pid, SIGKILL);
	    }
	}
	closedir(d); 
} 
#else

void ComponentManager::findAndKillProcess(QString ) 
{ 

} 
#endif
