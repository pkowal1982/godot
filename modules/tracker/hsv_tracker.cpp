/*************************************************************************/
/*  hsv_tracker.cpp                                                      */
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

#include "hsv_tracker.h"
#include "core/os/os.h"

void HsvTracker::_bind_methods() {
	ClassDB::bind_method(D_METHOD("track", "image", "example", "threshold", "destination_image"), &HsvTracker::track, DEFVAL(Ref<Image>()));
}

Vector2 HsvTracker::track(Ref<Image> image, Color example, Vector3 threshold, Ref<Image> destination_image) {
	uint64_t begin = OS::get_singleton()->get_ticks_usec();

	Vector2 center = Vector2(-1, -1);
	ERR_FAIL_NULL_V_MSG(image, center, "Cannot track null image");
	ERR_FAIL_COND_V_MSG(wrong_format(image), center, "Cannot track image in format other than FORMAT_RGB8 or FORMAT_RGBA8");

	uint32_t count = 0;
	uint32_t horizontal = 0;
	uint32_t vertical = 0;
	const uint8_t *src = image->get_data().ptr();

	Vector<uint8_t> destination_data;
	destination_data.resize(image->get_width() * image->get_height());
	uint8_t *dst = destination_data.ptrw();
	memset(dst, 0, destination_data.size());

	HSV hsv_example = HSV::from_argb32(example.to_argb32());
	HSV hsv_threshold = HSV(CLAMP(threshold.x * 255, 0, 255), CLAMP(threshold.y * 255, 0, 255), CLAMP(threshold.z * 255, 0, 255));
	int skip = image->get_format() == Image::FORMAT_RGBA8 ? 1 : 0;

	for (int y = 0; y < image->get_height(); y++) {
		for (int x = 0; x < image->get_width(); x++) {
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
		center = Vector2(horizontal, vertical) / count;
	}

	if (destination_image != nullptr) {
		update_destination_image(destination_image, Vector2i(image->get_width(), image->get_height()), destination_data, center);
	}

	uint64_t end = OS::get_singleton()->get_ticks_usec();
	print_line(vformat("Tracking took %d microseconds, center at: %d, %d", end - begin, center.x, center.y));
	return center;
}

bool HsvTracker::wrong_format(Ref<Image> image) {
	return image == nullptr || (image->get_format() != Image::FORMAT_RGB8 && image->get_format() != Image::FORMAT_RGBA8);
}

void HsvTracker::update_destination_image(Ref<Image> destination_image, Vector2i size, Vector<uint8_t> data, Vector2i center) {
	destination_image->create(size.width, size.height, false, Image::FORMAT_L8, data);
	if (center.x != -1) {
		draw_cross(destination_image, center);
	}
}

void HsvTracker::draw_cross(Ref<Image> image, Vector2i center) {
	Color white = Color::named("WHITE");
	for (int x = MAX(center.x - 16, 0); x < MIN(center.x + 16, image->get_width()); x++) {
		image->set_pixel(x, center.y, white);
	}
	for (int y = MAX(center.y - 16, 0); y < MIN(center.y + 16, image->get_height()); y++) {
		image->set_pixel(center.x, y, white);
	}
}
