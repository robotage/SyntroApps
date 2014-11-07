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

    m_servicePort = clientAddService(SyntroUtils::insertStreamNameInPath(source, SYNTRO_STREAMNAME_NAV),
                                        SERVICETYPE_MULTICAST, false);
}

void ViewClient::appClientReceiveMulticast(int servicePort, SYNTRO_EHEAD *multiCast, int totalLength)
{
    SYNTRO_NAVDATA *data;
    int fieldsValid;
    RTIMU_DATA localData;

    SYNTRO_RECORD_HEADER *head = (SYNTRO_RECORD_HEADER *)(multiCast + 1);
    int recordType = SyntroUtils::convertUC2ToUInt(head->type);

    if (recordType != SYNTRO_RECORD_TYPE_NAV) {
        qDebug() << "Expecting nav record, received record type" << recordType;
    } else {
        data = (SYNTRO_NAVDATA *)(head + 1);

        while (totalLength > (int)sizeof(SYNTRO_NAVDATA)) {
            fieldsValid = SyntroUtils::convertUC2ToInt(data->validFields);

            if (fieldsValid & SYNTRO_NAVDATA_VALID_FUSIONPOSE) {
                localData.fusionPose.setX(data->fusionPose[0]);
                localData.fusionPose.setY(data->fusionPose[1]);
                localData.fusionPose.setZ(data->fusionPose[2]);
                localData.fusionPoseValid = true;
            } else {
                localData.fusionPoseValid = false;
            }

            if (fieldsValid & SYNTRO_NAVDATA_VALID_FUSIONQPOSE) {
                localData.fusionQPose.setScalar(data->fusionQPose[0]);
                localData.fusionQPose.setX(data->fusionQPose[1]);
                localData.fusionQPose.setY(data->fusionQPose[2]);
                localData.fusionQPose.setZ(data->fusionQPose[3]);
                localData.fusionQPoseValid = true;
            } else {
                localData.fusionQPoseValid = false;
            }

            if (fieldsValid & SYNTRO_NAVDATA_VALID_GYRO) {
                localData.gyro.setX(data->gyro[0]);
                localData.gyro.setY(data->gyro[1]);
                localData.gyro.setZ(data->gyro[2]);
                localData.gyroValid = true;
            } else {
                localData.gyroValid = false;
            }

            if (fieldsValid & SYNTRO_NAVDATA_VALID_ACCEL) {
                localData.accel.setX(data->accel[0]);
                localData.accel.setY(data->accel[1]);
                localData.accel.setZ(data->accel[2]);
                localData.accelValid = true;
            } else {
                localData.accelValid = false;
            }

            if (fieldsValid & SYNTRO_NAVDATA_VALID_COMPASS) {
                localData.compass.setX(data->compass[0]);
                localData.compass.setY(data->compass[1]);
                localData.compass.setZ(data->compass[2]);
                localData.compassValid = true;
            } else {
                localData.compassValid = false;
            }

            if (fieldsValid & SYNTRO_NAVDATA_VALID_PRESSURE) {
                localData.pressure = data->pressure;
                localData.pressureValid = true;
            } else {
                localData.pressureValid = false;
            }

            if (fieldsValid & SYNTRO_NAVDATA_VALID_TEMPERATURE) {
                localData.temperature = data->temperature;
                localData.temperatureValid = true;
            } else {
                localData.temperatureValid = false;
            }

            if (fieldsValid & SYNTRO_NAVDATA_VALID_HUMIDITY) {
                localData.humidity = data->humidity;
                localData.humidityValid = true;
            } else {
                localData.humidityValid = false;
            }

            localData.timestamp = SyntroUtils::convertUC8ToInt64(data->timestamp);

            emit newIMUData(localData);
            totalLength -= sizeof(SYNTRO_NAVDATA);
            data++;
        }
    }
    clientSendMulticastAck(servicePort);
    free(multiCast);
}
