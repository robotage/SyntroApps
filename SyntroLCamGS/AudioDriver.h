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

#ifndef AUDIODRIVER_H
#define AUDIODRIVER_H

#include "SyntroLib.h"
#include <QSize>
#include <QSettings>

//  Size in bits of sample

#define AUDIO_FIXED_SIZE            16

#define ALSA_PCM_NEW_HW_PARAMS_API
#include <alsa/asoundlib.h>

//  group for audio related entries

#define	AUDIO_GROUP                   "AudioGroup"

// service enable flag

#define	AUDIO_ENABLE                  "AudioEnable"

// audio source to use

#define	AUDIO_INPUT_CARD               "AudioInputCard"
#define	AUDIO_INPUT_DEVICE             "AudioInputDevice"

//  parameters to use

#define AUDIO_CHANNELS                 "AudioChannels"
#define AUDIO_SAMPLERATE               "AudioSampleRate"

class AudioDriver : public SyntroThread
{
	Q_OBJECT

public:
    AudioDriver();

public slots:
    void newAudioSrc();

signals:
    void newAudio(QByteArray);
    void audioState(QString);
    void audioFormat(int sampleRate, int channels, int sampleSize);

protected:
    void initThread();
    void timerEvent(QTimerEvent *event);
    void finishThread();

private:
    bool loadSettings();
    bool deviceExists();
    void startCapture();
    void stopCapture();
    bool openDevice();
    void closeDevice();

    int m_audioDevice;
    int m_audioCard;

    snd_pcm_t *m_handle;
    snd_pcm_hw_params_t *m_params;
    unsigned char *m_buffer;

    int m_audioChannels;
    int m_audioSampleRate;
    int m_audioFramesPerBlock;
    int m_bytesPerBlock;

    bool m_enabled;

    int m_state;
    int m_ticks;
    int m_timer;
};

#endif // AUDIODRIVER_H
