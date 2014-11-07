///
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

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/select.h>

#include <qfile.h>
#include <qfileinfo.h>

#include "VideoDriver.h"
#include "CamClient.h"
#include <qbuffer.h>

#define DEFAULT_CAMERA 0
#define DEFAULT_WIDTH  640
#define DEFAULT_HEIGHT 480
#define MAXIMUM_RATE   100
#define DEFAULT_RATE   10

#define MAX_CONSECUTIVE_BAD_FRAMES 50


VideoDriver::VideoDriver() : SyntroThread("VideoDriver", "SyntroCam")
{
	m_fd = -1;
	m_consecutiveBadFrames = 0;
	m_cameraNum = DEFAULT_CAMERA;
	m_width = DEFAULT_WIDTH;
	m_height = DEFAULT_HEIGHT;
	m_frameRate = DEFAULT_RATE;
	m_frameCount = 0;
	m_mmBuffLen = 0;
	m_rgbBuff = NULL;
}

VideoDriver::~VideoDriver()
{
	closeDevice();
}

void VideoDriver::loadSettings()
{
	QSettings *settings = SyntroUtils::getSettings();

	settings->beginGroup(CAMERA_GROUP);

	if (!settings->contains(CAMERA_CAMERA))
		settings->setValue(CAMERA_CAMERA, DEFAULT_CAMERA);

	if (!settings->contains(CAMERA_WIDTH))
		settings->setValue(CAMERA_WIDTH, DEFAULT_WIDTH);

	if (!settings->contains(CAMERA_HEIGHT))
		settings->setValue(CAMERA_HEIGHT, DEFAULT_HEIGHT);

	if (!settings->contains(CAMERA_FRAMERATE))
		settings->setValue(CAMERA_FRAMERATE, DEFAULT_RATE);

	if (!settings->contains(CAMERA_FORMAT))
		settings->setValue(CAMERA_FORMAT, "MJPG");


	m_cameraNum = settings->value(CAMERA_CAMERA).toInt();
	m_preferredWidth = settings->value(CAMERA_WIDTH).toInt();
	m_preferredHeight = settings->value(CAMERA_HEIGHT).toInt();
	m_preferredFrameRate = settings->value(CAMERA_FRAMERATE).toInt();

	QString format = settings->value(CAMERA_FORMAT).toString().toUpper();

	if (format == "YUYV")
		m_preferredFormat = V4L2_PIX_FMT_YUYV;
	else
		m_preferredFormat = V4L2_PIX_FMT_MJPEG;

	settings->endGroup();

	delete settings;
}


#define STATE_DISCONNECTED  0
#define STATE_DETECTED      1
#define STATE_CONNECTED     2
#define STATE_CAPTURING     3

#define TICK_DURATION_MS        500
#define DISCONNECT_MIN_TICKS    4
#define DETECT_MIN_TICKS        10
#define CONNECT_MIN_TICKS       6

void VideoDriver::initThread()
{
	m_timer = -1;
	newCamera();
}

void VideoDriver::finishThread()
{
	closeDevice();
}

void VideoDriver::newCamera()
{
	closeDevice();
	loadSettings();

	// optimize the typical case on startup
	if (deviceExists() && openDevice()) {
		m_state = STATE_CONNECTED;
		emit cameraState("Connected");
		m_ticks = CONNECT_MIN_TICKS;
	}
	else {
		m_state = STATE_DISCONNECTED;
		m_ticks = 0;
		emit cameraState("Disconnected");
	}

	m_timer = startTimer(25);
}

void VideoDriver::timerEvent(QTimerEvent *)
{
	switch (m_state) {
	case STATE_DISCONNECTED:
		if (++m_ticks > DISCONNECT_MIN_TICKS) {
			if (deviceExists()) {
				m_state = STATE_DETECTED;
				emit cameraState("Detected");
				m_ticks = 0;
			}
		}

		thread()->msleep(TICK_DURATION_MS);
		break;

	case STATE_DETECTED:
		if (++m_ticks > DETECT_MIN_TICKS) {
			if (openDevice()) {
				m_state = STATE_CONNECTED;
				emit cameraState("Connected");
				m_ticks = 0;
			}
		}

		thread()->msleep(TICK_DURATION_MS);
		break;

	case STATE_CONNECTED:
		if (++m_ticks <= CONNECT_MIN_TICKS) {
			thread()->msleep(TICK_DURATION_MS);
			break;
		}

		if (streamOn()) {
			m_state = STATE_CAPTURING;
			emit cameraState("Running");
		}
		else {
			emit cameraState("Disconnected");
			newCamera();
		}

		break;

	case STATE_CAPTURING:
		if (!readFrame()) {
			emit cameraState("Disconnected");
			newCamera();
		}

		break;
	}
}


bool VideoDriver::handleFrame()
{
	bool result;
	struct v4l2_buffer buf;

	memset(&buf, 0, sizeof(buf));

	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;

	if (-1 == xioctl(VIDIOC_DQBUF, &buf)) {
		if (errno == EAGAIN) {
			// no buffer ready
			return true;
		}
		else {
			appLogError(QString("VIDIOC_DQBUF - %1").arg(strerror(errno)));
			return false;
		}
	}

	if ((int)buf.index >= m_mmBuff.count()) {
		appLogError(QString("VIDIOC_DQBUF returned invalid buf.index: %1").arg(buf.index));
		return false;
	}

	if (m_pixelFormat == V4L2_PIX_FMT_MJPEG)
		result = handleJpeg(buf.index, buf.bytesused);
	else
		result = handleYUYV(buf.index, buf.bytesused);

	if (!result)
		return false;

	emit newFrame();

	return queueV4LBuffer(buf.index);
}

bool VideoDriver::handleJpeg(quint32 index, quint32 size)
{
	QByteArray frame;
	quint32 copyStart;

	uchar *p = (uchar *)m_mmBuff[index];

	if (p[0] != 0xff || p[1] != 0xd8 || p[2] != 0xff) {
		appLogDebug(QString("Not a jpeg, data starts with %1 %2 %3")
			.arg(p[0], 2, 16, QChar('0'))
			.arg(p[1], 2, 16, QChar('0'))
			.arg(p[2], 2, 16, QChar('0')));

		// we tolerate a few bad frames
		return (m_consecutiveBadFrames++ < MAX_CONSECUTIVE_BAD_FRAMES);
	}

	if (p[3] == 0xe0) {
		if (!memcmp(p + 6, "JFIF", 4)) {
			// no fixup required
			frame.append((char *)p, size);
		}
		else if (!memcmp(p + 6, "AVI1", 4)) {
			// should only need a huffman table,
			// put it right after the header p[4-5] is the header length
			copyStart = 4 + ((p[4] << 8) + p[5]);
			frame.append((char *)p, copyStart);
			frame.append((const char *)VideoDriver::huffmanTable, HUFFMAN_TABLE_SIZE);
			frame.append((char *)(p + copyStart), size - copyStart);
		}
		else {
			// todo if we find a camera that does this
			appLogDebug(QString("Unhandled JPEG header bytes 6-9 : %1 %2 %3 %4")
				.arg(p[6], 2, 16, QChar('0'))
				.arg(p[7], 2, 16, QChar('0'))
				.arg(p[8], 2, 16, QChar('0'))
				.arg(p[9], 2, 16, QChar('0')));

			// we tolerate a few bad frames
			return (m_consecutiveBadFrames++ < MAX_CONSECUTIVE_BAD_FRAMES);
		}
	}
	else if (p[3] == 0xdb) {
		// data started with a quantization table
		// assume we have an AVI1 but with a missing AVI1 header
		// so we need to add both an AVI header and huffman table
		copyStart = 2;
		frame.append((const char *)VideoDriver::jpegAviHeader, AVI_HEADER_SIZE);
		frame.append((const char *)VideoDriver::huffmanTable, HUFFMAN_TABLE_SIZE);
		frame.append((char *)(p + copyStart), size - copyStart);
	}

	m_consecutiveBadFrames = 0;
	emit newJPEG(frame);

/*
	// debugging
	//frame.append((char *)p, size);

	m_frameCount++;

	if ((m_frameCount % 50) == 0) {
		dumpRaw(frame);
		emit newJPEG(frame);
	}
*/

	return true;
}

void VideoDriver::dumpRaw(QByteArray frame)
{
	QFile file;
	file.setFileName(QString("raw%1.jpg").arg(m_frameCount));
	file.open(QIODevice::WriteOnly | QIODevice::Truncate);
	file.write(frame);
	file.close();
}

bool VideoDriver::handleYUYV(quint32 index, quint32)
{
	QImage img = YUYV2RGB(index);

	if (!img.isNull()) {
		QByteArray jpegArray;
		QBuffer buffer(&jpegArray);
		buffer.open(QIODevice::WriteOnly);
		img.save(&buffer, "JPEG", 70);
		emit newJPEG(jpegArray);
	}

	return true;
}

QImage VideoDriver::YUYV2RGB(quint32 index)
{
	int r, g, b;
	int y, u, v;

	const uchar *yuyv = reinterpret_cast<const uchar *>(m_mmBuff[index]);

	int stride = m_width * 3;

	for (int i = 0; i < m_height; i++)	{
		uchar *p = m_rgbBuff + (i * stride);

		for (int j = 0; j < m_width; j += 2) {
			y = yuyv[0] << 8;
			u = yuyv[1] - 128;
			v = yuyv[3] - 128;

			r = (y + (359 * v)) >> 8;
			g = (y - (88 * u) - (183 * v)) >> 8;
			b = (y + (454 * u)) >> 8;

			*(p++) = (r > 255) ? 255 : ((r < 0) ? 0 : r);
			*(p++) = (g > 255) ? 255 : ((g < 0) ? 0 : g);
			*(p++) = (b > 255) ? 255 : ((b < 0) ? 0 : b);

			y = yuyv[2] << 8;

			r = (y + (359 * v)) >> 8;
			g = (y - (88 * u) - (183 * v)) >> 8;
			b = (y + (454 * u)) >> 8;

			*(p++) = (r > 255) ? 255 : ((r < 0) ? 0 : r);
			*(p++) = (g > 255) ? 255 : ((g < 0) ? 0 : g);
			*(p++) = (b > 255) ? 255 : ((b < 0) ? 0 : b);

			yuyv += 4;
		}
	}

	return QImage(m_rgbBuff, m_width, m_height, m_width * 3, QImage::Format_RGB888);
}

bool VideoDriver::readFrame()
{
	fd_set fds;
	struct timeval tv;

	FD_ZERO(&fds);
	FD_SET(m_fd, &fds);
	tv.tv_sec = 5;
	tv.tv_usec = 0;

	int result = select(m_fd + 1, &fds, NULL, NULL, &tv);

	if (result == -1) {
		if (errno == EINTR) {
			return true;
        	} 
		else {
			appLogError(QString("select - %1").arg(strerror(errno)));
			return false;
		}
	}

	if (result == 0) {
		appLogError("VideoDriver: select timeout");
		return false;
	}

	return handleFrame();
}

bool VideoDriver::deviceExists()
{
	QFileInfo info(QString("/dev/video%1").arg(m_cameraNum));

	return info.exists();
}

bool VideoDriver::openDevice()
{
	static bool first_open = true;
	static bool first_check = true;
	int priority;
	char file[24];

	if (isDeviceOpen())
		return true;

	sprintf(file, "/dev/video%d", m_cameraNum);

	if (!deviceExists()){
		if (first_check) {
			appLogError(QString("Device %1 does not exist").arg(file));
			first_check = false;
		}

		return false;
	}

	m_fd = open(file, O_RDWR | O_EXCL);

	if (m_fd < 0) {
		if (first_open) {
			appLogError(QString("Open %1 failed: %2").arg(file).arg(strerror(errno)));	
			first_open = false;
		}

		return false;
	}

	appLogDebug(QString("Opened %1 with fd %2").arg(file).arg(m_fd));

	// attempt to claim exclusive control of the device
	priority = V4L2_PRIORITY_RECORD;

	if (-1 == xioctl(VIDIOC_S_PRIORITY, &priority)) {
		// not supported on all kernels, ignore if that's the case
		if (errno != ENOTTY) { 
			if (first_open) {
        			appLogError(QString("VIDIOC_S_PRIORITY - %1").arg(strerror(errno)));
				appLogError(QString("Failed to get exclusive access to %1").arg(file));	
				first_open = false;
			}

			closeDevice();
			return false;
		}
	}

	m_formatList.clear();
	m_sizeList.clear();
	m_rateList.clear();

	if (m_rgbBuff) {
		delete [] m_rgbBuff;
		m_rgbBuff = NULL;
	}

	queryAvailableFormats();

	if (!choosePixelFormat()) {
		closeDevice();
		return false;
	}

	queryAvailableSizes();

	if (!chooseFrameSize()) {
		closeDevice();
		return false;
	}

	queryAvailableRates();

	if (!chooseFrameRate()) {
		closeDevice();
		return false;
	}

	if (!setImageFormat()) {
		closeDevice();
		return false;
	}

	if (!setFrameRate()) {
		closeDevice();
		return false;
	}

	if (!allocMmapBuffers()) {
		closeDevice();
		return false;
	}

	if (m_pixelFormat == V4L2_PIX_FMT_YUYV) {
		m_rgbBuff = new uchar[m_width * m_height * 3];

		if (!m_rgbBuff) {
			closeDevice();
			return false;
		}
	}

	emit pixelFormat(m_pixelFormat);
	emit videoFormat(m_width, m_height, m_frameRate);

	return true;
}

void VideoDriver::queryAvailableFormats()
{
	struct v4l2_fmtdesc f;
	char fmt[4];

	memset(&f, 0, sizeof(f));
	f.index = 0;
	f.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    	appLogInfo("=== Available Formats ===");

	while (xioctl(VIDIOC_ENUM_FMT, &f) == 0) {
		fmt[0] = f.pixelformat & 0xff;
		fmt[1] = (f.pixelformat >> 8) & 0xff;
		fmt[2] = (f.pixelformat >> 16) & 0xff;
		fmt[3] = (f.pixelformat >> 24) & 0xff;

        	appLogInfo(QString("[%1] %2%3%4%5")
			.arg(f.index)
			.arg(fmt[0])
			.arg(fmt[1])
			.arg(fmt[2])
			.arg(fmt[3]));

		m_formatList.append(f.pixelformat);
		f.index++;
	}
}

bool VideoDriver::choosePixelFormat()
{
	bool have_mjpg = false;
	bool have_yuyv = false;

	for (int i = 0; i < m_formatList.count(); i++) {
		if (m_formatList.at(i) == V4L2_PIX_FMT_YUYV)
			have_yuyv = true;
		else if (m_formatList.at(i) == V4L2_PIX_FMT_MJPEG)
			have_mjpg = true;
	}

	if ((m_preferredFormat == V4L2_PIX_FMT_YUYV) && have_yuyv) {
		m_pixelFormat = V4L2_PIX_FMT_YUYV;
        	appLogInfo("Choosing format YUYV");
		return true;
	}
	else if (have_mjpg) {
		m_pixelFormat = V4L2_PIX_FMT_MJPEG;
        	appLogInfo("Choosing format MJPG");
		return true;
	}
	else if (have_yuyv) {
		m_pixelFormat = V4L2_PIX_FMT_YUYV;
        	appLogInfo("Choosing format YUYV");
		return true;
	}

    	appLogError("No supported formats available. Suported formats are MJPG or YUYV.");
	return false;
}

// We need to know m_pixelFormat before calling this
void VideoDriver::queryAvailableSizes()
{
	struct v4l2_frmsizeenum f;

	memset(&f, 0, sizeof(f));

	f.index = 0;
	f.pixel_format = m_pixelFormat;
	f.type = V4L2_FRMSIZE_TYPE_DISCRETE;

	while (xioctl(VIDIOC_ENUM_FRAMESIZES, &f) == 0) {
		if (f.discrete.width > 0 && f.discrete.height > 0)
			m_sizeList.append(QSize(f.discrete.width, f.discrete.height));

		f.index++;
	}

	qSort(m_sizeList.begin(), m_sizeList.end(), VideoDriver::frameSizeLessThan);

	appLogInfo("=== Available Frame Sizes (W x H) ===");

	for (int i = 0; i < m_sizeList.count(); i++) {
		appLogInfo(QString("[%1] %2 x %3")
			.arg(i)
			.arg(m_sizeList.at(i).width())
			.arg(m_sizeList.at(i).height()));
	}
}

// Try for an exact match, but if that fails find the biggest size
// that does not exceed the requested size, but if all else fails
// take the smallest frame size.
bool VideoDriver::chooseFrameSize()
{
	int i, j;
	int search_width, search_height;
	bool found = false;

	search_width = m_preferredWidth;
	search_height = m_preferredHeight;

	if (m_preferredFormat != V4L2_PIX_FMT_MJPEG) {
		if (m_preferredWidth > 320 || m_preferredHeight > 240) {
			search_width = 320;
			search_height = 240;
            		appLogWarn("Restricting frame size to (320 x 240) due to non-MJPG format");
		}
	}

	for (i = 0; i < m_sizeList.count(); i++) {
		if (m_sizeList.at(i).width() == search_width && m_sizeList.at(i).height() == search_height) {
			found = true;
			break;
		}
	}

	if (!found) {
		for (i = 0; i < m_sizeList.count(); i++) {
			if (search_width <= m_sizeList.at(i).width())
				break;
		}

		// if i == 0 then we are done, take i
		if (i > 0) {
			// otherwise we overshot, so back up
			i--;

			// now check the heights
			for (j = i; j < m_sizeList.count(); j++) {
				if (m_sizeList.at(i).width() != m_sizeList.at(j).width())
					break;

				if (search_height <= m_sizeList.at(j).height())
					break;
			}

			// if they are the same we are done, but if not then
			// we overshot j so back up one
			if (j > i)
				i = j - 1;
		}
	}

	m_width = m_sizeList.at(i).width();
	m_height = m_sizeList.at(i).height();

    	appLogInfo(QString("Requested frame size was (%1 x %2) choosing (%3 x %4)")
		.arg(m_preferredWidth)
		.arg(m_preferredHeight)
		.arg(m_width)
		.arg(m_height));

	if (m_pixelFormat == V4L2_PIX_FMT_YUYV && m_width > 640)
        	appLogInfo("Chosen size with format YUYV may have performance issues");

	return true;
}

// We need to know m_pixelFormat, m_width and m_height before calling
void VideoDriver::queryAvailableRates()
{
	struct v4l2_frmivalenum f;

	memset(&f, 0, sizeof(f));

	f.index = 0;
	f.pixel_format = m_pixelFormat;
	f.width = m_width;
	f.height = m_height;

	if (-1 == xioctl(VIDIOC_ENUM_FRAMEINTERVALS, &f))
		return;

	if (f.type != V4L2_FRMIVAL_TYPE_DISCRETE) {
        	appLogWarn(QString("TODO: Unhandled frame interval type %1").arg(f.type));
		return;
	}

	while (xioctl(VIDIOC_ENUM_FRAMEINTERVALS, &f) == 0) {
		if (f.discrete.numerator > 0 && f.discrete.denominator > 0)
			m_rateList.append(QSize(f.discrete.numerator, f.discrete.denominator));

		f.index++;
	}

	qSort(m_rateList.begin(), m_rateList.end(), VideoDriver::frameRateLessThan);

	appLogInfo("=== Available Frame Rates (fps) ===");
	for (int i = 0; i < m_rateList.count(); i++) {
		qreal rate = (qreal)m_rateList.at(i).height() / (qreal)m_rateList.at(i).width();
		appLogInfo(QString("[%1] %2").arg(i).arg(rate));
	}
}

// Choose the fastest rate not faster then the preferred rate
bool VideoDriver::chooseFrameRate()
{
	m_frameRate = m_preferredFrameRate;
	m_frameRateIndex = -1;

	for (int i = m_rateList.count() - 1; i > 0; i--) {
		qreal rate = (qreal)m_rateList.at(i).height() / (qreal)m_rateList.at(i).width();

		if (rate <= m_preferredFrameRate) {
			m_frameRate = (qreal)m_rateList.at(i).height() / (qreal)m_rateList.at(i).width();
			m_frameRateIndex = i;
			break;
		}
	}

	appLogInfo(QString("Requested frame rate was (%1) choosing (%2)")
		.arg(m_preferredFrameRate)
		.arg(m_frameRate));

	return true;
}

bool VideoDriver::setImageFormat()
{
	struct v4l2_format fmt;

	memset(&fmt, 0, sizeof(fmt));

	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = m_width;
	fmt.fmt.pix.height = m_height;
	fmt.fmt.pix.pixelformat = m_pixelFormat;
	fmt.fmt.pix.field = V4L2_FIELD_NONE;

	if (-1 == xioctl(VIDIOC_S_FMT, &fmt)) {
//        	appLogError(QString("VIDIOC_S_FMT - %1").arg(strerror(errno)));
		return false;
	}

	if ((int)fmt.fmt.pix.width != m_width) {
        	appLogError("Width changed after VIDIOC_S_FMT");
        	appLogError(QString("Requested width %1  Got %2").arg(m_width).arg(fmt.fmt.pix.width));
		return false;
	}

	if ((int)fmt.fmt.pix.height != m_height) {
        	appLogError("Height changed after VIDIOC_S_FMT");
        	appLogError(QString("Requested height %1  Got %2").arg(m_height).arg(fmt.fmt.pix.height));
		return false;
	}

	if (fmt.fmt.pix.pixelformat != m_pixelFormat) {
        	appLogError("pixelformat changed after VIDIOC_S_FMT");
		return false;
	}

	return true;
}

bool VideoDriver::setFrameRate()
{
	struct v4l2_streamparm sp;

	if (m_frameRateIndex < 0 || m_frameRateIndex >= m_rateList.count())
		return true;

	memset(&sp, 0, sizeof(sp));

	sp.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (-1 == xioctl(VIDIOC_G_PARM, &sp)) {
        	appLogError("Failed to get stream parameters for frame rate adjustment");
		return false;
	}

	if (!(sp.parm.capture.capability & V4L2_CAP_TIMEPERFRAME)) {
        	appLogWarn("Frame rate adjustment not supported");
		return true;
	}

	if ((int)sp.parm.capture.timeperframe.numerator == m_rateList.at(m_frameRateIndex).width()
			&& (int)sp.parm.capture.timeperframe.denominator == m_rateList.at(m_frameRateIndex).height()) {
		// nothing to do
		return true;
	}

	sp.parm.capture.timeperframe.numerator = m_rateList.at(m_frameRateIndex).width();
	sp.parm.capture.timeperframe.denominator = m_rateList.at(m_frameRateIndex).height();

	if (-1 == xioctl(VIDIOC_S_PARM, &sp)) {
        	appLogError(QString("Failed to set frame rate to %1 fps").arg(m_frameRate));
		return false;
	}

	return true;
}

bool VideoDriver::allocMmapBuffers()
{
	struct v4l2_requestbuffers req;
	struct v4l2_buffer buf;

	freeMmapBuffers();

	memset(&req, 0, sizeof(req));

	// ask for 4
	req.count = 4;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;

	if (-1 == xioctl(VIDIOC_REQBUFS, &req)) {
        	appLogError(QString("VIDIOC_REQBUFS - %1").arg(strerror(errno)));
		return false;
	}

	// but we're cool with whatever we get
	if (req.count < 1) {
		// TODO: go to fallback mode where we provide copy buffers
        	appLogError("mmap buffers not supported by camera");
		return false;
	}

	// get the addresses for our mmap'd buffers
	for (quint32 i = 0; i < req.count; i++) {
		memset(&buf, 0, sizeof(buf));

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;

		if (-1 == xioctl(VIDIOC_QUERYBUF, &buf)) {
            		appLogError(QString("VIDIOC_QUERYBUF - %1").arg(strerror(errno)));
			return false;
		}

		if (i == 0) {
			m_mmBuffLen = buf.length;
		}
		else if (m_mmBuffLen != buf.length) {
			// they should always be the same size
            		appLogError("mmap buffers are not the same size");
			return false;
		}

		char *p = (char *) mmap(NULL, buf.length, PROT_READ | PROT_WRITE,
					MAP_SHARED, m_fd, buf.m.offset);

		if (MAP_FAILED == p) {
            		appLogError(QString("mmap - %1").arg(strerror(errno)));
			return false;
		}

		m_mmBuff.append(p);
	}

	return true;
}

void VideoDriver::freeMmapBuffers()
{
	for (int i = 0; i < m_mmBuff.count(); i++) {
		char *p = m_mmBuff[i];

		if (p) {
			munmap(p, m_mmBuffLen);
			m_mmBuff[i] = NULL;
		}
	}

	m_mmBuff.clear();
	m_mmBuffLen = 0;
}

bool VideoDriver::queueV4LBuffer(quint32 index)
{
	struct v4l2_buffer buf;

	memset(&buf, 0, sizeof(buf));

	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	buf.index = index;

	if (-1 == xioctl (VIDIOC_QBUF, &buf)) {
        	appLogError(QString("VIDIOC_QBUF - %1").arg(strerror(errno)));
		return false;
	}

	return true;
}

bool VideoDriver::streamOn()
{
	for (int i = 0; i < m_mmBuff.count(); i++) {
		if (!queueV4LBuffer(i))
			return false;
	}

	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (-1 == xioctl(VIDIOC_STREAMON, &type)) {
        	appLogError(QString("VIDIOC_STREAMON - %1").arg(strerror(errno)));
		return false;
	}

	return true;
}

void VideoDriver::streamOff()
{
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	xioctl(VIDIOC_STREAMOFF, &type);
}

int VideoDriver::xioctl(int request, void *arg)
{
	int r;

	do {
		r = ioctl(m_fd, request, arg);
	} while (-1 == r && EINTR == errno);

	return r;
}

void VideoDriver::closeDevice()
{
    if (m_timer != -1)
        killTimer(m_timer);
	m_timer = -1;
	streamOff();

	if (isDeviceOpen()) {
		close(m_fd);
		m_fd = -1;
	}

	freeMmapBuffers();

	if (m_rgbBuff) {
		delete [] m_rgbBuff;
		m_rgbBuff = NULL;
	}
}

bool VideoDriver::isDeviceOpen()
{
	return (m_fd != -1);
}

QSize VideoDriver::getImageSize()
{
	return QSize(m_width, m_height);
}

bool VideoDriver::frameSizeLessThan(const QSize &a, const QSize &b)
{
	if (a.width() < b.width())
		return true;
	else if (a.width() > b.width())
		return false;
	else if (a.height() < b.height())
		return true;
	else
		return false;
}

// height = denominator, width = numerator
bool VideoDriver::frameRateLessThan(const QSize &a, const QSize &b)
{
	qreal sa = (qreal)a.height() / (qreal)a.width();
	qreal sb = (qreal)b.height() / (qreal)b.width();

	return sa < sb;
}

const unsigned char VideoDriver::jpegAviHeader[AVI_HEADER_SIZE] = {
	0xff, 0xd8, 0xff, 0xe0, 0x00, 0x21, 0x41, 0x56,
	0x49, 0x31, 0x00, 0x01, 0x01, 0x01, 0x00, 0x78,
	0x00, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char VideoDriver::huffmanTable[HUFFMAN_TABLE_SIZE] = {
	0xff, 0xc4, 0x01, 0xa2, 0x00, 0x00, 0x01, 0x05,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02,
	0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
	0x0b, 0x01, 0x00, 0x03, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
	0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x10, 0x00,
	0x02, 0x01, 0x03, 0x03, 0x02, 0x04, 0x03, 0x05,
	0x05, 0x04, 0x04, 0x00, 0x00, 0x01, 0x7d, 0x01,
	0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21,
	0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07, 0x22,
	0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08, 0x23,
	0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0, 0x24,
	0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16, 0x17,
	0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28, 0x29,
	0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a,
	0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a,
	0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a,
	0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a,
	0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a,
	0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a,
	0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99,
	0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8,
	0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
	0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6,
	0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4, 0xd5,
	0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2, 0xe3,
	0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xf1,
	0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9,
	0xfa, 0x11, 0x00, 0x02, 0x01, 0x02, 0x04, 0x04,
	0x03, 0x04, 0x07, 0x05, 0x04, 0x04, 0x00, 0x01,
	0x02, 0x77, 0x00, 0x01, 0x02, 0x03, 0x11, 0x04,
	0x05, 0x21, 0x31, 0x06, 0x12, 0x41, 0x51, 0x07,
	0x61, 0x71, 0x13, 0x22, 0x32, 0x81, 0x08, 0x14,
	0x42, 0x91, 0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33,
	0x52, 0xf0, 0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16,
	0x24, 0x34, 0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19,
	0x1a, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x35, 0x36,
	0x37, 0x38, 0x39, 0x3a, 0x43, 0x44, 0x45, 0x46,
	0x47, 0x48, 0x49, 0x4a,	0x53, 0x54, 0x55, 0x56,
	0x57, 0x58, 0x59, 0x5a, 0x63, 0x64, 0x65, 0x66,
	0x67, 0x68, 0x69, 0x6a, 0x73, 0x74, 0x75, 0x76,
	0x77, 0x78, 0x79, 0x7a, 0x82, 0x83, 0x84, 0x85,
	0x86, 0x87, 0x88, 0x89, 0x8a, 0x92, 0x93, 0x94,
	0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3,
	0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2,
	0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba,
	0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9,
	0xca, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8,
	0xd9, 0xda, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7,
	0xe8, 0xe9, 0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6,
	0xf7, 0xf8, 0xf9, 0xfa
};

