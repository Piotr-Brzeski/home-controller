//
//  bulb.cpp
//  home-controller
//
//  Created by Piotr Brzeski on 2023-12-17.
//  Copyright © 2023-2024 Brzeski.net. All rights reserved.
//

#include "bulb.h"
#include "exception.h"
//#include <cpp-log/log.h>

using namespace home;

namespace {

}

bulb::bulb(std::string const& name, system_base& system)
	: m_name(name)
	, m_system(system)
{
}

//std::string const& bulb::name() const {
//	return m_name;
//}

system_base& bulb::system() {
	return m_system;
}

std::uint8_t bulb::brightness() const {
	return m_brightness;
}

void bulb::set(std::uint8_t brightness) {
	m_system.set(m_name, brightness);
}

void bulb::update(std::uint8_t brightness) {
	m_brightness = brightness;
}
//	if(brightness != m_brightness) {
//		auto previous_brightness = m_brightness;
//		m_brightness = brightness;
//		logger::log("[" + m_name + "] update brightness: " + std::to_string(previous_brightness) + " -> " + std::to_string(brightness));
//	}
//}



//void mqtt_bulb::set(bool enabled) {
//	set(enabled ? max_brightness : zero_brightness);
//}
//
//void mqtt_bulb::toggle() {
//	set(brightness() == zero_brightness ? max_brightness : zero_brightness);
//}
//
//void mqtt_bulb::increase() {
//	if(brightness() < max_brightness) {
//		set(static_cast<std::uint8_t>(m_brightness + 1));
//	}
//}
//
//void mqtt_bulb::decrease() {
//	if(brightness() > zero_brightness) {
//		set(static_cast<std::uint8_t>(m_brightness - 1));
//	}
//}
