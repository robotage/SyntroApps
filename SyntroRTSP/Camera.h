//
//  Copyright (c) 2014 Richard Barnett
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


#ifndef CAMERA_H
#define CAMERA_H

#include <QSize>
#include <QSettings>

#include "SyntroAV/SyntroAVDefs.h"

//		Settings keys

//	Stream group

#define	SYNTRORTSP_STREAM_GROUP				"StreamGroup"				// group for stream-related entries

#define	SYNTRORTSP_HIGHRATE_VIDEO_MININTERVAL	"HighRateVideoMinInterval"	// min frame interval in mS
#define	SYNTRORTSP_HIGHRATE_VIDEO_MAXINTERVAL	"HighRateVideoMaxInterval"	// max full frame interval in mS
#define	SYNTRORTSP_HIGHRATE_VIDEO_NULLINTERVAL "HighRateVideoNullInterval"	// max null frame interval in mS

#define	SYNTRORTSP_LOWRATE_VIDEO_MININTERVAL	"LowRateVideoMinInterval"	// min interval in mS
#define	SYNTRORTSP_LOWRATE_VIDEO_MAXINTERVAL	"LowRateVideoMaxInterval"	// max full frame interval in mS
#define	SYNTRORTSP_LOWRATE_VIDEO_NULLINTERVAL "LowRateVideoNullInterval"	// max null frame interval in mS

//  Motion group

#define SYNTRORTSP_MOTION_GROUP         "MotionGroup"

#define	SYNTRORTSP_MOTION_MIN_DELTA     "MotionMinDelta"                // min delta for image changed flag

// interval between frames checked for deltas in mS. 0 means never check - always send image

#define SYNTRORTSP_MOTION_DELTA_INTERVAL  "MotionDeltaInterval"

//  length of preroll. 0 turns off the feature

#define SYNTRORTSP_MOTION_PREROLL         "MotionPreroll"

// length of postroll. 0 turns off the feature

#define SYNTRORTSP_MOTION_POSTROLL        "MotionPostroll"

#define	SYNTRORTSP_CAMERA_GROUP			"CameraGroup"					// group for camera related entries

#define	SYNTRORTSP_CAMERA_IPADDRESS		"CameraIPAddress"				// camera IP address
#define	SYNTRORTSP_CAMERA_USERNAME		"CameraUsername"				// username
#define	SYNTRORTSP_CAMERA_PASSWORD		"CameraPassword"				// password
#define	SYNTRORTSP_CAMERA_TCPPORT       "CameraTcpport"					// TCP port to use

#endif // CAMERA_H
