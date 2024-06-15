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
	, m_homekit(m_configuration.homekit_configuration())
{
	// systems
	m_systems.add(std::make_unique<mqtt_system>(m_configuration.mqtt_configuration()));
	auto bulb_configs = std::map<std::string, std::vector<bulb::callback>>();
	auto groups_definition = m_configuration.groups();
	for(auto& group_definition : groups_definition) {
		auto devices_group = std::make_unique<group>();
		for(auto& device_name : group_definition.second) {
			auto& bulb_update_callbacks = bulb_configs[device_name];
			bulb_update_callbacks.push_back(
				[homekit = &m_homekit, group = devices_group.get(), group_name = group_definition.first](){
					auto brightness = group->get_brightness();
					homekit->send_update(group_name, brightness);
				});
		}
		m_groups.emplace(group_definition.first, std::move(devices_group));
	}
	m_systems.start(bulb_configs);
	// groups
	auto group_names = std::vector<std::string>();
	group_names.reserve(groups_definition.size());
	for(auto& group_definition : groups_definition) {
		auto const& group_name = group_definition.first;
		group_names.push_back(group_name);
		auto devices_group = get_group(group_name);
		for(auto& device_name : group_definition.second) {
			devices_group->add(m_systems.bulb_getter(device_name), m_systems.bulb_setter(device_name));
		}
	}
	// Homekit
	m_homekit.start(
		std::move(group_names),
		[this](std::string const& name, std::uint8_t brightness) {
			auto group = get_group(name);
			group->set_brigntness(brightness);
		}
	);
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
	return it->second.get();
}
