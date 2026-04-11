//
//  configuration.cpp
//  home-controller
//
//  Created by Piotr Brzeski on 2023-06-13.
//  Copyright © 2023-2026 Brzeski.net. All rights reserved.
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

std::string get_string(json_value& json, std::string const& key) {
	auto value = json.get(key);
	if(value) {
		return value->get_string();
	}
	return "";
}

configuration::group_operation::type group_operation_type(std::string const& name) {
	if(name == "toggle") {
		return configuration::group_operation::type::toggle;
	}
	if(name == "increase") {
		return configuration::group_operation::type::increase;
	}
	if(name == "decrease") {
		return configuration::group_operation::type::decrease;
	}
	throw exception("Invaid group operation type: \"" + name + "\".");
}

configuration::outlet_operation::type outlet_operation_type(std::string const& name) {
	if(name == "enable") {
		return configuration::outlet_operation::type::enable;
	}
	if(name == "disable") {
		return configuration::outlet_operation::type::disable;
	}
	throw exception("Invaid outlet operation type: \"" + name + "\".");
}

mqtt_config mqtt_configuration(json_value& json) {
	mqtt_config config;
	config.address = json["address"].get_string();
	config.queue_name = json["queue_name"].get_string();
	return config;
}

dirigera_config dirigera_configuration(json_value& json) {
	dirigera_config config;
	config.address = json["address"].get_string();
	config.access_token = json["access_token"].get_string();
	return config;
}

}

configuration::configuration(const char* path)
	: m_json(load_file(path))
	, m_devices(devices())
	, m_operations(operations())
{
}

std::vector<configuration::system> configuration::systems_configuration() const {
	auto systems = std::vector<configuration::system>();
	auto json_systems = m_json["systems"];
	for(std::size_t i = 0; i < json_systems.size(); ++i) {
		auto json_system = json_systems[i];
		auto type = json_system["type"].get_string();
		if(type == "mqtt") {
			systems.push_back(::mqtt_configuration(json_system));
		}
		else if(type == "dirigera") {
			systems.push_back(::dirigera_configuration(json_system));
		}
		else {
			throw exception("Invalid system type: \"" + type + "\".");
		}
	}
	return systems;
}

mqtt_config configuration::homekit_configuration() const {
	return mqtt_configuration("homekit");
}

mqtt_config configuration::switches_configuration() const {
	return mqtt_configuration("switches_mqtt");
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

std::map<std::string, configuration::group> configuration::groups() const {
	auto groups = std::map<std::string, configuration::group>();
	auto json_groups = m_json["groups"];
	for(std::size_t i = 0; i < json_groups.size(); ++i) {
		auto json_group = json_groups[i];
		auto name = json_group["name"].get_string();
		auto json_devices = json_group["devices"];
		auto config = group();
		for(std::size_t i = 0; i < json_devices.size(); ++i) {
			config.devices.push_back(json_devices[i].get_string());
		}
		auto switch_name = json_group.get("switch");
		if(switch_name) {
			config.switch_name = switch_name->get_string();
		}
		auto added = groups.try_emplace(std::move(name), config).second;
		if(!added) {
			throw exception("Duplicated group name: \"" + name + "\".");
		}
	}
	return groups;
}

std::map<std::string, configuration::operation> configuration::operations() {
	auto operations = std::map<std::string, operation>();
	auto json_operations = m_json["operations"];
	for(std::size_t i = 0; i < json_operations.size(); ++i) {
		auto json_operation = json_operations[i];
		auto name = json_operation["name"].get_string();
		bool added = false;
		auto group_name = get_string(json_operation, "group");
		if(!group_name.empty()) {
			auto type = group_operation_type(json_operation["type"].get_string());
			auto operation_description = group_operation{std::move(group_name), type};
			added = operations.try_emplace(std::move(name), std::move(operation_description)).second;
			if(!added) {
				throw exception("Duplicated operation name: \"" + name + "\".");
			}
		}
		auto outlet_name = get_string(json_operation, "outlet");
		if(!outlet_name.empty()) {
			if(added) {
				throw exception("Invalid operation \"" + name + "\": both group and outlet specified.");
			}
			auto type = outlet_operation_type(json_operation["type"].get_string());
			auto operation_description = outlet_operation{outlet_name, type};
			added = operations.try_emplace(std::move(name), std::move(operation_description)).second;
			if(!added) {
				throw exception("Duplicated operation name: \"" + name + "\".");
			}
			m_outlet_names.insert(outlet_name);
		}
		if(!added) {
			throw exception("Invalid operation \"" + name + "\": no group nor outlet specified.");
		}
	}
	return operations;
}

configuration::operation configuration::get_operation(std::string const& name) const {
	auto it = m_operations.find(name);
	if(it == m_operations.end()) {
		throw exception("Invalid group operation name: \"" + name + "\".");
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

mqtt_config configuration::mqtt_configuration(std::string const& name) const {
	auto json_config = m_json[name];
	return ::mqtt_configuration(json_config);
}

std::map<std::string, configuration::switch_spec> configuration::switches() const {
	auto switches = std::map<std::string, switch_spec>();
	auto switches_value = m_json.get("switches");
	if(switches_value) {
		auto json_switches = *switches_value;
		for(std::size_t i = 0; i < json_switches.size(); ++i) {
			auto json_switch = json_switches[i];
			auto name = json_switch["name"].get_string();
			auto toggle = get_string(json_switch, "toggle");
			auto up = get_string(json_switch, "up");
			auto down = get_string(json_switch, "down");
			auto alt_up = get_string(json_switch, "alt_up");
			auto alt_down = get_string(json_switch, "alt_down");
			auto spec = switch_spec{
				toggle.empty() ? std::nullopt : std::optional<operation>(get_operation(toggle)),
				up.empty() ? std::nullopt : std::optional<operation>(get_operation(up)),
				down.empty() ? std::nullopt : std::optional<operation>(get_operation(down)),
				alt_up.empty() ? std::nullopt : std::optional<operation>(get_operation(alt_up)),
				alt_down.empty() ? std::nullopt : std::optional<operation>(get_operation(alt_down))
			};
			auto added = switches.emplace(std::move(name), std::move(spec)).second;
			if(!added) {
				throw exception("Duplicated switch definition \"" + name + "\".");
			}
		}
	}
	return switches;
}
