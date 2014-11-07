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

#ifndef SYNTROREVIEW_H
#define SYNTROREVIEW_H

#define	PRODUCT_TYPE	"SyntroReview"

#include <QDialogButtonBox>
#include <QRadioButton>
#include "ui_SyntroReview.h"
#include "SyntroLib.h"

#if defined(Q_OS_OSX) || defined(Q_OS_WIN)
#include <QAudioOutput>
#else
#include <alsa/asoundlib.h>
#endif

#define	SYNTROREVIEW_BGND_INTERVAL		(SYNTRO_CLOCKS_PER_SEC / 100)// background timer
#define	SYNTROREVIEW_DIR_INTERVAL		(SYNTRO_CLOCKS_PER_SEC * 4)	// directory refresh interval when not playing

#define	SYNTROREVIEW_SAVE_INTERVAL		(SYNTRO_CLOCKS_PER_SEC / 30)	// interval between frames of saved video

//	CFS interface states

enum
{
	SYNTROREVIEW_CFS_STATE_WAITING_FOR_DIRECTORY = 0,		// initial state after link up with empty directory
	SYNTROREVIEW_CFS_STATE_IDLE,							// have directory but no open file
	SYNTROREVIEW_CFS_STATE_OPENING,							// opening file
	SYNTROREVIEW_CFS_STATE_OPEN,							// a file is open
	SYNTROREVIEW_CFS_STATE_CLOSING							// in process of being closed
};

//	Play modes

enum
{
	SYNTROREVIEW_PLAY_MODE_STOPPED = 0,						// stopped means at start of file, not playing
	SYNTROREVIEW_PLAY_MODE_PAUSED,							// was playing, now paused
	SYNTROREVIEW_PLAY_MODE_PLAY,							// playing normally
	SYNTROREVIEW_PLAY_MODE_REVERSE,							// playing in reverse
	SYNTROREVIEW_PLAY_MODE_FASTFORWARD,						// fast forwarding
	SYNTROREVIEW_PLAY_MODE_FASTREVERSE						// fast reversing
};


class ReviewClient;

class SyntroReview : public QMainWindow
{
	Q_OBJECT

public:
	SyntroReview(QWidget *parent = 0);
	~SyntroReview();

public slots:
	void showImage(QImage frame, QByteArray frameCompressed, unsigned int recordIndex, QDateTime m_timestamp);
	void newAudio(QByteArray data, int rate, int channels, int size);
	void onAudioSetup();
	void onAbout();
	void onOpen();
	void onClose();
	void onBasicSetup();
	void onCFSSelection();
	void newDirectory(QStringList directory);	
	void newCFSState(int state);
	void newFileLength(unsigned int fileLength);			
	void newPlayMode(int playMode);	
	void playButtonClicked(QAbstractButton * button);
	void saveButtonClicked(QAbstractButton * button);
	void sliderMoved(int index);
	void dirResponse(QStringList directory);

#if defined(Q_OS_OSX) || defined(Q_OS_WIN)
    void handleAudioOutStateChanged(QAudio::State);
#endif

signals:
	void openFile(QString filePath);
	void closeFile();
	void setPlayMode(int playMode, bool fullSpeed);
	void setFrameIndex(unsigned int frameIndex, int playMode, bool fullSpeed);
	void requestDir();
	void newCFSList();

protected:
	void closeEvent(QCloseEvent *event);
	void timerEvent(QTimerEvent *event);

private:
	void layoutWindow();
	void saveWindowState();
	void restoreWindowState();
	void setDisabledPlayControls();
	void setStoppedPlayControls();
	void setPausedPlayControls();
	void setPlayingPlayControls();
	void setReversePlayControls();
	void setFastReversePlayControls();
	void setFastForwardPlayControls();
    QByteArray convertToMac(const QByteArray& audioData);

	QString displayTimecode(QDateTime& timecode);
	void saveCurrentFrame();								// save the currently displayed image as a jpeg

	Ui::SyntroReviewClass ui;

	ReviewClient *m_client;
	QStringList m_clientDirectory;
	int m_statusTimer;
	int m_directoryTimer;
	QStringList m_directory;								// local copy of integrated SyntroCFS directory
	QString m_filePath;										// the SyntroCFS file path
	int m_CFSState;											// current state of the CFS interface
	int m_playMode;											// current play mode
	unsigned int m_fileLength;

	QDateTime m_currentTimecode;							// current timecode
	unsigned int m_currentIndex;

	QLabel *m_cameraView;
	QLabel *m_timecode;
	QLabel *m_timecodeLabel;
	QLabel *m_file;
	QLabel *m_fileLabel;
	QSlider *m_slider;
	QDialogButtonBox *m_playControls;
	QPushButton *m_buttonPlay;
	QPushButton *m_buttonFramePlus;
	QPushButton *m_buttonPause;
	QPushButton *m_buttonStop;
	QPushButton *m_buttonFrameMinus;
	QPushButton *m_buttonReverse;
	QPushButton *m_buttonFastReverse;
	QPushButton *m_buttonFastForward;
	QRadioButton *m_buttonLoop;

	QDialogButtonBox *m_saveControls;
	QPushButton *m_saveFrame;

	QLabel *m_controlStatus;
	QLabel *m_CFSStatus;

	QString m_saveFilePath;									// where the file is

	QByteArray m_frameCompressed;							// the last compressed frame
	QImage m_frame;											// last uncompressed frame
	QDateTime m_lastSavedTimecode;							// the timecode of the last saved frame
	bool m_firstSaved;										// if this is actually the first saved frame in the video
	int m_savedCount;										// the saved frame count

#if defined(Q_OS_OSX) || defined(Q_OS_WIN)
    QAudioOutput *m_audioOut;
    QIODevice *m_audioOutDevice;
#else
    snd_pcm_t *m_audioOutHandle;
    bool m_audioOutIsOpen;
    int m_audioOutSampleSize;
#endif

	void audioOutClose();

	bool audioOutOpen(int rate, int channels, int size);
	bool audioOutWrite(const QByteArray& audioData);
	bool m_audioEnabled;

	int m_audioChannels;
	int m_audioSize;
	int m_audioRate;
};

#endif // SYNTROREVIEW_H
