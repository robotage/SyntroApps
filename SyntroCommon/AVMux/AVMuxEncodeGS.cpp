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

#include "AVMuxEncodeGS.h"

#include <qimage.h>
#include <qbuffer.h>
#include <qbytearray.h>

#define GSTBUSMSG

#define PIPELINE_MS_TO_NS       ((qint64)1000000)

#ifdef USING_GSTREAMER
static void newVideoSinkData (GstElement * /*appsink*/,
          gpointer user_data)
{
    ((AVMuxEncode *)user_data)->processVideoSinkData();
}

static void newAudioSinkData (GstElement * /*appsink*/,
          gpointer user_data)
{
    ((AVMuxEncode *)user_data)->processAudioSinkData();
}

static void needAudioSrcData (GstElement * /*appsrc*/, guint /*unused_size*/, gpointer user_data)
{
    ((AVMuxEncode *)user_data)->needAudioData();
}

#ifdef GSTBUSMSG
static gboolean videoBusMessage(GstBus * /*bus*/, GstMessage *message, gpointer data)
{
    GstState old_state, new_state;

    if ((GST_MESSAGE_TYPE(message) == GST_MESSAGE_STATE_CHANGED)  &&
            ((GstElement *)(message->src) == ((AVMuxEncode *)data)->m_videoPipeline)){
         gst_message_parse_state_changed (message, &old_state, &new_state, NULL);
           g_print ("Video element %s changed state from %s to %s.\n",
            GST_OBJECT_NAME (message->src),
            gst_element_state_get_name (old_state),
            gst_element_state_get_name (new_state));
    }
    return TRUE;
}


static gboolean audioBusMessage(GstBus * /*bus*/, GstMessage *message, gpointer data)
{
    GstState old_state, new_state;

    if ((GST_MESSAGE_TYPE(message) == GST_MESSAGE_STATE_CHANGED)  &&
            ((GstElement *)(message->src) == ((AVMuxEncode *)data)->m_audioPipeline)){
         gst_message_parse_state_changed (message, &old_state, &new_state, NULL);
           g_print ("Audio element %s changed state from %s to %s.\n",
            GST_OBJECT_NAME (message->src),
            gst_element_state_get_name (old_state),
            gst_element_state_get_name (new_state));
    }
    return TRUE;
}
#endif  // GSTBUSMSG

static int g_pipelineIndex = 0;

AVMuxEncode::AVMuxEncode(int AVMode) : SyntroThread("AVMuxEncode", "AVMuxEncode")
{
    m_AVMode = AVMode;

    m_videoPipeline = NULL;
    m_audioPipeline = NULL;
    m_appAudioSink = NULL;
    m_appAudioSrc = NULL;
    m_appVideoSink = NULL;
    m_appAudioSrc = NULL;
    m_videoCaps = NULL;
    m_audioCaps = NULL;
    m_videoBusWatch = -1;
    m_audioBusWatch = -1;
    m_audioTimestamp = -1;
    m_pipelinesActive = false;
    m_videoCompressionRate = 1000000;
    m_audioCompressionRate = 128000;
}


void AVMuxEncode::initThread()
{
    m_timer = startTimer(AVMUXENCODE_INTERVAL);
}

void AVMuxEncode::finishThread()
{
    killTimer(m_timer);
    m_pipelinesActive = false;
    deletePipelines();
}

void AVMuxEncode::timerEvent(QTimerEvent * /*event*/)
{
    qint64 now = QDateTime::currentMSecsSinceEpoch() * PIPELINE_MS_TO_NS;

    if (!m_pipelinesActive)
        return;

    if ((m_videoPipeline != NULL) && (now >= m_nextVideoTime)) {
         needVideoData();
        m_nextVideoTime += m_videoInterval;
    }
}

void AVMuxEncode::newVideoData(QByteArray videoData, qint64 timestamp, int param)
{
    AVMUX_QUEUEDATA *qd = new AVMUX_QUEUEDATA;
    qd->data = videoData;
    qd->timestamp = timestamp;
    qd->param = param;

    if (m_AVMode != AVMUXENCODE_AV_TYPE_MJPPCM) {
        m_videoSrcLock.lock();
        m_videoSrcQ.enqueue(qd);
        m_videoSrcLock.unlock();
    } else {
        m_videoSinkLock.lock();
        m_videoSinkQ.enqueue(qd);
        m_videoSinkLock.unlock();
    }
}

void AVMuxEncode::newVideoData(QByteArray videoData)
{
    AVMUX_QUEUEDATA *qd = new AVMUX_QUEUEDATA;
    qd->data = videoData;

    if (m_AVMode != AVMUXENCODE_AV_TYPE_MJPPCM) {
        m_videoSrcLock.lock();
        m_videoSrcQ.enqueue(qd);
        m_videoSrcLock.unlock();
    } else {
        m_videoSinkLock.lock();
        m_videoSinkQ.enqueue(qd);
        m_videoSinkLock.unlock();
    }
}

void AVMuxEncode::newAudioData(QByteArray audioData, qint64 timestamp, int param)
{
    AVMUX_QUEUEDATA *qd = new AVMUX_QUEUEDATA;
    qd->data = audioData;
    qd->timestamp = timestamp;
    qd->param = param;

    if (m_AVMode != AVMUXENCODE_AV_TYPE_MJPPCM) {
        m_audioSrcLock.lock();
        m_audioSrcQ.enqueue(qd);
        m_audioSrcLock.unlock();
    } else {
        m_audioSinkLock.lock();
        m_audioSinkQ.enqueue(qd);
        m_audioSinkLock.unlock();
    }
}

void AVMuxEncode::newAudioData(QByteArray audioData)
{
    AVMUX_QUEUEDATA *qd = new AVMUX_QUEUEDATA;
    qd->data = audioData;

    if (m_AVMode != AVMUXENCODE_AV_TYPE_MJPPCM) {
        m_audioSrcLock.lock();
        m_audioSrcQ.enqueue(qd);
        m_audioSrcLock.unlock();
    } else {
        m_audioSinkLock.lock();
        m_audioSinkQ.enqueue(qd);
        m_audioSinkLock.unlock();
    }
}

bool AVMuxEncode::getCompressedVideo(QByteArray& videoData, qint64& timestamp, int& param)
{
    QMutexLocker lock(&m_videoSinkLock);
    AVMUX_QUEUEDATA *qd;

    if (m_videoSinkQ.empty())
        return false;

    qd = m_videoSinkQ.dequeue();
    videoData = qd->data;
    timestamp = qd->timestamp;
    param = qd->param;
    delete qd;
    return true;
}

bool AVMuxEncode::getCompressedVideo(QByteArray& videoData)
{
    QMutexLocker lock(&m_videoSinkLock);
    AVMUX_QUEUEDATA *qd;

    if (m_videoSinkQ.empty())
        return false;

    qd = m_videoSinkQ.dequeue();
    videoData = qd->data;
    delete qd;
    return true;
}

bool AVMuxEncode::getCompressedAudio(QByteArray& audioData, qint64& timestamp, int& param)
{
    QMutexLocker lock(&m_audioSinkLock);
    AVMUX_QUEUEDATA *qd;

    if (m_audioSinkQ.empty())
        return false;

    qd = m_audioSinkQ.dequeue();
    audioData = qd->data;
    timestamp = qd->timestamp;
    param = qd->param;
    delete qd;
    return true;
}

bool AVMuxEncode::getCompressedAudio(QByteArray& audioData)
{
    QMutexLocker lock(&m_audioSinkLock);
    AVMUX_QUEUEDATA *qd;

    if (m_audioSinkQ.empty())
        return false;

    qd = m_audioSinkQ.dequeue();
    audioData = qd->data;
    delete qd;
    return true;
}

void AVMuxEncode::processVideoSinkData()
{
    GstBuffer *buffer;
    AVMUX_QUEUEDATA *qd;

    m_videoSinkLock.lock();
    if ((buffer = gst_app_sink_pull_buffer((GstAppSink *)(m_appVideoSink))) != NULL) {
        qd = new AVMUX_QUEUEDATA;
        qd->data = QByteArray((const char *)GST_BUFFER_DATA(buffer), GST_BUFFER_SIZE(buffer));
        qd->timestamp = m_lastQueuedVideoTimestamp;
        qd->param = m_lastQueuedVideoParam;
        m_videoSinkQ.enqueue(qd);

        if (m_videoCaps == NULL)
            m_videoCaps = gst_caps_to_string(GST_BUFFER_CAPS(buffer));
//        qDebug() << "Video ts " << GST_BUFFER_TIMESTAMP(buffer);
        gst_buffer_unref(buffer);
     }
    m_videoSinkLock.unlock();
}

void AVMuxEncode::processAudioSinkData()
{
    GstBuffer *buffer;

    m_audioSinkLock.lock();
    if ((buffer = gst_app_sink_pull_buffer((GstAppSink *)(m_appAudioSink))) != NULL) {
        AVMUX_QUEUEDATA *qd = new AVMUX_QUEUEDATA;
        qd->data = QByteArray((const char *)GST_BUFFER_DATA(buffer), GST_BUFFER_SIZE(buffer));
        qd->timestamp = m_lastQueuedAudioTimestamp;
        qd->param = m_lastQueuedAudioParam;
        m_audioSinkQ.enqueue(qd);
        if (m_audioCaps == NULL)
            m_audioCaps = gst_caps_to_string(GST_BUFFER_CAPS(buffer));
        gst_buffer_unref(buffer);
    }
    m_audioSinkLock.unlock();
}

//----------------------------------------------------------
//
//  Pipeline management stuff

bool AVMuxEncode::newPipelines(SYNTRO_AVPARAMS *avParams)
{
    gchar *videoLaunch;
    gchar *audioLaunch;
    GError *error = NULL;
    GstStateChangeReturn ret;

    m_avParams = *avParams;

    printf("width=%d, height=%d, rate=%d\n", avParams->videoWidth, avParams->videoHeight, avParams->videoFramerate);
    printf("channels=%d, rate=%d, size=%d\n", avParams->audioChannels, avParams->audioSampleRate, avParams->audioSampleSize);

    //  Construct the pipelines

    g_pipelineIndex++;

    if (m_AVMode == AVMUXENCODE_AV_TYPE_RTPMP4) {
        videoLaunch = g_strdup_printf (
              " appsrc name=videoSrc%d ! jpegdec ! queue ! ffenc_mpeg4 bitrate=%d "
              " ! queue ! rtpmp4vpay pt=96 ! queue ! appsink name=videoSink%d"
                , g_pipelineIndex, m_videoCompressionRate, g_pipelineIndex);
    } else {
        videoLaunch = g_strdup_printf (
              " appsrc name=videoSrc%d ! jpegdec ! queue ! x264enc bitrate=%d tune=zerolatency rc-lookahead=0"
              " ! queue ! rtph264pay pt=96 ! queue ! appsink name=videoSink%d"
                , g_pipelineIndex, m_videoCompressionRate / 1000, g_pipelineIndex);

    }

    m_videoPipeline = gst_parse_launch(videoLaunch, &error);
    g_free(videoLaunch);

    if (error != NULL) {
        g_print ("could not construct video pipeline: %s\n", error->message);
        g_error_free (error);
        m_videoPipeline = NULL;
        return false;
    }

    audioLaunch = g_strdup_printf (
#ifdef GST_IMX6
                " appsrc name=audioSrc%d ! faac bitrate=%d ! rtpmp4apay pt=97 ! appsink name=audioSink%d "
#else
                " appsrc name=audioSrc%d ! faac bitrate=%d ! rtpmp4apay pt=97 min-ptime=1000000000 ! appsink name=audioSink%d "
#endif
             , g_pipelineIndex, m_audioCompressionRate, g_pipelineIndex);

    m_audioPipeline = gst_parse_launch(audioLaunch, &error);
    g_free(audioLaunch);

    if (error != NULL) {
        g_print ("could not construct audio pipeline: %s\n", error->message);
        g_error_free (error);
        gst_object_unref(m_videoPipeline);
        m_videoPipeline = NULL;
        m_audioPipeline = NULL;
        return false;
    }

    //  find the appsrcs and appsinks

    gchar *videoSink = g_strdup_printf("videoSink%d", g_pipelineIndex);
    if ((m_appVideoSink = gst_bin_get_by_name (GST_BIN (m_videoPipeline), videoSink)) == NULL) {
        g_printerr("Unable to find video appsink\n");
        g_free(videoSink);
        deletePipelines();
        return false;
    }
    g_free(videoSink);

    gchar *videoSrc = g_strdup_printf("videoSrc%d", g_pipelineIndex);
    if ((m_appVideoSrc = gst_bin_get_by_name (GST_BIN (m_videoPipeline), videoSrc)) == NULL) {
            g_printerr("Unable to find video appsrc\n");
            g_free(videoSrc);
            deletePipelines();
            return false;
        }
    g_free(videoSrc);

    gchar *audioSink = g_strdup_printf("audioSink%d", g_pipelineIndex);
    if ((m_appAudioSink = gst_bin_get_by_name (GST_BIN (m_audioPipeline), audioSink)) == NULL) {
        g_printerr("Unable to find audio appsink\n");
        g_free(audioSink);
        deletePipelines();
        return false;
    }
    g_free(audioSink);

    gchar *audioSrc = g_strdup_printf("audioSrc%d", g_pipelineIndex);
    if ((m_appAudioSrc = gst_bin_get_by_name (GST_BIN (m_audioPipeline), audioSrc)) == NULL) {
            g_printerr("Unable to find audio appsrc\n");
            g_free(audioSrc);
            deletePipelines();
            return false;
        }
    g_free(audioSrc);

    g_signal_connect (m_appVideoSink, "new-buffer", G_CALLBACK (newVideoSinkData), this);
    gst_app_sink_set_emit_signals((GstAppSink *)(m_appVideoSink), TRUE);

    g_signal_connect (m_appAudioSink, "new-buffer", G_CALLBACK (newAudioSinkData), this);
    gst_app_sink_set_emit_signals((GstAppSink *)(m_appAudioSink), TRUE);

    gst_app_src_set_caps((GstAppSrc *) (m_appVideoSrc),
             gst_caps_new_simple ("image/jpeg",
             "width", G_TYPE_INT, m_avParams.videoWidth,
             "height", G_TYPE_INT, m_avParams.videoHeight,
             "framerate", GST_TYPE_FRACTION, m_avParams.videoFramerate, 1,
             NULL));
    gst_app_src_set_stream_type((GstAppSrc *)(m_appVideoSrc), GST_APP_STREAM_TYPE_STREAM);

    gst_app_src_set_caps((GstAppSrc *) (m_appAudioSrc),
            gst_caps_new_simple ("audio/x-raw-int",
                      "width", G_TYPE_INT, (gint)m_avParams.audioSampleSize,
                      "depth", G_TYPE_INT, (gint)m_avParams.audioSampleSize,
                      "channels" ,G_TYPE_INT, (gint)m_avParams.audioChannels,
                      "rate",G_TYPE_INT, m_avParams.audioSampleRate,
                      "endianness",G_TYPE_INT,(gint)1234,
                      "signed", G_TYPE_BOOLEAN, (gboolean)TRUE,
                      NULL));

    gst_app_src_set_stream_type((GstAppSrc *)(m_appAudioSrc), GST_APP_STREAM_TYPE_STREAM);
    g_signal_connect(m_appAudioSrc, "need-data", G_CALLBACK (needAudioSrcData), this);

    ret = gst_element_set_state (m_videoPipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr ("Unable to set the video pipeline to the play state.\n");
        deletePipelines();
        return false;
    }

#ifdef GSTBUSMSG
    GstBus *bus;

    bus = gst_pipeline_get_bus(GST_PIPELINE (m_videoPipeline));
    m_videoBusWatch = gst_bus_add_watch (bus, videoBusMessage, this);
    gst_object_unref (bus);

    bus = gst_pipeline_get_bus(GST_PIPELINE (m_audioPipeline));
    m_audioBusWatch = gst_bus_add_watch (bus, audioBusMessage, this);
    gst_object_unref (bus);
#endif

    ret = gst_element_set_state (m_audioPipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr ("Unable to set the audio pipeline to the play state.\n");
        deletePipelines();
        return false;
    }
    m_videoInterval = (1000 * PIPELINE_MS_TO_NS) / m_avParams.videoFramerate;
    m_nextVideoTime = QDateTime::currentMSecsSinceEpoch() * PIPELINE_MS_TO_NS;

    qDebug() << "Pipelines established";
    m_pipelinesActive = true;
    return true;
}

void AVMuxEncode::deletePipelines()
{
    m_videoSrcLock.lock();
    m_audioSrcLock.lock();
    m_videoSinkLock.lock();
    m_audioSinkLock.lock();

    if (m_videoPipeline != NULL) {
        gst_app_sink_set_emit_signals((GstAppSink *)(m_appVideoSink), FALSE);
        g_signal_handlers_disconnect_by_data(m_appVideoSink, this);
        gst_element_set_state (m_videoPipeline, GST_STATE_NULL);
        gst_object_unref(m_videoPipeline);
        m_videoPipeline = NULL;
    }

#ifdef GSTBUSMSG
    if (m_videoBusWatch != -1) {
        g_source_remove(m_videoBusWatch);
        m_videoBusWatch = -1;
    }
#endif

    if (m_audioPipeline != NULL) {
        gst_app_sink_set_emit_signals((GstAppSink *)(m_appAudioSink), FALSE);
        gst_element_set_state (m_audioPipeline, GST_STATE_NULL);
        g_signal_handlers_disconnect_by_data(m_appAudioSink, this);
        gst_object_unref(m_audioPipeline);
        m_audioPipeline = NULL;
    }

#ifdef GSTBUSMSG
    if (m_audioBusWatch != -1) {
        g_source_remove(m_audioBusWatch);
        m_audioBusWatch = -1;
    }
#endif

    if (m_videoCaps != NULL)
        g_free(m_videoCaps);
    if (m_audioCaps != NULL)
        g_free(m_audioCaps);
    m_videoCaps = NULL;
    m_audioCaps = NULL;

    m_appAudioSink = NULL;
    m_appAudioSrc = NULL;
    m_appVideoSink = NULL;
    m_appAudioSrc = NULL;

    while (!m_videoSrcQ.empty())
        delete m_videoSrcQ.dequeue();

    while (!m_audioSrcQ.empty())
        delete m_audioSrcQ.dequeue();

    while (!m_videoSinkQ.empty())
        delete m_videoSinkQ.dequeue();

    while (!m_audioSinkQ.empty())
        delete m_audioSinkQ.dequeue();

    m_audioTimestamp = -1;

    m_videoSrcLock.unlock();
    m_audioSrcLock.unlock();
    m_videoSinkLock.unlock();
    m_audioSinkLock.unlock();
}

void AVMuxEncode::needVideoData()
{
    GstFlowReturn ret;
    GstBuffer *buffer;

    m_videoSrcLock.lock();
    if (m_videoSrcQ.empty()) {
        m_videoSrcLock.unlock();
        return;
    }

    AVMUX_QUEUEDATA *qd = m_videoSrcQ.dequeue();
    QByteArray frame = qd->data;
    m_lastQueuedVideoTimestamp = qd->timestamp;
    m_lastQueuedVideoParam = qd->param;
    delete qd;

    buffer = gst_buffer_new_and_alloc(frame.length());
    memcpy(GST_BUFFER_DATA(buffer), (unsigned char *)frame.data(), frame.length());
    m_videoSrcLock.unlock();

    ret = gst_app_src_push_buffer((GstAppSrc *)(m_appVideoSrc), buffer);

    if (ret != GST_FLOW_OK) {
        qDebug() << "video push error ";
    }
}

void AVMuxEncode::needAudioData()
{
    GstFlowReturn ret;
    GstBuffer *buffer;
    quint64 audioLength;
    QByteArray audio;
    qint64 duration;

    m_audioSrcLock.lock();
    if (m_audioSrcQ.empty()) {
//        qDebug() << "Using dummy audio";

        audioLength = (m_avParams.audioSampleRate *
                (m_avParams.audioSampleSize / 8) *
                m_avParams.audioChannels) / 10;

        buffer = gst_buffer_new_and_alloc(audioLength);
        guint8 *ad = GST_BUFFER_DATA(buffer);
        for (unsigned int i = 0; i < audioLength; i++)
            ad[i] = 0;
    } else {
//        qDebug() << "Queue " << m_audioSrcQ.count();
        AVMUX_QUEUEDATA *qd = m_audioSrcQ.dequeue();
        QByteArray audio = qd->data;
        m_lastQueuedAudioTimestamp = qd->timestamp;
        m_lastQueuedAudioParam = qd->param;
        delete qd;

        audioLength = audio.length();
        buffer = gst_buffer_new_and_alloc(audio.length());
        memcpy(GST_BUFFER_DATA(buffer), (unsigned char *)audio.data(), audio.length());
    }
    m_audioSrcLock.unlock();

    guint64 bytesPerSecond = m_avParams.audioSampleRate *
                            (m_avParams.audioSampleSize / 8) *
                            m_avParams.audioChannels;
    duration = ((quint64)1000000000 * audioLength) / bytesPerSecond;
//    GST_BUFFER_DURATION (buffer) = duration;

    if (m_audioTimestamp == -1)
        m_audioTimestamp = 0;
//    GST_BUFFER_TIMESTAMP(buffer) = m_audioTimestamp;
    m_audioTimestamp += duration;

    GstCaps *caps = gst_caps_new_simple(
                "audio/x-raw-int",
                "width", G_TYPE_INT, (gint)m_avParams.audioSampleSize,
                "depth", G_TYPE_INT, (gint)m_avParams.audioSampleSize,
                "channels" ,G_TYPE_INT, (gint)m_avParams.audioChannels,
                "rate",G_TYPE_INT,m_avParams.audioSampleRate,
                "endianness",G_TYPE_INT,(gint)1234,
                "signed", G_TYPE_BOOLEAN, (gboolean)TRUE,
                NULL);

    GST_BUFFER_CAPS(buffer) = caps;

    ret = gst_app_src_push_buffer((GstAppSrc *)(m_appAudioSrc), buffer);
    if (ret != GST_FLOW_OK) {
        qDebug() << "audio push error";
    }
}

void AVMuxEncode::setCompressionRates(int videoCompressionRate, int audioCompressionRate)
{
    m_videoCompressionRate = videoCompressionRate;
    m_audioCompressionRate = audioCompressionRate;
}
#endif
