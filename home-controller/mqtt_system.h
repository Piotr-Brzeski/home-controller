//
//  mqtt_system.h
//  home-controller
//
//  Created by Piotr Brzeski on 2023-12-17.
//  Copyright © 2023 Brzeski.net. All rights reserved.
//

#pragma once

#include "system_base.h"
#include "mqtt.h"
#include <set>
/*
#include "mqtt_bulb.h"
#include <string>
#include <functional>
#include <memory>
*/

namespace home {

class mqtt_system: public system_base {
public:
	struct configuration {
		std::string address;
		std::string queue_name;
	};
	
	mqtt_system(configuration const& configuration);
	
	device_type get_device_type(std::string const& name) const override;
	void start(std::vector<std::string> const& names) override;
	void ping(std::string const& name) override;
	void set(std::string const& name, std::uint8_t brightness) override;
	/*
	bool is_device(std::string const& device_name) const;
	std::function<void(bool)> set_operation(std::string const& device_name);
	std::function<std::uint8_t()> brightness_operation(std::string const& device_name);
	std::function<void()> toggle_operation(std::string const& device_name);
	std::function<void()> increase_operation(std::string const& device_name);
	std::function<void()> decrease_operation(std::string const& device_name);
	std::function<void()> update_operation(std::string const& device_name);
	*/
private:
	void enumerate_devices();
	void publish(std::string const& name, std::string const& message);
	void call(std::string const& channel, std::string message);
	//	mqtt_bulb& get_device(std::string const& name);
	
	configuration         m_configuration;
	std::set<std::string> m_bulb_names;
	mqtt                  m_publisher;
	mqtt                  m_updater;
//	std::vector<std::unique_ptr<mqtt_bulb>> m_bulbs;
};

}
