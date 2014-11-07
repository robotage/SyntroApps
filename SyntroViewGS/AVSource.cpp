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

#include "AVSource.h"


AVSource::AVSource(QString streamName)
{
	m_name = streamName;
	m_decoder = NULL;
	m_servicePort = -1;
	m_audioEnabled = false;
	m_lastUpdate = 0;
	m_stats = new DisplayStatsData();
	connect(this, SIGNAL(updateStats(int)), m_stats, SLOT(updateBytes(int)));
}

AVSource::~AVSource()
{
	stopBackgroundProcessing();

	if (m_stats) {
		delete m_stats;
		m_stats = NULL;
	}
}

QString AVSource::name() const
{
	return m_name;
}

int AVSource::servicePort() const
{
	return m_servicePort;
}

void AVSource::setServicePort(int port)
{
	m_servicePort = port;

	if (port == -1) {
		stopBackgroundProcessing();
		return;
	}
	else if (!m_decoder) {
        m_decoder = new AVMuxDecode();

		connect(this, SIGNAL(newAVMuxData(QByteArray)), m_decoder, SLOT(newAVMuxData(QByteArray)));

		connect(m_decoder, SIGNAL(newImage(QImage, qint64)), 
			this, SLOT(newImage(QImage, qint64)));
	
		connect(m_decoder, SIGNAL(newAudioSamples(QByteArray, qint64, int, int, int)), 
			this, SLOT(newAudioSamples(QByteArray, qint64, int, int, int)));

		m_decoder->resumeThread();
	}
}

qint64 AVSource::lastUpdate() const
{
	return m_lastUpdate;
}

void AVSource::setLastUpdate(qint64 timestamp)
{
	m_lastUpdate = timestamp;
}

QImage AVSource::image()
{
	return m_image;
}

qint64 AVSource::imageTimestamp()
{
	return m_imageTimestamp;
}

void AVSource::stopBackgroundProcessing()
{
	if (m_decoder) {
		disconnect(this, SIGNAL(newAVMuxData(QByteArray)), m_decoder, SLOT(newAVMuxData(QByteArray)));

		disconnect(m_decoder, SIGNAL(newImage(QImage, qint64)),  
			this, SLOT(newImage(QImage, qint64)));

		disconnect(m_decoder, SIGNAL(newAudioSamples(QByteArray, qint64, int, int, int)), 
			this, SLOT(newAudioSamples(QByteArray, qint64, int, int, int)));

		m_decoder->exitThread();
		m_decoder = NULL;
	}
}

void AVSource::enableAudio(bool enable)
{
	m_audioEnabled = enable;
}

bool AVSource::audioEnabled() const
{
	return m_audioEnabled;
}

// feed new raw data to the decoder, called from the Syntro client thread
void AVSource::setAVMuxData(QByteArray data)
{
	emit updateStats(data.size());
	emit newAVMuxData(data);
}

// signal from the decoder, processed image
void AVSource::newImage(QImage image, qint64 timestamp)
{
    if (!image.isNull()) {
		m_image = image;
        m_imageTimestamp = timestamp;
    }
	m_lastUpdate = SyntroClock();
}

// signal from the decoder, processed sound
void AVSource::newAudioSamples(QByteArray data, qint64, int rate, int channels, int size)
{
	if (m_audioEnabled)
		emit newAudio(data, rate, channels, size);
}

DisplayStatsData* AVSource::stats()
{
	return m_stats;
}
