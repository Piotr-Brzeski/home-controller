//
//  mqtt_queue.cpp
//  home-controller
//
//  Created by Piotr Brzeski on 2024-06-15.
//  Copyright © 2023-2024 Brzeski.net. All rights reserved.
//

#include "mqtt_queue.h"

using namespace home;


mqtt_reader_queue::mqtt_reader_queue(mqtt_config configuration)
	: m_configuration(std::move(configuration))
{
	m_reader.connect(m_configuration.address);
}

void mqtt_reader_queue::subscribe(std::vector<std::string> const& channel_names, mqtt::callback_t callback) {
	auto channels = std::vector<std::string>();
	channels.reserve(channel_names.size());
	for(auto& channel_name : channel_names) {
		channels.push_back(m_configuration.queue_name + '/' + channel_name);
	}
	m_reader.subscribe(channels, [callback, this](std::string const& channel, std::string const& message) {
		auto name = channel.substr(m_configuration.queue_name.size() + 1);
		callback(name, message);
	});
}

mqtt_queue::mqtt_queue(mqtt_config configuration)
	: mqtt_reader_queue(std::move(configuration))
{
	m_publisher.connect(m_configuration.address);
}

void mqtt_queue::publish(std::string const& name, std::string const& message) {
	auto channel = m_configuration.queue_name + '/' + name + "/set";
	m_publisher.publish(channel, message);
}
