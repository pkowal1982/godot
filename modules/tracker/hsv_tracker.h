/**************************************************************************/
/*  hsv_tracker.h                                                         */
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

#ifndef HSV_TRACKER_H
#define HSV_TRACKER_H

#include "core/io/image.h"
#include "core/math/vector2.h"
#include "core/object/ref_counted.h"
#include "core/string/print_string.h"
#include "core/variant/variant.h"
#include "hsv.h"
#include <stdint.h>

class HsvTracker : public RefCounted {
	GDCLASS(HsvTracker, RefCounted);

protected:
	static void _bind_methods();

public:
	Vector3 track(Ref<Image> p_image, Color p_example, Vector3 p_threshold, Ref<Image> p_destination_image);

private:
	bool wrong_format(Ref<Image> p_image);
	void update_destination_image(Ref<Image> p_destination_image, Vector2i p_size, Vector<uint8_t> p_data, Vector2i p_center);
	void draw_cross(Ref<Image> p_image, Vector2i p_center);
};

#endif // HSV_TRACKER_H
