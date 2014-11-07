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

#ifndef EXECCONSOLE_H
#define EXECCONSOLE_H

#include <qthread.h>
#include <QSettings>

class	ComponentManager;

class ExecConsole : public QThread
{
	Q_OBJECT

public:
	ExecConsole(QObject *parent);
	~ExecConsole() {}

public slots:
	void managerRunning();

signals:
	void loadComponent(int);								// load specified component

protected:
	void run();

private:
	void displayComponents();
	void displayManagedComponents();
	void showHelp();

	ComponentManager *m_manager;
};

#endif // EXECCONSOLE_H
