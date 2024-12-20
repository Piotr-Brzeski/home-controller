//
//  configuration.h
//  home-controller
//
//  Created by Piotr Brzeski on 2023-06-13.
//  Copyright © 2023-2024 Brzeski.net. All rights reserved.
//

#pragma once

#include "json.h"
#include "mqtt_system.h"
#include "dirigera_system.h"
#include <home-link/types.h>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <variant>
#include <optional>

namespace home {

class configuration {
public:
	using system = std::variant<mqtt_config, dirigera_config>;
	
	struct group_operation {
		enum class type { toggle, increase, decrease };
		std::string group;
		type        operation;
	};
	struct outlet_operation {
		enum class type { enable, disable };
		std::string outlet;
		type        operation;
	};
	using operation = std::variant<group_operation, outlet_operation>;

	configuration(const char* path);
	
	std::vector<system> systems_configuration() const;
	mqtt_config homekit_configuration() const;
	mqtt_config switches_configuration() const;
	int port() const;
	
	struct group {
		std::vector<std::string> devices;
		std::string              switch_name;
	};
	std::map<std::string, group> groups() const;
	std::map<homelink::device_state, operation> commands() const;
	
	struct switch_spec {
		std::optional<operation> toggle;
		std::optional<operation> up;
		std::optional<operation> down;
		std::optional<operation> alt_up;
		std::optional<operation> alt_down;
	};
	std::map<std::string, switch_spec> switches() const;
	
	std::set<std::string> const& outlet_names() const {
		return m_outlet_names;
	}
	
private:
	std::map<std::string, homelink::device_id> devices() const;
	homelink::device_id get_device(std::string const& name) const;
	std::map<std::string, operation> operations();
	operation get_operation(std::string const& name) const;
//	std::map<std::string, outlet_operation> outlet_operations() const;
//	outlet_operation get_outlet_operation(std::string const& name) const;
	mqtt_config mqtt_configuration(std::string const& name) const;
	
	json                                       m_json;
	std::set<std::string>                      m_outlet_names;
	std::map<std::string, homelink::device_id> m_devices;
	std::map<std::string, operation>           m_operations;

};

}
