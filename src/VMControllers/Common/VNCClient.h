#include <Common.h>
#include <rfb/rfbclient.h>

namespace CollabVM {
	
	// Default JPEG quality
	constexpr byte DEFAULT_JPEG_QUALITY = 75;

	// Options that the VNC Client can be configured to use.
	struct VNCClientOptions {

		// Hostname of the VNC server.
		std::string hostname;

		// Port of the VNC server.
		uint16 port;

		// password (if needed to connect)
		std::string password;

		// Set this to true if you want to register the QEMU audio extension.
		//
		// Please note that registering the extension is *not* the same
		// as requiring the extension; the client extension is only
		// invoked if requested. If you encounter a problematic VNC server,
		// this can be turned off; however, I have doubts that many VNC servers
		// would be problematic.
		bool register_qemu_audio;

		// The image format that the VNC Client
		// should output regions as. This is configurable per-VNC client.
		// TODO: specify any restriction on on-the-fly usage?
		enum class OutputRegionType {
			// Use PNG to encode regions.
			// Higher quality, but it may use more bandwidth.
			PngRegion,

			// Use JPEG to encode regions.
			// Not lossless like PNG, however far smaller.
			JpegRegion
		} output_region_type;

		// JPEG region compression quality.
		// This field is only applicable if output_region_type is JpegRegion.
		byte jpeg_compression_quality;

	};

	// NOTE the below code was programmed while placebo was playing
	// its incomphrensiblity may be helped by that factor

	// VNC Client object.
	struct VNCClient {

	
	private:

		// The options that this VNC Client is using.
		VNCClientOptions options;

		// libvncclient client object
		rfbClient* client;
	};

}