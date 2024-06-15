//
//  mqtt_system.h
//  home-controller
//
//  Created by Piotr Brzeski on 2023-12-17.
//  Copyright © 2023-2024 Brzeski.net. All rights reserved.
//

#pragma once

#include "system_base.h"
#include "mqtt_queue.h"
#include <set>

namespace home {

class mqtt_system: public system_base, public mqtt_queue {
public:
	mqtt_system(mqtt_queue::configuration configuration);
	
	device_type get_device_type(std::string const& name) const override;
	void start(std::vector<std::string> const& names) override;
	void ping(std::string const& name) override;
	void set(std::string const& name, std::uint8_t brightness) override;
	
private:
	void enumerate_devices();
	void call(std::string const& channel, std::string message);
	
	std::set<std::string> m_bulb_names;
};

}
