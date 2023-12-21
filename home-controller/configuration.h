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
#include <cpp-ikea/tradfri.h>
#include <cpp-ikea/dirigera.h>
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
	
	ikea::tradfri::configuration tradfri_configuration() const;
	ikea::dirigera::configuration dirigera_configuration() const;
	mqtt_system::configuration mqtt_configuration() const;
	int port() const;
	std::map<std::string, std::vector<std::string>> groups() const;
	std::map<homelink::device_state, operation> commands() const;
	
private:
	std::map<std::string, homelink::device_id> devices() const;
	homelink::device_id get_device(std::string const& name) const;
	std::map<std::string, operation> operations() const;
	operation get_operation(std::string const& name) const;
	
	json                                       m_json;
	std::map<std::string, homelink::device_id> m_devices;
	std::map<std::string, operation>           m_operations;
	
};

}
