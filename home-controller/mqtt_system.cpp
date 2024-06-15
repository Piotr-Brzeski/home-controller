//
//  mqtt_system.cpp
//  home-controller
//
//  Created by Piotr Brzeski on 2023-12-17.
//  Copyright © 2023-2024 Brzeski.net. All rights reserved.
//

#include "mqtt_system.h"
#include "json.h"
#include "exception.h"
#include <array>
#include <vector>
#include <cassert>

#include <iostream>

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

mqtt_system::mqtt_system(mqtt_queue::configuration configuration)
	: mqtt_queue(std::move(configuration))
{
	enumerate_devices();
}

void mqtt_system::enumerate_devices() {
	static auto const model_id_key = std::string("model_id");
	static auto const name_key = std::string("friendly_name");
	auto devices = mqtt::get_message(m_configuration.address, m_configuration.queue_name + "/bridge/devices");
	auto devices_json = json(std::move(devices));
	auto devices_list = devices_json.get();
	for(std::size_t i = 0; i < devices_list.size(); ++ i) {
		auto device_description = devices_list[i];
		auto model_value = device_description.get(model_id_key);
		if(model_value) {
			auto model = model_value->get_string();
			if(model == "TRADFRI bulb GU10 WS 400lm") {
				auto name = device_description[name_key].get_string();
				m_bulb_names.insert(std::move(name));
			}
			else {
				std::cout << "\n*** " + model << std::endl;
			}
		}
	}
}

device_type mqtt_system::get_device_type(std::string const& name) const {
	auto it = m_bulb_names.find(name);
	if(it != m_bulb_names.end()) {
		return device_type::bulb;
	}
	return device_type::none;
}

void mqtt_system::start(std::vector<std::string> const& names) {
	if(!m_callback) {
		return;
	}
	subscribe(names, std::bind(&mqtt_system::call, this, std::placeholders::_1, std::placeholders::_2));
}

void mqtt_system::ping(std::string const& name) {
	static auto const ping_command = std::string("{\"color_temp\":250}");
	publish(name, ping_command);
}

void mqtt_system::set(std::string const& name, std::uint8_t brightness) {
	publish(name, command(brightness));
}

void mqtt_system::call(std::string const& name, std::string message) {
	try {
		//	{"brightness":100,"color_mode":"color_temp","color_temp":250,"color_temp_startup":454,"linkquality":248,"power_on_behavior":"previous","state":"ON","update":{"installed_version":587814449,"latest_version":587814449,"state":"idle"}}
		static auto const brightness_key = std::string("brightness");
		auto state_json = json(std::move(message));
		auto raw_brightness_value = state_json.get().get(brightness_key);
		// TODO: Handle (log) error
		assert(raw_brightness_value);
		auto raw_brightness = raw_brightness_value->get_int();
		auto brightness = brightness_from_raw(raw_brightness);
		m_callback(name, brightness);
	}
	catch(...) {
		// TODO: Log error
		assert(false);
	}
}
