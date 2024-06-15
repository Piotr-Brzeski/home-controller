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
#include "bulb.h"
#include <functional>
#include <vector>
#include <map>
#include <memory>


namespace home {

class systems {
public:
	using bulb_get = std::function<std::uint8_t()>;
	using bulb_set = std::function<void(std::uint8_t)>;
	
	void add(std::unique_ptr<system_base> system);
	void start(std::map<std::string, std::vector<bulb::callback>> const& bulbs);

	bulb_get bulb_getter(std::string const& name);
	bulb_set bulb_setter(std::string const& name);

private:
	bulb* get(std::string const& name);
	void update(std::string const& name, std::uint8_t brightness);

	std::map<std::string, std::unique_ptr<bulb>> m_bulbs;
	std::vector<std::unique_ptr<system_base>>    m_systems;
	commands_queue                               m_commands;
};

}
