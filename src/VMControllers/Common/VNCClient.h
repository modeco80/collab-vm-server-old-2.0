#include <Common.h>
#include <Logger.h>
#include <rfb/rfbclient.h>
#include "Surface.h"

namespace CollabVM {
	
	// Default JPEG quality.
	// Tuned for a mix of performance and quality.
	constexpr byte DEFAULT_JPEG_QUALITY = 65;

	// Options that the VNC Client can be configured to use.
	struct VNCClientOptions {

		// Hostname of the VNC server.
		std::string hostname;

		// Port of the VNC server.
		uint16 port;

		// Password.
		// Leave blank if one isn't needed to connect.
		std::string password;

		// Amount of times we should retry the VNC connection before giving up and
		// leaving the state as disconnected.
		uint16 retry_count;

		// Set this to true if you want to register the QEMU audio extension.
		//
		// Please note that registering the extension is *not* the same as requiring the extension; 
		// the client extension is only invoked if requested. 
		//
		// If you encounter a problematic VNC server, this can be turned off; 
		// however, I have doubts that many VNC servers
		// would be problematic or stupid enough to clobber over this extension.
		bool register_qemu_audio;

		// The image format that the VNC Client
		// should output regions as. This is configurable per-VNC client.
		enum class OutputRegionType {
			// Use PNG to encode regions.
			// Higher quality, but it may use more bandwidth.
			PngRegion,

			// Use JPEG to encode regions.
			// Not lossless like PNG, however far smaller.
			JpegRegion
		} output_region_type;

		// JPEG region compression quality.
		// This field is only applicable if output_region_type is JpegRegion, and is ignored otherwise.
		byte jpeg_compression_quality;

	};

	// Region data structure
	struct VNCRegion {
		// Where the region updated.
		int16 x;
		int16 y;

		// The width and height of the region.
		int16 width;
		int16 height;

		// Active region type (what the data buffer will contain.)
		VNCClientOptions::OutputRegionType region_type;

		// Data buffer of the region (copied from the total data buffer), encoded into the proper region type.
		std::vector<byte> data;
	};

	struct VNCCursor {
		// The width and height of the region.
		int16 width;
		int16 height;

		// Cursor surface. It's up to the user of this client
		// to figure out what to do with the surface
		// (I mean, to be fair, cursors could be sent uncompressed.)
		Surface& surface;
	};

	// VNC Client object.
	struct VNCClient : public std::enable_shared_from_this<VNCClient> {

		enum class State : byte {
			Disconnected,
			ConnectingToServer,
			Connected
		};

		~VNCClient();

		// "asynchronously" start connecting to the VNC server.
		// Spawns a new thread.
		void Connect();

		void SetOptions(VNCClientOptions& new_options);
	
		// returns current state
		inline State GetState() {
			// State is modified by the client thread.
			std::lock_guard<std::mutex> l(state_lock);
			return current_state;
		}


		// Function callbacks.
		// These run on the VNC client thread

		std::function<void()> OnConnect;
		std::function<void()> OnClose;
		std::function<void(std::shared_ptr<VNCRegion>)> OnScreenUpdate;

		// cursor is bound to be small (128*128 max is sensible)
		// so we don't handle encoding it ourselves.
		std::function<void(std::shared_ptr<VNCCursor>)> OnCursorUpdate;

	private:

		void ClientThread();
		
		// lock controlling state,
		// this should be renamed as it's client wide
		std::mutex state_lock;

		std::thread vnc_thread;

		State current_state = State::Disconnected;

		// The options that this VNC Client is using.
		VNCClientOptions options;

		// libvncclient client object
		rfbClient* client;
		
		// Cursor surface
		Surface cursor;

		// desktop surface
		Surface desktop;
		
		Logger logger = Logger::GetLogger("VNCClient");
	};

}