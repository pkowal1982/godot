/*************************************************************************/
/*  marker_tracker.cpp                                                   */
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

#include "marker_tracker.h"
#include "core/os/os.h"

void MarkerTracker::_bind_methods() {
	ClassDB::bind_method(D_METHOD("track", "image", "marker", "destination_image"), &MarkerTracker::track, DEFVAL(Ref<Image>()));
}

Vector2i MarkerTracker::track(Ref<Image> image, Ref<Image> marker, Ref<Image> destination_image = nullptr) {
	uint64_t begin = OS::get_singleton()->get_ticks_usec();

	Vector2i best_match = Vector2(-1, -1);
	ERR_FAIL_NULL_V_MSG(image, best_match, "Cannot track null image");
	ERR_FAIL_COND_V_MSG(image->get_format() != Image::FORMAT_L8, best_match, "Cannot track image in format other than FORMAT_L8");

	Vector<Vector2i> candidates = MarkerTracker::candidates(image, 4, 12);

	uint32_t marker_signature[A_COUNT * R_COUNT];
	polar(marker, Vector2i(0, 0), marker_signature);

	uint32_t minDifference = 1500;
	uint32_t signature[A_COUNT * R_COUNT];
	for (int i = 0; i < candidates.size(); i++) {
		Vector2i bottom_left = candidates[i] + Vector2i(-HALF, -HALF);
		polar(image, bottom_left, signature);
		uint32_t difference = MarkerTracker::difference(signature, marker_signature, A_COUNT * R_COUNT);
		if (difference < minDifference) {
			minDifference = difference;
			best_match = candidates[i];
		}
	}

	if (destination_image != nullptr) {
		update_destination_image(destination_image, Vector2i(image->get_width(), image->get_height()), candidates, best_match);
	}

	uint64_t end = OS::get_singleton()->get_ticks_usec();
	print_line(vformat("Tracking took %d microseconds, %d candidates, best match: %d, %d", end - begin, candidates.size(), best_match.x, best_match.y));
	return best_match;
}

uint32_t MarkerTracker::average_radius(Ref<Image> image, Vector2i bottom_left) {
	uint32_t sum = 0;
	int count = 0;
	const uint8_t *data = image->get_data().ptr();
	for (int y = 0; y < SIZE; y++) {
		for (int x = 0; x < SIZE; x++) {
			if (data[(bottom_left.y + y) * image->get_width() + bottom_left.x + x] < WHITE) {
				sum += RADIUSES[y * SIZE + x];
				count++;
			}
		}
	}
	return count > 0 ? sum / count : 0;
}

void MarkerTracker::calculate_angles() {
	double center = (15.0 + 16.0) / 2.0;
	for (int y = 0; y < SIZE; y++) {
		for (int x = 0; x < SIZE; x++) {
			double dx = x - center;
			double dy = y - center;
			int angle = (int)Math::round((A_COUNT - 1) * (Math_PI + Math::atan2(dy, dx)) / Math_TAU);
			ANGLES[y * SIZE + x] = angle;
		}
	}
}

void MarkerTracker::calculate_radiuses() {
	int center = ((15 + 16) << FRACTION_BITS) / 2;
	for (int y = 0; y < 32; y++) {
		for (int x = 0; x < 32; x++) {
			int dx = (x << FRACTION_BITS) - center;
			int dy = (y << FRACTION_BITS) - center;
			uint32_t radius = (int)Math::round(Math::sqrt((double)(dx * dx + dy * dy)));
			RADIUSES[y * 32 + x] = radius;
		}
	}
}

Vector<Vector2i> MarkerTracker::candidates(Ref<Image> image, uint8_t min_weight, uint8_t max_weight) {
	int width = image->get_width();
	int height = image->get_height();

	uint32_t *horizontal_weights = new uint32_t[width * height];
	uint32_t *vertical_weights = new uint32_t[width * height];

	MarkerTracker::create_horizontal_weights(image, horizontal_weights);
	MarkerTracker::create_vertical_weights(image, vertical_weights);

	Vector<Vector2i> candidates;

	// TODO one horizontal and one vertical line is missing
	for (int y = HALF; y < height - HALF; y++) {
		for (int x = HALF; x < width - HALF; x++) {
			int left = horizontal_weights[y * width + x] - horizontal_weights[y * width + x - HALF];
			int right = horizontal_weights[y * width + x + HALF] - horizontal_weights[y * width + x];
			int top = vertical_weights[y * width + x] - vertical_weights[(y - HALF) * width + x];
			int bottom = vertical_weights[(y + HALF) * width + x] - vertical_weights[y * width + x];
			if (left >= min_weight && left <= max_weight && right >= min_weight && right <= max_weight && top >= min_weight && top <= max_weight && bottom >= min_weight && bottom <= max_weight) {
				candidates.append(Vector2i(x, y));
			}
		}
	}

	delete[] horizontal_weights;
	delete[] vertical_weights;

	return candidates;
}

void MarkerTracker::create_horizontal_weights(Ref<Image> image, uint32_t dst[]) {
	const uint8_t *data = image->get_data().ptr();
	for (int y = 0; y < image->get_height(); y++) {
		dst[y * image->get_width()] = data[y * image->get_width()] < WHITE ? 0 : 1;
		for (int x = 1; x < image->get_width(); x++) {
			dst[y * image->get_width() + x] = dst[y * image->get_width() + x - 1] + (data[y * image->get_width() + x] < WHITE ? 0 : 1);
		}
	}
}

void MarkerTracker::create_vertical_weights(Ref<Image> image, uint32_t dst[]) {
	const uint8_t *data = image->get_data().ptr();
	for (int x = 0; x < image->get_width(); x++) {
		dst[x] = data[x] < WHITE ? 0 : 1;
		for (int y = 1; y < image->get_height(); y++) {
			dst[y * image->get_width() + x] = dst[(y - 1) * image->get_width() + x] + (data[y * image->get_width() + x] < WHITE ? 0 : 1);
		}
	}
}

uint32_t MarkerTracker::difference(uint32_t a[], uint32_t b[], int length) {
	int result = 0;
	for (int i = 0; i < length; i++) {
		uint32_t d = a[i] - b[i];
		result += d * d;
	}
	return result;
}

void MarkerTracker::polar(Ref<Image> image, Vector2i bottom_left, uint32_t dst[]) {
	const uint8_t *data = image->get_data().ptr();
	memset(dst, 0, 12 * 24 * sizeof(uint32_t));
	uint32_t average_radius = MarkerTracker::average_radius(image, bottom_left);
	if (average_radius == 0) {
		return;
	}
	for (int y = 0; y < SIZE; y++) {
		for (int x = 0; x < SIZE; x++) {
			int radius = NORMALIZED_RADIUS * RADIUSES[y * SIZE + x] / average_radius;
			// TODO what's the max value?
			if (radius < R_COUNT && data[(bottom_left.y + y) * image->get_width() + bottom_left.x + x] < WHITE) {
				dst[radius * A_COUNT + ANGLES[y * SIZE + x]] += 1;
			}
		}
	}
}

void MarkerTracker::update_destination_image(Ref<Image> destination_image, Vector2i size, Vector<Vector2i> candidates, Vector2i best_match) {
	Vector<uint8_t> data;
	data.resize(size.width * size.height);
	uint8_t *dst = data.ptrw();
	memset(dst, 0, data.size());
	for (int i = 0; i < candidates.size(); i++) {
		dst[candidates[i].y * size.width + candidates[i].x] = 0xC0;
	}
	destination_image->create(size.width, size.height, false, Image::FORMAT_L8, data);
	if (best_match.x != -1) {
		draw_frame(destination_image, best_match);
	}
}

void MarkerTracker::draw_frame(Ref<Image> image, Vector2i center) {
	Color white = Color::named("WHITE");
	for (int i = -HALF; i < HALF; i++) {
		image->set_pixel(center.x + i, center.y - HALF, white);
		image->set_pixel(center.x + i, center.y + HALF - 1, white);
		image->set_pixel(center.x - HALF, center.y + i, white);
		image->set_pixel(center.x + HALF - 1, center.y + i, white);
	}
}

MarkerTracker::MarkerTracker() {
	calculate_angles();
	calculate_radiuses();
}
