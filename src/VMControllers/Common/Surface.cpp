#include <Common.h>
#include "Surface.h"

namespace CollabVM {

	Surface::~Surface() {
		// Destroy the Cairo surface
		if(surface)
			cairo_surface_destroy(surface);
	}

	void Surface::Setup(uint16 width, uint16 height, SurfaceFormat format) {
		// TODO: DO NOT IGNORE SurfaceFormat!!!!
		// also actually make surface :P

		auto stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);
		buffer.resize(width*height*stride);
		surface = cairo_image_surface_create_for_data(buffer.data(), CAIRO_FORMAT_ARGB32, width, height, stride);

		if(cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
			// uh-oh...
			buffer.clear();
		}
	}


	Surface Surface::GetSubSurf(uint16 x, uint16 y, uint16 width, uint16 height) {
		// stub TODO
		return {};
	}

	void Surface::Draw(Surface& Other, uint16 x, uint16 y, uint16 width, uint16 height) {
		// stub TODO
	}

}