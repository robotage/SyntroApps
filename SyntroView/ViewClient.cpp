//
//  Copyright (c) 2014 Scott Ellis and Richard Barnett
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

#include "ViewClient.h"

#define	VIEWCLIENT_BACKGROUND_INTERVAL (SYNTRO_CLOCKS_PER_SEC/100)


ViewClient::ViewClient()
    : Endpoint(VIEWCLIENT_BACKGROUND_INTERVAL, "ViewClient")
{
}

void ViewClient::appClientConnected()
{
	emit clientConnected();
}

void ViewClient::appClientClosed()
{
	emit clientClosed();
}

void ViewClient::requestDir()
{
	requestDirectory();
}

void ViewClient::appClientReceiveDirectory(QStringList directory)
{
	emit dirResponse(directory);
}

void ViewClient::enableService(AVSource *avSource)
{
	QString service = SyntroUtils::insertStreamNameInPath(avSource->name(), SYNTRO_STREAMNAME_AVMUX);

	int servicePort = clientAddService(service, SERVICETYPE_MULTICAST, false);

	if (servicePort >= 0) {
		clientSetServiceDataPointer(servicePort, (void *)avSource);
		avSource->setServicePort(servicePort);
	}
}

void ViewClient::disableService(int servicePort)
{
	clientRemoveService(servicePort);
}

void ViewClient::appClientReceiveMulticast(int servicePort, SYNTRO_EHEAD *multiCast, int len)
{
	AVSource *avSource = reinterpret_cast<AVSource *>(clientGetServiceDataPointer(servicePort));

	if (avSource) {
		SYNTRO_RECORD_AVMUX *avmuxHeader = reinterpret_cast<SYNTRO_RECORD_AVMUX *>(multiCast + 1);
		int recordType = SyntroUtils::convertUC2ToUInt(avmuxHeader->recordHeader.type);

		if (recordType != SYNTRO_RECORD_TYPE_AVMUX) {
			qDebug() << "Expecting avmux record, received record type" << recordType;
		}
		else {
			avSource->setAVMuxData(QByteArray((const char *)avmuxHeader, len));  
			clientSendMulticastAck(servicePort);
		}
	}
	else {
		logWarn(QString("Multicast received to invalid port %1").arg(servicePort));
	}

	free(multiCast);
}
