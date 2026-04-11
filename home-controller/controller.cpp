//
//  controller.cpp
//  home-controller
//
//  Created by Piotr Brzeski on 2023-01-20.
//  Copyright © 2023-2026 Brzeski.net. All rights reserved.
//

#include "controller.h"
#include "exception.h"
#include <algorithm>

using namespace home;

controller::controller(const char* configuration_path)
	: m_configuration(configuration_path)
	, m_homekit(m_configuration.homekit_configuration())
	, m_switches(m_configuration.switches_configuration())
{
	// systems
	auto systems = m_configuration.systems_configuration();
	for(auto& system_config : systems) {
		if(auto config_mqtt = std::get_if<mqtt_config>(&system_config)) {
			m_systems.add(std::make_unique<mqtt_system>(*config_mqtt));
		}
		else if(auto config_dirigera = std::get_if<dirigera_config>(&system_config)) {
			m_systems.add(std::make_unique<dirigera_system>(*config_dirigera));
		}
		else {
			assert(false); // Configuration should not return invalid system configuration
			throw exception("Invalid system configuration.");
		}
	}
	auto bulb_configs = std::map<std::string, std::vector<bulb::callback>>();
	auto groups_definition = m_configuration.groups();
	for(auto& group_definition : groups_definition) {
		auto devices_group = std::make_unique<bulbs_group>();
		for(auto& device_name : group_definition.second.devices) {
			auto& bulb_update_callbacks = bulb_configs[device_name];
			bulb_update_callbacks.push_back(
				[homekit = &m_homekit, group = devices_group.get(), group_name = group_definition.first](){
					auto brightness = group->get_brightness();
					homekit->send_update(group_name, brightness);
				});
		}
		m_bulb_groups.emplace(group_definition.first, std::move(devices_group));
	}
	auto& outlet_names = m_configuration.outlet_names();
	m_systems.start(bulb_configs, outlet_names);
	// single outlets
	for(auto& name : outlet_names) {
		auto outlet = std::make_unique<single_outlet>(m_systems.outlet_setter(name));
		m_outlets.emplace(name, std::move(outlet));
	}
	// bulb groups
	auto homekit_group_names = std::vector<std::string>();
	homekit_group_names.reserve(groups_definition.size());
	for(auto& group_definition : groups_definition) {
		auto const& group_name = group_definition.first;
		auto bulbs_group = get_group(group_name);
		for(auto& device_name : group_definition.second.devices) {
			bulbs_group->add(m_systems.bulb_getter(device_name), m_systems.bulb_setter(device_name));
		}
		homekit_group_names.push_back(group_name);
	}
	// Homekit
	m_homekit.start(
		std::move(homekit_group_names),
		[this](std::string const& name, std::uint8_t brightness) {
			auto group = get_group(name);
			group->set_brigntness(brightness);
		}
	);
	// Switches
	auto switches = m_configuration.switches();
	for(auto& switch_def : switches) {
		m_switches.add(
			switch_def.first,
			to_operation(switch_def.second.toggle),
			to_operation(switch_def.second.up),
			to_operation(switch_def.second.down),
			to_operation(switch_def.second.alt_up),
			to_operation(switch_def.second.alt_down));
	}
	m_switches.start();
	// Commands
	auto commands = m_configuration.commands();
	for(auto& command : commands) {
		m_controller.add(command.first, to_operation(command.second));
	}
}

bulbs_group* controller::get_group(std::string const& name) {
	auto it = m_bulb_groups.find(name);
	if(it == m_bulb_groups.end()) {
		throw exception("Group \"" + name + "\" not found.");
	}
	return it->second.get();
}

single_outlet* controller::get_outlet(std::string const& name) {
	auto it = m_outlets.find(name);
	if(it == m_outlets.end()) {
		throw exception("Outlet \"" + name + "\" not found.");
	}
	return it->second.get();
}

controller::operation controller::to_operation(std::optional<configuration::operation> const& operation) {
	if(!operation) {
		return [](){};
	}
	
	if(auto group_operation = std::get_if<configuration::group_operation>(&(*operation))) {
		auto group = get_group(group_operation->group);
		switch(group_operation->operation) {
			case configuration::group_operation::type::toggle:
				return [group](){ group->toggle(); };
			case configuration::group_operation::type::increase:
				return [group](){ group->increase(); };
			case configuration::group_operation::type::decrease:
				return [group](){ group->decrease(); };
		}
	}
	if(auto outlet_operation = std::get_if<configuration::outlet_operation>(&(*operation))) {
		auto outlet = get_outlet(outlet_operation->outlet);
		switch(outlet_operation->operation) {
			case configuration::outlet_operation::type::enable:
				return [outlet](){ outlet->enable(); };
			case configuration::outlet_operation::type::disable:
				return [outlet](){ outlet->disable(); };
		}
	}
	assert(false); // Configuration should not return invalid operation definition
	throw exception("Invalid command definition.");
}
