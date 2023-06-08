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
#include <fstream>
#include <memory>

using namespace home;

namespace {

auto tradfri_configuration(json const& configuration) {
	tradfri::system::configuration system_configuration;
	auto tradfri_config = configuration["tradfri"];
	system_configuration.ip = tradfri_config["address"].get_string();
	system_configuration.identity = tradfri_config["identity"].get_string();
	system_configuration.key = tradfri_config["key"].get_string();
	return system_configuration;
}

auto device_id(std::string const& type, int number) {
	auto id = link::device_id();
	if(type == "button") {
		id.type = link::device_type::button;
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
auto load_devices(json const& configuration) {
	auto devices = std::map<std::string, link::device_id>();
	auto json_devices = configuration["devices"];
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
auto get_device(std::map<std::string, link::device_id> const& devices, std::string const& device_name) {
	auto it = devices.find(device_name);
	if(it == devices.end()) {
		throw exception("Invalid device name: \"" + device_name + "\".");
	}
	return it->second;
}

struct operation_data {
	operation_data(std::string&& type, std::string&& device)
		: type(std::move(type))
		, device(std::move(device))
	{
	}
	
	std::string type;
	std::string device;
};
auto load_operations(json const& configuration) {
	auto operations = std::map<std::string, operation_data>();
	auto json_operations = configuration["operations"];
	for(std::size_t i = 0; i < json_operations.size(); ++i) {
		auto json_operation = json_operations[i];
		auto name = json_operation["name"].get_string();
		auto type = json_operation["type"].get_string();
		auto device_name = json_operation["device"].get_string();
		auto added = operations.try_emplace(std::move(name), std::move(type), std::move(device_name)).second;
		if(!added) {
			throw exception("Duplicated operation name: \"" + name + "\".");
		}
	}
	return operations;
}
auto& get_operation_description(std::map<std::string, operation_data>& operations, std::string const& operation_name) {
	auto it = operations.find(operation_name);
	if(it == operations.end()) {
		throw exception("Invalid operation name: \"" + operation_name + "\".");
	}
	return it->second;
}
auto get_operation(operation_data const& operation_description, tradfri::system& tradfri_system) {
	if(operation_description.type == "toggle") {
		return tradfri_system.toggle_operation(operation_description.device);
	}
	throw exception("Invalid operation type: \"" + operation_description.type + "\".");
}

auto get_state_value(std::string const& name) {
	if(name == "click") {
		return link::state_value::click;
	}
	throw exception("Invalid state type: \"" + name + "\".");
}

auto load_commands(json const& configuration, tradfri::system& tradfri_system) {
	tradfri_system.enumerate_devices();
	auto commands = std::map<link::device_state, link::controller::operation>();
	auto devices = load_devices(configuration);
	auto operations = load_operations(configuration);
	auto json_commands = configuration["commands"];
	for(std::size_t i = 0; i < json_commands.size(); ++i) {
		auto json_command = json_commands[i];
		auto state_name = json_command["state"].get_string();
		auto state = get_state_value(state_name);
		auto operation_name = json_command["operation"].get_string();
		auto& operation_description = get_operation_description(operations, operation_name);
		auto operation = get_operation(operation_description, tradfri_system);
		auto device_name = json_command["device"].get_string();
		auto device_id = get_device(devices, device_name);
		auto device_state = link::device_state{{device_id, link::state_type::event}, state};
		auto added = commands.try_emplace(device_state, std::move(operation)).second;
		if(!added) {
			// TODO: Allow multiple operations
			throw exception("Duplicated command trigger.");
		}
	}
	return commands;
}

std::unique_ptr<json> configuration_json;
json const& configuration() {
	return *configuration_json;
}
json const& configuration(std::string_view configuration_path) {
	auto configuration_content = std::string();
	auto configuration_stream = std::ifstream(configuration_path);
	std::getline(configuration_stream, configuration_content, '\0');
	configuration_json = std::make_unique<json>(std::move(configuration_content));
	return *configuration_json;
}

}

controller::controller(std::string_view configuration_path)
	: m_tradfri_system(tradfri_configuration(configuration(configuration_path)))
{
	auto commands = load_commands(configuration(), m_tradfri_system);
	for(auto& command : commands) {
		m_controller.add(command.first, command.second);
	}
}

void controller::start() {
	auto port = configuration()["link"]["port"].get_int();
	m_controller.start(port);
}
