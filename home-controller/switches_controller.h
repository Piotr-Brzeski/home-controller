//
//  switches_controller.h
//  home-controller
//
//  Created by Piotr Brzeski on 2024-06-15.
//  Copyright © 2024 Brzeski.net. All rights reserved.
//

#pragma once

#include "mqtt_queue.h"
#include <functional>
#include <string>
#include <map>

namespace home {

class switches_controller : public mqtt_reader_queue {
public:
	using mqtt_reader_queue::mqtt_reader_queue;
	using callback_t = std::function<void()>;
	
	void add(std::string const& name, callback_t toggle, callback_t increase, callback_t decrease);
	void start();
	
private:
	struct callbacks {
		callback_t toggle;
		callback_t increase;
		callback_t decrease;
	};
	std::map<std::string, callbacks> m_callbacks;
};

}
