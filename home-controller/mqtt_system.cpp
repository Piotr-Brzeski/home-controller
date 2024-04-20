//
//  mqtt_system.cpp
//  home-controller
//
//  Created by Piotr Brzeski on 2023-12-17.
//  Copyright © 2023 Brzeski.net. All rights reserved.
//

#include "mqtt_system.h"
#include "json.h"
#include "exception.h"
#include <array>
#include <vector>
#include <cassert>
/*
#include <map>
#include <algorithm>
*/

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

mqtt_system::mqtt_system(configuration const& configuration)
	: m_configuration(configuration)
{
	m_publisher.connect(m_configuration.address);
	m_updater.connect(m_configuration.address);
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
	auto channels = std::vector<std::string>();
	channels.reserve(names.size());
	for(auto& name : names) {
		channels.push_back(m_configuration.queue_name + '/' + name);
	}
	m_updater.subscribe(channels, std::bind(&mqtt_system::call, this, std::placeholders::_1, std::placeholders::_2));
}

void mqtt_system::ping(std::string const& name) {
	static auto const ping_command = std::string("{\"color_temp\":250}");
	publish(name, ping_command);
}

void mqtt_system::set(std::string const& name, std::uint8_t brightness) {
	publish(name, command(brightness));
}

void mqtt_system::publish(std::string const& name, std::string const& message) {
	auto channel = m_configuration.queue_name + '/' + name + "/set";
	m_publisher.publish(channel, message);
}

void mqtt_system::call(std::string const& channel, std::string message) {
	//	{"brightness":100,"color_mode":"color_temp","color_temp":250,"color_temp_startup":454,"linkquality":248,"power_on_behavior":"previous","state":"ON","update":{"installed_version":587814449,"latest_version":587814449,"state":"idle"}}
	static auto const brightness_key = std::string("brightness");
	auto state_json = json(std::move(message));
	auto raw_brightness_value = state_json.get().get(brightness_key);
	// TODO: Handle (log) error
	assert(raw_brightness_value);
	auto raw_brightness = raw_brightness_value->get_int();
	auto brightness = brightness_from_raw(raw_brightness);
	auto name = channel.substr(m_configuration.address.size());
	m_callback(name, brightness);
}




/*
	auto updaters = std::map<std::string, std::function<void(std::string const&)>>();
	auto channels = std::vector<std::string>();
	for(auto& bulb : m_bulbs) {
		auto& device = *bulb;
		auto& name = device.name();
		channels.push_back(m_configuration.queue_name + '/' + name);
		auto added = updaters.emplace(name, [&device](std::string message){ device.update(std::move(message)); }).second;
		if(!added) {
			throw exception("mqtt_system error: can not add updater for device with name \"" + name + "\".");
		}
	}
	auto pos = m_configuration.address.size();
	m_updater.subscribe(channels, [updaters, pos](std::string channel, std::string message){
		auto name = channel.substr(pos);
		auto it = updaters.find(name);
		if(it != updaters.end()) {
			it->second(std::move(message));
		}
		else {
			assert(false);
		}
	});
	for(auto& bulb : m_bulbs) {
		bulb->trigger_update();
	}
}

bool mqtt_system::is_device(std::string const& name) const {
	return std::ranges::find(m_bulbs, name, [](auto& d){ return d->name(); }) != m_bulbs.end();
}

std::function<void(bool)> mqtt_system::set_operation(std::string const& device_name) {
	auto& device = get_device(device_name);
	return [&device](bool enabled){ device.set(enabled); };
}

std::function<std::uint8_t()> mqtt_system::brightness_operation(std::string const& device_name) {
	auto& device = get_device(device_name);
	return [&device](){ return device.brightness(); };
}

std::function<void()> mqtt_system::toggle_operation(std::string const& device_name) {
	auto& device = get_device(device_name);
	return [&device](){ device.toggle(); };
}

std::function<void()> mqtt_system::increase_operation(std::string const& device_name) {
	auto& device = get_device(device_name);
	return [&device](){ device.increase(); };
}

std::function<void()> mqtt_system::decrease_operation(std::string const& device_name) {
	auto& device = get_device(device_name);
	return [&device](){ device.decrease(); };
}

std::function<void()> mqtt_system::update_operation(std::string const& device_name) {
	auto& device = get_device(device_name);
	return [&device](){ device.trigger_update(); };
}

mqtt_bulb& mqtt_system::get_device(std::string const& name) {
	for(auto& device : m_bulbs) {
		if(device->name() == name) {
			return *device;
		}
	}
	throw exception("mqtt_system error: device with name \"" + name + "\" not found.");
}
*/

