//
//  configuration.cpp
//  home-controller
//
//  Created by Piotr Brzeski on 2023-06-13.
//  Copyright © 2023 Brzeski.net. All rights reserved.
//

#include "configuration.h"
#include "exception.h"
#include <fstream>

using namespace home;

namespace {


std::string load_file(const char* path) {
	auto content = std::string();
	auto stream = std::ifstream(path);
	std::getline(stream, content, '\0');
	return content;
}

auto device_id(std::string const& type, int number) {
	auto id = homelink::device_id();
	if(type == "button") {
		id.type = homelink::device_type::button;
	}
	else {
		throw exception("Invalid device type: \"" + type + "\".");
	}
	if(number >= std::numeric_limits<std::uint8_t>::min() && number <= std::numeric_limits<std::uint8_t>::max()) {
		id.number = static_cast<std::uint8_t>(number);
	}
	else {
		throw exception("Invalid device number: \"" + std::to_string(number) + "\".");
	}
	return id;
}

auto get_state_value(std::string const& name) {
	if(name == "click") {
		return homelink::state_value::click;
	}
	if(name == "plus") {
		return homelink::state_value::plus;
	}
	if(name == "minus") {
		return homelink::state_value::minus;
	}
	if(name == "alt_plus") {
		return homelink::state_value::alt_plus;
	}
	if(name == "alt_minus") {
		return homelink::state_value::alt_minus;
	}
	throw exception("Invalid state type: \"" + name + "\".");
}

}

configuration::configuration(const char* path)
	: m_json(load_file(path))
	, m_devices(devices())
	, m_operations(operations())
{
}

ikea::tradfri::configuration configuration::tradfri_configuration() const {
	ikea::tradfri::configuration system_configuration;
	auto tradfri_config = m_json["tradfri"];
	system_configuration.ip = tradfri_config["address"].get_string();
	system_configuration.identity = tradfri_config["identity"].get_string();
	system_configuration.key = tradfri_config["key"].get_string();
	return system_configuration;
}

ikea::dirigera::configuration configuration::dirigera_configuration() const {
	ikea::dirigera::configuration system_configuration;
	auto dirigera_config = m_json["dirigera"];
	system_configuration.ip = dirigera_config["address"].get_string();
	system_configuration.access_token = dirigera_config["access_token"].get_string();
	return system_configuration;
}

mqtt_system::configuration configuration::mqtt_configuration() const {
	mqtt_system::configuration system_configuration;
	auto mqtt_config = m_json["mqtt"];
	system_configuration.address = mqtt_config["address"].get_string();
	system_configuration.queue_name = mqtt_config["queue_name"].get_string();
	return system_configuration;
}

int configuration::port() const {
	return m_json["link"]["port"].get_int();
}

std::map<std::string, homelink::device_id> configuration::devices() const {
	auto devices = std::map<std::string, homelink::device_id>();
	auto json_devices = m_json["devices"];
	for(std::size_t i = 0; i < json_devices.size(); ++i) {
		auto json_device = json_devices[i];
		auto name = json_device["name"].get_string();
		auto type = json_device["type"].get_string();
		auto number = json_device["number"].get_int();
		auto id = device_id(type, number);
		auto added = devices.try_emplace(std::move(name), id).second;
		if(!added) {
			throw exception("Duplicated device name: \"" + name + "\".");
		}
	}
	return devices;
}

homelink::device_id configuration::get_device(std::string const& name) const {
	auto it = m_devices.find(name);
	if(it == m_devices.end()) {
		throw exception("Invalid device name: \"" + name + "\".");
	}
	return it->second;
}

std::map<std::string, std::vector<std::string>> configuration::groups() const {
	auto groups = std::map<std::string, std::vector<std::string>>();
	auto json_groups = m_json["groups"];
	for(std::size_t i = 0; i < json_groups.size(); ++i) {
		auto json_group = json_groups[i];
		auto name = json_group["name"].get_string();
		auto json_devices = json_group["devices"];
		auto devices = std::vector<std::string>();
		for(std::size_t i = 0; i < json_devices.size(); ++i) {
			devices.push_back(json_devices[i].get_string());
		}
		auto added = groups.try_emplace(std::move(name), std::move(devices)).second;
		if(!added) {
			throw exception("Duplicated group name: \"" + name + "\".");
		}
	}
	return groups;
}

std::map<std::string, configuration::operation> configuration::operations() const {
	auto operations = std::map<std::string, operation>();
	auto json_operations = m_json["operations"];
	for(std::size_t i = 0; i < json_operations.size(); ++i) {
		auto json_operation = json_operations[i];
		auto name = json_operation["name"].get_string();
		auto type = json_operation["type"].get_string();
		auto device_name = json_operation["device"].get_string();
		auto operation_description = operation{std::move(type), std::move(device_name)};
		auto added = operations.try_emplace(std::move(name), std::move(operation_description)).second;
		if(!added) {
			throw exception("Duplicated operation name: \"" + name + "\".");
		}
	}
	return operations;
}

configuration::operation configuration::get_operation(std::string const& name) const {
	auto it = m_operations.find(name);
	if(it == m_operations.end()) {
		throw exception("Invalid operation name: \"" + name + "\".");
	}
	return it->second;
}

std::map<homelink::device_state, configuration::operation> configuration::commands() const {
	auto commands = std::map<homelink::device_state, configuration::operation>();
	auto json_commands = m_json["commands"];
	for(std::size_t i = 0; i < json_commands.size(); ++i) {
		auto json_command = json_commands[i];
		auto device_name = json_command["device"].get_string();
		auto device_id = get_device(device_name);
		auto state_name = json_command["state"].get_string();
		auto state = get_state_value(state_name);
		auto device_state = homelink::device_state{{device_id, homelink::state_type::event}, state};
		auto operation_name = json_command["operation"].get_string();
		auto operation = get_operation(operation_name);
		auto added = commands.try_emplace(device_state, std::move(operation)).second;
		if(!added) {
			// TODO: Allow multiple operations
			throw exception("Duplicated command trigger.");
		}
	}
	return commands;
}
