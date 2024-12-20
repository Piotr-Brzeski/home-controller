//
//  systems.cpp
//  home-controller
//
//  Created by Piotr Brzeski on 2024-04-15.
//  Copyright © 2024 Brzeski.net. All rights reserved.
//

#include "systems.h"
#include <chrono>
#include <algorithm>
#include <iterator>
#include <cassert>

using namespace home;

void systems::add(std::unique_ptr<system_base> system) {
	system->set(std::bind(&systems::update, this, std::placeholders::_1, std::placeholders::_2));
	m_systems.push_back(std::move(system));
}

void systems::start(std::map<std::string, std::vector<bulb::callback>> const& bulbs, std::set<std::string> const& outlet_names) {
	auto systems = std::map<system_base*, std::vector<std::string>>();
	for(auto& bulb_config : bulbs) {
		auto const& bulb_name = bulb_config.first;
		auto system_with_bulb = get_system(bulb_name, device_type::bulb);
		systems[system_with_bulb].push_back(bulb_name);
		auto const& bulb_update_callbacks = bulb_config.second;
		m_bulbs.emplace(bulb_name, std::make_unique<bulb>(bulb_name, *system_with_bulb, bulb_update_callbacks));
	}
	for(auto& outlet_name : outlet_names) {
		auto system_with_outlet = get_system(outlet_name, device_type::outlet);
		systems[system_with_outlet].push_back(outlet_name);
		m_outlets.emplace(outlet_name, std::make_unique<outlet>(outlet_name, *system_with_outlet));
	}
	for(auto system_with_devices : systems) {
		auto system = system_with_devices.first;
		auto const& names = system_with_devices.second;
		system->start(names);
		for(auto& name : names) {
			m_commands.execute([system, name](){ system->ping(name); });
		}
	}
}

systems::bulb_get systems::bulb_getter(const std::string &name) {
	return [bulb = get_bulb(name)]() {
		return bulb->state();
	};
}

systems::bulb_set systems::bulb_setter(const std::string &name) {
	return [this, bulb = get_bulb(name)](std::uint8_t brightness) {
		static constexpr auto timeout = std::chrono::milliseconds(700);
		auto cmd = [bulb, brightness]() {
			bulb->set(brightness);
		};
		m_commands.execute_and_set(bulb, cmd, std::chrono::steady_clock::now() + timeout);
	};
}

systems::outlet_get systems::outlet_getter(const std::string &name) {
	return [outlet = get_outlet(name)]() {
		return outlet->state();
	};
}

systems::outlet_set systems::outlet_setter(const std::string &name) {
	return [this, outlet = get_outlet(name)](bool state) {
		static constexpr auto timeout = std::chrono::milliseconds(700);
		auto cmd = [outlet, state]() {
			outlet->set(state);
		};
		m_commands.execute_and_set(outlet, cmd, std::chrono::steady_clock::now() + timeout);
	};
}

system_base* systems::get_system(std::string const& dev_name, device_type dev_type) {
	for(auto& system : m_systems) {
		if(system->get_device_type(dev_name) == dev_type) {
			return system.get();
		}
	}
	// TODO: throw
	assert(false);
}

bulb* systems::get_bulb(std::string const& name) {
	auto it = m_bulbs.find(name);
	assert(it != m_bulbs.end());
	return it->second.get();
}

outlet* systems::get_outlet(std::string const& name) {
	auto it = m_outlets.find(name);
	assert(it != m_outlets.end());
	return it->second.get();
}

void systems::update(const std::string &name, std::uint8_t brightness) {
	auto bulb = get_bulb(name);
	auto cmd = [bulb, brightness]() {
		bulb->update(brightness);
	};
	m_commands.execute(bulb, cmd);
}
