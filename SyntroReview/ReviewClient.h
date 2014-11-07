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

#ifndef REVIEWCLIENT_H
#define REVIEWCLIENT_H

#include "SyntroLib.h"
#include "SyntroReview.h"

class CFSServerInfo
{
public:
	QString servicePath;									// the path of the service to be look up
	int port;												// the port assigned
	QStringList directory;									// the directory from this server
	bool waitingForDirectory;								// indicates request sent
};

class	CSpacesReview;

class ReviewClient : public Endpoint
{
	Q_OBJECT

public:
	ReviewClient(QObject *parent);           
	~ReviewClient();

public slots:
	void openFile(QString filePath);
	void closeFile();
	void setPlayMode(int playMode, bool fullSpeed);
	void setFrameIndex(unsigned int frameIndex, int playMode, bool fullSpeed);
	void requestDir();
	void newCFSList();

signals:
	void showImage(QImage frame, QByteArray frameCompressed, unsigned int recordIndex, QDateTime m_timestamp);	// emitted when a frame received
	void newAudio(QByteArray data, int rate, int channels, int size);
	void newDirectory(QStringList directory);				// emitted when a new CFS directory is received
	void newCFSState(int state);							// emitted when state changes
	void newFileLength(unsigned int fileLength);			// emitted when a file is opened
	void newPlayMode(int playMode);							// when the ReviewClient forces a play mode change
	void dirResponse(QStringList);							// when a SyntroControl directory is received

protected:
	void appClientInit();
	void appClientBackground();
    void appClientReceiveDirectory(QStringList);

private:
	void loadCFSStores(QString group, QString src);

	QList <CFSServerInfo *> m_serverInfo;					// server-specific info
	QStringList m_directory;								// the integrated directory
	CFSServerInfo *m_activeCFS;								// the CFS in use

	void CFSDirResponse(int remoteServiceEP, unsigned int responseCode, QStringList filePaths); 
	void CFSOpenResponse(int remoteServiceEP, unsigned int responseCode, int handle, unsigned int fileLength); 
	void CFSCloseResponse(int remoteServiceEP, unsigned int responseCode, int handle); 
	void CFSKeepAliveTimeout(int remoteServiceEP, int handle);
	void CFSReadAtIndexResponse(int remoteServiceEP, int handle, unsigned int recordIndex, 
		unsigned int responseCode, unsigned char *fileData, int length);

	void videoDecode(SYNTRO_RECORD_HEADER *pSRH, int length);
	void processFrameReceived();
	void setCFSStateIdle();
	void getInitialFrame();
	void getNextFrameForward();
	void getNextFrameReverse();
	void calculateNextDisplayTime();
	void emitUpdatedDirectory();

	qint64	m_lastDirReq;									// when the last directory request was obtained
	QString m_activeFile;									// the name of the active file
	int m_handle;											// the file handle
	int m_state;											// the CFS interface state
	unsigned int m_fileLength;								// length of the current file
	unsigned int m_recordIndex;								// where we are in the file
	unsigned int m_frameDelta;								// interval between frames

	QDateTime m_timecode;									// the timecode of the last received frame
	QDateTime m_previousTimecode;							// the timecode of the previous frame

	int m_playMode;											// the play mode
	QImage m_frame;											// the next frame to be displayed
	QByteArray m_frameCompressed;							// the compressed version
	qint64 m_nextDisplayTime;								// when the next frame is to be displayed
	bool m_readOutstanding;									// if there's a CFS read outstanding
	bool m_nextFrameReceived;								// means that we have the next frame
	int m_requestedRecordIndex;								// a requested record index if it couldn't be done immediately
	bool m_useRequestedIndex;								// true if need to use requested index instead of natural one
	bool m_fullSpeed;										// if ignore timecodes and go at full speed
};

#endif // REVIEWCLIENT_H

