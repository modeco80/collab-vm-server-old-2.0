#include "Server.h"
#include "User.h"

namespace CollabVM {
	
	Server::~Server() {
		IPDataCleanupTimer.cancel();
	}

	void Server::Start(tcp::endpoint& ep) {
		logger.verbose("Starting processing thread before WebSockets");
		WorkThread = std::thread(&Server::ProcessActions, this);
		StartIPDataTimer();

		BaseServer::Start(ep);
		
		// Start the WebSocket server loop and wait for the work thread to end
		WorkThread.join();
	}

	void Server::Stop() {
		StopWorking = true;
		BaseServer::Stop();

		// clean up everything in the server we can,
		// even if we don't have to clean it up
		std::lock_guard<std::mutex> ulock(UsersLock);
		std::lock_guard<std::mutex> ilock(IPDataLock);

		for(auto& ip : ipv4data)
			if(ip.second)
				delete ip.second;

		for(auto& ip : ipv6data)
			if(ip.second)
				delete ip.second;

		for(auto& user : users)
			if(user.second)
				delete user.second;
	}

#if 0
	bool Server::OnWebsocketValidate(BaseServer::handle_type userHdl) {
		// TODO
	}
#endif

	void Server::OnWebsocketOpen(BaseServer::handle_type handle) {
		auto& session = GetSessionFromHandle(handle);

		logger.info("OnWebsocketOpen()");
	}

	void Server::OnWebsocketMessage(BaseServer::handle_type handle, BaseServer::message_type message) {
		auto& session = GetSessionFromHandle(handle);

		logger.info("message: ", beast::buffers_to_string(message.message.data()));
	}

	void Server::OnWebsocketClose(BaseServer::handle_type handle) {
		auto& session = GetSessionFromHandle(handle);

		logger.info("OnWebsocketClose()");
	}

	IPData* Server::FindIPData(net::ip::address& address) {
		std::lock_guard<std::mutex> lock(IPDataLock);

		if(address.is_v4()) {
			auto v4addr = address.to_v4();
			auto it = ipv4data.find(v4addr.to_uint());
			if (it == ipv4data.end())
				return nullptr;
			return it->second;
		} else if(address.is_v6()) {
			auto v6addr = address.to_v6();
			if(v6addr.is_v4_mapped()) {
				auto v4addr = address.to_v6().to_v4();
				auto it = ipv4data.find(v4addr.to_uint());
				if (it == ipv4data.end())
					return nullptr;
				return it->second;
			} else {
				auto it = ipv6data.find(v6addr.to_bytes());
				if (it == ipv6data.end())
					return nullptr;
				return it->second;
			}
		}
		return nullptr;
	}

	void Server::CreateIPData(net::ip::address& address) {
		std::lock_guard<std::mutex> lock(IPDataLock);

		if(address.is_v4()) {
			auto v4addr = address.to_v4();
			uint32 ip = v4addr.to_uint();
			ipv4data[ip] = new IPData(address);
		} else if(address.is_v6()) {
			auto v6addr = address.to_v6();
			if(v6addr.is_v4_mapped()) {
				auto v4addr = address.to_v6().to_v4();
				uint32 ip = v4addr.to_uint();
				ipv4data[ip] = new IPData(address);
			} else {
				auto ip = v6addr.to_bytes();
				ipv6data[ip] = new IPData(address);
			}
		}
	}

	void Server::CleanupIPData() {
		std::lock_guard<std::mutex> lock(IPDataLock);

		auto ipv4it = ipv4data.begin();
		while(ipv4it != ipv4data.end()) {
			IPData* ipData = ipv4it->second;

			if(ipData) {
				if(ipData->SafeToDelete()) {
					logger.verbose("Erasing IPData for ", ipData->str() , " @ ", &ipData);

					auto addr = ipData->address;
					delete ipData;
					ipv4data.erase(ipv4data.find(addr.to_v4().to_uint()));
					ipv4it = ipv4data.begin();
					continue;
				}
			}
			ipv4it++;
		}

		auto ipv6it = ipv6data.begin();
		while(ipv6it != ipv6data.end()) {
			IPData* ipData = ipv6it->second;

			if(ipData) {
				if(ipData->SafeToDelete()) {
					logger.verbose("Erasing IPData for ", ipData->str() , " @ ", &ipData);

					auto addr = ipData->address;
					delete ipData;
					ipv6data.erase(ipv6data.find(addr.to_v6().to_bytes()));
					ipv6it = ipv6data.begin();
					continue;
				}
			}
			ipv6it++;
		}

		// Call StartIPDataTimer() again so we keep being called
		StartIPDataTimer();
	}

	void Server::ProcessActions() {
		logger.verbose("Work thread started");

		while(!StopWorking) {
			std::unique_lock<std::mutex> lock(WorkLock);

			if(StopWorking == true)
				break;

			while(work.empty())
				WorkReady.wait(lock);

			IWork* action = work.front();
			work.pop_front();

			switch(action->type) {
				case WorkType::AddConnection: {
					std::lock_guard<std::mutex> lock(UsersLock);
					ConnectionAddWork* add = (ConnectionAddWork*)action;
					//auto address = add->conPtr->get_raw_socket().remote_endpoint().address();

					//if(f)

					//IPData* data = FindIPData(address);
					
						
					//users[add->conPtr] = new User(add->conPtr, data);
					//logger.info("User Connected (IP: ", data->str(), ")");
				} break;
					
				case WorkType::RemoveConnection: {
#if 0 
					std::lock_guard<std::mutex> lock(UsersLock);
					ConnectionRemoveWork* add = (ConnectionRemoveWork*)action;

					auto it = users.find(&add->stream);
					
					IPData* data = FindIPData(it->second->ipData->address);

					// decrement connection count in IPData
					data->connection_count--;

					logger.info("User Disconnect (IP: ", it->second->ipData->str(), ")");

					if(it->second)
						delete it->second;

					users.erase(it);
#endif
				} break;

				case WorkType::Message: {
					// TODO
				} break;

				default:
					logger.verbose("Work type ", (int)action->type, " not implemented");
					break;
			}

			// delete memory to avoid leaks
			delete action;
		}
	}


}