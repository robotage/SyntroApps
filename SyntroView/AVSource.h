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

#ifndef AVSOURCE_H
#define AVSOURCE_H

#ifdef USING_GSTREAMER
#include "AVMuxDecodeGS.h"
#else
#include "AVMuxDecode.h"
#endif
#include "DisplayStatsData.h"

class AVSource : public QObject
{
	Q_OBJECT

public:
	AVSource(QString streamName);
	~AVSource();

	QString name() const;

	int servicePort() const;
	void setServicePort(int port);

	qint64 lastUpdate() const;
	void setLastUpdate(qint64 timestamp);

	QImage image();
	qint64 imageTimestamp();

	void setAVMuxData(QByteArray rawData);
	void stopBackgroundProcessing();

	void enableAudio(bool enable);
	bool audioEnabled() const;

	DisplayStatsData *stats();

signals:
	void newAudio(QByteArray data, int rate, int channels, int size);
	void newAVMuxData(QByteArray data);
	void updateStats(int bytes);

public slots:
	void newImage(QImage image, qint64 timestamp);
	void newAudioSamples(QByteArray data, qint64 timestamp, int rate, int channels, int size);

private:
	QString m_name;
	int m_servicePort;

	qint64 m_lastUpdate;

	QImage m_image;
	qint64 m_imageTimestamp;

	bool m_audioEnabled;

	AVMuxDecode *m_decoder;

	int m_statsTimer;
	DisplayStatsData *m_stats;
};

#endif // AVSOURCE_H
