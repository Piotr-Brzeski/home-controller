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
#include "bulbs_group.h"
#include "single_outlet.h"
#include "homekit_controller.h"
#include "switches_controller.h"
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
	bulbs_group* get_group(std::string const& name);
	single_outlet* get_outlet(std::string const& name);
	
	using operation = std::function<void()>;
	operation to_operation(std::optional<configuration::operation> const& operation);
	
	configuration                                         m_configuration;
	systems                                               m_systems;
	homelink::controller                                  m_controller;
	homekit                                               m_homekit;
	switches_controller                                   m_switches;
	std::map<std::string, std::unique_ptr<bulbs_group>>   m_bulb_groups;
	std::map<std::string, std::unique_ptr<single_outlet>> m_outlets;
};

}
