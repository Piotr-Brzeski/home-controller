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
#include <vector>
#include <map>
#include <memory>

namespace home {

class systems {
public:
	void add(std::unique_ptr<system_base> system);
	void start(std::vector<std::string> const& bulb_names);
	bulb* get(std::string const& name);
	void set(std::string const& name, std::uint8_t brightness);
	
private:
	void update(std::string const& name, std::uint8_t brightness);
	
	std::map<std::string, std::unique_ptr<bulb>> m_bulbs;
	std::vector<std::unique_ptr<system_base>>    m_systems;
	commands_queue                               m_commands;
};

}
