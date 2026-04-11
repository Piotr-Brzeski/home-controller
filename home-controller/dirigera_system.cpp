//
//  dirigera_system.cpp
//  home-controller
//
//  Created by Piotr Brzeski on 2024-11-30.
//  Copyright © 2024-2026 Brzeski.net. All rights reserved.
//

#include "dirigera_system.h"
#include "json.h"
#include <set>
#include <cassert>
//#include <iostream>

using namespace home;

namespace {

std::string const& command(bool enabled) {
	static auto on  = std::string("[{\"attributes\": {\"isOn\": true}}]");
	static auto off = std::string("[{\"attributes\": {\"isOn\": false}}]");
	if(enabled) {
		return on;
	}
	return off;
}

}

dirigera_system::dirigera_system(dirigera_config const& configuration)
	: dirigera_system(configuration.address, configuration.access_token)
{
}

dirigera_system::dirigera_system(std::string const& address, std::string const& access_token)
	: m_uri("https://" + address + ":8443/v1/devices/")
	, m_get_connection(access_token)
	, m_ws("wss://" + address + ":8443/", access_token)
{
	enumerate_devices();
}

device_type dirigera_system::get_device_type(std::string const& name) const {
	auto it = m_outlets.find(name);
	if(it != m_outlets.end()) {
		return device_type::outlet;
	}
	return device_type::none;
}

void dirigera_system::start(std::vector<std::string> const& names) {
	std::set<std::string> ids;
	std::vector<std::string> unknown_names;
	for(auto const& name : names) {
		auto it = m_outlets.find(name);
		if(it == m_outlets.end()) {
			unknown_names.push_back(name);
		}
		else {
			auto patch = http_patch(m_uri + it->second, m_get_connection.access_token());
			m_patch_connections.erase(name);
			m_patch_connections.emplace(name, std::move(patch));
			ids.insert(it->second);
		}
	}
	m_ws.start([ids](std::string message) {
		auto message_json = json(std::move(message));
		if(message_json["type"].get_string() != "deviceStateChanged") {
			return;
		}
		auto device_json = message_json["data"];
		auto id = device_json["id"].get_string();
		if(ids.contains(id)) {
//			auto enabled = device_json["attributes"]["isOn"].get_bool();
//			std::cout << id << " : " << enabled << std::endl;
		}
	});
}

void dirigera_system::ping([[maybe_unused]] std::string const& name) {
	
}

void dirigera_system::set(std::string const& name, std::uint8_t brightness) {
	auto it = m_patch_connections.find(name);
	if(it == m_patch_connections.end()) {
		// TODO: Log
		assert(false);
		return;
	}
	bool enabled = brightness > 0;
	auto& state = command(enabled);
	it->second.send({state});
}

void dirigera_system::enumerate_devices() {
	auto response = m_get_connection.get(m_uri);
//	std::cout << response << std::endl;
	auto devices_json = json(std::move(response));
	auto devices = devices_json.get();
	for(std::size_t i = 0; i < devices.size(); ++i) {
		auto device_json = devices[i];
		auto type = device_json["deviceType"].get_string();
//		std::cout << "Type: " << type << std::endl;
		if(type == "outlet") {
			auto name = device_json["attributes"]["customName"].get_string();
			auto id = device_json["id"].get_string();
			m_outlets.emplace(std::pair(std::move(name), std::move(id)));
//			m_outlets.emplace_back(m_uri, m_get_connection, device_json);
		}
//		else if(type == dirigera_bulb::device_type) {
//			m_bulbs.emplace_back(m_uri, m_get_connection, device_json);
//		}
		else {
//			std::cout << "Type: " << type << ", name: " << device_json["customName"].get_string() << std::endl;
		}
	}
}
