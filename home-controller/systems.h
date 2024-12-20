//
//  systems.h
//  home-controller
//
//  Created by Piotr Brzeski on 2024-04-15.
//  Copyright © 2024 Brzeski.net. All rights reserved.
//

#pragma once

#include "system_base.h"
#include "commands_queue.h"
#include "device.h"
#include <functional>
#include <vector>
#include <map>
#include <set>
#include <memory>

namespace home {

class systems {
public:
	using bulb_get = std::function<std::uint8_t()>;
	using bulb_set = std::function<void(std::uint8_t)>;
	using outlet_get = std::function<bool()>;
	using outlet_set = std::function<void(bool)>;

	void add(std::unique_ptr<system_base> system);
	void start(std::map<std::string, std::vector<bulb::callback>> const& bulbs, std::set<std::string> const& outlet_names);

	bulb_get bulb_getter(std::string const& name);
	bulb_set bulb_setter(std::string const& name);
	outlet_get outlet_getter(std::string const& name);
	outlet_set outlet_setter(std::string const& name);

private:
	system_base* get_system(std::string const& dev_name, device_type dev_type);
	bulb* get_bulb(std::string const& name);
	outlet* get_outlet(std::string const& name);
	void update(std::string const& name, std::uint8_t brightness);

	std::map<std::string, std::unique_ptr<bulb>>   m_bulbs;
	std::map<std::string, std::unique_ptr<outlet>> m_outlets;
	std::vector<std::unique_ptr<system_base>>      m_systems;
	commands_queue                                 m_commands;
};

}
