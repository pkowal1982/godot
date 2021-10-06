/*************************************************************************/
/*  marker_tracker.h                                                     */
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

#ifndef MARKER_TRACKER_H
#define MARKER_TRACKER_H

#include "core/error/error_macros.h"
#include "core/io/image.h"
#include "core/math/math_funcs.h"
#include "core/math/vector2.h"
#include "core/object/ref_counted.h"
#include "core/string/print_string.h"
#include "core/variant/variant.h"
#include <stdint.h>

class MarkerTracker : public RefCounted {
	GDCLASS(MarkerTracker, RefCounted);

protected:
	static void _bind_methods();

public:
	Vector2i track(Ref<Image> image, Ref<Image> marker, Ref<Image> destination_image);
	MarkerTracker();

private:
	static const uint8_t WHITE = 0x80;
	static const int SIZE = 32;
	static const int HALF = 16;
	static const int R_COUNT = 24;
	static const int A_COUNT = 12;
	static const int FRACTION_BITS = 10;
	static const uint32_t NORMALIZED_RADIUS = 16;

	void calculate_radiuses();
	void calculate_angles();
	void create_horizontal_weights(Ref<Image> image, uint32_t dst[]);
	void create_vertical_weights(Ref<Image> image, uint32_t dst[]);
	uint32_t average_radius(Ref<Image> image, Vector2i bottomLeft);
	void polar(Ref<Image> image, Vector2i bottomLeft, uint32_t dst[]);
	Vector<Vector2i> candidates(Ref<Image> image, uint8_t min_weight, uint8_t max_weight);
	uint32_t difference(uint32_t a[], uint32_t b[], int length);
	uint32_t RADIUSES[SIZE * SIZE];
	uint32_t ANGLES[SIZE * SIZE];
	void update_destination_image(Ref<Image> destination_image, Vector2i size, Vector<Vector2i> candidates, Vector2i best_match);
	void draw_frame(Ref<Image> image, Vector2i center);
};

#endif
