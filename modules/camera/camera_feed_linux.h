/**************************************************************************/
/*  camera_feed_linux.h                                                   */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#ifndef CAMERA_FEED_LINUX_H
#define CAMERA_FEED_LINUX_H

#include "buffer_decoder.h"
#include "core/os/thread.h"
#include "servers/camera/camera_feed.h"
#include <fcntl.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

struct streaming_buffer;

class CameraFeedLinux : public CameraFeed {
private:
	SafeFlag exit_flag;
	Thread *thread;
	String device_name;
	int file_descriptor;
	streaming_buffer *buffers;
	unsigned int buffer_count;
	BufferDecoder *buffer_decoder;

	static void update_buffer_thread_func(void *p);

	void update_buffer();
	void query_device(String device_name);
	void add_format(v4l2_fmtdesc description, v4l2_frmsize_discrete size, int frame_numerator, int frame_denominator);
	bool request_buffers();
	bool start_capturing();
	void read_frame();
	void stop_capturing();
	void unmap_buffers(unsigned int count);
	BufferDecoder *create_buffer_decoder();
	void start_thread();

public:
	CameraFeedLinux(String p_device_name);
	virtual ~CameraFeedLinux();

	String get_device_name() const;
	bool activate_feed();
	void deactivate_feed();
	bool set_format(int index, const Dictionary &parameters);
};

#endif // CAMERA_FEED_LINUX_H
