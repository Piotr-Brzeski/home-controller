//
//  homekit_controller.cpp
//  home-controller
//
//  Created by Piotr Brzeski on 2024-06-15.
//  Copyright © 2024 Brzeski.net. All rights reserved.
//

#include "homekit_controller.h"

using namespace home;

void homekit::start(std::vector<std::string> names, mqtt::callback_t callback) {
	for(auto& queue_name : names) {
		queue_name += "/get";
	}
//	subscribe(names, callback);
}

void homekit::send_update(std::string const& name, std::uint8_t brightness) {
	publish(name, std::to_string(brightness));
}
