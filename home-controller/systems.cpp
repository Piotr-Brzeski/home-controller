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

void systems::start(std::map<std::string, std::vector<bulb::callback>> const& bulbs) {
	auto bulb_systems = std::map<system_base*, std::vector<std::string>>();
	for(auto& bulb_config : bulbs) {
		auto const& bulb_name = bulb_config.first;
		system_base* system_with_bulb = nullptr;
		for(auto& system : m_systems) {
			if(system->get_device_type(bulb_name) == device_type::bulb) {
				system_with_bulb = system.get();
				break;
			}
		}
		// TODO: throw
		assert(system_with_bulb != nullptr);
		bulb_systems[system_with_bulb].push_back(bulb_name);
		auto const& bulb_update_callbacks = bulb_config.second;
		m_bulbs.emplace(bulb_name, std::make_unique<bulb>(bulb_name, *system_with_bulb, bulb_update_callbacks));
	}
	for(auto bulb_system : bulb_systems) {
		auto system = bulb_system.first;
		auto const& names = bulb_system.second;
		system->start(names);
		for(auto& name : names) {
			m_commands.execute([system, name](){ system->ping(name); });
		}
	}
}

systems::bulb_get systems::bulb_getter(const std::string &name) {
	return [bulb = get(name)]() {
		return bulb->brightness();
	};
}

systems::bulb_set systems::bulb_setter(const std::string &name) {
	return [this, bulb = get(name)](std::uint8_t brightness) {
		static constexpr auto timeout = std::chrono::milliseconds(700);
		auto cmd = [bulb, brightness]() {
			bulb->set(brightness);
		};
		m_commands.execute_and_set(bulb, cmd, std::chrono::steady_clock::now() + timeout);
	};
}

bulb* systems::get(std::string const& name) {
	auto it = m_bulbs.find(name);
	assert(it != m_bulbs.end());
	return it->second.get();
}

void systems::update(const std::string &name, std::uint8_t brightness) {
	auto bulb = get(name);
	auto cmd = [bulb, brightness]() {
		bulb->update(brightness);
	};
	m_commands.execute(bulb, cmd);
}
