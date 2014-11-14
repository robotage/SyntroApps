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

#include "AVMuxDecodeGS.h"

#include <qimage.h>
#include <qbuffer.h>
#include <qbytearray.h>

#define PIPELINE_MS_TO_NS       ((qint64)1000000)

#define GSTBUSMSG

#ifdef USE_GST10
static void sinkEOS(GstAppSink * /*appsink*/, gpointer /*user_data*/)
{
}

static GstFlowReturn newVideoSinkPreroll(GstAppSink * /*appsink*/, gpointer /* user_data */)
{
    return GST_FLOW_OK;
}

static GstFlowReturn newAudioSinkPreroll(GstAppSink * /*appsink*/, gpointer /* user_data */)
{
    return GST_FLOW_OK;
}
#endif

static GstFlowReturn newVideoSinkData (GstAppSink * /*appsink*/,
          gpointer user_data)
{
    ((AVMuxDecode *)user_data)->processVideoSinkData();
    return GST_FLOW_OK;
}

static GstFlowReturn newAudioSinkData (GstAppSink * /*appsink*/,
          gpointer user_data)
{
    ((AVMuxDecode *)user_data)->processAudioSinkData();
    return GST_FLOW_OK;
}

#ifdef GSTBUSMSG
static gboolean videoBusMessage(GstBus * /*bus*/, GstMessage *message, gpointer data)
{
    GstState old_state, new_state;

    if ((GST_MESSAGE_TYPE(message) == GST_MESSAGE_STATE_CHANGED)  &&
            ((GstElement *)(message->src) == ((AVMuxDecode *)data)->m_videoPipeline)){
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
            ((GstElement *)(message->src) == ((AVMuxDecode *)data)->m_audioPipeline)){
         gst_message_parse_state_changed (message, &old_state, &new_state, NULL);
           g_print ("Audio element %s changed state from %s to %s.\n",
            GST_OBJECT_NAME (message->src),
            gst_element_state_get_name (old_state),
            gst_element_state_get_name (new_state));
    }
    return TRUE;
}
#endif  // GSTBUSMSG

AVMuxDecode::AVMuxDecode() : SyntroThread("AVMuxDecode", "AVMuxDecode")
{
    m_videoPipeline = NULL;
    m_audioPipeline = NULL;
    m_appAudioSink = NULL;
    m_appAudioSrc = NULL;
    m_appVideoSink = NULL;
    m_appAudioSrc = NULL;
    m_pipelinesActive = false;
    m_videoBusWatch = -1;
    m_audioBusWatch = -1;

 }

void AVMuxDecode::initThread()
{
    m_timer = startTimer(AVMUXDECODE_INTERVAL);
}

void AVMuxDecode::finishThread()
{
    killTimer(m_timer);

    deletePipelines();
}

void AVMuxDecode::timerEvent(QTimerEvent * /*event*/)
{
    GstBuffer *buffer;
    uchar *data;

    while (1) {
        m_avmuxLock.lock();
        if (m_avmuxQ.empty())
            break;

        QByteArray data = m_avmuxQ.dequeue();
        m_avmuxLock.unlock();
        processAVData(data);
    }
    m_avmuxLock.unlock();

    m_videoLock.lock();
    if (m_videoPipeline != NULL) {
         while (!m_videoSinkQ.empty()) {
            buffer = m_videoSinkQ.dequeue();
#ifdef USE_GST10
            GstMemory *sinkMem = gst_buffer_get_all_memory(buffer);
            GstMapInfo *info = new GstMapInfo;
            gst_memory_map(sinkMem, info, GST_MAP_READ);
            data = (uchar *)malloc(sinkMem->size);
            memcpy(data, info->data, sinkMem->size);
            gst_memory_unmap(sinkMem, info);
            delete info;
            gst_memory_unref(sinkMem);
#else
            data = (uchar *)malloc(GST_BUFFER_SIZE(buffer));
            memcpy(data, GST_BUFFER_DATA(buffer), GST_BUFFER_SIZE(buffer));
#endif
            QImage image(data, m_avParams.videoWidth, m_avParams.videoHeight, QImage::Format_RGB888, free, data);
            emit newImage(image, m_lastVideoTimestamp);
            gst_buffer_unref(buffer);
        }
    }
    m_videoLock.unlock();

    m_audioLock.lock();
    if (m_audioPipeline != NULL) {
        while (!m_audioSinkQ.empty()) {
            buffer = m_audioSinkQ.dequeue();
#ifdef USE_GST10
            GstMemory *sinkMem = gst_buffer_get_all_memory(buffer);
            GstMapInfo *info = new GstMapInfo;
            gst_memory_map(sinkMem, info, GST_MAP_READ);
            QByteArray audio((const char *)info->data, sinkMem->size);
            gst_memory_unmap(sinkMem, info);
            delete info;
            gst_memory_unref(sinkMem);
            gst_buffer_unref(buffer);
            emit newAudioSamples(audio,
                              m_lastAudioTimestamp, m_avParams.audioSampleRate, m_avParams.audioChannels, m_avParams.audioSampleSize);
#else
            emit newAudioSamples(QByteArray((const char *)GST_BUFFER_DATA(buffer), GST_BUFFER_SIZE(buffer)),
                              m_lastAudioTimestamp, m_avParams.audioSampleRate, m_avParams.audioChannels, m_avParams.audioSampleSize);
            gst_buffer_unref(buffer);
#endif
        }
    }
    m_audioLock.unlock();
}

void AVMuxDecode::processVideoSinkData()
{
    GstBuffer *buffer;

    m_videoLock.lock();
    if (m_appVideoSink != NULL) {
#ifdef USE_GST10
        GstSample *sinkSample = gst_app_sink_pull_sample((GstAppSink *)(m_appVideoSink));
        if (sinkSample != NULL) {
            buffer = gst_sample_get_buffer(sinkSample);
            m_videoSinkQ.enqueue(buffer);
        }
#else
        if ((buffer = gst_app_sink_pull_buffer((GstAppSink *)(m_appVideoSink))) != NULL) {
            m_videoSinkQ.enqueue(buffer);
        }
#endif
    }
    m_videoLock.unlock();
}

void AVMuxDecode::processAudioSinkData()
{
    GstBuffer *buffer;

    m_audioLock.lock();
    if (m_appAudioSink != NULL) {
#ifdef USE_GST10
        GstSample *sinkSample = gst_app_sink_pull_sample((GstAppSink *)(m_appAudioSink));
        if (sinkSample != NULL) {
            buffer = gst_sample_get_buffer(sinkSample);
            m_audioSinkQ.enqueue(buffer);
        }
#else
        if ((buffer = gst_app_sink_pull_buffer((GstAppSink *)(m_appAudioSink))) != NULL) {
            m_audioSinkQ.enqueue(buffer);
        }
#endif
    }
    m_audioLock.unlock();
}


void AVMuxDecode::newAVMuxData(QByteArray avmuxArray)
{
    m_avmuxLock.lock();
    m_avmuxQ.enqueue(avmuxArray);
    m_avmuxLock.unlock();
}

void AVMuxDecode::processAVData(QByteArray avmuxArray)
{
    SYNTRO_RECORD_AVMUX *avmux = (SYNTRO_RECORD_AVMUX *)avmuxArray.constData();

    m_oldAvParams = m_avParams;                             // save previous version
    SyntroUtils::avmuxHeaderToAVParams(avmux, &m_avParams);
    switch (m_avParams.avmuxSubtype) {
        case SYNTRO_RECORD_TYPE_AVMUX_MJPPCM:
            processMJPPCM(avmux);
            break;

        case SYNTRO_RECORD_TYPE_AVMUX_RTP:
            if (!m_pipelinesActive) {
                m_oldAvParams = m_avParams;
                if (m_videoPipeline == NULL) {
                    newPipelines(&m_avParams);
                    if (m_videoPipeline == NULL)
                    return;
                }
                if (m_audioPipeline == NULL) {
                    newPipelines(&m_avParams);
                    if (m_audioPipeline == NULL)
                        return;
                }
            }
            if (m_pipelinesActive)
                processRTP(avmux);
            break;

        default:
            printf("Unsupported avmux type %d\n", m_avParams.avmuxSubtype);
            break;
    }
}

void AVMuxDecode::processMJPPCM(SYNTRO_RECORD_AVMUX *avmux)
{
    unsigned char *ptr;
    int muxSize;
    int videoSize;
    int audioSize;
    int param;

    muxSize = SyntroUtils::convertUC4ToInt(avmux->muxSize);
    videoSize = SyntroUtils::convertUC4ToInt(avmux->videoSize);
    audioSize = SyntroUtils::convertUC4ToInt(avmux->audioSize);
    param = SyntroUtils::convertUC2ToInt(avmux->recordHeader.param);

    ptr = (unsigned char *)(avmux + 1) + muxSize;        // pointer to video area

    if ((videoSize != 0) || (param == SYNTRO_RECORDHEADER_PARAM_NOOP)) {
        // there is video data present

        if ((videoSize < 0) || (videoSize > 300000)) {
            printf("Illegal video data size %d\n", videoSize);
            return;
        }
        QImage image;
        image.loadFromData((const uchar *)ptr, videoSize, "JPEG");
        emit newImage(image, SyntroUtils::convertUC8ToInt64(avmux->recordHeader.timestamp));

        ptr += videoSize;                                   // move to next data area
    }

    if (audioSize != 0) {
        // there is audio data present

        if ((audioSize < 0) || (audioSize > 300000)) {
            printf("Illegal audio data size %d\n", audioSize);
            return;
        }
        emit newAudioSamples(QByteArray((const char *)ptr, audioSize),
            SyntroUtils::convertUC8ToInt64(avmux->recordHeader.timestamp),
            m_avParams.audioSampleRate, m_avParams.audioChannels, m_avParams.audioSampleSize);
        ptr += audioSize;                        // move to next data area
    }
}

void AVMuxDecode::processRTP(SYNTRO_RECORD_AVMUX *avmux)
{
    unsigned char *ptr;
    int muxSize;
    int videoSize;
    int audioSize;

    // check if stream has changed in any meaningful way

    if ((m_oldAvParams.videoWidth != m_avParams.videoWidth) || (m_oldAvParams.videoHeight != m_avParams.videoHeight)) {
        // something has changed - restart pipelines
        deletePipelines();
        return;                                             // the next packet will set up the pipelines again
    }

    muxSize = SyntroUtils::convertUC4ToInt(avmux->muxSize);
    videoSize = SyntroUtils::convertUC4ToInt(avmux->videoSize);
    audioSize = SyntroUtils::convertUC4ToInt(avmux->audioSize);

    if (muxSize != 0) {
        printf("RTPMP4 with non zero mux size %d\n", muxSize);
        return;
    }
    ptr = (unsigned char *)(avmux + 1);                     // pointer to first data area

    if (videoSize != 0) {
        // there is video data present

        if ((videoSize < 0) || (videoSize > 300000)) {
            printf("Illegal video data size %d\n", videoSize);
            return;
        }
        if (avmux->videoSubtype == SYNTRO_RECORD_TYPE_VIDEO_RTPCAPS) {
            putRTPVideoCaps(ptr, videoSize);
        } else {
            int param = SyntroUtils::convertUC2ToInt(avmux->recordHeader.param);
            if ((param == SYNTRO_RECORDHEADER_PARAM_NOOP) ||
                    (param == SYNTRO_RECORDHEADER_PARAM_REFRESH)) {
                QImage image;
                image.loadFromData((const uchar *)ptr, videoSize, "JPEG");
                emit newImage(image, SyntroUtils::convertUC8ToInt64(avmux->recordHeader.timestamp));
            } else {
                if (!m_waitingForVideoCaps) {
                    m_lastVideoTimestamp = SyntroUtils::convertUC8ToInt64(avmux->recordHeader.timestamp);
                    putVideoData((const unsigned char *)ptr, videoSize);
                }
            }
        }

        ptr += videoSize;                                 // move to next data area
    }

    if (audioSize != 0) {
        // there is audio data present

        if ((audioSize < 0) || (audioSize > 300000)) {
            printf("Illegal audio data size %d\n", audioSize);
            return;

        }
        if (avmux->audioSubtype == SYNTRO_RECORD_TYPE_AUDIO_RTPCAPS) {
            putRTPAudioCaps(ptr, audioSize);
        } else {
            if (!m_waitingForAudioCaps) {
                m_lastAudioTimestamp = SyntroUtils::convertUC8ToInt64(avmux->recordHeader.timestamp);
                putAudioData((const unsigned char *)ptr, audioSize);
            }
        }

        ptr += audioSize;                                 // move to next data area
    }
}

//----------------------------------------------------------
//
//  Pipeline management stuff

bool AVMuxDecode::newPipelines(SYNTRO_AVPARAMS *avParams)
{
    static int pipelineIndex = 0;
    int index;

    gchar *videoLaunch;
    gchar *audioLaunch;
    GError *error = NULL;

    m_avParams = *avParams;
    index = pipelineIndex++;

//    printf("width=%d, height=%d, rate=%d\n", avParams->videoWidth, avParams->videoHeight, avParams->videoFramerate);
//    printf("channels=%d, rate=%d, size=%d\n", avParams->audioChannels, avParams->audioSampleRate, avParams->audioSampleSize);

    m_videoOffset = 0;
    m_audioOffset = 0;
    m_waitingForVideoCaps = true;
    m_waitingForAudioCaps = true;

    //  Construct the pipelines

    if (avParams->videoSubtype == SYNTRO_RECORD_TYPE_VIDEO_RTPMPEG4) {
#ifdef USE_GST10
        videoLaunch = g_strdup_printf (
           " appsrc name=videoSrc%d ! rtpmp4vdepay ! queue ! avdec_mpeg4 "
                    " ! videoconvert ! capsfilter caps=\"video/x-raw,format=RGB\" ! appsink name=videoSink%d"
           , index, index);
#else
        videoLaunch = g_strdup_printf (
           " appsrc name=videoSrc%d ! rtpmp4vdepay ! queue ! ffdec_mpeg4 "
                    " ! ffmpegcolorspace ! capsfilter caps=\"video/x-raw-rgb\" ! appsink name=videoSink%d"
           , index, index);
#endif
    } else {
#ifdef USE_GST10
        videoLaunch = g_strdup_printf (
             " appsrc name=videoSrc%d ! rtph264depay ! queue ! h264parse ! avdec_h264"
             " ! videoconvert ! capsfilter caps=\"video/x-raw,format=RGB\" ! appsink name=videoSink%d"
                    , index, index);
#else
        videoLaunch = g_strdup_printf (
           " appsrc name=videoSrc%d ! rtph264depay ! queue ! ffdec_h264 "
                    " ! ffmpegcolorspace ! capsfilter caps=\"video/x-raw-rgb\" ! appsink name=videoSink%d"
           , index, index);
#endif
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
             " appsrc name=audioSrc%d ! queue ! rtpmp4adepay ! queue ! faad ! appsink name=audioSink%d "
             , index, index);

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

    gchar *videoSink = g_strdup_printf("videoSink%d", index);
    if ((m_appVideoSink = gst_bin_get_by_name (GST_BIN (m_videoPipeline), videoSink)) == NULL) {
        g_printerr("Unable to find video appsink\n");
        g_free(videoSink);
        deletePipelines();
        return false;
    }
    g_free(videoSink);

    gchar *videoSrc = g_strdup_printf("videoSrc%d", index);
    if ((m_appVideoSrc = gst_bin_get_by_name (GST_BIN (m_videoPipeline), videoSrc)) == NULL) {
            g_printerr("Unable to find video appsrc\n");
            g_free(videoSrc);
            deletePipelines();
            return false;
        }
    g_free(videoSrc);

    gchar *audioSink = g_strdup_printf("audioSink%d", index);
    if ((m_appAudioSink = gst_bin_get_by_name (GST_BIN (m_audioPipeline), audioSink)) == NULL) {
        g_printerr("Unable to find audio appsink\n");
        g_free(audioSink);
        deletePipelines();
        return false;
    }
    g_free(audioSink);

    gchar *audioSrc = g_strdup_printf("audioSrc%d", index);
    if ((m_appAudioSrc = gst_bin_get_by_name (GST_BIN (m_audioPipeline), audioSrc)) == NULL) {
            g_printerr("Unable to find audio appsrc\n");
            g_free(audioSrc);
            deletePipelines();
            return false;
        }
    g_free(audioSrc);

#ifdef USE_GST10
    GstAppSinkCallbacks vcb, acb;
    vcb.eos = sinkEOS;
    vcb.new_preroll = newVideoSinkPreroll;
    vcb.new_sample = newVideoSinkData;
    gst_app_sink_set_callbacks((GstAppSink *)m_appVideoSink, &vcb, this, NULL);

    acb.eos = sinkEOS;
    acb.new_preroll = newAudioSinkPreroll;
    acb.new_sample = newAudioSinkData;
    gst_app_sink_set_callbacks((GstAppSink *)m_appAudioSink, &acb, this, NULL);

    g_object_set(m_appVideoSrc, "format", GST_FORMAT_TIME, NULL);
    g_object_set(m_appAudioSrc, "format", GST_FORMAT_TIME, NULL);

#else
    g_signal_connect (m_appVideoSink, "new-buffer", G_CALLBACK (newVideoSinkData), this);
    gst_app_sink_set_emit_signals((GstAppSink *)(m_appVideoSink), TRUE);

    g_signal_connect (m_appAudioSink, "new-buffer", G_CALLBACK (newAudioSinkData), this);
    gst_app_sink_set_emit_signals((GstAppSink *)(m_appAudioSink), TRUE);
#endif

#ifdef GSTBUSMSG
    GstBus *bus;

    bus = gst_pipeline_get_bus(GST_PIPELINE (m_videoPipeline));
    m_videoBusWatch = gst_bus_add_watch (bus, videoBusMessage, this);
    gst_object_unref (bus);

    bus = gst_pipeline_get_bus(GST_PIPELINE (m_audioPipeline));
    m_audioBusWatch = gst_bus_add_watch (bus, audioBusMessage, this);
    gst_object_unref (bus);
#endif

    GstStateChangeReturn ret = gst_element_set_state (m_videoPipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr ("Unable to set the video pipeline to the play state.\n");
    }
    m_pipelinesActive = true;
    return true;
}

void AVMuxDecode::deletePipelines()
{
    m_pipelinesActive = false;
    m_videoLock.lock();
    if (m_videoPipeline != NULL) {
#ifdef USE_GST10
        GstAppSinkCallbacks cb;
        cb.eos = NULL;
        cb.new_preroll = NULL;
        cb.new_sample = NULL;
        gst_app_sink_set_callbacks((GstAppSink *)m_appVideoSink, &cb, this, NULL);
#else
        gst_app_sink_set_emit_signals((GstAppSink *)(m_appVideoSink), FALSE);
        g_signal_handlers_disconnect_by_data(m_appVideoSink, this);
#endif
        gst_element_set_state (m_videoPipeline, GST_STATE_NULL);
        gst_object_unref(m_videoPipeline);
    }

#ifdef GSTBUSMSG
    if (m_videoBusWatch != -1) {
        g_source_remove(m_videoBusWatch);
        m_videoBusWatch = -1;
    }
#endif

    while (!m_videoSinkQ.empty())
        gst_buffer_unref(m_videoSinkQ.dequeue());
    m_videoPipeline = NULL;
    m_appVideoSink = NULL;
    m_appVideoSrc = NULL;
    m_videoLock.unlock();

    m_audioLock.lock();
    if (m_audioPipeline != NULL) {
#ifdef USE_GST10
        GstAppSinkCallbacks cb;
        cb.eos = NULL;
        cb.new_preroll = NULL;
        cb.new_sample = NULL;
        gst_app_sink_set_callbacks((GstAppSink *)m_appAudioSink, &cb, this, NULL);
#else
        gst_app_sink_set_emit_signals((GstAppSink *)(m_appAudioSink), FALSE);
        g_signal_handlers_disconnect_by_data(m_appAudioSink, this);
#endif
        gst_element_set_state (m_audioPipeline, GST_STATE_NULL);
        gst_object_unref(m_audioPipeline);
    }

#ifdef GSTBUSMSG
    if (m_audioBusWatch != -1) {
        g_source_remove(m_audioBusWatch);
        m_audioBusWatch = -1;
    }
#endif

    while (!m_audioSinkQ.empty())
        gst_buffer_unref(m_audioSinkQ.dequeue());
    m_audioPipeline = NULL;
    m_appAudioSink = NULL;
    m_appAudioSrc = NULL;
    m_audioLock.unlock();

//    m_avmuxLock.lock();
    m_avmuxQ.clear();
//    m_avmuxLock.unlock();
}

void AVMuxDecode::putRTPVideoCaps(unsigned char *data, int length)
{
    QMutexLocker lock(&m_videoLock);
    GstStateChangeReturn ret;

    if (m_videoPipeline == NULL)
        return;

    if (!m_waitingForVideoCaps)
        return;

    if (m_appVideoSrc == NULL)
        return;

    data[length-1] = 0;                                     // make sure 0 terminated

    gst_app_src_set_stream_type((GstAppSrc *)m_appVideoSrc, GST_APP_STREAM_TYPE_STREAM);
    gst_app_src_set_caps((GstAppSrc *)m_appVideoSrc, gst_caps_from_string((gchar *)data));

    m_waitingForVideoCaps = false;

#ifdef GSTBUSMSG
    qDebug() << "Video caps " << QString((char *)data);
#endif

    ret = gst_element_set_state (m_videoPipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr ("Unable to set the video pipeline to the play state.\n");
    } else {
//        qDebug() << "Video pipeline running";
    }
}

void AVMuxDecode::putRTPAudioCaps(unsigned char *data, int length)
{
    QMutexLocker lock(&m_audioLock);

    GstStateChangeReturn ret;
    if (m_audioPipeline == NULL)
        return;

    if (!m_waitingForAudioCaps)
        return;

    if (m_appAudioSrc == NULL)
        return;

    data[length-1] = 0;                                     // make sure 0 terminated

    gst_app_src_set_stream_type((GstAppSrc *)(m_appAudioSrc), GST_APP_STREAM_TYPE_STREAM);
    gst_app_src_set_caps((GstAppSrc *)m_appAudioSrc, gst_caps_from_string((gchar *)data));

    m_waitingForAudioCaps = false;

#ifdef GSTBUSMSG
    qDebug() << "Audio caps " << QString((char *)data);
#endif

    ret = gst_element_set_state (m_audioPipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr ("Unable to set the audio pipeline to the play state.\n");
    } else {
//        qDebug() << "Audio pipeline running";
    }
}

void AVMuxDecode::putVideoData(const unsigned char *data, int length)
{
    QMutexLocker lock(&m_videoLock);

    GstBuffer *buffer;
    int segLength;
    GstFlowReturn ret;

    if (m_videoPipeline == NULL)
        return;

    if (m_waitingForVideoCaps)
        return;

    if (m_appVideoSrc == NULL)
        return;

    while (length > (int)sizeof(SYNTRO_UC4)) {
        segLength = SyntroUtils::convertUC4ToInt((unsigned char *)data);
        data += sizeof(SYNTRO_UC4);
        length -= sizeof(SYNTRO_UC4);
        if ((segLength <= 0) || (segLength > length)) {
            qDebug() << "RTP video with incorrect segLength " << segLength;
            return;
        }
        buffer = gst_buffer_new_and_alloc(segLength);
#ifdef USE_GST10
        GstMemory *sinkMem = gst_buffer_get_all_memory(buffer);
        GstMapInfo *info = new GstMapInfo;
        gst_memory_map(sinkMem, info, GST_MAP_WRITE);
        memcpy(info->data, data, segLength);
        gst_memory_unmap(sinkMem, info);
        delete info;
        gst_memory_unref(sinkMem);
#else
        memcpy(GST_BUFFER_DATA(buffer), data, segLength);
#endif

        GST_BUFFER_OFFSET(buffer) = m_videoOffset;
        m_videoOffset += segLength;
        GST_BUFFER_OFFSET_END(buffer) = m_videoOffset;
#ifdef USE_GST10
        GST_BUFFER_PTS(buffer) = 0;
#else
        GST_BUFFER_TIMESTAMP(buffer) = 0;
#endif

        ret = gst_app_src_push_buffer((GstAppSrc *)(m_appVideoSrc), buffer);
        if (ret != GST_FLOW_OK)
            qDebug() << "Video push error";
        data += segLength;
        length -= segLength;
    }
    if (length != 0) {
        qDebug() << "Video RTP length error " << length;
    }
}

void AVMuxDecode::putAudioData(const unsigned char *data, int length)
{
    QMutexLocker lock(&m_audioLock);
    GstBuffer *buffer;
    int segLength;
    GstFlowReturn ret;

    if (m_audioPipeline == NULL)
        return;

    if (m_waitingForAudioCaps)
        return;

    if (m_appAudioSrc == NULL)
        return;

    while (length > (int)sizeof(SYNTRO_UC4)) {
        segLength = SyntroUtils::convertUC4ToInt((unsigned char *)data);
        data += sizeof(SYNTRO_UC4);
        length -= sizeof(SYNTRO_UC4);
        if ((segLength <= 0) || (segLength > length)) {
            qDebug() << "RTP audio with incorrect segLength " << segLength;
            return;
        }
        buffer = gst_buffer_new_and_alloc(segLength);

#ifdef USE_GST10
        GstMemory *sinkMem = gst_buffer_get_all_memory(buffer);
        GstMapInfo *info = new GstMapInfo;
        gst_memory_map(sinkMem, info, GST_MAP_WRITE);
        memcpy(info->data, data, segLength);
        gst_memory_unmap(sinkMem, info);
        delete info;
        gst_memory_unref(sinkMem);
#else
        memcpy(GST_BUFFER_DATA(buffer), data, segLength);
#endif

        GST_BUFFER_OFFSET(buffer) = m_audioOffset;
        m_audioOffset += segLength;
        GST_BUFFER_OFFSET_END(buffer) = m_audioOffset;

        ret = gst_app_src_push_buffer((GstAppSrc *)(m_appAudioSrc), buffer);
        if (ret != GST_FLOW_OK)
            qDebug() << "Audio push error";
        data += segLength;
        length -= segLength;
    }
    if (length != 0) {
        qDebug() << "Audio RTP length error " << length;
    }
}
