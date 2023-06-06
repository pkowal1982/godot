/**************************************************************************/
/*  marker_tracker.h                                                      */
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

#ifndef MARKER_TRACKER_H
#define MARKER_TRACKER_H

#include "core/error/error_macros.h"
#include "core/io/image.h"
#include "core/math/math_funcs.h"
#include "core/math/vector2.h"
#include "core/object/ref_counted.h"
#include "core/string/print_string.h"
#include "core/variant/variant.h"
#include "segment.h"
#include <stdint.h>

class MarkerTracker : public RefCounted {
	GDCLASS(MarkerTracker, RefCounted);

protected:
	static void _bind_methods();

public:
	Vector3 track(Ref<Image> p_image, Vector3 p_previous_position);
	MarkerTracker();

private:
	static const float THRESHOLD;
	static const int THRESHOLD_GRAYSCALE = 0xC0;
	static const float MAX_DISTANCE;

	int index = -1;

	HashMap<int, List<Segment *>> find_areas(Ref<Image> p_image, HashSet<int> &p_excluded);
	void filter_areas(HashMap<int, List<Segment *>> p_areas, HashSet<int> &p_excluded);
	List<Segment *> find_segments(Ref<Image> p_image, int p_row);
	List<Segment *> find_segments_grayscale(Ref<Image> p_image, int p_row);
	void connect_segments(List<Segment *> p_top, List<Segment *> p_bottom, HashSet<int> &p_excluded);
	Vector3 find_marker(HashMap<int, List<Segment *>> p_areas, HashSet<int> p_excluded);
	Vector3 find_center(Ref<Image> p_image, Vector3 p_previous_position);
	Rect2 find_center_and_bounding_box(List<Segment *> p_segments);
};

#endif // MARKER_TRACKER_H
