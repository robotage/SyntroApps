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


#ifndef VIEWCLIENT_H
#define VIEWCLIENT_H

#include "SyntroLib.h"
#include "SyntroNavDefs.h"

#include "RTIMULib.h"

Q_DECLARE_METATYPE(RTIMU_DATA);

class ViewClient : public Endpoint
{
	Q_OBJECT

public:
    ViewClient();

public slots:
    void newSource(const QString source);
	void requestDir();

signals:
    void newIMUData(const RTIMU_DATA& data);
	void dirResponse(QStringList directory);

protected:
    void appClientExit();
    void appClientReceiveMulticast(int servicePort, SYNTRO_EHEAD *multiCast, int totalLength);
    void appClientReceiveDirectory(QStringList);

private:
    int m_servicePort;
};

#endif // VIEWCLIENT_H

