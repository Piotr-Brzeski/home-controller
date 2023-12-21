//
//  mqtt_bulb.cpp
//  home-controller
//
//  Created by Piotr Brzeski on 2023-12-17.
//  Copyright © 2023 Brzeski.net. All rights reserved.
//

#include "mqtt_bulb.h"
#include "json.h"
#include "exception.h"
#include <cpp-log/log.h>
#include <array>

using namespace home;

namespace {

constexpr auto raw_brightness_levels = std::array<int, 8>{0, 2, 40, 83, 125, 165, 208, 249};

std::uint8_t brightness_from_raw(int raw_value) {
	if(raw_value < raw_brightness_levels[1]) return 0;
	if(raw_value < raw_brightness_levels[2]) return 1;
	if(raw_value < raw_brightness_levels[3]) return 2;
	if(raw_value < raw_brightness_levels[4]) return 3;
	if(raw_value < raw_brightness_levels[5]) return 4;
	if(raw_value < raw_brightness_levels[6]) return 5;
	if(raw_value < raw_brightness_levels[7]) return 6;
	return 7;
}

std::string const& command(std::uint8_t brightness) {
	static auto commands = std::vector<std::string>(8);
	if(brightness >= commands.size()) {
		throw exception("Invalid brightness value");
	}
	auto& command = commands[brightness];
	if(command.empty()) {
		command = "{\"brightness\":" + std::to_string(raw_brightness_levels[brightness]) + ",\"color_temp\":250}";
	}
	return command;
}

}

mqtt_bulb::mqtt_bulb(std::string const& name, mqtt& publisher, std::string const& queue_name)
	: m_name(name)
	, m_publisher(publisher)
	, m_set_channel(queue_name + '/' + name + "/set")
{
}

void mqtt_bulb::trigger_update() {
	m_publisher.publish(m_set_channel, "{\"color_temp\":250}");
}

void mqtt_bulb::update(std::string state) {
//	{"brightness":100,"color_mode":"color_temp","color_temp":250,"color_temp_startup":454,"linkquality":248,"power_on_behavior":"previous","state":"ON","update":{"installed_version":587814449,"latest_version":587814449,"state":"idle"}}
	static auto const brightness_key = std::string("brightness");
	auto state_json = json(std::move(state));
//		auto raw_brightness = state_json[brightness_key].get_int();
	auto raw_brightness_value = state_json.get().get(brightness_key);
	if(raw_brightness_value) {
		auto raw_brightness = raw_brightness_value->get_int();
		auto previous_brightness = m_brightness.load();
		auto new_brightness = brightness_from_raw(raw_brightness);
		m_brightness = new_brightness;
		logger::log("[" + m_name + "] update brightness: " + std::to_string(previous_brightness) + " -> " + std::to_string(new_brightness));
	}
}

std::string const& mqtt_bulb::name() const {
	return m_name;
}

std::uint8_t mqtt_bulb::brightness() const {
	return m_brightness;
}

void mqtt_bulb::set(std::uint8_t brightness) {
	m_publisher.publish(m_set_channel, command(brightness));
}

void mqtt_bulb::set(bool enabled) {
	set(enabled ? max_brightness : zero_brightness);
}

void mqtt_bulb::toggle() {
	set(brightness() == zero_brightness ? max_brightness : zero_brightness);
}

void mqtt_bulb::increase() {
	if(brightness() < max_brightness) {
		set(static_cast<std::uint8_t>(m_brightness + 1));
	}
}

void mqtt_bulb::decrease() {
	if(brightness() > zero_brightness) {
		set(static_cast<std::uint8_t>(m_brightness - 1));
	}
}
