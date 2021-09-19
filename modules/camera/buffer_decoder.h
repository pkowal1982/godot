/*************************************************************************/
/*  buffer_decoder.h                                                     */
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

#ifndef BUFFERDECODER_H
#define BUFFERDECODER_H

#include "core/io/image.h"
#include "core/templates/vector.h"
#include "servers/camera_server.h"
#include <linux/videodev2.h>
#include <stdint.h>

class CameraFeed;

struct streaming_buffer {
	void *start;
	size_t length;
};

class BufferDecoder {
protected:
	CameraFeed *camera_feed;
	int width;
	int height;

public:
	virtual void decode(streaming_buffer buffer) = 0;

	BufferDecoder(CameraFeed *camera_feed);
	virtual ~BufferDecoder(){};
};

class AbstractYuyvBufferDecoder : public BufferDecoder {
protected:
	int *component_indexes;

public:
	AbstractYuyvBufferDecoder(CameraFeed *camera_feed);
	~AbstractYuyvBufferDecoder();
};

class SeparateYuyvBufferDecoder : public AbstractYuyvBufferDecoder {
private:
	Vector<uint8_t> y_image_data;
	Vector<uint8_t> cbcr_image_data;

public:
	SeparateYuyvBufferDecoder(CameraFeed *camera_feed);
	void decode(streaming_buffer buffer);
};

class YuyvToGrayscaleBufferDecoder : public AbstractYuyvBufferDecoder {
private:
	Vector<uint8_t> image_data;

public:
	YuyvToGrayscaleBufferDecoder(CameraFeed *camera_feed);
	void decode(streaming_buffer buffer);
};

class YuyvToRgbBufferDecoder : public AbstractYuyvBufferDecoder {
private:
	Vector<uint8_t> image_data;

public:
	YuyvToRgbBufferDecoder(CameraFeed *camera_feed);
	void decode(streaming_buffer buffer);
};

class CopyBufferDecoder : public BufferDecoder {
private:
	Vector<uint8_t> image_data;
	bool rgba;

public:
	CopyBufferDecoder(CameraFeed *camera_feed, bool rgba);
	void decode(streaming_buffer buffer);
};

class JpegBufferDecoder : public BufferDecoder {
private:
	Vector<uint8_t> image_data;

public:
	JpegBufferDecoder(CameraFeed *camera_feed);
	void decode(streaming_buffer buffer);
};

#endif
