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

#ifndef SYNTROVIEW_H
#define SYNTROVIEW_H

#ifdef USING_GSTREAMER
#define	PRODUCT_TYPE "SyntroViewGS"
#else
#define	PRODUCT_TYPE "SyntroView"
#endif

#include <QtGui>

#include "ui_SyntroView.h"
#include "SyntroLib.h"
#include "SyntroServer.h"
#include "DisplayStats.h"
#include "ViewClient.h"
#include "ImageWindow.h"
#include "ViewSingleCamera.h"

#if defined(Q_OS_OSX) || defined(Q_OS_WIN)
#include <QAudioOutput>
#else
#include <alsa/asoundlib.h>
#endif

class SyntroView : public QMainWindow
{
	Q_OBJECT

public:
    SyntroView();

public slots:
	void onStats();
	void onAbout();
	void onBasicSetup();
	void onChooseVideoStreams();
	void onAudioSetup();
    void onShowName();
	void onShowDate();
	void onShowTime();
	void onTextColor();
	void imageMousePress(QString name);
	void imageDoubleClick(QString name);
	void singleCameraClosed();
	void clientConnected();
	void clientClosed();
	void dirResponse(QStringList directory);

	void newAudio(QByteArray data, int rate, int channels, int size);

#if defined(Q_OS_OSX) || defined(Q_OS_WIN)
    void handleAudioOutStateChanged(QAudio::State);
#endif

signals:
	void requestDir();
	void enableService(AVSource *avSource);
	void disableService(int servicePort);

protected:
	void closeEvent(QCloseEvent *event);
	void timerEvent(QTimerEvent *event);

private:
	bool addAVSource(QString name);

	void layoutGrid();
	void initStatusBar();
	void initMenus();
	void saveWindowState();
	void restoreWindowState();
	void startControlServer();
    QByteArray convertAudioFormat(const QByteArray& audioData);

	Ui::SyntroViewClass ui;

	SyntroServer *m_controlServer;
	ViewClient *m_client;
	QStringList m_clientDirectory;

	QList<AVSource *> m_avSources;
	QList<ImageWindow *> m_windowList;
	QList<AVSource *> m_delayedDeleteList;

	DisplayStats *m_displayStats;
	QLabel *m_controlStatus;

	int m_statusTimer;
	int m_directoryTimer;

	bool m_showName;
	bool m_showDate;
	bool m_showTime;
	QColor m_textColor;

	ViewSingleCamera *m_singleCamera;
	int m_selectedSource;

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

    int m_outputRate;
    int m_outputChannels;

	QString m_logTag;
};

#endif // SYNTROVIEW_H
