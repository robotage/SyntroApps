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

#include "AudioDriver.h"

#include "SyntroLib.h"
#include "CamClient.h"
#include "SyntroLCam.h"

#include <QFile>

#define STATE_DISCONNECTED  0
#define STATE_DETECTED      1
#define STATE_CAPTURING     2

#define TICK_DURATION_MS        500
#define DISCONNECT_MIN_TICKS    4
#define DETECT_MIN_TICKS        10
#define CONNECT_MIN_TICKS       6
#define TIMER_INTERVAL          10


AudioDriver::AudioDriver() : SyntroThread("AudioDriver", PRODUCT_TYPE)
{
    m_buffer = NULL;
    m_handle = NULL;
    m_params = NULL;
    m_timer = -1;
}

void AudioDriver::newAudioSrc()
{
    stopCapture();
    startCapture();
}

void AudioDriver::initThread()
{
    startCapture();
}

void AudioDriver::finishThread()
{
    stopCapture();
}

void AudioDriver::startCapture()
{
    loadSettings();

    if (!m_enabled)
        return;

    // optimize the typical case on startup
    if (deviceExists() && openDevice()) {
        m_state = STATE_CAPTURING;
        emit audioState(QString("%1/%2/%3").arg(m_audioChannels).arg(AUDIO_FIXED_SIZE).arg(m_audioSampleRate));
        m_ticks = CONNECT_MIN_TICKS;
    } 
    else {
        m_state = STATE_DISCONNECTED;
        m_ticks = 0;
        emit audioState("Disconnected");
    }

    m_timer = startTimer(TIMER_INTERVAL);
}

void AudioDriver::stopCapture()
{
    if (m_timer != -1)
        killTimer(m_timer);

    m_timer = -1;

    closeDevice();
}

bool AudioDriver::loadSettings()
{
    QSettings *settings = SyntroUtils::getSettings();

    settings->beginGroup(AUDIO_GROUP);

    if (!settings->contains(AUDIO_INPUT_DEVICE))
        settings->setValue(AUDIO_INPUT_DEVICE, 0);

    if (!settings->contains(AUDIO_INPUT_CARD))
        settings->setValue(AUDIO_INPUT_CARD, 1);

    if (!settings->contains(AUDIO_ENABLE))
        settings->setValue(AUDIO_ENABLE, true);

    if (!settings->contains(AUDIO_CHANNELS))
        settings->setValue(AUDIO_CHANNELS, 2);

    if (!settings->contains(AUDIO_SAMPLERATE))
        settings->setValue(AUDIO_SAMPLERATE, 8000);

    m_audioDevice = settings->value(AUDIO_INPUT_DEVICE).toInt();
    m_audioCard = settings->value(AUDIO_INPUT_CARD).toInt();
    m_enabled = settings->value(AUDIO_ENABLE).toBool();

    m_audioChannels = settings->value(AUDIO_CHANNELS).toInt();
    m_audioSampleRate = settings->value(AUDIO_SAMPLERATE).toInt();

    emit audioFormat(m_audioSampleRate, m_audioChannels, AUDIO_FIXED_SIZE);

    //  Make blocks last 100mS

    m_audioFramesPerBlock = m_audioSampleRate / 10;

    m_bytesPerBlock = m_audioChannels * (AUDIO_FIXED_SIZE / 8) * m_audioFramesPerBlock;

    settings->endGroup();

    delete settings;

    return true;
}

void AudioDriver::timerEvent(QTimerEvent *)
{
    int rc;

    switch (m_state) {
    case STATE_DISCONNECTED:
        if (++m_ticks > DISCONNECT_MIN_TICKS) {
            if (deviceExists()) {
                m_state = STATE_DETECTED;
                emit audioState("Detected");
                m_ticks = 0;
            }
        }

        msleep(TICK_DURATION_MS);
        break;

    case STATE_DETECTED:
        if (++m_ticks > DETECT_MIN_TICKS) {
            if (openDevice()) {
                m_state = STATE_CAPTURING;
                emit audioState(QString("%1/%2/%3").arg(m_audioChannels).arg(AUDIO_FIXED_SIZE).arg(m_audioSampleRate));
                m_ticks = 0;
            }
        }

        msleep(TICK_DURATION_MS);
        break;

    case STATE_CAPTURING:
        if ((rc = snd_pcm_readi(m_handle, m_buffer, m_audioFramesPerBlock)) != m_audioFramesPerBlock) {
            appLogError(QString("Read from audio interface failed: %1").arg(snd_strerror(rc)));
            closeDevice();
            m_state = STATE_DISCONNECTED;
            m_ticks = 0;
            emit audioState("Disconnected");
            msleep(TICK_DURATION_MS);
        }
        else {
            emit newAudio(QByteArray((const char *)m_buffer, m_bytesPerBlock));
        }

        break;
    }
}

bool AudioDriver::deviceExists()
{
    QFile devices(QString("/proc/asound/pcm"));
    QString pcmEntry;
    QString pcmSearch;

    pcmSearch.sprintf("%02d-%02d", m_audioCard, m_audioDevice);

   if (!devices.open(QIODevice::ReadOnly))
       return false;

   while (true) {
       pcmEntry = devices.readLine();

       if (pcmEntry.size() == 0)
           break;

       if (pcmEntry.startsWith(pcmSearch)) {
           devices.close();
           return true;
       }
   }

   devices.close();

   return false;
}

bool AudioDriver::openDevice()
{
    int rc;
    static bool first_open = true;

    QString audioDevice = QString("plughw:%1,%2").arg(m_audioCard).arg(m_audioDevice);

    if ((rc = snd_pcm_open(&m_handle, audioDevice.toLatin1(), SND_PCM_STREAM_CAPTURE, 0)) < 0) {
	if (first_open) {
            appLogError(QString("Failed to open audio device %1 - %2")
                .arg(audioDevice).arg(snd_strerror(rc)));

            first_open = false;
	}

        return false;
    }

    if ((rc = snd_pcm_hw_params_malloc(&m_params)) < 0) {
        appLogError(QString("Failed to allocate audio hardware parameter structure: %1").arg(snd_strerror(rc)));
        closeDevice();
        return false;
    }

    if ((rc = snd_pcm_hw_params_any(m_handle, m_params)) < 0) {
        appLogError(QString("Failed to initialize audio hardware parameter structure: %1").arg(snd_strerror(rc)));
        closeDevice();
        return false;
    }

    if ((rc = snd_pcm_hw_params_set_access(m_handle, m_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
        appLogError(QString("Failed to set audio hardware access: %1").arg(snd_strerror(rc)));
        closeDevice();
        return false;
    }

    if ((rc = snd_pcm_hw_params_set_format(m_handle, m_params, SND_PCM_FORMAT_S16_LE)) < 0) {
        appLogError(QString("Failed to set audio sample format: %1").arg(snd_strerror(rc)));
        closeDevice();
        return false;
    }

    if ((rc = snd_pcm_hw_params_set_rate_near(m_handle, m_params, (unsigned int *)&m_audioSampleRate, 0)) < 0) {
        appLogError(QString("Failed to set audio sample rate: %1").arg(snd_strerror(rc)));
        closeDevice();
        return false;
    }

    if ((rc = snd_pcm_hw_params_set_channels(m_handle, m_params, m_audioChannels)) < 0) {
        appLogError(QString("Failed to set audio channel count: %1").arg(snd_strerror(rc)));
        closeDevice();
        return false;
    }

    if ((rc = snd_pcm_hw_params(m_handle, m_params)) < 0) {
        appLogError(QString("Failed to set audio parameters: %1").arg(snd_strerror(rc)));
        closeDevice();
        return false;
    }

    snd_pcm_hw_params_free(m_params);
    m_params = NULL;

    if ((rc = snd_pcm_prepare (m_handle)) < 0) {
        appLogError(QString("Failed to prepare audio interface for use: %1").arg(snd_strerror(rc)));
        closeDevice();
        return false;
    }

    m_buffer = (unsigned char *)malloc(m_bytesPerBlock);

    return true;
}

void AudioDriver::closeDevice()
{
    if (m_handle != NULL) {
        snd_pcm_close(m_handle);
        m_handle = NULL;
    }

    if (m_buffer != NULL) {
        free(m_buffer);
        m_buffer = NULL;
    }

    if (m_params != NULL) {
        snd_pcm_hw_params_free(m_params);
        m_params = NULL;
    }
}

