/*************************************************************************/
/*  camera_feed_linux.cpp                                                */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2021 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2021 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "camera_feed_linux.h"

void CameraFeedLinux::update_buffer_thread_func(void *p) {
	if (p) {
		CameraFeedLinux *camera_feed_linux = (CameraFeedLinux *)p;
		camera_feed_linux->update_buffer();
	}
}

void CameraFeedLinux::update_buffer() {
	while (!exit_flag.is_set()) {
		read_frame();
		usleep(10000);
	}
}

void CameraFeedLinux::query_device(String device_name) {
	file_descriptor = open(device_name.ascii(), O_RDWR | O_NONBLOCK, 0);

	struct v4l2_capability capability;
	if (ioctl(file_descriptor, VIDIOC_QUERYCAP, &capability) == -1) {
		print_error("Cannot query device");
		return;
	}
	name = String((char *)capability.card);

	for (int index = 0;; index++) {
		struct v4l2_fmtdesc fmtdesc;
		memset(&fmtdesc, 0, sizeof(fmtdesc));
		fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		fmtdesc.index = index;

		if (ioctl(file_descriptor, VIDIOC_ENUM_FMT, &fmtdesc) == -1) {
			break;
		}

		for (int res_index = 0;; res_index++) {
			struct v4l2_frmsizeenum frmsizeenum;
			memset(&frmsizeenum, 0, sizeof(frmsizeenum));
			frmsizeenum.pixel_format = fmtdesc.pixelformat;
			frmsizeenum.index = res_index;

			if (ioctl(file_descriptor, VIDIOC_ENUM_FRAMESIZES, &frmsizeenum) == -1) {
				break;
			}

			for (int framerate_index = 0;; framerate_index++) {
				struct v4l2_frmivalenum frmivalenum;
				memset(&frmivalenum, 0, sizeof(frmivalenum));
				frmivalenum.pixel_format = fmtdesc.pixelformat;
				frmivalenum.width = frmsizeenum.discrete.width;
				frmivalenum.height = frmsizeenum.discrete.height;
				frmivalenum.index = framerate_index;

				if (ioctl(file_descriptor, VIDIOC_ENUM_FRAMEINTERVALS, &frmivalenum) == -1) {
					if (framerate_index == 0) {
						add_format(fmtdesc, frmsizeenum.discrete, -1, 1);
					}
					break;
				}

				add_format(fmtdesc, frmsizeenum.discrete, frmivalenum.discrete.numerator, frmivalenum.discrete.denominator);
			}
		}
	}

	close(file_descriptor);
}

void CameraFeedLinux::add_format(v4l2_fmtdesc description, v4l2_frmsize_discrete size, int frame_numerator, int frame_denominator) {
	FeedFormat feed_format;
	feed_format.width = size.width;
	feed_format.height = size.height;
	feed_format.format = String((char *)description.description);
	feed_format.frame_numerator = frame_numerator;
	feed_format.frame_denominator = frame_denominator;
	feed_format.pixel_format = description.pixelformat;
	print_verbose(vformat("%s %dx%d@%d/%dfps", (char *)description.description, size.width, size.height, frame_denominator, frame_numerator));
	formats.push_back(feed_format);
}

bool CameraFeedLinux::request_buffers() {
	struct v4l2_requestbuffers requestbuffers;

	memset(&requestbuffers, 0, sizeof(requestbuffers));
	requestbuffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	requestbuffers.memory = V4L2_MEMORY_MMAP;
	requestbuffers.count = 4;

	if (ioctl(file_descriptor, VIDIOC_REQBUFS, &requestbuffers) == -1) {
		print_error(vformat("ioctl(VIDIOC_REQBUFS) error: %d", errno));
		return false;
	}

	if (requestbuffers.count < 2) {
		/* Free the buffers here. */
		print_error("Not enough buffers granted");
		return false;
	}

	buffer_count = requestbuffers.count;
	buffers = new streaming_buffer[buffer_count];

	for (unsigned int i = 0; i < buffer_count; i++) {
		struct v4l2_buffer buffer;

		memset(&buffer, 0, sizeof(buffer));
		buffer.type = requestbuffers.type;
		buffer.memory = V4L2_MEMORY_MMAP;
		buffer.index = i;

		if (ioctl(file_descriptor, VIDIOC_QUERYBUF, &buffer) == -1) {
			print_error(vformat("ioctl(VIDIOC_QUERYBUF) error: %d", errno));
			return false;
		}

		buffers[i].length = buffer.length;
		buffers[i].start = mmap(NULL, buffer.length, PROT_READ | PROT_WRITE, MAP_SHARED, file_descriptor, buffer.m.offset);

		if (buffers[i].start == MAP_FAILED) {
			for (unsigned int b = 0; b < i; b++) {
				unmap_buffers(i);
			}
			print_error("Mapping buffers failed");
			return false;
		}
	}

	return true;
}

bool CameraFeedLinux::start_capturing() {
	for (unsigned int i = 0; i < buffer_count; i++) {
		struct v4l2_buffer buffer;

		memset(&buffer, 0, sizeof(buffer));
		buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buffer.memory = V4L2_MEMORY_MMAP;
		buffer.index = i;

		if (ioctl(file_descriptor, VIDIOC_QBUF, &buffer) == -1) {
			print_error(vformat("ioctl(VIDIOC_QBUF) error: %d", errno));
			return false;
		}
	}

	enum v4l2_buf_type type;
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (ioctl(file_descriptor, VIDIOC_STREAMON, &type) == -1) {
		print_error(vformat("ioctl(VIDIOC_STREAMON) error: %d", errno));
		return false;
	}

	return true;
}

void CameraFeedLinux::read_frame() {
	struct v4l2_buffer buffer;
	memset(&buffer, 0, sizeof(buffer));
	buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buffer.memory = V4L2_MEMORY_MMAP;

	if (ioctl(file_descriptor, VIDIOC_DQBUF, &buffer) == -1) {
		if (errno != EAGAIN) {
			print_error(vformat("ioctl(VIDIOC_DQBUF) error: %d", errno));
			exit_flag.set();
		}
		return;
	}

	buffer_decoder->decode(buffers[buffer.index]);

	if (ioctl(file_descriptor, VIDIOC_QBUF, &buffer) == -1) {
		print_error(vformat("ioctl(VIDIOC_QBUF) error: %d", errno));
	}

	emit_signal(SNAME("frame_changed"));
}

void CameraFeedLinux::stop_capturing() {
	enum v4l2_buf_type type;
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (ioctl(file_descriptor, VIDIOC_STREAMOFF, &type) == -1) {
		print_error(vformat("ioctl(VIDIOC_STREAMOFF) error: %d", errno));
	}
}

void CameraFeedLinux::unmap_buffers(unsigned int count) {
	for (unsigned int i = 0; i < count; i++) {
		munmap(buffers[i].start, buffers[i].length);
	}
}

void CameraFeedLinux::start_thread() {
	exit_flag.clear();
	thread = memnew(Thread);
	thread->start(CameraFeedLinux::update_buffer_thread_func, this);
}

CameraFeedLinux::CameraFeedLinux(String device_name) :
		CameraFeed() {
	this->device_name = device_name;
	query_device(device_name);
	set_format(0, Dictionary());
}

CameraFeedLinux::~CameraFeedLinux() {
	if (is_active()) {
		deactivate_feed();
	}
}

String CameraFeedLinux::get_device_name() const {
	return device_name;
}

bool CameraFeedLinux::activate_feed() {
	file_descriptor = open(device_name.ascii(), O_RDWR | O_NONBLOCK, 0);
	if (request_buffers() && start_capturing()) {
		buffer_decoder = create_buffer_decoder();
		start_thread();
	} else {
		print_error("Could not activate feed");
		return false;
	}
	return true;
}

BufferDecoder *CameraFeedLinux::create_buffer_decoder() {
	switch (formats[selected_format].pixel_format) {
		case V4L2_PIX_FMT_MJPEG:
		case V4L2_PIX_FMT_JPEG:
			return memnew(JpegBufferDecoder(this));
		case V4L2_PIX_FMT_YUYV:
		case V4L2_PIX_FMT_YYUV:
		case V4L2_PIX_FMT_YVYU:
		case V4L2_PIX_FMT_UYVY:
		case V4L2_PIX_FMT_VYUY: {
			String output = parameters["output"];
			if (output == "separate") {
				return memnew(SeparateYuyvBufferDecoder(this));
			}
			if (output == "grayscale") {
				return memnew(YuyvToGrayscaleBufferDecoder(this));
			}
			if (output == "copy") {
				return memnew(CopyBufferDecoder(this, false));
			}
			return memnew(YuyvToRgbBufferDecoder(this));
		}
		default:
			return memnew(CopyBufferDecoder(this, true));
	}
}

void CameraFeedLinux::deactivate_feed() {
	exit_flag.set();
	thread->wait_to_finish();
	memdelete(thread);
	stop_capturing();
	unmap_buffers(buffer_count);
	memdelete(buffer_decoder);
	for (int i = 0; i < CameraServer::FEED_IMAGES; i++) {
		RID placeholder = RenderingServer::get_singleton()->texture_2d_placeholder_create();
		RenderingServer::get_singleton()->texture_replace(texture[i], placeholder);
	}
	base_width = 0;
	base_height = 0;
	close(file_descriptor);

	emit_signal(SNAME("format_changed"));
}

bool CameraFeedLinux::set_format(int index, const Dictionary &parameters) {
	ERR_FAIL_COND_V(!CameraFeed::set_format(index, parameters), false);

	selected_format = index;

	FeedFormat feed_format = formats[index];

	file_descriptor = open(device_name.ascii(), O_RDWR | O_NONBLOCK, 0);

	struct v4l2_format format;
	memset(&format, 0, sizeof(format));
	format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	format.fmt.pix.width = feed_format.width;
	format.fmt.pix.height = feed_format.height;
	format.fmt.pix.pixelformat = feed_format.pixel_format;

	if (ioctl(file_descriptor, VIDIOC_S_FMT, &format) == -1) {
		print_error(vformat("Cannot set format, error: %d", errno));
		close(file_descriptor);
		return false;
	}

	if (feed_format.frame_numerator > 0) {
		struct v4l2_streamparm param;
		memset(&param, 0, sizeof(param));

		param.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		param.parm.capture.capability = V4L2_CAP_TIMEPERFRAME;
		param.parm.capture.timeperframe.numerator = feed_format.frame_numerator;
		param.parm.capture.timeperframe.denominator = feed_format.frame_denominator;

		if (ioctl(file_descriptor, VIDIOC_S_PARM, &param) == -1) {
			print_error(vformat("Cannot set framerate, error: %d", errno));
			close(file_descriptor);
			return false;
		}
	}
	close(file_descriptor);

	emit_signal(SNAME("format_changed"));

	return true;
}
