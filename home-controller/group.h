//
//  group.h
//  home-controller
//
//  Created by Piotr Brzeski on 2023-06-13.
//  Copyright © 2023-2024 Brzeski.net. All rights reserved.
//

#pragma once

#include "bulb.h"
#include <vector>
#include <mutex>
#include <thread>
#include <chrono>
#include <optional>
#include <condition_variable>

namespace home {

class group {
public:
	using bulb_get = std::function<std::uint8_t()>;
	using bulb_set = std::function<void(std::uint8_t)>;
	
	group();
	~group();
	
	void add(bulb_get get, bulb_set set);
	
	void toggle();
	void increase();
	void decrease();
	
private:
	using clock = std::chrono::steady_clock;
	
	std::size_t size() const {
		return m_members.size();
	}
	
	void prepare_status();
	void send();
	void send_status();
	void set(std::size_t index, std::uint8_t brightness);
	
	struct bulb_operations {
		bulb_get get;
		bulb_set set;
	};
	
	std::vector<bulb_operations>     m_members;
	std::condition_variable          m_condition;
	std::mutex                       m_mutex;
	std::thread                      m_thread;
	bool                             m_run = true;
	clock::time_point                m_last_set;
	std::optional<clock::time_point> m_send_time;
	std::vector<std::uint8_t>        m_status;
	std::vector<bool>                m_to_set;
	
};

}
