/*************************************************************************/
/*  hsv.cpp                                                              */
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

#include "hsv.h"

HSV HSV::from_argb32(uint32_t argb) {
	return from_rgb(argb >> 16 & 0xFF, argb >> 8 & 0xFF, argb & 0xFF);
}

HSV HSV::from_rgb(uint8_t r, uint8_t g, uint8_t b) {
	uint8_t v = max(r, g, b);

	int tmp = min(r, g, b);
	int h = 0;
	int s = 0;
	if (tmp != v) {
		if (r == v) {
			h = (g - b) * 43 / (v - tmp);
		} else {
			if (g == v) {
				h = 85 + (b - r) * 43 / (v - tmp);
			} else {
				h = 171 + (r - g) * 43 / (v - tmp);
			}
		}

		if (h < 0) {
			h += 255;
		}
		if (v != 0) {
			s = 255 * (v - tmp) / v;
		}
	}
	return HSV(h, s, v);
}

HSV::HSV(uint8_t h, uint8_t s, uint8_t v) {
	this->h = h;
	this->s = s;
	this->v = v;
}

bool HSV::is_similar(HSV hsv, HSV threshold) {
	return is_similar(this->h, hsv.h, threshold.h) && is_similar(this->v, hsv.v, threshold.v) && is_similar(this->s, hsv.s, threshold.s);
}
