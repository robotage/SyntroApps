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

#ifndef _SYNTRONAVDEFS_H
#define _SYNTRONAVDEFS_H

#include "SyntroDefs.h"

//  This is the structure sent in NAV messages. A NAV message can consist of a sequence
//  of these structures in order to optimize network traffic

typedef struct
{
    SYNTRO_UC8 timestamp;           // this is the timestamp of the sample with microsecond resolution
    SYNTRO_UC2 validFields;         // indicates which fields are valid (bit mask)
    SYNTRO_UC2 space;               // to keep on 32 bit boundary
    float fusionPose[3];            // the fused pose as Euler angles in radians
    float fusionQPose[4];           // the fused pose as a normalized quaternion
    float gyro[3];                  // the gyro outputs in radians per second
    float accel[3];                 // the accel outputs in gs
    float compass[3];               // the compass outputs in uT
    float pressure;                 // pressure in mbars
    float temperature;              // temperature in degrees C
    float humidity;                 // %RH
} SYNTRO_NAVDATA;


//  Valid field masks

#define SYNTRO_NAVDATA_VALID_FUSIONPOSE     0x0001
#define SYNTRO_NAVDATA_VALID_FUSIONQPOSE    0x0002
#define SYNTRO_NAVDATA_VALID_GYRO           0x0004
#define SYNTRO_NAVDATA_VALID_ACCEL          0x0008
#define SYNTRO_NAVDATA_VALID_COMPASS        0x0010
#define SYNTRO_NAVDATA_VALID_PRESSURE       0x0020
#define SYNTRO_NAVDATA_VALID_TEMPERATURE    0x0040
#define SYNTRO_NAVDATA_VALID_HUMIDITY       0x0080

#endif	// _SYNTRONAVDEFS_H

