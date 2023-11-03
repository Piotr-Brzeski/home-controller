//
//  controller.h
//  home-controller
//
//  Created by Piotr Brzeski on 2023-01-20.
//  Copyright © 2023 Brzeski.net. All rights reserved.
//

#pragma once

#include "configuration.h"
#include "group.h"
#include <cpp-ikea/system.h>
#include <home-link/controller.h>
#include <string>
#include <map>

namespace home {

class controller {
public:
	controller(const char* configuration_path);
	void start();
	void wait();
	
private:
	group create_group(std::vector<std::string> const& devices);
	group* get_group(std::string const& name);
	std::function<void()> get_operation(configuration::operation const& operation);
	
	configuration                               m_configuration;
	ikea::system<ikea::dirigera, ikea::tradfri> m_ikea_system;
	homelink::controller                        m_controller;
	std::map<std::string, group>                m_groups;
};

}
