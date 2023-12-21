//
//  mqtt_bulb.h
//  home-controller
//
//  Created by Piotr Brzeski on 2023-12-17.
//  Copyright © 2023 Brzeski.net. All rights reserved.
//

#pragma once

#include "mqtt.h"
#include <cstdint>
#include <atomic>

namespace home {

class mqtt_bulb {
public:
	static constexpr std::uint8_t zero_brightness = 0;
	static constexpr std::uint8_t min_brightness = 1;
	static constexpr std::uint8_t max_brightness = 7;
	
	mqtt_bulb(std::string const& name, mqtt& publisher, std::string const& queue_name);
	
	void trigger_update();
	void update(std::string state);
	
	std::string const& name() const;
	
	std::uint8_t brightness() const;
	void set(std::uint8_t brightness);
	void set(bool enabled);
	void toggle();
	void increase();
	void decrease();
	
private:
	std::string               m_name;
	std::atomic<std::uint8_t> m_brightness = 0;
	mqtt&                     m_publisher;
	std::string               m_set_channel;
};

}
