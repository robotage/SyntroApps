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

#include "ReviewClient.h"

ReviewClient::ReviewClient(QObject *)
		: Endpoint(SYNTROREVIEW_BGND_INTERVAL, "Review")
{
}

ReviewClient::~ReviewClient()
{
}

void ReviewClient::appClientInit()
{
	newCFSList();
	m_lastDirReq = SyntroClock();
	m_state = SYNTROREVIEW_CFS_STATE_WAITING_FOR_DIRECTORY;
	emit newCFSState(m_state);
	m_playMode = SYNTROREVIEW_PLAY_MODE_STOPPED;
	m_readOutstanding = false;
	m_nextFrameReceived = false;
	m_frameDelta = 1;
	m_useRequestedIndex = false;
	return;
}

void ReviewClient::newCFSList()
{
	CFSServerInfo *serverInfo;
	//	delete any current list

	for (int i = 0; i < m_serverInfo.count(); i++) {
		serverInfo = m_serverInfo.at(i);
		clientRemoveService(serverInfo->port);
		CFSDeleteEP(serverInfo->port);		
		delete(m_serverInfo.at(i));
	}

	m_serverInfo.clear();

	//	Set up SyntroCFS endpoints

	loadCFSStores(SYNTRO_PARAMS_CFS_STORES, SYNTRO_PARAMS_CFS_STORE);
	for (int i = 0; i < m_serverInfo.count(); i++) {
		serverInfo = m_serverInfo.at(i);
		serverInfo->port = clientAddService(serverInfo->servicePath + "/" + SYNTRO_STREAMNAME_CFS, SERVICETYPE_E2E, false);
		CFSAddEP(serverInfo->port);							// indicate to CEndpoint that this uses the SyntroCFS API
		serverInfo->waitingForDirectory = false;			// no outstanding requests
	}

}

void ReviewClient::appClientBackground()
{
	if (clientIsConnected() && (m_state != SYNTROREVIEW_CFS_STATE_OPEN)) {
		if ((SyntroClock() - m_lastDirReq) > SYNTROREVIEW_DIR_INTERVAL) {
			m_lastDirReq = SyntroClock();
			for (int i = 0; i < m_serverInfo.count(); i++) {
				if (!m_serverInfo.at(i)->waitingForDirectory) {
					if (!(m_serverInfo.at(i)->waitingForDirectory = CFSDir(m_serverInfo.at(i)->port))) {
						logWarn(QString("Failed to get directory from port %1").arg(m_serverInfo.at(i)->port));
						m_serverInfo.at(i)->directory.clear();
						emitUpdatedDirectory();
					}
				}
			}
		}
	}
	if (m_nextFrameReceived)
		processFrameReceived();								// see if time to display yet
}

void ReviewClient::openFile(QString filePath)
{
	CFSServerInfo *serverInfo;

	if (m_state != SYNTROREVIEW_CFS_STATE_IDLE)
		return;

	// must find which SyntroCFS server it is on

	serverInfo = NULL;

	for (int i = 0; i < m_serverInfo.size(); i++) {
		if (filePath.startsWith(m_serverInfo.at(i)->servicePath)) {
			serverInfo = m_serverInfo.at(i);
			break;
		}
	}

	if (serverInfo == NULL) {
		logError(QString("Failed to find server for file %1").arg(filePath));
		return;
	}

	m_activeCFS = serverInfo;

	//	strip service path and '/' from file path

	filePath.remove(0, m_activeCFS->servicePath.length() + 1);

	if ((m_handle = CFSOpenStructuredFile(m_activeCFS->port, filePath)) == -1)
		return;												// open failed

	m_activeFile = filePath;
	m_state = SYNTROREVIEW_CFS_STATE_OPENING;
	m_readOutstanding = false;
	m_useRequestedIndex = false;
	emit newCFSState(m_state);
}

void ReviewClient::closeFile()
{
	if (m_state != SYNTROREVIEW_CFS_STATE_OPEN)
		return;
	if (CFSClose(m_activeCFS->port, m_handle))
		m_state = SYNTROREVIEW_CFS_STATE_CLOSING;
	else
		m_state = SYNTROREVIEW_CFS_STATE_IDLE;
	emit newCFSState(m_state);
	m_readOutstanding = false;
}

void ReviewClient::CFSDirResponse(int remoteServiceEP, unsigned int responseCode, QStringList filePaths)
{
	CFSServerInfo *serverInfo;
	int i;

	serverInfo = NULL;
	for (i = 0; i < m_serverInfo.size(); i++) {
		if (m_serverInfo.at(i)->port == remoteServiceEP) {
			serverInfo = m_serverInfo.at(i);
			break;
		}
	}

	if (serverInfo == NULL) {
		logWarn(QString("Received directory on unmatched port %1").arg(remoteServiceEP));
		return;
	}

	serverInfo->waitingForDirectory = false;

	if (responseCode != SYNTROCFS_SUCCESS) {
		logWarn(QString("DirResponse - got error code %1 from port %2").arg(responseCode).arg(remoteServiceEP));
		serverInfo->directory.clear();
		emitUpdatedDirectory();
		return;
	}
	serverInfo->directory.clear();
	for (i = 0; i < filePaths.size(); i++) {				// add on service path prefix
		serverInfo->directory.append(serverInfo->servicePath + SYNTRO_SERVICEPATH_SEP + filePaths.at(i));
	}

	emitUpdatedDirectory();
	if (m_state == 	SYNTROREVIEW_CFS_STATE_WAITING_FOR_DIRECTORY) {
		m_state = SYNTROREVIEW_CFS_STATE_IDLE;
		emit newCFSState(m_state);
	}
}

void ReviewClient::emitUpdatedDirectory()
{
	m_directory.clear();

	for (int i = 0; i < m_serverInfo.size(); i++) {
		m_directory.append(m_serverInfo.at(i)->directory);
	}

	emit newDirectory(m_directory);							// tell anyone that wants to know
}

void ReviewClient::CFSOpenResponse(int remoteServiceEP, unsigned int responseCode, int handle, unsigned int fileLength)
{
	if (responseCode != SYNTROCFS_SUCCESS) {
		logWarn(QString("Open response - got error code %1 from %2").arg(responseCode).arg(m_activeFile));
		m_state = SYNTROREVIEW_CFS_STATE_IDLE;
		emit newCFSState(m_state);
		return;
	}
	m_state = SYNTROREVIEW_CFS_STATE_OPEN;
	emit newCFSState(m_state);
	emit newFileLength(fileLength);
	m_fileLength = fileLength;
	logDebug(QString("Opened %s on port %1, handle %2").arg(m_activeFile).arg(remoteServiceEP, handle));
	handle += 0;												// to stop compiler warnings
	remoteServiceEP += 0;										// to stop compiler warnings
	getInitialFrame();
}

void ReviewClient::CFSReadAtIndexResponse(int remoteServiceEP, int handle, unsigned int recordIndex, 
		unsigned int responseCode, unsigned char *fileData, int length)
{
	m_readOutstanding = false;
	logDebug("ReadAtIndexResponse");
	if (responseCode != SYNTROCFS_SUCCESS) {					// close on read failure
//		if (!CFSClose(remoteServiceEP, handle)) {
//			setCFSStateIdle();
//		}
	    m_nextFrameReceived = true;
		return;
	}
	if (recordIndex != m_recordIndex) {
		logWarn(QString("Got read at %1, expected %2").arg(recordIndex).arg(m_recordIndex));
		free(fileData);
		return;
	}
	m_nextFrameReceived = true;
	videoDecode((SYNTRO_RECORD_HEADER *)fileData, length);
}

void ReviewClient::CFSCloseResponse(int , unsigned int responseCode, int )
{
	setCFSStateIdle();
	logWarn(QString("Close response - got response code %1 from %2").arg(responseCode).arg(m_activeFile));
	responseCode += 0;										// to keep compiler happy
}


void ReviewClient::CFSKeepAliveTimeout(int remoteServiceEP, int handle)
{
	setCFSStateIdle();
	logWarn(QString("Got keep alive timeout on port %1, slot %2").arg(remoteServiceEP).arg(handle)); 
	remoteServiceEP += 0;									// to keep compiler happy
	handle += 0;											// to keep compiler happy
}

void ReviewClient::loadCFSStores(QString group, QString src)
{
	CFSServerInfo *serverInfo;
	QSettings *settings = SyntroUtils::getSettings();

	int count = settings->beginReadArray(group);

	for (int i = 0; i < count; i++) {
		settings->setArrayIndex(i);
		serverInfo = new CFSServerInfo;
		serverInfo->servicePath = settings->value(src).toString();
		m_serverInfo.append(serverInfo);
	}

	settings->endArray();

	delete settings;
}


void ReviewClient::videoDecode(SYNTRO_RECORD_HEADER *header, int length)
{
	SYNTRO_RECORD_VIDEO *videoRecord;
	SYNTRO_RECORD_AVMUX *avmuxRecord;

	int muxLength;
	int videoLength;
	int audioLength;
	unsigned char *videoPtr;
	unsigned char *audioPtr;

	switch (SyntroUtils::convertUC2ToInt(header->type))
	{
		case SYNTRO_RECORD_TYPE_VIDEO:
            if (length < (int)sizeof(SYNTRO_RECORD_VIDEO)) {
				logWarn(QString("Received video record that was too short: ") + length);
				break;
			}
			length -= sizeof(SYNTRO_RECORD_VIDEO);
			videoRecord = (SYNTRO_RECORD_VIDEO *)(header);
			m_previousTimecode = m_timecode;				// save old and record new timecode
			m_timecode = QDateTime::fromMSecsSinceEpoch(SyntroUtils::convertUC8ToInt64(videoRecord->recordHeader.timestamp));
			calculateNextDisplayTime();
			videoLength = SyntroUtils::convertUC4ToInt(videoRecord->size);

			if (length < videoLength) {
				logWarn(QString("Received video record that was too short: ") + length);
				break;
			}

			switch (SyntroUtils::convertUC2ToInt(header->subType))
			{
				case SYNTRO_RECORD_TYPE_VIDEO_MJPEG:
					m_frameCompressed.clear();
					m_frameCompressed.insert(0, reinterpret_cast<const char *>(videoRecord + 1), videoLength);
					m_frame.loadFromData(reinterpret_cast<const unsigned char *>(videoRecord + 1), videoLength, "JPEG");				
					processFrameReceived();
					break;
			}
			break;

		case SYNTRO_RECORD_TYPE_AVMUX:
			avmuxRecord = (SYNTRO_RECORD_AVMUX *)(header);
			if (!SyntroUtils::avmuxHeaderValidate(avmuxRecord, length, 
					NULL, muxLength, &videoPtr, videoLength, &audioPtr, audioLength)) {
				logWarn("avmux record failed validation");
				break;
			}
			length -= sizeof(SYNTRO_RECORD_AVMUX);
			m_previousTimecode = m_timecode;				// save old and record new timecode
			m_timecode = QDateTime::fromMSecsSinceEpoch(SyntroUtils::convertUC8ToInt64(avmuxRecord->recordHeader.timestamp));
			calculateNextDisplayTime();

			switch (SyntroUtils::convertUC2ToInt(header->subType))
			{
				case SYNTRO_RECORD_TYPE_AVMUX_MJPPCM:
					m_frameCompressed.clear();
					m_frameCompressed.insert(0, (const char *)videoPtr, videoLength);
					m_frame.loadFromData(videoPtr, videoLength, "JPEG");				
					processFrameReceived();
					if (audioLength > 0) {
						emit newAudio(QByteArray((char *)audioPtr, audioLength), 
							SyntroUtils::convertUC4ToInt(avmuxRecord->audioSampleRate),
							SyntroUtils::convertUC2ToInt(avmuxRecord->audioChannels),
							SyntroUtils::convertUC2ToInt(avmuxRecord->audioSampleSize));
					}
					break;
			}
			break;


	}
	free(header);
}

void ReviewClient::processFrameReceived()
{
	if (!m_nextFrameReceived)
		return;												// still waiting for next frame

	if (SyntroClock() < m_nextDisplayTime)
		return;												// not time yet

	emit showImage(m_frame, m_frameCompressed, m_recordIndex, m_timecode);		// display it
	logDebug(QString("Display index %1").arg(m_recordIndex));
	m_nextFrameReceived = false;

	if (m_useRequestedIndex) {
		m_recordIndex = m_requestedRecordIndex;
		m_useRequestedIndex = false;
		CFSReadAtIndex(m_activeCFS->port, m_handle, m_recordIndex);
		m_readOutstanding = true;
		return;
	}
	switch (m_playMode) {
		case SYNTROREVIEW_PLAY_MODE_STOPPED:
			break;											// wait for more instructions

		case SYNTROREVIEW_PLAY_MODE_PAUSED:
			break;											// wait here

		case SYNTROREVIEW_PLAY_MODE_PLAY:
		case SYNTROREVIEW_PLAY_MODE_FASTFORWARD:
			getNextFrameForward();
			break;

		case SYNTROREVIEW_PLAY_MODE_REVERSE:
		case SYNTROREVIEW_PLAY_MODE_FASTREVERSE:
			getNextFrameReverse();
			break;

		default:
			logWarn(QString("Illegal play mode %1").arg(m_playMode));
			m_playMode = SYNTROREVIEW_PLAY_MODE_STOPPED;
			emit newPlayMode(m_playMode);
			break;
	}
}

void ReviewClient::setPlayMode(int playMode, bool fullSpeed)
{
	logDebug(QString("Going to play mode %1 from %2").arg(playMode).arg(m_playMode));
	if ((playMode == m_playMode) && (m_playMode != SYNTROREVIEW_PLAY_MODE_PAUSED))
		return;												// already in this mode
	m_playMode = playMode;
	switch (m_playMode) {
		case SYNTROREVIEW_PLAY_MODE_STOPPED:
			getInitialFrame();								// reset to start
			break;									

		case SYNTROREVIEW_PLAY_MODE_PAUSED:
			m_nextDisplayTime = SyntroClock();				// force display as soon as received
			m_frameDelta = 0;
			getNextFrameForward();
			break;										

		case SYNTROREVIEW_PLAY_MODE_PLAY:
			m_fullSpeed = fullSpeed;
			m_nextDisplayTime = SyntroClock();				// force display as soon as received
			m_frameDelta = 1;
			getNextFrameForward();
			break;

		case SYNTROREVIEW_PLAY_MODE_FASTFORWARD:
			m_nextDisplayTime = SyntroClock();				// force display as soon as received
			m_frameDelta = 5;
			getNextFrameForward();
			break;

		case SYNTROREVIEW_PLAY_MODE_REVERSE:
			m_nextDisplayTime = SyntroClock();				// force display as soon as received
			m_frameDelta = 1;
			getNextFrameReverse();
			break;

		case SYNTROREVIEW_PLAY_MODE_FASTREVERSE:
			m_nextDisplayTime = SyntroClock();				// force display as soon as received
			m_frameDelta = 5;
			getNextFrameReverse();
			break;

		default:
			logWarn(QString("Illegal play mode %1").arg(m_playMode));
			m_playMode = SYNTROREVIEW_PLAY_MODE_STOPPED;
			getInitialFrame();
			emit newPlayMode(m_playMode);
			break;
	}
}

void ReviewClient::setFrameIndex(unsigned int frameIndex, int playMode, bool fullSpeed)
{
	if (m_readOutstanding) {
		m_requestedRecordIndex = frameIndex;
		m_useRequestedIndex = true;
	} else {
		m_recordIndex = frameIndex;
	}
	setPlayMode(playMode, fullSpeed);	
}

void ReviewClient::setCFSStateIdle()
{
	m_state = SYNTROREVIEW_CFS_STATE_IDLE;
	m_playMode = SYNTROREVIEW_PLAY_MODE_STOPPED;
	emit newPlayMode(SYNTROREVIEW_PLAY_MODE_STOPPED);
	emit newCFSState(m_state);
}

void ReviewClient::getInitialFrame()
{
	if (m_readOutstanding) {
		m_requestedRecordIndex = 0;
		m_useRequestedIndex = true;
		return;												// must do later
	}
	m_recordIndex = 0;
	m_playMode = SYNTROREVIEW_PLAY_MODE_STOPPED;
	m_nextDisplayTime = SyntroClock();						// indicates display as soon as received
	m_nextFrameReceived = false;							// indicate waiting for next (first) frame
	CFSReadAtIndex(m_activeCFS->port, m_handle, 0);
	m_readOutstanding = true;
}

void ReviewClient::getNextFrameForward()
{
	int index;

	if (m_useRequestedIndex)
		return;												// calculation has already been done
	index = m_recordIndex;
	if (index + m_frameDelta >= m_fileLength) {				// hit end of file
		index = m_fileLength - 1;							// hold at last frame
		m_playMode = SYNTROREVIEW_PLAY_MODE_PAUSED;			// and change to paused
		emit newPlayMode(m_playMode);
	} else {
		index += m_frameDelta;
	}
	if (m_readOutstanding) {								// must defer
		m_requestedRecordIndex = index;
		m_useRequestedIndex = true;
		return;
	}
	m_recordIndex = index;
	CFSReadAtIndex(m_activeCFS->port, m_handle, m_recordIndex);
	m_readOutstanding = true;
}

void ReviewClient::getNextFrameReverse()
{
	unsigned int index;

	if (m_useRequestedIndex)
		return;												// calculation has already been done
	index = m_recordIndex;
	if (index < m_frameDelta) {								// hit start of file
		index = 0;											// hold at first frame
		m_playMode = SYNTROREVIEW_PLAY_MODE_STOPPED;		// and change to stopped
		emit newPlayMode(m_playMode);
	} else {
		index -= m_frameDelta;
	}
	if (m_readOutstanding) {								// must defer
		m_requestedRecordIndex = index;
		m_useRequestedIndex = true;
		return;
	}
	m_recordIndex = index;
	CFSReadAtIndex(m_activeCFS->port, m_handle, m_recordIndex);
	m_readOutstanding = true;
}

void ReviewClient::calculateNextDisplayTime()
{
	qint64 delta;

	delta = 0;
	switch (m_playMode) {
		case SYNTROREVIEW_PLAY_MODE_STOPPED:
			m_nextDisplayTime = SyntroClock();
			break;									

		case SYNTROREVIEW_PLAY_MODE_PAUSED:
			m_nextDisplayTime = SyntroClock();
			break;										

		case SYNTROREVIEW_PLAY_MODE_PLAY:
		case SYNTROREVIEW_PLAY_MODE_FASTFORWARD:
			if (m_fullSpeed)
				m_nextDisplayTime = SyntroClock() - 1;		// immediate			
			else
				delta = m_previousTimecode.msecsTo(m_timecode) / m_frameDelta;
			break;

		case SYNTROREVIEW_PLAY_MODE_REVERSE:
		case SYNTROREVIEW_PLAY_MODE_FASTREVERSE:
			delta = m_timecode.msecsTo(m_previousTimecode) / m_frameDelta;
			break;

		default:
			logWarn(QString("Illegal play mode %1 in calculateNextDisplayTime").arg(m_playMode));
			break;
	}
	// we won't allow more than a 1 second interval
	if (delta > 1000)
		delta = 1000;
	if (delta < -1000)
		delta = -1000;
	m_nextDisplayTime += delta;
}

void ReviewClient::requestDir()
{
	requestDirectory();
}

void ReviewClient::appClientReceiveDirectory(QStringList directory)
{
	emit dirResponse(directory);
}
