//
//  homekit_controller.h
//  home-controller
//
//  Created by Piotr Brzeski on 2024-06-15.
//  Copyright © 2024 Brzeski.net. All rights reserved.
//

#pragma once

#include "mqtt_queue.h"
#include <string>
#include <vector>
#include <cstdint>

namespace home {

class homekit : public mqtt_queue {
public:
	using mqtt_queue::mqtt_queue;
	
	void start(std::vector<std::string> names, mqtt::callback_t callback);
	void send_update(std::string const& name, std::uint8_t brightness);
};

}
