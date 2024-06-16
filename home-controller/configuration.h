//
//  configuration.h
//  home-controller
//
//  Created by Piotr Brzeski on 2023-06-13.
//  Copyright © 2023 Brzeski.net. All rights reserved.
//

#pragma once

#include "json.h"
#include "mqtt_system.h"
#include <home-link/types.h>
#include <vector>
#include <map>
#include <string>

namespace home {

class configuration {
public:
	struct operation {
		std::string type;
		std::string device;
	};
	
	configuration(const char* path);
	
	mqtt_config mqtt_configuration() const;
	mqtt_config homekit_configuration() const;
	int port() const;
	
	struct group {
		std::vector<std::string> devices;
		std::string              switch_name;
	};
	std::map<std::string, group> groups() const;
	std::map<homelink::device_state, operation> commands() const;
	
private:
	std::map<std::string, homelink::device_id> devices() const;
	homelink::device_id get_device(std::string const& name) const;
	std::map<std::string, operation> operations() const;
	operation get_operation(std::string const& name) const;
	mqtt_config mqtt_configuration(std::string const& name) const;
	
	json                                       m_json;
	std::map<std::string, homelink::device_id> m_devices;
	std::map<std::string, operation>           m_operations;
	
};

}
