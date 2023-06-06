/**************************************************************************/
/*  hsv_tracker.cpp                                                       */
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

#include "hsv_tracker.h"
#include "core/os/os.h"

void HsvTracker::_bind_methods() {
	ClassDB::bind_method(D_METHOD("track", "image", "example", "threshold", "destination_image"), &HsvTracker::track, DEFVAL(Ref<Image>()));
}

Vector3 HsvTracker::track(Ref<Image> p_image, Color p_example, Vector3 p_threshold, Ref<Image> p_destination_image) {
	//uint64_t begin = OS::get_singleton()->get_ticks_usec();

	Vector3 center = Vector3(-1, -1, -1);
	ERR_FAIL_NULL_V_MSG(p_image, center, "Cannot track null image");
	ERR_FAIL_COND_V_MSG(wrong_format(p_image), center, "Cannot track image in format other than FORMAT_RGB8 or FORMAT_RGBA8");

	uint32_t count = 0;
	uint32_t horizontal = 0;
	uint32_t vertical = 0;
	const uint8_t *src = p_image->get_data().ptr();

	Vector<uint8_t> destination_data;
	destination_data.resize(p_image->get_width() * p_image->get_height());
	uint8_t *dst = destination_data.ptrw();
	memset(dst, 0, destination_data.size());

	HSV hsv_example = HSV::from_argb32(p_example.to_argb32());
	HSV hsv_threshold = HSV(CLAMP(p_threshold.x * 255, 0, 255), CLAMP(p_threshold.y * 255, 0, 255), CLAMP(p_threshold.z * 255, 0, 255));
	int skip = p_image->get_format() == Image::FORMAT_RGBA8 ? 1 : 0;

	for (int y = 0; y < p_image->get_height(); y++) {
		for (int x = 0; x < p_image->get_width(); x++) {
			uint8_t r = *(src++);
			uint8_t g = *(src++);
			uint8_t b = *(src++);

			src += skip;
			HSV hsv = HSV::from_rgb(r, g, b);
			if (hsv.is_similar(hsv_example, hsv_threshold)) {
				horizontal += x;
				vertical += y;
				count++;
				*dst = 0xC0;
			}
			dst++;
		}
	}

	if (count != 0) {
		center = Vector3(horizontal / count, vertical / count, sqrt(1.333 * count));
	}

	if (p_destination_image != nullptr) {
		p_destination_image->set_data(p_image->get_width(), p_image->get_height(), false, Image::FORMAT_L8, destination_data);
		update_destination_image(p_destination_image, Vector2i(p_image->get_width(), p_image->get_height()), destination_data, Vector2(center.x, center.y));
	}

	//uint64_t end = OS::get_singleton()->get_ticks_usec();
	//print_line(vformat("Tracking took %d microseconds, center at: %d, %d, diameter: %d", end - begin, center.x, center.y, center.z));
	return center;
}

bool HsvTracker::wrong_format(Ref<Image> image) {
	return image == nullptr || (image->get_format() != Image::FORMAT_RGB8 && image->get_format() != Image::FORMAT_RGBA8);
}

void HsvTracker::update_destination_image(Ref<Image> destination_image, Vector2i size, Vector<uint8_t> data, Vector2i center) {
	destination_image->create_from_data(size.width, size.height, false, Image::FORMAT_L8, data);
	if (center.x != -1) {
		draw_cross(destination_image, center);
	}
}

void HsvTracker::draw_cross(Ref<Image> p_image, Vector2i p_center) {
	Color white = Color::named("WHITE");
	for (int x = MAX(p_center.x - 16, 0); x < MIN(p_center.x + 16, p_image->get_width()); x++) {
		p_image->set_pixel(x, p_center.y, white);
	}
	for (int y = MAX(p_center.y - 16, 0); y < MIN(p_center.y + 16, p_image->get_height()); y++) {
		p_image->set_pixel(p_center.x, y, white);
	}
}
