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

#include "SyntroImageData.h"

SyntroImageData::SyntroImageData()
{
    // set some sort of defaults
    m_imageFormat = SYNTRO_RECORD_IMAGE_JPEG;
    m_width = 640;
    m_height = 480;
    m_roll = m_pitch = m_yaw = 0;
    m_latitude = m_longitude = m_altitude = m_bearing = m_speed = 0;
    m_imageSize = 0;
    m_locationDataValid = 0;
}

void SyntroImageData::crackImageData(SYNTRO_IMAGE_DATA *imageData)
{
    m_imageFormat = imageData->imageFormat;
    m_locationDataValid = imageData->locationDataValid;

    m_width = SyntroUtils::convertUC2ToInt(imageData->width);
    m_height = SyntroUtils::convertUC2ToInt(imageData->height);

    if (m_locationDataValid & SYNTRO_RECORD_IMAGE_POSE_VALID) {
        m_roll = imageData->roll;
        m_pitch = imageData->pitch;
        m_yaw = imageData->yaw;
    } else {
        m_roll = m_pitch = m_yaw = 0;
    }

    if (m_locationDataValid & SYNTRO_RECORD_IMAGE_GPS_VALID) {
        m_latitude = imageData->latitude;
        m_longitude = imageData->longitude;
        m_altitude = imageData->altitude;
        m_speed = imageData->speed;
        m_bearing = imageData->bearing;
    } else {
        m_latitude = m_longitude = m_altitude = m_speed = m_bearing = 0;
    }
    m_imageSize = SyntroUtils::convertUC4ToInt(imageData->imageSize);
}

void SyntroImageData::buildImageData(SYNTRO_IMAGE_DATA *imageData)
{
    imageData->imageFormat = m_imageFormat;
    imageData->locationDataValid = m_locationDataValid;

    SyntroUtils::convertIntToUC2(m_width, imageData->width);
    SyntroUtils::convertIntToUC2(m_height, imageData->height);

    imageData->roll = m_roll;
    imageData->pitch = m_pitch;
    imageData->yaw = m_yaw;

    imageData->latitude = m_latitude;
    imageData->longitude = m_longitude;
    imageData->altitude = m_altitude;
    imageData->speed = m_speed;
    imageData->bearing = m_bearing;
}