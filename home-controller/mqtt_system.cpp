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
#include <map>
#include <cassert>
#include <algorithm>

#include <iostream>

using namespace home;

mqtt_system::mqtt_system(configuration const& configuration)
	: m_configuration(configuration)
{
	m_publisher.connect(m_configuration.address);
	m_updater.connect(m_configuration.address);
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
				m_bulbs.push_back(std::make_unique<mqtt_bulb>(name, m_publisher, m_configuration.queue_name));
			}
			else {
				std::cout << "*** " + model << std::endl;
			}
		}
	}
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

mqtt_bulb& mqtt_system::get_device(std::string const& name) {
	for(auto& device : m_bulbs) {
		if(device->name() == name) {
			return *device;
		}
	}
	throw exception("mqtt_system error: device with name \"" + name + "\" not found.");
}
