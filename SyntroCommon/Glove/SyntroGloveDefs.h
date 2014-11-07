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

#ifndef _SYNTROGLOVEDEFS_H
#define _SYNTROGLOVEDEFS_H

#include "SyntroDefs.h"

#define IMU_ROLE_PALM                   0                   // indicates the palm IMU
#define IMU_ROLE_THUMB                  1                   // indicates the thumb IMU
#define IMU_ROLE_FINGER                 2                   // indicates the finger IMU

#define IMU_ROLE_COUNT                  3                   // total number of IMUs

//  This is the structure sent in GLOVE messages. A GLOVE message can consist of a sequence
//  of these structures in order to optimize network traffic

typedef struct
{
    SYNTRO_UC8 timestamp;                                   // this is the timestamp of the sample with microsecond resolution
    float fusionQPosePalm[4];                               // the palm fused quaternion pose 
    float fusionQPoseThumb[4];                              // the thumb fused quaternion pose 
    float fusionQPoseFinger[4];                             // the finger fused quaternion pose 
} SYNTRO_GLOVEDATA;


#endif	// _SYNTROGLOVEDEFS_H

