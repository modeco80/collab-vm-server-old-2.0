#include <Common.h>
#include "VNCClient.h"
#include <cairo_jpg.h>

#ifdef _MSC_VER
#define strdup _strdup
#endif

namespace CollabVM {

	// Key for user data to store the pointer to the active VNC client.
	// This is used so that all of the libvncserver code can access the VNCClient object it's tied to.
	// (I'm waiting for people to call me cheeky :P)
	const static uint32 VNCCLIENT_KEY = 0x796C694C;

	// cairo write data structure
	struct cairo_write_data {
		int datasize = 0;
		std::vector<byte> buffer;
	};

	// Called to resize the surface
	rfbBool ResizeSurface(rfbClient* client) {
		// get the client instance controlling this client
		// (confusing, I know)
		VNCClient* thatClient = (VNCClient*)rfbClientGetClientData(client, (void*)&VNCCLIENT_KEY);

		if(!thatClient)
			return FALSE; // not controlled by us probably, not sure how that happened, but no thanks

		int w = client->width;
		int h = client->height;
		int bpp = client->format.bitsPerPixel;

		SurfaceFormat fmt = (SurfaceFormat)0;

		switch(bpp) {
		case 16:
			fmt = SurfaceFormat::BPP16;
			break;

		case 32:
			fmt = SurfaceFormat::BPP32;
			break;

		default:
			thatClient->logger.error("Unhandled bpp value ", bpp, "!");
			return FALSE;
			break;
		}

		// Setup the desktop surface to be the right w/h
		thatClient->desktop.Setup(w, h, fmt);
		
		client->frameBuffer = thatClient->desktop.Buffer().data();

		SetFormatAndEncodings(client);

		return TRUE;
	}

	cairo_status_t cairo_write_func(void* closure, const unsigned char* data, unsigned int length) {
		cairo_write_data* wd = (cairo_write_data*)closure;

		int next = wd->datasize + length;

		if(next > wd->buffer.size()) {
			do {
				wd->buffer.resize(wd->buffer.size() * 2);
			} while((next > wd->buffer.size()));
		}

		memcpy(&wd->buffer[wd->datasize], data, length);
		wd->datasize += length;

		return CAIRO_STATUS_SUCCESS;
	}

	void UpdateSurface(rfbClient* client, int x, int y, int w, int h) {
		VNCClient* thatClient = (VNCClient*)rfbClientGetClientData(client, (void*)&VNCCLIENT_KEY);

		if(!thatClient)
			return;

		// framebuffer will already have the relevant pixels in it.
		// the passed x,y,w,h is a rectangle defining the updated region.
		// so now, we create a region structure
		std::shared_ptr<VNCRegion> region = std::make_shared<VNCRegion>();

		cairo_write_data writeData;

		switch(thatClient->options.output_region_type) {
			
		case VNCClientOptions::OutputRegionType::JpegRegion: {
			auto cairos = thatClient->desktop.Raw();
			cairo_image_surface_write_to_jpeg_stream(cairos, cairo_write_func, &writeData, thatClient->options.jpeg_compression_quality);
		} break;

		case VNCClientOptions::OutputRegionType::PngRegion: {
			auto cairos = thatClient->desktop.Raw();
			cairo_surface_write_to_png_stream(cairos, cairo_write_func, &writeData);
		} break;

		default:
			return;
			break;
		}

		// now we have the encoded region.
		// so we set the region
		region->data = writeData.buffer;
		region->x = x;
		region->y = y;
		region->width = w;
		region->height = h;

		if(thatClient->OnScreenUpdate)
			thatClient->OnScreenUpdate(region);
	}

	VNCClient::~VNCClient() {
		if(client) {
			// free our strdup()'d hostname string to avoid memory leaking
			free(client->serverHost);
			rfbClientCleanup(client);
		}
	}

	void VNCClient::Connect() {
		// Connect() simply starts the thread.
		vnc_thread = std::thread(&VNCClient::ClientThread, shared_from_this());
		vnc_thread.detach();
	}

	
	void VNCClient::SetOptions(VNCClientOptions& new_options) {
		std::lock_guard<std::mutex> l(state_lock);
		options = new_options;
	}

	void VNCClient::ClientThread() {

		// Set state, locking the state mutex
		auto SetState = [&](State newState) {
			std::lock_guard<std::mutex> l(state_lock);
			current_state = newState;

			if(OnStateChange)
				OnStateChange();
		};

		SetState(State::ConnectingToServer);

		// get a 32bpp client & set the client data
		client = rfbGetClient(8, 3, 4);
		rfbClientSetClientData(client, (void*)&VNCCLIENT_KEY, this);

		client->serverHost = strdup(options.hostname.data());
		client->serverPort = options.port;

		if(options.register_qemu_audio) {
			logger.info("Registering QEMU Audio extension");
		}

		if(rfbInitClient(client, 0, NULL)) {
			// Initalization succedded

			client->canHandleNewFBSize = TRUE;
			client->MallocFrameBuffer = ResizeSurface;
			client->GotFrameBufferUpdate = UpdateSurface;
			
			// mark client as connected
			SetState(State::Connected);

			// go into a loop, waiting and handling server messages.
			while(true) {
				int i = WaitForMessage(client, 500);
				if(i > 0)
					break;

				if(!HandleRFBServerMessage(client))
					break;
			}

			// if we broke out of the above loop,
			// it's probably because we disconnected.
			SetState(State::Disconnected);
			
			
		} else {
			// client initalization failed
			SetState(State::Disconnected);
			return;
		}
	}
}