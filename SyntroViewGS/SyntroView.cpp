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

#include <qcolordialog.h>

#include "SyntroView.h"
#include "SyntroAboutDlg.h"
#include "BasicSetupDlg.h"
#include "StreamDialog.h"
#include "AudioOutputDlg.h"

#define GRID_SPACING 3

SyntroView::SyntroView()
	: QMainWindow()
{
    m_logTag = "SyntroView";
	ui.setupUi(this);

	m_singleCamera = NULL;
	m_selectedSource = -1;

	m_audioSize = -1;
	m_audioRate = -1;
	m_audioChannels = -1;


#if defined(Q_OS_OSX) || defined(Q_OS_WIN32)
		m_audioOut = NULL;
		m_audioOutDevice = NULL;
#else
		m_audioOutIsOpen = false;
#endif

    QSettings *settings = SyntroUtils::getSettings();

    settings->beginGroup(AUDIO_GROUP);

    if (!settings->contains(AUDIO_OUTPUT_DEVICE))

#ifdef Q_OS_OSX
        settings->setValue(AUDIO_OUTPUT_DEVICE, AUDIO_DEFAULT_DEVICE_MAC);
#else
#ifdef Q_OS_LINUX
        settings->setValue(AUDIO_OUTPUT_DEVICE, 0);

    if (!settings->contains(AUDIO_OUTPUT_CARD))
        settings->setValue(AUDIO_OUTPUT_CARD, 0);
#else
        settings->setValue(AUDIO_OUTPUT_DEVICE, AUDIO_DEFAULT_DEVICE);
#endif
#endif

    if (!settings->contains(AUDIO_ENABLE))
        settings->setValue(AUDIO_ENABLE, true);

	m_audioEnabled = settings->value(AUDIO_ENABLE).toBool();

	settings->endGroup();
	delete settings;

	m_displayStats = new DisplayStats(this);

	SyntroUtils::syntroAppInit();

	startControlServer();

	m_client = new ViewClient();

	connect(ui.actionExit, SIGNAL(triggered()), this, SLOT(close()));

	connect(m_client, SIGNAL(clientConnected()), this, SLOT(clientConnected()));
	connect(m_client, SIGNAL(clientClosed()), this, SLOT(clientClosed()));

	connect(m_client, SIGNAL(dirResponse(QStringList)), this, SLOT(dirResponse(QStringList)));
	connect(this, SIGNAL(requestDir()), m_client, SLOT(requestDir()));

	connect(this, SIGNAL(enableService(AVSource *)), m_client, SLOT(enableService(AVSource *)));
	connect(this, SIGNAL(disableService(int)), m_client, SLOT(disableService(int)));

	m_client->resumeThread();

	m_statusTimer = startTimer(2000);
	m_directoryTimer = startTimer(10000);

	restoreWindowState();
	initStatusBar();
	initMenus();
	layoutGrid();

	setWindowTitle(QString("%1 - %2")
		.arg(SyntroUtils::getAppType())
		.arg(SyntroUtils::getAppName()));
}

void SyntroView::startControlServer()
{
	QSettings *settings = SyntroUtils::getSettings();

	if (settings->value(SYNTRO_PARAMS_LOCALCONTROL).toBool()) {
		m_controlServer = new SyntroServer();
		m_controlServer->resumeThread();
	} 
	else {
		m_controlServer = NULL;
	}

	delete settings;
}

void SyntroView::onStats()
{
	m_displayStats->activateWindow();
	m_displayStats->show();
}

void SyntroView::closeEvent(QCloseEvent *)
{
 	killTimer(m_statusTimer);
	killTimer(m_directoryTimer);

	if (m_singleCamera) {
		disconnect(m_singleCamera, SIGNAL(closed()), this, SLOT(singleCameraClosed()));
		m_singleCamera->close();
	}

	if (m_client) {
		m_client->exitThread(); 
		m_client = NULL;
	}

	if (m_controlServer) {
		m_controlServer->exitThread();
		m_controlServer = NULL;
	}

	saveWindowState();

	SyntroUtils::syntroAppExit();
}

void SyntroView::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_directoryTimer) {
		emit requestDir();

		while (m_delayedDeleteList.count() > 0) {
			qint64 lastUpdate = m_delayedDeleteList.at(0)->lastUpdate();

			if (!SyntroUtils::syntroTimerExpired(SyntroClock(), lastUpdate, 5 * SYNTRO_CLOCKS_PER_SEC))
				break;
				
			AVSource *avSource = m_delayedDeleteList.at(0);
			m_delayedDeleteList.removeAt(0);	
			delete avSource;
		}
	}
	else {
		m_controlStatus->setText(m_client->getLinkState());
	}
}

void SyntroView::clientConnected()
{
	emit requestDir();
}

void SyntroView::clientClosed()
{
	ui.actionVideoStreams->setEnabled(false);
	m_clientDirectory.clear();
}

void SyntroView::dirResponse(QStringList directory)
{
	m_clientDirectory = directory;

	if (m_clientDirectory.length() > 0) {
		if (!ui.actionVideoStreams->isEnabled())
			ui.actionVideoStreams->setEnabled(true);
	}
}

void SyntroView::singleCameraClosed()
{
	if (m_singleCamera) {
		delete m_singleCamera;
		m_singleCamera = NULL;

		if (m_selectedSource >= 0 && m_selectedSource < m_windowList.count()) {
			m_windowList[m_selectedSource]->setSelected(false);
			m_avSources[m_selectedSource]->enableAudio(false);
		}

		m_selectedSource = -1;
	}
}

void SyntroView::imageMousePress(QString name)
{
	m_selectedSource = -1;

    for (int i = 0; i < m_windowList.count(); i++) {
		if (m_avSources.at(i)->name() == name) {
			if (m_windowList[i]->selected()) {
				m_windowList[i]->setSelected(false);
				m_avSources[i]->enableAudio(false);
			}
			else {
				m_windowList[i]->setSelected(true);
				m_avSources[i]->enableAudio(true);
				m_selectedSource = i;
			}
		}
		else {
			m_windowList[i]->setSelected(false);
			m_avSources[i]->enableAudio(false);
		}
	}

	if (!m_singleCamera)
		return;

	if (m_selectedSource == -1)
		return;

	m_singleCamera->setSource(m_avSources[m_selectedSource]);
}

void SyntroView::imageDoubleClick(QString name)
{
	// mousePress handles this
	if (m_singleCamera)
		return;

	m_selectedSource = -1;

    for (int i = 0; i < m_windowList.count(); i++) {
		if (m_avSources.at(i)->name() == name) {
			m_selectedSource = i;
			break;
		}
	}

	if (m_selectedSource == -1)
		return;

	m_singleCamera = new ViewSingleCamera(NULL, m_avSources[m_selectedSource]);

	if (!m_singleCamera)
		return;

	connect(m_singleCamera, SIGNAL(closed()), this, SLOT(singleCameraClosed()));
	m_singleCamera->show();

	m_windowList[m_selectedSource]->setSelected(true);
	m_avSources[m_selectedSource]->enableAudio(true);
}

void SyntroView::onShowName()
{
	m_showName = ui.actionShow_name->isChecked();

	for (int i = 0; i < m_windowList.count(); i++)
		m_windowList[i]->setShowName(m_showName);
}

void SyntroView::onShowDate()
{
	m_showDate = ui.actionShow_date->isChecked();

	for (int i = 0; i < m_windowList.count(); i++)
		m_windowList[i]->setShowDate(m_showDate);
}

void SyntroView::onShowTime()
{
	m_showTime = ui.actionShow_time->isChecked();

	for (int i = 0; i < m_windowList.count(); i++)
		m_windowList[i]->setShowTime(m_showTime);
}

void SyntroView::onTextColor()
{
	m_textColor = QColorDialog::getColor(m_textColor, this);

	for (int i = 0; i < m_windowList.count(); i++)
		m_windowList[i]->setTextColor(m_textColor);
}

void SyntroView::onChooseVideoStreams()
{
	QStringList oldStreams;

	for (int i = 0; i < m_avSources.count(); i++)
		oldStreams.append(m_avSources.at(i)->name());

	StreamDialog dlg(this, m_clientDirectory, oldStreams);

	if (dlg.exec() != QDialog::Accepted)
		return;

	if (m_singleCamera)
		m_singleCamera->close();

	QStringList newStreams = dlg.newStreams();

	QList<AVSource *> oldSourceList = m_avSources;
	m_avSources.clear();

	// don't tear down existing streams if we can rearrange them
	for (int i = 0; i < newStreams.count(); i++) {
		int oldIndex = oldStreams.indexOf(newStreams.at(i));

		if (oldIndex == -1)
			addAVSource(newStreams.at(i));
		else
			m_avSources.append(oldSourceList.at(oldIndex));
	}

	// delete streams we no longer need
	for (int i = 0; i < oldStreams.count(); i++) {
		if (newStreams.contains(oldStreams.at(i)))
			continue;

		AVSource *avSource = oldSourceList.at(i);

		if (avSource->servicePort() >= 0) {
			emit disableService(avSource->servicePort());

			// neither of these is really necessary
			avSource->setServicePort(-1);
			avSource->stopBackgroundProcessing();
		}
		
		// we will delete 5 seconds from now
		avSource->setLastUpdate(SyntroClock());
		disconnect(avSource, SIGNAL(newAudio(QByteArray, int, int, int)), this, SLOT(newAudio(QByteArray, int, int, int)));
		m_delayedDeleteList.append(avSource);
		m_displayStats->removeSource(avSource->name());
	}

	layoutGrid();
}

bool SyntroView::addAVSource(QString name)
{
	AVSource *avSource = new AVSource(name);

	if (!avSource)
		return false;

	connect(avSource, SIGNAL(newAudio(QByteArray, int, int, int)), this, SLOT(newAudio(QByteArray, int, int, int)));

	m_avSources.append(avSource);
	m_displayStats->addSource(avSource);

	emit enableService(avSource);

	return true;
}

void SyntroView::layoutGrid()
{
	int rows = 1;
	int count = m_avSources.count();

	m_windowList.clear();

	QWidget *newCentralWidget = new QWidget();

	if (count > 30)
		count = 30;
	
	if (count < 3)
		rows = 1;
	else if (count < 7)
		rows = 2;
	else if (count < 13)
		rows = 3;
	else if (count < 23)
		rows = 4;
	else
		rows = 5;

	int cols = count / rows;

	if (count % rows)
		cols++;

    QGridLayout *grid = new QGridLayout();
	grid->setSpacing(3);
	grid->setContentsMargins(1, 1, 1, 1);
	
	for (int i = 0, k = 0; i < rows && k < count; i++) {
		for (int j = 0; j < cols && k < count; j++) {
			ImageWindow *win = new ImageWindow(m_avSources.at(k), m_showName, m_showDate, m_showTime, m_textColor, newCentralWidget);
			connect(win, SIGNAL(imageMousePress(QString)), this, SLOT(imageMousePress(QString)));
			connect(win, SIGNAL(imageDoubleClick(QString)), this, SLOT(imageDoubleClick(QString)));
			m_windowList.append(win);

			grid->addWidget(m_windowList.at(k), i, j);
			k++;
		}
	}

	for (int i = 0; i < rows; i++)
		grid->setRowStretch(i, 1);

	for (int i = 0; i < cols; i++)
		grid->setColumnStretch(i, 1);

	newCentralWidget->setLayout(grid);

	QWidget *oldCentralWidget = centralWidget();

	setCentralWidget(newCentralWidget);

	delete oldCentralWidget;
}

void SyntroView::initStatusBar()
{
	m_controlStatus = new QLabel(this);
	m_controlStatus->setAlignment(Qt::AlignLeft);
	ui.statusBar->addWidget(m_controlStatus, 1);
}

void SyntroView::initMenus()
{
	connect(ui.actionAbout, SIGNAL(triggered()), this, SLOT(onAbout()));
	connect(ui.actionBasicSetup, SIGNAL(triggered()), this, SLOT(onBasicSetup()));

	connect(ui.actionVideoStreams, SIGNAL(triggered()), this, SLOT(onChooseVideoStreams()));
	ui.actionVideoStreams->setEnabled(false);

	connect(ui.actionAudioSetup, SIGNAL(triggered()), this, SLOT(onAudioSetup()));

	connect(ui.onStats, SIGNAL(triggered()), this, SLOT(onStats()));
	connect(ui.actionShow_name, SIGNAL(triggered()), this, SLOT(onShowName()));
	connect(ui.actionShow_date, SIGNAL(triggered()), this, SLOT(onShowDate()));
	connect(ui.actionShow_time, SIGNAL(triggered()), this, SLOT(onShowTime()));
	connect(ui.actionText_color, SIGNAL(triggered()), this, SLOT(onTextColor()));

	ui.actionShow_name->setChecked(m_showName);
	ui.actionShow_date->setChecked(m_showDate);
	ui.actionShow_time->setChecked(m_showTime);
}

void SyntroView::saveWindowState()
{
	QSettings *settings = SyntroUtils::getSettings();

	settings->beginGroup("Window");
	settings->setValue("Geometry", saveGeometry());
	settings->setValue("State", saveState());
	settings->setValue("showName", m_showName);
	settings->setValue("showDate", m_showDate);
	settings->setValue("showTime", m_showTime);
	settings->setValue("textColor", m_textColor);
	settings->endGroup();
	
	settings->beginWriteArray("streamSources");

	for (int i = 0; i < m_avSources.count(); i++) {
		settings->setArrayIndex(i);
		settings->setValue("source", m_avSources[i]->name());
	}

	settings->endArray();

	delete settings;
}

void SyntroView::restoreWindowState()
{
	QSettings *settings = SyntroUtils::getSettings();

	settings->beginGroup("Window");
	restoreGeometry(settings->value("Geometry").toByteArray());
	restoreState(settings->value("State").toByteArray());

	if (settings->contains("showName")) 
		m_showName = settings->value("showName").toBool();
	else
		m_showName = true;

	if (settings->contains("showDate"))
		m_showDate = settings->value("showDate").toBool();
	else
		m_showDate = true;

	if (settings->contains("showTime"))
		m_showTime = settings->value("showTime").toBool();
	else
		m_showTime = true;

	if (settings->contains("textColor"))
		m_textColor = settings->value("textColor").value<QColor>();
	else
		m_textColor = Qt::white;

	settings->endGroup();
	
	int count = settings->beginReadArray("streamSources");
	
	for (int i = 0; i < count; i++) {
		settings->setArrayIndex(i);
		QString name = settings->value("source", "").toString();

		if (name.length() > 0)
			addAVSource(name);
	}
	
	settings->endArray();

	delete settings;
}

void SyntroView::onAudioSetup()
{
	AudioOutputDlg *aod = new AudioOutputDlg(this);
	if (aod->exec()) {
		audioOutClose();
	    QSettings *settings = SyntroUtils::getSettings();
		settings->beginGroup(AUDIO_GROUP);
		m_audioEnabled = settings->value(AUDIO_ENABLE).toBool();
		settings->endGroup();
		delete settings;
	}
}

void SyntroView::onAbout()
{
	SyntroAbout dlg(this);
	dlg.exec();
}

void SyntroView::onBasicSetup()
{
	BasicSetupDlg dlg(this);
	dlg.exec();
}

void SyntroView::newAudio(QByteArray data, int rate, int channels, int size)
{
	if (!m_audioEnabled)
		return;

	if ((m_audioRate != rate) || (m_audioSize != size) || (m_audioChannels != channels)) {
		if (!audioOutOpen(rate, channels, size)) {
            qDebug() << "Failed to open audio out device";
            return;
        }

        m_audioRate = rate;
		m_audioSize = size;
		m_audioChannels = channels;
	}

	audioOutWrite(data);
}

#if defined(Q_OS_WIN) || defined(Q_OS_OSX)
bool SyntroView::audioOutOpen(int rate, int channels, int size)
{
    QString outputDeviceName;
    QAudioDeviceInfo outputDeviceInfo;
    bool found = false;
    int bufferSize;

	if (m_audioOut != NULL)
		audioOutClose();

    if ((rate == 0) || (channels == 0) || (size == 0))
        return false;

    QSettings *settings = SyntroUtils::getSettings();
    settings->beginGroup(AUDIO_GROUP);
    outputDeviceName = settings->value(AUDIO_OUTPUT_DEVICE).toString();
    settings->endGroup();
    delete settings;

    if (outputDeviceName != AUDIO_DEFAULT_DEVICE) {
        foreach (const QAudioDeviceInfo& deviceInfo, QAudioDeviceInfo::availableDevices(QAudio::AudioOutput)) {
            qDebug() << "Device name: " << deviceInfo.deviceName();
            qDebug() << "    codec: " << deviceInfo.supportedCodecs();
            qDebug() << "    channels:" << deviceInfo.supportedChannelCounts();
            qDebug() << "    rates:" << deviceInfo.supportedSampleRates();
            qDebug() << "    sizes:" << deviceInfo.supportedSampleSizes();
            qDebug() << "    types:" << deviceInfo.supportedSampleTypes();
            qDebug() << "    order:" << deviceInfo.supportedByteOrders();

            if (deviceInfo.deviceName() == outputDeviceName) {
                outputDeviceInfo = deviceInfo;
                found = true;
            }
        }
        if (!found) {
            qWarning() << "Could not find audio device " << outputDeviceName;
            return false;
        }
    } else {
        outputDeviceInfo = QAudioDeviceInfo::defaultOutputDevice();
    }


    QAudioFormat format;

#ifdef Q_OS_OSX
    if (rate == 8000) {
        format.setSampleRate(48000);
        bufferSize = 48000 * 2 * (size / 8) / 3;
    } else {
        format.setSampleRate(rate);
        bufferSize = rate * 2 * (size / 8) / 3;
    }
    // Mac built-in only supports 2 channels

    format.setChannelCount(2);
#else
    bufferSize = rate * channels * (size / 8) / 3;
    format.setSampleRate(rate);
    format.setChannelCount(channels);
#endif
    format.setSampleSize(size);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);

     if (!outputDeviceInfo.isFormatSupported(format)) {
        qWarning() << "Cannot play audio.";
        return false;
    }

	if (m_audioOut != NULL) {
        delete m_audioOut;
		m_audioOut = NULL;
		m_audioOutDevice = NULL;
	}

    m_audioOut = new QAudioOutput(outputDeviceInfo, format, this);
    m_audioOut->setBufferSize(bufferSize);

//    qDebug() << "Buffer size: " << m_audioOut->bufferSize();

//    connect(m_audioOut, SIGNAL(stateChanged(QAudio::State)), this, SLOT(handleAudioOutStateChanged(QAudio::State)));

	m_audioOutDevice = m_audioOut->start();

	return true;
}

void SyntroView::audioOutClose()
{
	if (m_audioOut != NULL) {
		delete m_audioOut;
		m_audioOut = NULL;
	}

	m_audioOutDevice = NULL;
	m_audioRate = -1;
	m_audioSize = -1;
	m_audioChannels = -1;
}

bool SyntroView::audioOutWrite(const QByteArray& audioData)
{
	if (m_audioOutDevice == NULL)
		return false;

#ifdef Q_OS_OSX
    return m_audioOutDevice->write(convertToMac(audioData));
#else
	return m_audioOutDevice->write(audioData) == audioData.length();
#endif
}

void SyntroView::handleAudioOutStateChanged(QAudio::State /* state */)
{
//	qDebug() << "Audio state " << state;
}

#else

bool SyntroView::audioOutOpen(int rate, int channels, int size)
{
    int err;
    snd_pcm_hw_params_t *params;
    QString deviceString;

    if (m_audioOutIsOpen) 
		audioOutClose();
    if ((rate == 0) || (channels == 0) || (size == 0))
        return false;

    QSettings *settings = SyntroUtils::getSettings();

    settings->beginGroup(AUDIO_GROUP);

    int device = settings->value(AUDIO_OUTPUT_DEVICE).toInt();
    int card = settings->value(AUDIO_OUTPUT_CARD).toInt();

    settings->endGroup();
    delete settings;

    deviceString = QString("plughw:%1,%2").arg(card).arg(device);

    if ((err = snd_pcm_open(&m_audioOutHandle, qPrintable(deviceString), SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        return false;
    }
    snd_pcm_format_t sampleSize;

    switch (size) {
    case 8:
        sampleSize = SND_PCM_FORMAT_S8;
        break;

    case 32:
        sampleSize = SND_PCM_FORMAT_S32_LE;
        break;

    default:
        sampleSize = SND_PCM_FORMAT_S16_LE;
        break;

   }

    params = NULL;
    if (snd_pcm_hw_params_malloc(&params) < 0)
        goto openError;
    if (snd_pcm_hw_params_any(m_audioOutHandle, params) < 0)
        goto openError;
    if (snd_pcm_hw_params_set_access(m_audioOutHandle, params, SND_PCM_ACCESS_RW_INTERLEAVED) < 0)
        goto openError;
    if (snd_pcm_hw_params_set_format(m_audioOutHandle, params, sampleSize) < 0)
        goto openError;
    if (snd_pcm_hw_params_set_rate_near(m_audioOutHandle, params, (unsigned int *)&rate, 0) < 0)
        goto openError;
    if (snd_pcm_hw_params_set_channels(m_audioOutHandle, params, channels) < 0)
        goto openError;
    if (snd_pcm_hw_params(m_audioOutHandle, params) < 0)
        goto openError;
    if (snd_pcm_nonblock(m_audioOutHandle, 1) < 0)
        goto openError;
    snd_pcm_hw_params_free(params);
    params = NULL;

    if ((err = snd_pcm_prepare(m_audioOutHandle)) < 0)
        goto openError;

    m_audioOutSampleSize = channels * (size / 8);                       // bytes per sample

    m_audioOutIsOpen = true;
    return true;

openError:
    snd_pcm_close(m_audioOutHandle);
    if (params != NULL)
        snd_pcm_hw_params_free(params);
    m_audioOutIsOpen = false;
    return false;
}

void SyntroView::audioOutClose()
{
	if (m_audioOutIsOpen)
		snd_pcm_close(m_audioOutHandle);
    m_audioOutIsOpen = false;
	m_audioRate = -1;
	m_audioSize = -1;
	m_audioChannels = -1;
}

bool SyntroView::audioOutWrite(const QByteArray& audioData)
{
    int writtenLength;
    int samples = audioData.length() / m_audioOutSampleSize;

    writtenLength = snd_pcm_writei(m_audioOutHandle, audioData.constData(), samples);
    if (writtenLength == -EPIPE) {
        snd_pcm_prepare(m_audioOutHandle);
    }
    return writtenLength == samples;
}
#endif

QByteArray SyntroView::convertToMac(const QByteArray& audioData)
{
    QByteArray newData;
    int sampleLength;
    int sampleCount;
    int sampleOffset;
    int copyCount;

    if ((m_audioRate != 8000) && (m_audioChannels == 2))
        return audioData;

    sampleLength = (m_audioSize / 8) * m_audioChannels;
    sampleCount = audioData.count() / sampleLength;

    if (m_audioRate == 8000)
        copyCount = 6;
    else
        copyCount = 1;

    sampleOffset = 0;

    for (int sampleIndex = 0; sampleIndex < sampleCount; sampleIndex++, sampleOffset += sampleLength) {
        for (int copy = 0; copy < copyCount; copy++) {
            for (int i = 0; i < sampleLength; i++)
                newData.append(audioData[sampleOffset + i]);
            if (m_audioChannels == 1) {
                for (int i = 0; i < sampleLength; i++)
                    newData.append(audioData[sampleOffset + i]);
            }
        }

    }
    return newData;
}
