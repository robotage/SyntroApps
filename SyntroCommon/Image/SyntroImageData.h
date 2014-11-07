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

#ifndef _SYNTROIMAGEDATA_H
#define _SYNTROIMAGEDATA_H

#include "SyntroDefs.h"
#include "SyntroUtils.h"

//  Note: angles are in degrees, altitude is in meters, speed is in meters per second.

//  Image format defs

#define SYNTRO_RECORD_IMAGE_BITMAP      0
#define SYNTRO_RECORD_IMAGE_JPEG        1
#define SYNTRO_RECORD_IMAGE_PNG         2
#define SYNTRO_RECORD_IMAGE_GIF         3
#define SYNTRO_RECORD_IMAGE_TIFF        4
#define SYNTRO_RECORD_IMAGE_END         5

//  Location data valid flags

#define SYNTRO_RECORD_IMAGE_POSE_VALID  1                   // roll, pitch and yaw
#define SYNTRO_RECORD_IMAGE_GPS_VALID   2                   // lat, long, alt, speed, bearing

//  This is the structure sent in image data messages

typedef struct
{
	SYNTRO_RECORD_HEADER recordHeader;						// the record type header
    unsigned char imageFormat;                              // the compression type
    unsigned char locationDataValid;                        // location data valid flags
    SYNTRO_UC2 spare;
    SYNTRO_UC2 width;                                       // image width
    SYNTRO_UC2 height;                                      // image height
    float roll;                                             // roll angle
    float pitch;                                            // pitch angle
    float yaw;                                              // yaw angle
    float latitude;                                         // latitude
    float longitude;                                        // longitude
    float altitude;                                         // altitude
    float speed;                                            // speed
    float bearing;                                          // bearing
    SYNTRO_UC4 imageSize;                                   // length of the image data
} SYNTRO_IMAGE_DATA;

class SyntroImageData 
{
public:
    SyntroImageData();
    void crackImageData(SYNTRO_IMAGE_DATA *imageData);
    void buildImageData(SYNTRO_IMAGE_DATA *imageData);

    unsigned char m_imageFormat;
    unsigned char m_locationDataValid;

    int m_width;
    int m_height;

    float m_roll;
    float m_pitch;
    float m_yaw;

    float m_latitude;
    float m_longitude;
    float m_altitude;
    float m_speed;
    float m_bearing;

    int m_imageSize;
};

#endif	// _SYNTROIMAGEDATA_H

