//
//  controller.cpp
//  home-controller
//
//  Created by Piotr Brzeski on 2023-01-20.
//  Copyright © 2023 Brzeski.net. All rights reserved.
//

#include "controller.h"
#include "json.h"
#include "exception.h"
#include "mqtt_system.h"
#include <algorithm>
#include <fstream>
#include <memory>

using namespace home;

namespace {

using groups_t = std::map<std::string, group>;

class groups_updater {
public:
	groups_updater(std::map<std::string, group>& groups)
		: m_groups(groups)
		, m_group_it(m_groups.begin())
	{
	}
	
	void operator()() {
		if(m_groups.empty()) {
			return;
		}
		if(m_group_it == m_groups.end()) {
			m_group_it = m_groups.begin();
		}
		if(!m_group_it->second.update_member()) {
			++m_group_it;
		}
	}
	
private:
	groups_t&          m_groups;
	groups_t::iterator m_group_it;
};

template<class system1_t, class system2_t>
class two_systems {
public:
	two_systems(system1_t& system1, system2_t& system2)
		: system1(system1)
		, system2(system2)
	{
	}
	void enumerate_devices() {
		system1.enumerate_devices();
		system2.enumerate_devices();
	}
	std::function<std::uint8_t()> brightness_operation(std::string const& device_name) {
		if(system1.is_device(device_name)) {
			return system1.brightness_operation(device_name);
		}
		return system2.brightness_operation(device_name);
	}
	std::function<void(bool)> set_operation(std::string const& device_name) {
		if(system1.is_device(device_name)) {
			return system1.set_operation(device_name);
		}
		return system2.set_operation(device_name);
	}
	std::function<void()> toggle_operation(std::string const& device_name) {
		if(system1.is_device(device_name)) {
			return system1.toggle_operation(device_name);
		}
		return system2.toggle_operation(device_name);
	}
	std::function<void()> increase_operation(std::string const& device_name) {
		if(system1.is_device(device_name)) {
			return system1.increase_operation(device_name);
		}
		return system2.increase_operation(device_name);
	}
	std::function<void()> decrease_operation(std::string const& device_name) {
		if(system1.is_device(device_name)) {
			return system1.decrease_operation(device_name);
		}
		return system2.decrease_operation(device_name);
	}
	
private:
	system1_t& system1;
	system2_t& system2;
};

template<class system_t>
group create_group(system_t& system, std::vector<std::string> const& devices) {
	auto devices_group = group();
	std::ranges::for_each(devices, [&system, &devices_group](auto& device_name) {
		devices_group.add({
			system.brightness_operation(device_name),
			system.set_operation(device_name),
			system.increase_operation(device_name),
			system.decrease_operation(device_name)
		});
	});
	return devices_group;
}

group* get_group(groups_t& groups, std::string const& name) {
	auto it = groups.find(name);
	if(it != groups.end()) {
		return &it->second;
	}
	return nullptr;
}

template<class system_t>
std::function<void()> get_operation(system_t& system, groups_t& groups, configuration::operation const& operation) {
	group* group = get_group(groups, operation.device);
	if(group) {
		if(operation.type == "toggle") {
			return [group](){ group->toggle(); };
		}
		if(operation.type == "increase") {
			return [group](){ group->increase(); };
		}
		if(operation.type == "decrease") {
			return [group](){ group->decrease(); };
		}
		throw exception("Invalid operation type: \"" + operation.type + "\".");
	}
	if(operation.type == "toggle") {
		return system.toggle_operation(operation.device);
	}
	if(operation.type == "increase") {
		return system.increase_operation(operation.device);
	}
	if(operation.type == "decrease") {
		return system.decrease_operation(operation.device);
	}
	throw exception("Invalid operation type: \"" + operation.type + "\".");
}

template<class system_t>
void init(system_t& system, groups_t& groups, homelink::controller& controller, configuration const& config) {
	system.enumerate_devices();
	auto groups_definition = config.groups();
	for(auto& group_definition : groups_definition) {
		groups.emplace(group_definition.first, create_group(system, group_definition.second));
	}
	auto commands = config.commands();
	for(auto& command : commands) {
		controller.add(command.first, get_operation(system, groups, command.second));
	}
	//	m_controller.set_periodic_task(groups_updater(m_groups), std::chrono::seconds(100));
}

}

template<>
controller<ikea::system<ikea::dirigera, ikea::tradfri>>::controller(const char* configuration_path)
	: m_configuration(configuration_path)
	, m_mqtt_system(m_configuration.mqtt_configuration())
	, m_ikea_system(m_configuration.dirigera_configuration(), m_configuration.tradfri_configuration())
{
	auto systems = two_systems(m_mqtt_system, m_ikea_system);
	init(systems, m_groups, m_controller, m_configuration);
}

template<>
controller<ikea::system<ikea::dirigera, ikea::no_system>>::controller(const char* configuration_path)
	: m_configuration(configuration_path)
	, m_mqtt_system(m_configuration.mqtt_configuration())
	, m_ikea_system(m_configuration.dirigera_configuration(), {})
{
		auto systems = two_systems(m_mqtt_system, m_ikea_system);
		init(systems, m_groups, m_controller, m_configuration);
}
