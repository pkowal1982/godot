/**************************************************************************/
/*  marker_tracker.cpp                                                    */
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

#include "marker_tracker.h"
#include "core/os/os.h"
#include "segment.h"

void MarkerTracker::_bind_methods() {
	ClassDB::bind_method(D_METHOD("track", "image", "previous_position"), &MarkerTracker::track, DEFVAL(Vector3(-1.0, -1.0, -1.0)));
}

Vector3 MarkerTracker::track(Ref<Image> p_image, Vector3 p_previous_position = Vector3(-1.0, -1.0, -1.0)) {
//	uint64_t begin = OS::get_singleton()->get_ticks_usec();

	ERR_FAIL_NULL_V_MSG(p_image, p_previous_position, "Cannot track null image");

	HashSet<int> excluded;
	HashMap<int, List<Segment *>> areas = find_areas(p_image, excluded);
//	uint64_t find_areas_end = OS::get_singleton()->get_ticks_usec();
	filter_areas(areas, excluded);
//	uint64_t filter_areas_end = OS::get_singleton()->get_ticks_usec();
	Vector3 marker_position = find_marker(areas, excluded);
	if (marker_position.z < 0) {
		marker_position = find_center(p_image, p_previous_position);
	}
//	uint64_t find_marker_end = OS::get_singleton()->get_ticks_usec();

//	uint64_t end = OS::get_singleton()->get_ticks_usec();
//	print_verbose(vformat("Tracking took %d microseconds, %d areas, center at: %d, %d, diameter: %d", end - begin, areas.size(), marker_position.x, marker_position.y, marker_position.z));
//	print_verbose(vformat("Finding areas: %d, filtering areas: %d, finding marker: %d", find_areas_end - begin, filter_areas_end - begin, find_marker_end - begin));
	return marker_position;
}

HashMap<int, List<Segment *>> MarkerTracker::find_areas(Ref<Image> p_image, HashSet<int> &p_excluded) {
	int rows = p_image->get_height();

	List<List<Segment *>> segments = List<List<Segment *>>();
	index = 0;
	if (p_image->get_format() == Image::FORMAT_L8) {
		for (int row = 0; row < rows; row++) {
			segments.push_back(find_segments_grayscale(p_image, row));
		}
	} else {
		for (int row = 0; row < rows; row++) {
			segments.push_back(find_segments(p_image, row));
		}
	}
	List<Segment *> top = segments.front()->get();
	for (List<Segment *>::Element *E = top.front(); E; E = E->next()) {
		E->get()->set_index(index);
		index++;
	}
	for (int row = 1; row < rows; row++) {
		connect_segments(segments.get(row - 1), segments.get(row), p_excluded);
	}
	HashMap<int, List<Segment *>> areas;
	for (List<List<Segment *>>::Element *E = segments.front(); E; E = E->next()) {
		for (List<Segment *>::Element *F = E->get().front(); F; F = F->next()) {
			Segment *s = F->get();
			if (!p_excluded.has(s->index)) {
				if (!areas.has(s->index)) {
					areas.insert(s->index, List<Segment *>());
				}
				areas[s->index].push_back(s);
			}
		}
	}
	return areas;
}

void MarkerTracker::filter_areas(HashMap<int, List<Segment *>> p_areas, HashSet<int> &p_excluded) {
	for (const KeyValue<int, List<Segment *>> &E : p_areas) {
		int size = E.value.size();
		if (size < 8 || size > 64) {
			p_excluded.insert(E.key);
		}
	}
}

List<Segment *> MarkerTracker::find_segments(Ref<Image> p_image, int p_row) {
	List<Segment *> segments = List<Segment *>();
	Segment *segment = NULL;
	for (int column = 0; column < p_image->get_width(); column++) {
		if (p_image->get_pixel(column, p_row).get_v() > THRESHOLD) {
			if (segment) {
				segment->to = column - 1;
				segment = NULL;
			}
		} else {
			if (!segment) {
				segment = memnew(Segment(column, p_image->get_width() - 1, p_row));
				segments.push_back(segment);
			}
		}
	}
	return segments;
}

List<Segment *> MarkerTracker::find_segments_grayscale(Ref<Image> p_image, int p_row) {
	const uint8_t *src = p_image->get_data().ptr();
	src += p_row * p_image->get_width();
	List<Segment *> segments = List<Segment *>();
	Segment *segment = NULL;
	for (int column = 0; column < p_image->get_width(); column++, src++) {
		if (*src > THRESHOLD_GRAYSCALE) {
			if (segment) {
				segment->to = column - 1;
				segment = NULL;
			}
		} else {
			if (!segment) {
				segment = memnew(Segment(column, p_image->get_width() - 1, p_row));
				segments.push_back(segment);
			}
		}
	}
	return segments;
}

void MarkerTracker::connect_segments(List<Segment *> p_top, List<Segment *> p_bottom, HashSet<int> &p_excluded) {
	int t_index = 0;
	int b_index = 0;
	while (t_index < p_top.size() && b_index < p_bottom.size()) {
		Segment *t = p_top.get(t_index);
		Segment *b = p_bottom.get(b_index);
		if (t->is_connected_with(*b)) {
			if (!b->set_index(t->index)) {
				p_excluded.insert(t->index);
				p_excluded.insert(b->index);
			}
		}
		if (t->to < b->to) {
			t_index++;
		} else {
			b_index++;
		}
	}
	for (List<Segment *>::Element *E = p_bottom.front(); E; E = E->next()) {
		Segment *b = E->get();
		if (!b->has_index()) {
			b->set_index(index);
			index++;
		}
	}
}

Rect2 MarkerTracker::find_center_and_bounding_box(List<Segment *> p_segments) {
	Segment front = *p_segments.front()->get();
	int min_x = front.from;
	int max_x = front.to;
	int min_y = front.row;
	int max_y = p_segments.front()->get()->row;
	int x = 0;
	int y = 0;
	int total_length = 0;
	for (List<Segment *>::Element *E = p_segments.front(); E; E = E->next()) {
		Segment segment = *E->get();
		int length = segment.to - segment.from + 1;
		x += (segment.from + segment.to) * length / 2;
		y += segment.row * length;
		total_length += length;
		min_x = MIN(min_x, segment.from);
		max_x = MAX(max_x, segment.to);
	}
	Vector2 position = Vector2(x, y) / total_length;
	Vector2 size = Vector2(max_x - min_x + 1, max_y - min_y + 1);
	return Rect2(position, size);
}

Vector3 MarkerTracker::find_marker(HashMap<int, List<Segment *>> p_areas, HashSet<int> p_excluded) {
	for (int E : p_excluded) {
		p_areas.erase(E);
	}
	List<Rect2> statistics = List<Rect2>();
	for (const KeyValue<int, List<Segment *>> &E : p_areas) {
		statistics.push_back(find_center_and_bounding_box(E.value));
	}
	for (List<Rect2>::Element *S1 = statistics.front(); S1; S1 = S1->next()) {
		Rect2 s1 = S1->get();
		for (List<Rect2>::Element *S2 = statistics.front(); S2; S2 = S2->next()) {
			Rect2 s2 = S2->get();
			if (s1 != s2 && s1.position.distance_to(s2.position) < MAX_DISTANCE) {
				Vector2 position = (s1.position + s2.position) / 2;
				float diameter = MAX(s1.size.length(), s2.size.length()) / 2;
				return Vector3(position.x, position.y, diameter);
			}
		}
	}
	return Vector3(-1, -1, -1);
}

Vector3 MarkerTracker::find_center(Ref<Image> p_image, Vector3 p_previous_position) {
	int from_x = MAX(0, p_previous_position.x - p_previous_position.z);
	int to_x = MIN(p_previous_position.x + p_previous_position.z, p_image->get_width());
	int from_y = MAX(0, p_previous_position.y - p_previous_position.z);
	int to_y = MIN(p_previous_position.y + p_previous_position.z, p_image->get_height());
	int count = 0;
	int total_x = 0;
	int total_y = 0;
	for (int y = from_y; y < to_y; y++) {
		for (int x = from_x; x < to_x; x++) {
			if (p_image->get_pixel(x, y).get_v() <= THRESHOLD) {
				if (Vector2(p_previous_position.x - x, p_previous_position.y - y).length() <= p_previous_position.z) {
					total_x += x;
					total_y += y;
					count++;
				}
			}
		}
	}
	return count ? Vector3(total_x / count, total_y / count, p_previous_position.z) : p_previous_position;
}

MarkerTracker::MarkerTracker() {
}

const float MarkerTracker::THRESHOLD = 0.75f;
const float MarkerTracker::MAX_DISTANCE = 2.0f;
