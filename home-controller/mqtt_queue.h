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

struct mqtt_config {
	std::string address;
	std::string queue_name;
};

class mqtt_reader_queue {
public:
	mqtt_reader_queue(mqtt_config configuration);
	
protected:
	void subscribe(std::vector<std::string> const& channel_names, mqtt::callback_t callback);
	
	const mqtt_config m_configuration;
	
private:
	mqtt m_reader;
};

class mqtt_queue : public mqtt_reader_queue {
public:
	mqtt_queue(mqtt_config configuration);
	
protected:
	void publish(std::string const& name, std::string const& message);
	
private:
	mqtt m_publisher;
};

}
