#include "Server.h"
#include "User.h"

// Protocol functions
#include "Protocol.h"

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

		for (auto& user : users)
			if (user.second)
				user.second.reset();
	}


	bool Server::OnVerify(BaseServer::handle_type handle) {

		auto subprotocols = handle->GetSubprotocols();

		for(auto subprotocol : subprotocols) {
			if(subprotocol == "cvm2") {
				auto addr = handle->GetAddress();

				if (FindIPData(addr) == nullptr)
					CreateIPData(addr);

				IPData* data = FindIPData(addr);
				data->connection_count++;

				return true;
			}
		}

		return false;
	}

	void Server::OnOpen(BaseServer::handle_type handle) {
		AddWork(std::make_shared<ConnectionAddWork>(handle));
	}

	void Server::OnMessage(BaseServer::handle_type handle, BaseServer::message_type message) {
		// Return immediately if the message isn't a binary message.
		if(!message->binary)
			return;

		AddWork(std::make_shared<WSMessageWork>(handle, message));
	}

	void Server::OnClose(BaseServer::handle_type handle) {
		AddWork(std::make_shared<ConnectionRemoveWork>(handle));
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

			std::shared_ptr<IWork> action = work.front();
			work.pop_front();

			// Process work based on what work type it is.
			switch(action->type) {
				case WorkType::AddConnection: {
					std::lock_guard<std::mutex> lock(UsersLock);
					ConnectionAddWork* add = (ConnectionAddWork*)action.get();
					auto address = add->handle->GetAddress();

					IPData* data = FindIPData(address);
					
					// create user structure
					users[add->handle] = std::make_shared<User>(add->handle, data);
					logger.info("User Connected (IP: ", data->str(), ")");
				} break;
					
				case WorkType::RemoveConnection: {
					std::lock_guard<std::mutex> lock(UsersLock);
					ConnectionRemoveWork* remove = (ConnectionRemoveWork*)action.get();
					auto it = users.find(remove->handle);

					if(it == users.end())
						break; // stop but still free the action memory

					IPData* data = FindIPData(it->second->ipData->address);

					// decrement connection count in IPData
					if(data)
						data->connection_count--;

					logger.info("User Disconnect (IP: ", it->second->ipData->str(), ")");

					// TODO (when vms work): disconnect user from vms so that reference count drops down to just work thread
					// so that it becomes possible to delete when reset is called and/or we become the thread that deletes it

					users.erase(it);
					remove->handle.reset();
				} break;

				case WorkType::Message: {
					WSMessageWork* msg = (WSMessageWork*)action.get();
					auto user = users.find(msg->handle)->second;

					// try to deserialize a message..
					auto message = Protocol::DeserializeMessage(msg->message);
					

					msg->message.reset();
				} break;

				default:
					logger.verbose("Work type ", (int)action->type, " not implemented");
					break;
			}

			// delete memory to avoid leaks
			// (we end up being the only one who owns the memory, so we get to delete it)
			action.reset();
		}
	}


}