//
//  device.h
//  home-controller
//
//  Created by Piotr Brzeski on 2024-12-01.
//  Copyright © 2024 Brzeski.net. All rights reserved.
//

#pragma once

#include "system_base.h"
#include <vector>
#include <functional>
#include <atomic>
#include <thread>
#include <cassert>

namespace home {

template<typename State>
class device {
public:
	using callback = std::function<void()>;
	
	device(std::string const& name, system_base& system, std::vector<callback> update_callbacks = {})
		: m_name(name)
		, m_system(system)
		, m_update_callbacks(std::move(update_callbacks))
	{
	}
	
	void set(State state) {
		thread_checker();
		m_system.set(m_name, state);
	}
	
	// Thread-safe methods
	State state() const {
		return m_state;
	}
	void update(State state) {
		thread_checker();
		if(state != m_state) {
			m_state = state;
			for(auto& callback : m_update_callbacks) {
				callback();
			}
		}
	}
	
private:
	void thread_checker() {
		static auto first_thread = std::this_thread::get_id();
		auto thread = std::this_thread::get_id();
		assert(thread == first_thread);
	}
	
	std::string           m_name;
	system_base&          m_system;
	std::atomic<State>    m_state = {};
	std::vector<callback> m_update_callbacks;
};

using outlet = device<bool>;

class bulb: public device<std::uint8_t> {
public:
	static constexpr std::uint8_t zero_brightness = 0;
	static constexpr std::uint8_t min_brightness = 1;
	static constexpr std::uint8_t max_brightness = 7;
	
	using device::device;
};

}
