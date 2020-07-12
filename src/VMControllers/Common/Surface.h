#include <Common.h>
#include <cairo/cairo.h>

namespace CollabVM {

	// Surface format enumeration
	enum class SurfaceFormat : byte {
		BPP16,
		BPP24,
		BPP32
	};

	// Surface
	struct Surface {

		inline Surface() {
			// nothing
		}

		~Surface();

		inline Surface(uint16 width, uint16 height, SurfaceFormat format) 
			: width(width), height(height), format(format) {
			Setup(width, height, format);
		}

		
		void Setup(uint16 width, uint16 height, SurfaceFormat format);
		
		// Get a sub-surface of this one.
		Surface GetSubSurf(uint16 x, uint16 y, uint16 width, uint16 height);

		// draw the contents of another surface onto this one.
		// Essentially a blit without any form of ROP
		void Draw(Surface& Other, uint16 x, uint16 y, uint16 width, uint16 height);
		
		// retun the raw cairo surface this wraps
		inline cairo_surface_t* Raw() {
			if(surface)
				return surface;
		}

		// return the raw buffer.
		inline std::vector<byte>& Buffer() {
			return buffer;
		}

	private:


		uint16 width;

		uint16 height;

		SurfaceFormat format;

		// the buffer that this surface compreises of
		std::vector<byte> buffer;

		cairo_surface_t* surface;
	};

}