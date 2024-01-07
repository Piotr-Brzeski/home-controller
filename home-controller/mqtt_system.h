//
//  mqtt_system.h
//  home-controller
//
//  Created by Piotr Brzeski on 2023-12-17.
//  Copyright © 2023 Brzeski.net. All rights reserved.
//

#pragma once

#include "mqtt.h"
#include "mqtt_bulb.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace home {

class mqtt_system {
public:
	struct configuration {
		std::string address;
		std::string queue_name;
	};
	
	mqtt_system(configuration const& configuration);
	void enumerate_devices();
	bool is_device(std::string const& device_name) const;
	std::function<void(bool)> set_operation(std::string const& device_name);
	std::function<std::uint8_t()> brightness_operation(std::string const& device_name);
	std::function<void()> toggle_operation(std::string const& device_name);
	std::function<void()> increase_operation(std::string const& device_name);
	std::function<void()> decrease_operation(std::string const& device_name);
    std::function<void()> update_operation(std::string const& device_name);
	
private:
	mqtt_bulb& get_device(std::string const& name);
	
	configuration                           m_configuration;
	mqtt                                    m_publisher;
	mqtt                                    m_updater;
	std::vector<std::unique_ptr<mqtt_bulb>> m_bulbs;
};

}
