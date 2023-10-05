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
	std::map<std::string, group>&          m_groups;
	std::map<std::string, group>::iterator m_group_it;
};

}

controller::controller(const char* configuration_path)
	: m_configuration(configuration_path)
	, m_tradfri_system(m_configuration.tradfri_configuration())
{
	m_tradfri_system.enumerate_devices();
	auto groups = m_configuration.groups();
	for(auto& group_definition : groups) {
		m_groups.emplace(group_definition.first, create_group(group_definition.second));
	}
	auto commands = m_configuration.commands();
	for(auto& command : commands) {
		m_controller.add(command.first, get_operation(command.second));
	}
	m_controller.set_periodic_task(groups_updater(m_groups), std::chrono::seconds(30));
}

void controller::start() {
	auto port = m_configuration.port();
	m_controller.start(port);
}

void controller::wait() {
	m_controller.wait();
}

group controller::create_group(std::vector<std::string> const& devices) {
	auto devices_group = group();
	std::ranges::for_each(devices, [this, &devices_group](auto& device_name) {
		devices_group.add({
			m_tradfri_system.brightness_operation(device_name),
			m_tradfri_system.set_operation(device_name),
			m_tradfri_system.increase_operation(device_name),
			m_tradfri_system.decrease_operation(device_name)
		});
	});
	return devices_group;
}

group* controller::get_group(std::string const& name) {
	auto it = m_groups.find(name);
	if(it != m_groups.end()) {
		return &it->second;
	}
	return nullptr;
}

std::function<void()> controller::get_operation(configuration::operation const& operation) {
	auto group = get_group(operation.device);
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
		return m_tradfri_system.toggle_operation(operation.device);
	}
	if(operation.type == "increase") {
		return m_tradfri_system.increase_operation(operation.device);
	}
	if(operation.type == "decrease") {
		return m_tradfri_system.decrease_operation(operation.device);
	}
	throw exception("Invalid operation type: \"" + operation.type + "\".");
}
