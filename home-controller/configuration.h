//
//  configuration.h
//  home-controller
//
//  Created by Piotr Brzeski on 2023-06-13.
//  Copyright © 2023 Brzeski.net. All rights reserved.
//

#pragma once

#include <cpp-tradfri/system.h>
#include <home-link/types.h>
#include "json.h"
#include <vector>
#include <map>

namespace home {

class configuration {
public:
	struct operation {
		std::string type;
		std::string device;
	};
	
	configuration(std::string_view path);
	
	tradfri::system::configuration tradfri_configuration() const;
	int port() const;
	std::map<std::string, std::vector<std::string>> groups() const;
	std::map<link::device_state, operation> commands() const;
	
private:
	std::map<std::string, link::device_id> devices() const;
	link::device_id get_device(std::string const& name) const;
	std::map<std::string, operation> operations() const;
	operation get_operation(std::string const& name) const;
	
	json                                   m_json;
	std::map<std::string, link::device_id> m_devices;
	std::map<std::string, operation>       m_operations;
	
};

}
