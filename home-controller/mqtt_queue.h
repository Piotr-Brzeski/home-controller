//
//  mqtt_queue.h
//  home-controller
//
//  Created by Piotr Brzeski on 2024-06-15.
//  Copyright © 2023-2024 Brzeski.net. All rights reserved.
//

#pragma once

#include "mqtt.h"
#include <vector>
#include <string>

namespace home {

class mqtt_queue {
public:
	struct configuration {
		std::string address;
		std::string queue_name;
	};
	
	mqtt_queue(configuration configuration);
	
//	void start(std::vector<std::string> const& names) override;
	
protected:
	void publish(std::string const& name, std::string const& message);
	void subscribe(std::vector<std::string> const& channel_names, mqtt::callback_t callback);
	
	const configuration m_configuration;
	
private:
	mqtt                m_publisher;
	mqtt                m_updater;
};


}
