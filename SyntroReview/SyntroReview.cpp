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

#include <QFileDialog>
#include "SyntroReview.h"
#include "ReviewClient.h"
#include "SyntroAboutDlg.h"
#include "DBFileSelector.h"
#include "BasicSetupDlg.h"
#include "CFSDialog.h"
#include "AudioOutputDlg.h"

SyntroReview::SyntroReview(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);

	layoutWindow();
	restoreWindowState();

	connect(ui.actionExit, SIGNAL(triggered()), this, SLOT(close()));
	connect(ui.actionAbout, SIGNAL(triggered()), this, SLOT(onAbout()));
	connect(m_slider, SIGNAL(sliderMoved(int)), this, SLOT(sliderMoved(int)));
	connect(ui.actionBasicSetup, SIGNAL(triggered()), this, SLOT(onBasicSetup()));
	connect(ui.actionCFSSelection, SIGNAL(triggered()), this, SLOT(onCFSSelection()));
	connect(ui.actionAudioSetup, SIGNAL(triggered()), this, SLOT(onAudioSetup()));
	ui.actionOpen->setEnabled(false);
	ui.actionClose->setEnabled(false);
	ui.actionCFSSelection->setEnabled(false);

	SyntroUtils::syntroAppInit();

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
	m_client = new ReviewClient(this);
	connect(ui.actionOpen, SIGNAL(triggered()), this, SLOT(onOpen()));
	connect(ui.actionClose, SIGNAL(triggered()), this, SLOT(onClose()));
	connect(m_client, SIGNAL(newDirectory(QStringList)), this, SLOT(newDirectory(QStringList)), Qt::QueuedConnection);
	connect(m_client, SIGNAL(showImage(QImage, QByteArray, unsigned int, QDateTime)), 
			this, SLOT(showImage(QImage, QByteArray, unsigned int, QDateTime)), Qt::QueuedConnection);
	connect(m_client, SIGNAL(newAudio(QByteArray, int, int, int)), this, SLOT(newAudio(QByteArray, int, int, int)));
	connect(m_client, SIGNAL(newCFSState(int)), this, SLOT(newCFSState(int)), Qt::QueuedConnection);
	connect(m_client, SIGNAL(newFileLength(unsigned int)), this, SLOT(newFileLength(unsigned int)), Qt::QueuedConnection);
	connect(this, SIGNAL(openFile(QString)), m_client, SLOT(openFile(QString)), Qt::QueuedConnection);
	connect(this, SIGNAL(closeFile()), m_client, SLOT(closeFile()), Qt::QueuedConnection);
	connect(this, SIGNAL(setPlayMode(int, bool)), m_client, SLOT(setPlayMode(int, bool)), Qt::QueuedConnection);
	connect(m_client, SIGNAL(newPlayMode(int)), this, SLOT(newPlayMode(int)), Qt::QueuedConnection);
	connect(this, SIGNAL(setFrameIndex(unsigned int, int, bool)), m_client, SLOT(setFrameIndex(unsigned int, int, bool)), Qt::QueuedConnection);

	connect(m_client, SIGNAL(dirResponse(QStringList)), this, SLOT(dirResponse(QStringList)));
	connect(this, SIGNAL(requestDir()), m_client, SLOT(requestDir()));
	connect(this, SIGNAL(newCFSList()), m_client, SLOT(newCFSList()));

	m_client->resumeThread();

	m_statusTimer = startTimer(2000);
	m_directoryTimer = startTimer(10000);

	setWindowTitle(QString("%1 - %2")
		.arg(SyntroUtils::getAppType())
		.arg(SyntroUtils::getAppName()));

	setDisabledPlayControls();
}

SyntroReview::~SyntroReview()
{
}

void SyntroReview::showImage(QImage frame, QByteArray frameCompressed, unsigned int recordIndex, QDateTime timecode)
{	
	QMessageBox messageBox;

	m_timecode->setText(displayTimecode(timecode));
	m_currentIndex = recordIndex;
	m_currentTimecode = timecode;
	m_frameCompressed = frameCompressed;
	m_frame = frame;

	if (!m_slider->isSliderDown())							// this prevents feedback loop
		m_slider->setSliderPosition(recordIndex);

	if (frame.width() == 0)
		return;

	if (!isMinimized()) 
		m_cameraView->setPixmap(QPixmap::fromImage(frame.scaled(m_cameraView->size(), Qt::KeepAspectRatio)));
}

void SyntroReview::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_directoryTimer) {
		emit requestDir();
	}
	else {
		m_controlStatus->setText(m_client->getLinkState());
	}
}

void SyntroReview::closeEvent(QCloseEvent *)
{
	killTimer(m_statusTimer);
	killTimer(m_directoryTimer);
	saveWindowState();
	m_client->exitThread();
	SyntroUtils::syntroAppExit();
}

void SyntroReview::dirResponse(QStringList directory)
{
	m_clientDirectory = directory;
	if (m_clientDirectory.length() > 0) {
		if (!ui.actionCFSSelection->isEnabled())
			ui.actionCFSSelection->setEnabled(true);
	}
}

void SyntroReview::onCFSSelection()
{
	QMessageBox msgBox;

	onClose();

	CFSDialog dlg(this, m_clientDirectory);

	if (dlg.exec()) {
		emit newCFSList();
	}
}


void SyntroReview::onOpen()
{
    DBFileSelector selector(this, m_directory, SYNTRO_RECORD_SRF_RECORD_DOTEXT, &m_filePath);
	if (selector.exec() == QDialog::Accepted) {
		emit openFile(m_filePath);
		m_file->setText(m_filePath);
	}
}

void SyntroReview::onClose()
{
	QPixmap gray(size());
	gray.fill(Qt::gray);
	m_cameraView->setPixmap(gray);
	
	emit closeFile();
	m_file->setText("...");
}

void SyntroReview::onAbout()
{
	SyntroAbout dlg(this);
	dlg.exec();
}

void SyntroReview::onBasicSetup()
{
	BasicSetupDlg dlg(this);
	dlg.exec();
}

void SyntroReview::newDirectory(QStringList directory)
{
	m_directory = directory;
}

void SyntroReview::newCFSState(int state)
{
	m_CFSState = state;
	switch(state) {
		case SYNTROREVIEW_CFS_STATE_WAITING_FOR_DIRECTORY:
			m_CFSStatus->setText("Connecting to Cloud File System...");
			ui.actionOpen->setEnabled(false);
			ui.actionClose->setEnabled(false);
			setDisabledPlayControls();
			break;

		case SYNTROREVIEW_CFS_STATE_IDLE:
            m_CFSStatus->setText("Cloud File System interface ready");
			ui.actionOpen->setEnabled(true);
			ui.actionClose->setEnabled(false);
			setDisabledPlayControls();
			break;

		case SYNTROREVIEW_CFS_STATE_OPENING:	
			m_CFSStatus->setText("Opening file");
			ui.actionOpen->setEnabled(false);
			ui.actionClose->setEnabled(false);
			setDisabledPlayControls();
			break;

		case SYNTROREVIEW_CFS_STATE_OPEN:	
			m_CFSStatus->setText("File is active");
			ui.actionOpen->setEnabled(false);
			ui.actionClose->setEnabled(true);
			setStoppedPlayControls();
			setStoppedPlayControls();
			break;

		case SYNTROREVIEW_CFS_STATE_CLOSING:		
			m_CFSStatus->setText("Closing file");
			ui.actionOpen->setEnabled(false);
			ui.actionClose->setEnabled(false);
			setDisabledPlayControls();
			break;

		default:
            m_CFSStatus->setText("Illegal SyntroDB interface state");
			ui.actionOpen->setEnabled(false);
			ui.actionClose->setEnabled(false);
			m_CFSState = SYNTROREVIEW_CFS_STATE_IDLE;
			setDisabledPlayControls();
			break;
	}
}

void SyntroReview::newPlayMode(int playMode)
{
	int oldPlayMode;

	if (m_CFSState != SYNTROREVIEW_CFS_STATE_OPEN)
		return;												// no open file anyway
	oldPlayMode = m_playMode;
	m_playMode = playMode;
	if (playMode == SYNTROREVIEW_PLAY_MODE_STOPPED) { 		// now stopped due to hit start of file
		if ((oldPlayMode == SYNTROREVIEW_PLAY_MODE_REVERSE) || (oldPlayMode == SYNTROREVIEW_PLAY_MODE_FASTREVERSE)) {
			if (m_buttonLoop->isChecked()) {				// need to go into reverse play again
				emit setFrameIndex(m_fileLength - 1, oldPlayMode, false);	// reset to end of file
				m_playMode = oldPlayMode;					// restore the mode
				return;
			}
		}
		setStoppedPlayControls();
		return;
	}
	if (playMode == SYNTROREVIEW_PLAY_MODE_PAUSED) {		// now paused as hit end of file
		if ((oldPlayMode == SYNTROREVIEW_PLAY_MODE_PLAY) || (oldPlayMode == SYNTROREVIEW_PLAY_MODE_FASTFORWARD)) {
			if (m_buttonLoop->isChecked()) {				// need to go into forward play again
				emit setFrameIndex(0, oldPlayMode, false);	// reset to start of file
				m_playMode = oldPlayMode;					// restore the mode
				return;
			}
		}
		setPausedPlayControls();
		return;
	}
}


void SyntroReview::newFileLength(unsigned int fileLength)
{
	m_fileLength = fileLength;
	m_slider->setRange(0, fileLength-1);
	m_slider->setTickInterval(fileLength/20);
}

void SyntroReview::sliderMoved(int index)
{
	if (m_CFSState != SYNTROREVIEW_CFS_STATE_OPEN)
		return;												// only care if there's an open file
	if (m_playMode != SYNTROREVIEW_PLAY_MODE_PAUSED) {
		m_playMode = SYNTROREVIEW_PLAY_MODE_PAUSED;			// make sure we are paused
		setPausedPlayControls();
	}
	emit setFrameIndex(index, SYNTROREVIEW_PLAY_MODE_PAUSED, false);	// tell the client
}

void SyntroReview::layoutWindow()
{

	QWidget *centralWidget = new QWidget(this);
	QVBoxLayout *verticalLayout = new QVBoxLayout(centralWidget);
	verticalLayout->setSpacing(6);
	verticalLayout->setContentsMargins(10, 5, 10, 5);
	m_cameraView = new QLabel(centralWidget);

	QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	sizePolicy.setHorizontalStretch(0);
	sizePolicy.setVerticalStretch(0);
	sizePolicy.setHeightForWidth(m_cameraView->sizePolicy().hasHeightForWidth());
	m_cameraView->setSizePolicy(sizePolicy);
	m_cameraView->setMinimumSize(QSize(320, 240));
	m_cameraView->setAlignment(Qt::AlignCenter);

	// set up video information

	QHBoxLayout *hLayout = new QHBoxLayout();
	hLayout->setSpacing(6);
	hLayout->setContentsMargins(5, 5, 5, 5);
	hLayout->setAlignment(Qt::AlignLeft);

	m_fileLabel = new QLabel(this);
	m_fileLabel->setAlignment(Qt::AlignLeft);
	m_fileLabel->setText("File: ");
	hLayout->addWidget(m_fileLabel);

	m_file = new QLabel(this);
	m_file->setAlignment(Qt::AlignLeft);
	m_file->setText("...");
	m_file->setFrameStyle(QFrame::Sunken | QFrame::Panel);
	m_file->setMinimumWidth(400);
	hLayout->addWidget(m_file);

	m_timecodeLabel = new QLabel(this);
	m_timecodeLabel->setAlignment(Qt::AlignLeft);
	m_timecodeLabel->setText("Timecode: ");
	hLayout->addWidget(m_timecodeLabel);

	m_timecode = new QLabel(this);
	m_timecode->setAlignment(Qt::AlignLeft);
	m_timecode->setText("...");
	m_timecode->setFrameStyle(QFrame::Sunken | QFrame::Panel);
	m_timecode->setMinimumWidth(200);
	hLayout->addWidget(m_timecode);

	verticalLayout->addLayout(hLayout);

	verticalLayout->addWidget(m_cameraView);

	m_slider = new QSlider(Qt::Horizontal, centralWidget);
	m_slider->setMinimumWidth(320);
	m_slider->setTickPosition(QSlider::TicksBothSides);
	m_slider->setRange(0, 99);
	m_slider->setTickInterval(10);
	verticalLayout->addWidget(m_slider);

	m_playControls = new QDialogButtonBox(centralWidget);
	m_playControls->setCenterButtons(true);
	m_buttonFastReverse = m_playControls->addButton("Fast rev", QDialogButtonBox::ActionRole);
	m_buttonReverse = m_playControls->addButton("Reverse", QDialogButtonBox::ActionRole);
	m_buttonFrameMinus = m_playControls->addButton("Frame -", QDialogButtonBox::ActionRole);
	m_buttonStop = m_playControls->addButton("Stop", QDialogButtonBox::ActionRole);
	m_buttonPause = m_playControls->addButton("Pause", QDialogButtonBox::ActionRole);
	m_buttonFramePlus = m_playControls->addButton("Frame +", QDialogButtonBox::ActionRole);
	m_buttonPlay = m_playControls->addButton("Play", QDialogButtonBox::ActionRole);
	m_buttonFastForward = m_playControls->addButton("FastFwd", QDialogButtonBox::ActionRole);
	verticalLayout->addWidget(m_playControls);
	connect(m_playControls, SIGNAL(clicked(QAbstractButton *)), this, SLOT(playButtonClicked(QAbstractButton *))); 

	hLayout = new QHBoxLayout();
	hLayout->setSpacing(6);
	hLayout->setContentsMargins(5, 5, 5, 5);
	hLayout->setAlignment(Qt::AlignCenter);
	m_buttonLoop = new QRadioButton("Loop play", centralWidget);
	m_buttonLoop->setContentsMargins(10, 0, 0, 0);
	hLayout->addWidget(m_buttonLoop);
	verticalLayout->addLayout(hLayout);

	m_saveControls = new QDialogButtonBox(centralWidget);
	m_saveControls->setCenterButtons(true);
	m_saveFrame = m_saveControls->addButton("Save frame", QDialogButtonBox::ActionRole);
	verticalLayout->addWidget(m_saveControls);
	connect(m_saveControls, SIGNAL(clicked(QAbstractButton *)), this, SLOT(saveButtonClicked(QAbstractButton *))); 

	setCentralWidget(centralWidget);

	m_controlStatus = new QLabel(this);
	m_controlStatus->setAlignment(Qt::AlignLeft);
	m_controlStatus->setText("");
	ui.statusBar->addWidget(m_controlStatus, 1);

	m_CFSStatus = new QLabel(this);
	m_CFSStatus->setAlignment(Qt::AlignLeft);
	m_CFSStatus->setText("");
	ui.statusBar->addWidget(m_CFSStatus, 1);
}

void SyntroReview::saveWindowState()
{
	QSettings *settings = SyntroUtils::getSettings();
	
	settings->beginGroup("MainWindow");
	settings->setValue("Geometry", saveGeometry());
	settings->setValue("State", saveState());
	settings->endGroup();
	
	delete settings;
}

void SyntroReview::restoreWindowState()
{
	QSettings *settings = SyntroUtils::getSettings();
	
	settings->beginGroup("MainWindow");
	restoreGeometry(settings->value("Geometry").toByteArray());
	restoreState(settings->value("State").toByteArray());
	settings->endGroup();
	
	delete settings;
}

void SyntroReview::setDisabledPlayControls()
{
	m_buttonPlay->setDisabled(true);
	m_buttonFramePlus->setDisabled(true);
	m_buttonPause->setDisabled(true);
	m_buttonStop->setDisabled(true);
	m_buttonFrameMinus->setDisabled(true);
	m_buttonReverse->setDisabled(true);
	m_buttonFastReverse->setDisabled(true);
	m_buttonFastForward->setDisabled(true);
	m_saveFrame->setDisabled(true);
}

void SyntroReview::setStoppedPlayControls()
{
	m_buttonPlay->setDisabled(false);
	m_buttonFramePlus->setDisabled(false);
	m_buttonPause->setDisabled(true);
	m_buttonStop->setDisabled(true);
	m_buttonFrameMinus->setDisabled(true);
	m_buttonReverse->setDisabled(true);
	m_buttonFastReverse->setDisabled(true);
	m_buttonFastForward->setDisabled(false);
	m_saveFrame->setDisabled(false);
}

void SyntroReview::setPausedPlayControls()
{
	m_buttonPlay->setDisabled(false);
	m_buttonFramePlus->setDisabled(false);
	m_buttonPause->setDisabled(true);
	m_buttonStop->setDisabled(false);
	m_buttonFrameMinus->setDisabled(false);
	m_buttonReverse->setDisabled(false);
	m_buttonFastReverse->setDisabled(false);
	m_buttonFastForward->setDisabled(false);
	m_saveFrame->setDisabled(false);
}

void SyntroReview::setPlayingPlayControls()
{
	m_buttonPlay->setDisabled(true);
	m_buttonFramePlus->setDisabled(true);
	m_buttonPause->setDisabled(false);
	m_buttonStop->setDisabled(false);
	m_buttonFrameMinus->setDisabled(true);
	m_buttonReverse->setDisabled(false);
	m_buttonFastReverse->setDisabled(false);
	m_buttonFastForward->setDisabled(false);
	m_saveFrame->setDisabled(true);
}

void SyntroReview::setReversePlayControls()
{
	m_buttonPlay->setDisabled(false);
	m_buttonFramePlus->setDisabled(true);
	m_buttonPause->setDisabled(false);
	m_buttonStop->setDisabled(false);
	m_buttonFrameMinus->setDisabled(true);
	m_buttonReverse->setDisabled(true);
	m_buttonFastReverse->setDisabled(false);
	m_buttonFastForward->setDisabled(false);
	m_saveFrame->setDisabled(true);
}

void SyntroReview::setFastReversePlayControls()
{
	m_buttonPlay->setDisabled(false);
	m_buttonFramePlus->setDisabled(true);
	m_buttonPause->setDisabled(false);
	m_buttonStop->setDisabled(false);
	m_buttonFrameMinus->setDisabled(true);
	m_buttonReverse->setDisabled(false);
	m_buttonFastReverse->setDisabled(true);
	m_buttonFastForward->setDisabled(false);
	m_saveFrame->setDisabled(true);
}

void SyntroReview::setFastForwardPlayControls()
{
	m_buttonPlay->setDisabled(false);
	m_buttonFramePlus->setDisabled(true);
	m_buttonPause->setDisabled(false);
	m_buttonStop->setDisabled(false);
	m_buttonFrameMinus->setDisabled(true);
	m_buttonReverse->setDisabled(false);
	m_buttonFastReverse->setDisabled(false);
	m_buttonFastForward->setDisabled(true);
	m_saveFrame->setDisabled(true);
}

void SyntroReview::playButtonClicked(QAbstractButton *button)
{
	if (m_CFSState != SYNTROREVIEW_CFS_STATE_OPEN)
		return;												// no open file anyway

	if (button == m_buttonStop){
		setStoppedPlayControls();
		m_playMode = SYNTROREVIEW_PLAY_MODE_STOPPED;
	} else if (button == m_buttonPause){
		setPausedPlayControls();
		m_playMode = SYNTROREVIEW_PLAY_MODE_PAUSED;
	} else if (button == m_buttonFramePlus){
		setPausedPlayControls();
		m_playMode = SYNTROREVIEW_PLAY_MODE_PAUSED;
		if (m_currentIndex < m_fileLength -1)
			emit setFrameIndex(m_currentIndex + 1, SYNTROREVIEW_PLAY_MODE_PAUSED, false);
	} else if (button == m_buttonPlay){
		setPlayingPlayControls();
		m_playMode = SYNTROREVIEW_PLAY_MODE_PLAY;
	} else if (button == m_buttonFrameMinus){
		setPausedPlayControls();
		m_playMode = SYNTROREVIEW_PLAY_MODE_PAUSED;
		if (m_currentIndex > 1)
			emit setFrameIndex(m_currentIndex - 1, SYNTROREVIEW_PLAY_MODE_PAUSED, false);
	} else if (button == m_buttonReverse){
		setReversePlayControls();
		m_playMode = SYNTROREVIEW_PLAY_MODE_REVERSE;
	} else if (button == m_buttonFastReverse){
		setFastReversePlayControls();
		m_playMode = SYNTROREVIEW_PLAY_MODE_FASTREVERSE;
	} else if (button == m_buttonFastForward){
		setFastForwardPlayControls();
		m_playMode = SYNTROREVIEW_PLAY_MODE_FASTFORWARD;
	}
	emit setPlayMode(m_playMode, false);
}

void SyntroReview::saveButtonClicked(QAbstractButton *button)
{
	QFileDialog *fileDialog;

	if (m_CFSState != SYNTROREVIEW_CFS_STATE_OPEN)
		return;												// no open file anyway

	if (button == m_saveFrame){
		fileDialog = new QFileDialog(this, "JPEG (.jpg) file name");
		fileDialog->setAcceptMode(QFileDialog::AcceptSave);
		fileDialog->setFileMode(QFileDialog::AnyFile);
		fileDialog->selectFile(m_saveFilePath);
		fileDialog->setNameFilter("*.jpg");
		fileDialog->setDefaultSuffix("jpg");
		if (fileDialog->exec()) {
			m_saveFilePath = fileDialog->selectedFiles().at(0);
			saveCurrentFrame();
		}
	}
}

QString SyntroReview::displayTimecode(QDateTime& timecode)
{
	QString str = QString("%1/%2/%3 %4:%5:%6:%7")
		.arg(timecode.date().month(), 2, 10, QLatin1Char('0'))
		.arg(timecode.date().day(), 2, 10, QLatin1Char('0'))
		.arg(timecode.date().year())
		.arg(timecode.time().hour(), 2, 10, QLatin1Char('0'))
		.arg(timecode.time().minute(), 2, 10, QLatin1Char('0'))
		.arg(timecode.time().second(), 2, 10, QLatin1Char('0'))
		.arg(timecode.time().msec(), 3, 10, QLatin1Char('0'));
	return str;
}



void SyntroReview::saveCurrentFrame()
{
	QFile file(m_saveFilePath);
	QMessageBox messageBox;

	if (!file.open(QIODevice::Truncate | QIODevice::Append)) {
		return;
	}

	if (file.write(m_frameCompressed.data(), m_frameCompressed.length()) == m_frameCompressed.length()) {
		messageBox.setText(QString("Frame saved to ") + m_saveFilePath);
		messageBox.setIcon(QMessageBox::Information);
	} else {
		messageBox.setText(QString("Failed to write file to ") + m_saveFilePath);
		messageBox.setIcon(QMessageBox::Warning);
	}
	messageBox.setStandardButtons(QMessageBox::Ok);
	messageBox.exec();
	file.close();
}

void SyntroReview::onAudioSetup()
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

void SyntroReview::newAudio(QByteArray data, int rate, int channels, int size)
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
bool SyntroReview::audioOutOpen(int rate, int channels, int size)
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
/*            qDebug() << "Device name: " << deviceInfo.deviceName();
            qDebug() << "    codec: " << deviceInfo.supportedCodecs();
            qDebug() << "    channels:" << deviceInfo.supportedChannelCounts();
            qDebug() << "    rates:" << deviceInfo.supportedSampleRates();
            qDebug() << "    sizes:" << deviceInfo.supportedSampleSizes();
            qDebug() << "    types:" << deviceInfo.supportedSampleTypes();
            qDebug() << "    order:" << deviceInfo.supportedByteOrders();
			*/
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
        bufferSize = 48000 * 2 * (size / 8);
    } else {
        format.setSampleRate(rate);
        bufferSize = rate * 2 * (size / 8);
    }
    // Mac built-in only supports 2 channels

    format.setChannelCount(2);
#else
    bufferSize = rate * channels * (size / 8);
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

void SyntroReview::audioOutClose()
{
	if (m_audioOut != NULL)
		delete m_audioOut;
	m_audioOut = NULL;
	m_audioOutDevice = NULL;
	m_audioRate = -1;
	m_audioSize = -1;
	m_audioChannels = -1;
}

bool SyntroReview::audioOutWrite(const QByteArray& audioData)
{
	if (m_audioOutDevice == NULL)
		return false;

#ifdef Q_OS_OSX
    return m_audioOutDevice->write(convertToMac(audioData));
#else
	return m_audioOutDevice->write(audioData) == audioData.length();
#endif
}

void SyntroReview::handleAudioOutStateChanged(QAudio::State /* state */)
{
//	qDebug() << "Audio state " << state;
}

#else

bool SyntroReview::audioOutOpen(int rate, int channels, int size)
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

void SyntroReview::audioOutClose()
{
	if (m_audioOutIsOpen)
		snd_pcm_close(m_audioOutHandle);
    m_audioOutIsOpen = false;
	m_audioRate = -1;
	m_audioSize = -1;
	m_audioChannels = -1;
}

bool SyntroReview::audioOutWrite(const QByteArray& audioData)
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

QByteArray SyntroReview::convertToMac(const QByteArray& audioData)
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
