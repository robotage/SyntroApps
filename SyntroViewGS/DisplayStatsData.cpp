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

#include "DisplayStatsData.h"

#define STAT_TIMER_INTERVAL 5

DisplayStatsData::DisplayStatsData()
{
	clear();
	m_timer = startTimer(STAT_TIMER_INTERVAL * 1000);
}

int DisplayStatsData::totalRecords() const
{
	return m_totalRecords;
}

qint64 DisplayStatsData::totalBytes() const
{
	return m_totalBytes;
}

qreal DisplayStatsData::recordRate() const
{
	return m_recordRate;
}

qreal DisplayStatsData::byteRate() const
{
	return m_byteRate;
}

void DisplayStatsData::updateBytes(int bytes)
{
	m_totalRecords++;
	m_totalBytes += bytes;
	m_records++;
	m_bytes += bytes;
}

void DisplayStatsData::timerEvent(QTimerEvent *)
{
	updateRates(STAT_TIMER_INTERVAL);
}

void DisplayStatsData::updateRates(int secs)
{
	if (secs > 0) {
		m_recordRate = (qreal)m_records / (qreal)secs;
		m_byteRate = (qreal)m_bytes / (qreal)secs;
	}

	m_records = 0;
	m_bytes = 0;
}

void DisplayStatsData::clear()
{
	m_totalRecords = 0;
	m_totalBytes = 0;
	m_records = 0;
	m_bytes = 0;
	m_recordRate = 0.0;
	m_byteRate = 0.0;
}
