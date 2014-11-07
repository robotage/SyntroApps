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

#ifndef _AVMUXDECODE_H_
#define _AVMUXDECODE_H_

#include "SyntroLib.h"

#include <qmutex.h>

class AVMuxDecode : public SyntroThread
{
    Q_OBJECT

public:
    AVMuxDecode();

public slots:
    void newAVMuxData(QByteArray data);

signals:
    void newImage(QImage image, qint64 timestamp);
    void newAudioSamples(QByteArray dataArray, qint64 timestamp, int rate, int channels, int size);

private:
    void processMJPPCM(SYNTRO_RECORD_AVMUX *avmux);
    void processAVData(QByteArray avmuxArray);

	SYNTRO_AVPARAMS m_avParams;
};

#endif // _AVMUXDECODE_H_
