/**************************************************************************/
/*  hsv.h                                                                 */
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

#ifndef HSV_H
#define HSV_H

#include "core/typedefs.h"
#include <stdint.h>

class HSV {
private:
	uint8_t h;
	uint8_t s;
	uint8_t v;

	static inline uint8_t min(uint8_t p_r, uint8_t p_g, uint8_t p_b) {
		return MIN(MIN(p_r, p_g), p_b);
	}
	static inline uint8_t max(uint8_t p_r, uint8_t p_g, uint8_t p_b) {
		return MAX(MAX(p_r, p_g), p_b);
	}
	inline bool is_similar(uint8_t p_a, uint8_t p_b, uint8_t p_threshold) {
		return p_a > p_b ? (p_a - p_b <= p_threshold) : (p_b - p_a <= p_threshold);
	}

public:
	static HSV from_argb32(uint32_t p_argb32);
	static HSV from_rgb(uint8_t p_h, uint8_t p_s, uint8_t p_v);

	HSV(uint8_t p_h, uint8_t p_s, uint8_t p_v);

	bool is_similar(HSV p_hsv, HSV p_threshold);
};

#endif // HSV_H
