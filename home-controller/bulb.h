//
//  bulb.h
//  home-controller
//
//  Created by Piotr Brzeski on 2023-12-17.
//  Copyright © 2023-2024 Brzeski.net. All rights reserved.
//

#pragma once

#include "system_base.h"
#include <atomic>

namespace home {

class bulb {
public:
	static constexpr std::uint8_t zero_brightness = 0;
	static constexpr std::uint8_t min_brightness = 1;
	static constexpr std::uint8_t max_brightness = 7;
	
	bulb(std::string const& name, system_base& system);
	
//	std::string const& name() const;
	system_base& system();
	
	std::uint8_t brightness() const;
	void set(std::uint8_t brightness);
	void update(std::uint8_t brightness);
	
private:
	std::string               m_name;
	system_base&              m_system;
	std::atomic<std::uint8_t> m_brightness = 0;
};

}
