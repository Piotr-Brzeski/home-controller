//
//  bulb.cpp
//  home-controller
//
//  Created by Piotr Brzeski on 2023-12-17.
//  Copyright © 2023-2024 Brzeski.net. All rights reserved.
//

#include "bulb.h"
#include "exception.h"
#include <thread>
#include <cassert>

using namespace home;

namespace {

void thread_checker() {
	static auto first_thread = std::this_thread::get_id();
	auto thread = std::this_thread::get_id();
	assert(thread == first_thread);
}

}

bulb::bulb(std::string const& name, system_base& system, std::vector<callback> update_callbacks)
	: m_name(name)
	, m_system(system)
	, m_update_callbacks(std::move(update_callbacks))
{
}

//std::string const& bulb::name() const {
//	return m_name;
//}

//system_base& bulb::system() {
//	return m_system;
//}

void bulb::set(std::uint8_t brightness) {
	thread_checker();
	m_system.set(m_name, brightness);
}

// Thread-safe method
std::uint8_t bulb::brightness() const {
	return m_brightness;
}

// Thread-safe method
void bulb::update(std::uint8_t brightness) {
	thread_checker();
	if(brightness != m_brightness) {
		m_brightness = brightness;
		for(auto& callback : m_update_callbacks) {
			callback();
		}
	}
}
