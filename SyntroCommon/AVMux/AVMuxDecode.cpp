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

#include "AVMuxDecode.h"

#include <qimage.h>
#include <qbuffer.h>
#include <qbytearray.h>

AVMuxDecode::AVMuxDecode() 
	: SyntroThread("AVMuxDecode", "AVMuxDecode")
{
}

void AVMuxDecode::newAVMuxData(QByteArray data)
{
    if (data.length() < (int)sizeof(SYNTRO_RECORD_AVMUX))
		return;

    SYNTRO_RECORD_AVMUX *avmux = (SYNTRO_RECORD_AVMUX *)data.constData();

    SyntroUtils::avmuxHeaderToAVParams(avmux, &m_avParams);

    switch (m_avParams.avmuxSubtype) {
        case SYNTRO_RECORD_TYPE_AVMUX_MJPPCM:
            processMJPPCM(avmux);
            break;

        default:
            printf("Unsupported avmux type %d\n", m_avParams.avmuxSubtype);
            break;
    }
}

void AVMuxDecode::processMJPPCM(SYNTRO_RECORD_AVMUX *avmux)
{
    int videoSize = SyntroUtils::convertUC4ToInt(avmux->videoSize);

    unsigned char *ptr = (unsigned char *)(avmux + 1) + SyntroUtils::convertUC4ToInt(avmux->muxSize);

    if (videoSize != 0) {
        // there is video data present

        if ((videoSize < 0) || (videoSize > 300000)) {
            printf("Illegal video data size %d\n", videoSize);
            return;
        }

        QImage image;
        image.loadFromData((const uchar *)ptr, videoSize, "JPEG");
        emit newImage(image, SyntroUtils::convertUC8ToInt64(avmux->recordHeader.timestamp));

        ptr += videoSize;
    } else {
        if (SYNTRO_RECORDHEADER_PARAM_NOOP == SyntroUtils::convertUC2ToInt(avmux->recordHeader.param))
            emit newImage(QImage(), SyntroUtils::convertUC8ToInt64(avmux->recordHeader.timestamp));
	}

    int audioSize = SyntroUtils::convertUC4ToInt(avmux->audioSize);

    if (audioSize != 0) {
        // there is audio data present

        if ((audioSize < 0) || (audioSize > 300000)) {
            printf("Illegal audio data size %d\n", audioSize);
            return;
        }

        emit newAudioSamples(QByteArray((const char *)ptr, audioSize), 
            SyntroUtils::convertUC8ToInt64(avmux->recordHeader.timestamp),
			m_avParams.audioSampleRate, m_avParams.audioChannels, m_avParams.audioSampleSize);

        ptr += audioSize;
    }
}
