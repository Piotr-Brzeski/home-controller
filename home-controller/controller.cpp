//
//  controller.cpp
//  home-controller
//
//  Created by Piotr Brzeski on 2023-01-20.
//  Copyright © 2023-2024 Brzeski.net. All rights reserved.
//

#include "controller.h"
#include "exception.h"
#include <algorithm>

using namespace home;

controller::controller(const char* configuration_path)
	: m_configuration(configuration_path)
{
	// systems
	m_systems.add(std::make_unique<mqtt_system>(m_configuration.mqtt_configuration()));
	auto bulb_names = std::vector<std::string>();
	auto groups_definition = m_configuration.groups();
	for(auto& group_definition : groups_definition) {
		auto devices_group = group();
		for(auto& device_name : group_definition.second) {
			bulb_names.push_back(device_name);
		}
	}
	std::ranges::sort(bulb_names);
	bulb_names.erase(std::unique(bulb_names.begin(), bulb_names.end()), bulb_names.end());
	m_systems.start(bulb_names);
	// groups
	for(auto& group_definition : groups_definition) {
		auto devices_group = group();
		for(auto& device_name : group_definition.second) {
			devices_group.add(m_systems.bulb_getter(device_name), m_systems.bulb_setter(device_name));
		}
		m_groups.emplace(group_definition.first, std::move(devices_group));
	}
	// commands
	auto commands = m_configuration.commands();
	for(auto& command : commands) {
		auto group = get_group(command.second.device);
		if(command.second.type == "toggle") {
			m_controller.add(command.first, [group](){ group->toggle(); });
		}
		else if(command.second.type == "increase") {
			m_controller.add(command.first, [group](){ group->increase(); });
		}
		else if(command.second.type == "decrease") {
			m_controller.add(command.first, [group](){ group->decrease(); });
		}
		else {
			throw exception("Invalid operation type: \"" + command.second.type + "\".");
		}
	}
}

group* controller::get_group(std::string const& name) {
	auto it = m_groups.find(name);
	if(it == m_groups.end()) {
		throw exception("Group \"" + name + "\" not found.");
	}
	return &(it->second);
}
