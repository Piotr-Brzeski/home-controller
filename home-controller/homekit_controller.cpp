//
//  homekit_controller.cpp
//  home-controller
//
//  Created by Piotr Brzeski on 2024-06-15.
//  Copyright © 2024 Brzeski.net. All rights reserved.
//

#include "homekit_controller.h"
#include <cassert>

using namespace home;

void homekit::start(std::vector<std::string> names, callback_t callback) {
	for(auto& queue_name : names) {
		assert(!queue_name.empty());
		queue_name += "/get";
	}
	subscribe(names, [callback](std::string const& queue, std::string const& message) {
		try {
			assert(queue.size() > 4);
			auto name = queue.substr(0, queue.size() - 4);
			auto brightness = std::stoul(message);
			if(brightness > 100) {
				assert(false);
				brightness = 100;
			}
			callback(name, static_cast<std::uint8_t>(brightness));
		}
		catch(...) {
			// TODO: Log error
			assert(false);
		}
	});
}

void homekit::send_update(std::string const& name, std::uint8_t brightness) {
	publish(name, std::to_string(brightness));
}
