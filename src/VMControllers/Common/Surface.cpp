#include <Common.h>
#include "Surface.h"

namespace CollabVM {

	Surface::~Surface() {
		// Destroy the Cairo surface
		if(surface)
			cairo_surface_destroy(surface);
	}

	void Surface::Setup(uint16 width, uint16 height, SurfaceFormat format) {
		auto cairo_format = (cairo_format_t)0;

		// select Cairo format from SurfaceFormat enum
		switch(format) {

		case SurfaceFormat::BPP16:
			cairo_format = CAIRO_FORMAT_RGB16_565;
			break;
		case SurfaceFormat::BPP24:
			cairo_format = CAIRO_FORMAT_RGB24;
			break;
		case SurfaceFormat::BPP32:
			cairo_format = CAIRO_FORMAT_ARGB32;
			break;

		default:
			break;

		}

		auto stride = cairo_format_stride_for_width(cairo_format, width);
		buffer.resize(width*height*stride);
		surface = cairo_image_surface_create_for_data(buffer.data(), cairo_format, width, height, stride);

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