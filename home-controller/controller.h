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

template<class ikea_system1_t, class ikea_system2_t>
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
	configuration                                m_configuration;
	ikea::system<ikea_system1_t, ikea_system2_t> m_ikea_system;
	homelink::controller                         m_controller;
	std::map<std::string, group>                 m_groups;
};

}
