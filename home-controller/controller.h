//
//  controller.h
//  home-controller
//
//  Created by Piotr Brzeski on 2023-01-20.
//  Copyright © 2023-2024 Brzeski.net. All rights reserved.
//

#pragma once

#include "configuration.h"
#include "systems.h"
#include "group.h"
#include "homekit_controller.h"
#include <home-link/controller.h>
#include <string>
#include <map>
#include <memory>

namespace home {

class controller {
public:
	controller(const char* configuration_path);
	
	void start() {
		auto port = m_configuration.port();
		m_controller.start(port);
	}
	void wait() {
		m_controller.wait();
	}
	
private:
	group* get_group(std::string const& name);
	
	configuration                                 m_configuration;
	homekit                                       m_homekit;
	systems                                       m_systems;
	homelink::controller                          m_controller;
	std::map<std::string, std::unique_ptr<group>> m_groups;
};

}
