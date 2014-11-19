//
//  Copyright (c) 2014 Richard Barnett
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


#include "RTSPIF.h"
#include <qdebug.h>

//#define GSTBUSMSG

#ifdef USE_GST10
static void sinkEOS(GstAppSink * /*appsink*/, gpointer /*user_data*/)
{
}
#endif

static GstFlowReturn sinkPreroll(GstAppSink * /*appsink*/, gpointer user_data)
{
    ((RTSPIF *)user_data)->processVideoPrerollData();
    return GST_FLOW_OK;
}

static GstFlowReturn sinkSample(GstAppSink * /*appsink*/, gpointer user_data)
{
    ((RTSPIF *)user_data)->processVideoSinkData();
    return GST_FLOW_OK;
}

#ifdef GSTBUSMSG
static gboolean videoBusMessage(GstBus * /*bus*/, GstMessage *message, gpointer data)
{
    GstState old_state, new_state;

    if ((GST_MESSAGE_TYPE(message) == GST_MESSAGE_STATE_CHANGED)  &&
            ((GstElement *)(message->src) == ((RTSPIF *)data)->m_pipeline)){
         gst_message_parse_state_changed (message, &old_state, &new_state, NULL);
            qDebug() << "Video element " << GST_OBJECT_NAME (message->src) << " changed state from "
                << gst_element_state_get_name (old_state) << " to " << gst_element_state_get_name (new_state);
    }
    return TRUE;
}
#endif


RTSPIF::RTSPIF() : SyntroThread("RTSPIF", "SyntroRTSP")
{
    m_pipeline = NULL;
    m_pipelineActive = false;
    m_appVideoSink = NULL;
    m_widthPrefix = "width=(int)";
    m_heightPrefix = "height=(int)";
    m_frameratePrefix = "framerate=(fraction)";
}

RTSPIF::~RTSPIF()
{
}

void RTSPIF::initThread()
{
    thread()->setPriority(QThread::TimeCriticalPriority);
    open();
}

void RTSPIF::finishThread()
{
    close();
}

bool RTSPIF::open()
{
    QSettings *settings = SyntroUtils::getSettings();

    settings->beginGroup(SYNTRORTSP_CAMERA_GROUP);

    m_IPAddress = settings->value(SYNTRORTSP_CAMERA_IPADDRESS).toString();
    m_port = settings->value(SYNTRORTSP_CAMERA_TCPPORT).toInt();
    m_user = settings->value(SYNTRORTSP_CAMERA_USERNAME).toString();
    m_pw = settings->value(SYNTRORTSP_CAMERA_PASSWORD).toString();

    //	This is just to get started. Real value will come in from the camera

    m_width = 640;
    m_height = 480;
    m_frameRate = 10;
    m_frameSize = m_width * m_height * 3;

    newPipeline();

    m_frameCount = 0;
    m_lastFrameTime = SyntroClock();

    emit netcamStatus(QString("Connecting to ") + QString(m_IPAddress));


    settings->endGroup();

    delete settings;

    m_firstFrame = true;
    m_startTime = SyntroClock();
    m_timer = startTimer(RTSP_BGND_INTERVAL);

    return true;
}

void RTSPIF::close()
{
    killTimer(m_timer);
    deletePipeline();
}

void RTSPIF::timerEvent(QTimerEvent * /*event*/)
{
    qint64 now = SyntroClock();

    if ((now - m_startTime) > RTSP_PIPELINE_RESET) {
        close();
        logInfo("Resetting pipeline");
        open();
    }

    if ((now - m_lastFrameTime) > RTSP_NOFRAME_TIMEOUT) {
        close();
        m_lastFrameTime = now;
        logInfo("Retrying connect");
        open();
    }
}

void RTSPIF::newCamera()
{
    close();
    open();
}

int RTSPIF::getCapsValue(const QString &caps, const QString& key, const QString& terminator)
{
    QString sub;
    int start;
    int end;

    start = caps.indexOf(key);
    if (start == -1)
        return -1;

    start += key.length();
    sub = caps;
    sub = sub.remove(0, start);
    end = sub.indexOf(terminator);
    if (end == -1)
        return -1;
    sub = sub.left(end);
    return sub.toInt();
}


void RTSPIF::processVideoSinkData()
{
    GstBuffer *buffer;
    QImage frame;

    m_videoLock.lock();
    if (m_appVideoSink != NULL) {
#ifdef USE_GST10
        GstSample *sinkSample = gst_app_sink_pull_sample((GstAppSink *)(m_appVideoSink));
        if (sinkSample != NULL) {
            m_lastFrameTime = SyntroClock();

            frame = QImage(m_width, m_height, QImage::Format_RGB888);
            buffer = gst_sample_get_buffer(sinkSample);
            GstMemory *sinkMem = gst_buffer_get_all_memory(buffer);
            GstMapInfo *info = new GstMapInfo;
            gst_memory_map(sinkMem, info, GST_MAP_READ);
            memcpy(frame.bits(), info->data, m_frameSize);
            gst_memory_unmap(sinkMem, info);
            delete info;
            gst_memory_unref(sinkMem);
            gst_buffer_unref(buffer);
            emit newImage(frame);

            if (m_firstFrame) {
                emit netcamStatus(QString("Connected to ") + QString(m_IPAddress));
                m_firstFrame = false;
            }
        }
#else
        uchar *data;
        if ((buffer = gst_app_sink_pull_buffer((GstAppSink *)(m_appVideoSink))) != NULL) {
            m_lastFrameTime = SyntroClock();
            data = (uchar *)malloc(GST_BUFFER_SIZE(buffer));
            memcpy(data, GST_BUFFER_DATA(buffer), GST_BUFFER_SIZE(buffer));
            QImage image(data, m_width, m_height, QImage::Format_RGB888, free, data);
            emit newImage(image);
            gst_buffer_unref(buffer);
        }
#endif
    }
    m_videoLock.unlock();
}

void RTSPIF::processVideoPrerollData()
{
    int val;

    m_videoLock.lock();
    if (m_appVideoSink != NULL) {
        // get details of frame size and rate

#ifdef USE_GST10
        GstCaps *sinkCaps = gst_pad_get_current_caps(m_appVideoSinkPad);
#else
        GstCaps *sinkCaps = gst_pad_get_negotiated_caps(m_appVideoSinkPad);
#endif
        gchar *caps = gst_caps_to_string(sinkCaps);
        qDebug() << "Caps = " << caps;
        gst_caps_unref(sinkCaps);
        if (caps != m_caps) {
            // must have changed
            m_caps = caps;
            if ((val = getCapsValue(caps, m_widthPrefix, ",")) != -1)
                m_width = val;
            if ((val = getCapsValue(caps, m_heightPrefix, ",")) != -1)
                m_height = val;
            if ((val = getCapsValue(caps, m_frameratePrefix, "/")) != -1)
                m_frameRate = val;
            m_frameSize = m_width * m_height * 3;
            emit setVideoFormat(m_width, m_height, m_frameRate);
        }
        g_free(caps);
    }
    m_videoLock.unlock();
}


bool RTSPIF::newPipeline()
{
    gchar *launch;
    GError *error = NULL;
    GstStateChangeReturn ret;

    //  Construct the pipelines

    launch = g_strdup_printf (
#ifdef USE_GST10
            " rtspsrc location=rtsp://%s:%d/videoMain user-id=%s user-pw=%s "
             " ! decodebin ! queue ! autovideoconvert ! capsfilter caps=\"video/x-raw,format=RGB\" ! appsink name=videoSink0 "
             ,qPrintable(m_IPAddress), m_port, qPrintable(m_user), qPrintable(m_pw));
#else
            " rtspsrc location=rtsp://%s:%d/videoMain user-id=%s user-pw=%s "
            " ! gstrtpjitterbuffer ! rtph264depay ! queue ! nv_omx_h264dec "
            " ! capsfilter caps=\"video/x-raw-yuv\" ! ffmpegcolorspace ! capsfilter caps=\"video/x-raw-rgb\" ! queue ! appsink name=videoSink0 "
            ,qPrintable(m_IPAddress), m_port, qPrintable(m_user), qPrintable(m_pw));
#endif
    m_pipeline = gst_parse_launch(launch, &error);
    g_free(launch);

    if (error != NULL) {
        qDebug() << "Could not construct video pipeline: " << error->message;
        g_error_free (error);
        m_pipeline = NULL;
        return false;
    }

    //  find the appsink

    gchar *videoSink = g_strdup_printf("videoSink%d", 0);
    if ((m_appVideoSink = gst_bin_get_by_name (GST_BIN (m_pipeline), videoSink)) == NULL) {
        qDebug() << "Unable to find video appsink";
        g_free(videoSink);
        deletePipeline();
        return false;
    }
    g_free(videoSink);

    GList *pads = m_appVideoSink->pads;
    m_appVideoSinkPad = (GstPad *)pads->data;

#ifdef USE_GST10
    GstAppSinkCallbacks cb;
    cb.eos = sinkEOS;
    cb.new_preroll = sinkPreroll;
    cb.new_sample = sinkSample;
    gst_app_sink_set_callbacks((GstAppSink *)m_appVideoSink, &cb, this, NULL);
#else
    g_signal_connect (m_appVideoSink, "new-buffer", G_CALLBACK (sinkSample), this);
    g_signal_connect (m_appVideoSink, "new-preroll", G_CALLBACK (sinkPreroll), this);
    gst_app_sink_set_emit_signals((GstAppSink *)(m_appVideoSink), TRUE);

#endif

    ret = gst_element_set_state (m_pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        qDebug() << "Unable to set the video pipeline to the play state" ;
        deletePipeline();
        return false;
    }

#ifdef GSTBUSMSG
    GstBus *bus;

    bus = gst_pipeline_get_bus(GST_PIPELINE (m_pipeline));
    m_videoBusWatch = gst_bus_add_watch (bus, videoBusMessage, this);
    gst_object_unref (bus);
#endif

    m_pipelineActive = true;
    return true;
}

void RTSPIF::deletePipeline()
{
    if (!m_pipelineActive)
        return;

    m_pipelineActive = false;
    m_videoLock.lock();
    if (m_pipeline != NULL) {
#ifdef GSTBUSMSG
        if (m_videoBusWatch != -1) {
            g_source_remove(m_videoBusWatch);
            m_videoBusWatch = -1;
        }
#endif
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
        gst_element_set_state (m_pipeline, GST_STATE_NULL);
        gst_object_unref(m_pipeline);
    }

    m_pipeline = NULL;
    m_appVideoSink = NULL;
    m_videoLock.unlock();
    m_caps = "";
}

QSize RTSPIF::getImageSize()
{
    return QSize(m_width, m_height);
}


void RTSPIF::setPTZ(SYNTRO_PTZ * /* PTZ */)
{
}
