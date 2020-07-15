#include <Common.h>
#include <Server.h>
#include <User.h>
#include <list>

namespace CollabVM {

	struct UserList {

		template<class Function>
		inline void AddUser(std::shared_ptr<User> user, Function fun) {
			std::lock_guard<std::mutex> l(lock);
			bool duplicate = false;

			ForEach([&](auto it) {
				if (user == *it) {
					// duplicate user will be added if this goes through
					duplicate = true;
					return false;
				}

				return true;
			});

			// return if this will cause a duplicate user
			if(duplicate)
				return;

			// insert user
			users.push_back(user);
			std::sort(users.begin(), users.end());
			connected_users++;

			fun(user);
		}

		
		template<class Function>
		inline void RemoveUser(std::shared_ptr<User> user, Function fun) {
			std::lock_guard<std::mutex> l(lock);

			ForEach([&](auto it) {

				if (user == *it) {
					// remove if found
					(*it).reset();
					users.erase(it);
					connected_users--;

					fun();
					return false;
				}

				// not found yet
				return true;
			});
		}
	
		// signature: bool(iterator_type)
		// return false if you want to stop iterating
		template<class Function>
		inline void ForEach(Function callback) {
			if(users.size() == 0)
				return;

			// iterate through list stopping if user callback returns false
			for(auto it = users.begin(); it != users.end(); ++it)
				if(!callback(it))
					break;
		}		
		
		// version that acquires users lock
		// signature: bool(iterator_type)
		// return false if you want to stop iterating
		template<class Function>
		inline void ForEachLock(Function callback) {
			std::lock_guard<std::mutex> l(lock);
			ForEach(callback);
		}

	private:

		std::mutex lock;

		std::list<std::shared_ptr<User>> users;

		uint64 connected_users;
	};

}