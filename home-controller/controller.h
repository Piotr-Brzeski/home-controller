//
//  controller.h
//  home-controller
//
//  Created by Piotr Brzeski on 2023-01-20.
//  Copyright © 2023 Brzeski.net. All rights reserved.
//

#pragma once

#include <cpp-tradfri/system.h>
#include <home-link/controller.h>
#include <string>

namespace home {

class controller {
public:
	controller(std::string_view configuration_path);
	void start();
	
private:
	tradfri::system  m_tradfri_system;
	link::controller m_controller;
};

}
