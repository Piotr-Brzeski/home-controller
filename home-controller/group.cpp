//
//  group.cpp
//  home-controller
//
//  Created by Piotr Brzeski on 2023-06-13.
//  Copyright © 2023-2024 Brzeski.net. All rights reserved.
//

#include "group.h"
#include <algorithm>
#include <cassert>

using namespace home;

namespace {

constexpr auto timeout = std::chrono::milliseconds(600);

}

group::group() {
	m_thread = std::thread([this](){
		auto lock = std::unique_lock(m_mutex);
		while(m_run) {
			if(m_send_time) {
				if((*m_send_time) < clock::now()) {
					send_status();
					m_send_time.reset();
				}
				else {
					m_condition.wait_until(lock, *m_send_time);
				}
			}
			else {
				m_condition.wait(lock);
			}
		}
	});
}

group::~group() {
	{
		auto lock = std::lock_guard(m_mutex);
		m_run = false;
	}
	m_condition.notify_one();
	m_thread.join();
}

void group::add(bulb_get get, bulb_set set) {
	m_members.push_back({get, set});
	m_status.push_back(bulb::zero_brightness);
	m_to_set.push_back(false);
}

void group::toggle() {
	auto lock = std::lock_guard(m_mutex);
	prepare_status();
	
	bool is_on = std::ranges::any_of(m_status, [](auto& m){ return m != bulb::zero_brightness; });
	auto new_brightness = is_on ? bulb::zero_brightness : bulb::max_brightness;
	for(std::size_t i = 0; i < size(); ++i) {
		set(i, new_brightness);
	}
	send();
}

void group::increase() {
	auto lock = std::lock_guard(m_mutex);
	prepare_status();
	
	std::size_t index = 0;
	auto min_brightness = m_status[index];
	for(std::size_t i = 1; i < size(); ++i) {
		auto brightness = m_status[i];
		if(brightness < min_brightness) {
			min_brightness = brightness;
			index = i;
		}
	}
	if(min_brightness < bulb::max_brightness) {
		set(index, min_brightness + 1);
	}
	send();
}

void group::decrease() {
	auto lock = std::lock_guard(m_mutex);
	prepare_status();
	assert(size() > 0);
	std::size_t index = size() - 1;
	auto max_brightness = m_status[index];
	for(auto i = index - 1; i < size(); --i) {
		auto brightness = m_status[i];
		if(brightness > max_brightness) {
			max_brightness = brightness;
			index = i;
		}
	}
	if(max_brightness > bulb::zero_brightness) {
		set(index, max_brightness - 1);
	}
	send();
}

void group::prepare_status() {
	if(!m_send_time && clock::now() - m_last_set > timeout) {
		for(std::size_t i = 0; i < m_members.size(); ++i) {
			m_status[i] = m_members[i].get();
			m_to_set[i] = false;
		}
	}
}

void group::send() {
	if(m_send_time) {
		// Current status will be sent
		return;
	}
	auto now = clock::now();
	if(now - m_last_set > timeout) {
		send_status();
		return;
	}
	m_send_time = now + timeout;
	m_condition.notify_one();
}

void group::send_status() {
	for(std::size_t i = 0; i < size(); ++i) {
		if(m_to_set[i]) {
			m_members[i].set(m_status[i]);
			m_to_set[i] = false;
		}
	}
	m_last_set = clock::now();
}

void group::set(std::size_t index, std::uint8_t brightness) {
	m_status[index] = brightness;
	m_to_set[index] = true;
}
