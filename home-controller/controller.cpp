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
controller<ikea::dirigera, ikea::tradfri>::controller(const char* configuration_path)
	: m_configuration(configuration_path)
	, m_ikea_system(m_configuration.dirigera_configuration(), m_configuration.tradfri_configuration())
{
	init(m_ikea_system, m_groups, m_controller, m_configuration);
}

template<>
controller<ikea::dirigera, ikea::no_system>::controller(const char* configuration_path)
	: m_configuration(configuration_path)
	, m_ikea_system(m_configuration.dirigera_configuration(), {})
{
	init(m_ikea_system, m_groups, m_controller, m_configuration);
}
