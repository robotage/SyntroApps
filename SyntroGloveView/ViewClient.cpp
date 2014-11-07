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

#include "ViewClient.h"
#include "SyntroGloveDefs.h"

#define	VIEWCLIENT_BACKGROUND_INTERVAL (SYNTRO_CLOCKS_PER_SEC/100)


ViewClient::ViewClient()
    : Endpoint(VIEWCLIENT_BACKGROUND_INTERVAL, "ViewClient")
{
    m_servicePort = -1;
}

void ViewClient::requestDir()
{
    requestDirectory();
}

void ViewClient::appClientReceiveDirectory(QStringList directory)
{
    emit dirResponse(directory);
}

void ViewClient::appClientExit()
{
    if (m_servicePort != -1)
        clientRemoveService(m_servicePort);
    m_servicePort = -1;
}

void ViewClient::newSource(const QString source)
{
    if (m_servicePort != -1)
        clientRemoveService(m_servicePort);

    m_servicePort = clientAddService(SyntroUtils::insertStreamNameInPath(source, SYNTRO_STREAMNAME_GLOVE),
                                        SERVICETYPE_MULTICAST, false);
}

void ViewClient::appClientReceiveMulticast(int servicePort, SYNTRO_EHEAD *multiCast, int totalLength)
{
    SYNTRO_GLOVEDATA *data;
    RTQuaternion palmQuat;
    RTQuaternion thumbQuat;
    RTQuaternion fingerQuat;

    SYNTRO_RECORD_HEADER *head = (SYNTRO_RECORD_HEADER *)(multiCast + 1);
    int recordType = SyntroUtils::convertUC2ToUInt(head->type);

    if (recordType != SYNTRO_RECORD_TYPE_GLOVE) {
        qDebug() << "Expecting nav record, received record type" << recordType;
    } else {
        data = (SYNTRO_GLOVEDATA *)(head + 1);

        while (totalLength > (int)sizeof(SYNTRO_GLOVEDATA)) {
            palmQuat.setScalar(data->fusionQPosePalm[0]);
            palmQuat.setX(data->fusionQPosePalm[1]);
            palmQuat.setY(data->fusionQPosePalm[2]);
            palmQuat.setZ(data->fusionQPosePalm[3]);

            thumbQuat.setScalar(data->fusionQPoseThumb[0]);
            thumbQuat.setX(data->fusionQPoseThumb[1]);
            thumbQuat.setY(data->fusionQPoseThumb[2]);
            thumbQuat.setZ(data->fusionQPoseThumb[3]);

            fingerQuat.setScalar(data->fusionQPoseFinger[0]);
            fingerQuat.setX(data->fusionQPoseFinger[1]);
            fingerQuat.setY(data->fusionQPoseFinger[2]);
            fingerQuat.setZ(data->fusionQPoseFinger[3]);

            emit newIMUData(palmQuat, thumbQuat, fingerQuat);
            totalLength -= sizeof(SYNTRO_GLOVEDATA);
            data++;
        }
    }
    clientSendMulticastAck(servicePort);
    free(multiCast);
}
