//
//  bulbs_group.cpp
//  home-controller
//
//  Created by Piotr Brzeski on 2023-06-13.
//  Copyright © 2023-2024 Brzeski.net. All rights reserved.
//

#include "bulbs_group.h"
#include <algorithm>
#include <numeric>
#include <cassert>

using namespace home;

namespace {

constexpr auto timeout = std::chrono::milliseconds(600);

}

bulbs_group::bulbs_group() {
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

bulbs_group::~bulbs_group() {
	{
		auto lock = std::lock_guard(m_mutex);
		m_run = false;
	}
	m_condition.notify_one();
	m_thread.join();
}

void bulbs_group::add(bulb_get get, bulb_set set) {
	m_members.push_back({get, set});
	m_status.push_back(bulb::zero_brightness);
	m_to_set.push_back(false);
}

void bulbs_group::toggle() {
	auto lock = std::lock_guard(m_mutex);
	prepare_status(false);
	{
		bool is_on = std::ranges::any_of(m_status, [](auto& m){ return m != bulb::zero_brightness; });
		auto new_brightness = is_on ? bulb::zero_brightness : bulb::max_brightness;
		for(std::size_t i = 0; i < size(); ++i) {
			set(i, new_brightness);
		}
	}
	send();
}

void bulbs_group::increase() {
	auto lock = std::lock_guard(m_mutex);
	prepare_status(false);
	{
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
	}
	send();
}

void bulbs_group::decrease() {
	auto lock = std::lock_guard(m_mutex);
	prepare_status(false);
	{
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
	}
	send();
}

// 0 - 100
void bulbs_group::set_brigntness(std::uint8_t brightness) {
	if(brightness > 100) {
		assert(false);
		brightness = 100;
	}
	auto lock = std::lock_guard(m_mutex);
	prepare_status(false);
	{
		auto max = size() * bulb::max_brightness;
		auto new_brightness = std::round(brightness/100.0 * max);
		auto low_value = static_cast<std::uint8_t>(new_brightness / size());
		auto number_of_high_digits = static_cast<std::size_t>(new_brightness) % size();
		auto new_bulb_brightness = std::vector<std::uint8_t>(number_of_high_digits, low_value + 1);
		new_bulb_brightness.resize(size(), low_value);
		for(auto i = 0U; i < size(); ++i) {
			set(i, new_bulb_brightness[i]);
		}
	}
	send();
}

std::uint8_t bulbs_group::get_brightness() {
	auto lock = std::lock_guard(m_mutex);
	prepare_status(true);
	{
		auto sum = std::reduce(m_status.begin(), m_status.end());
		auto max = size() * bulb::max_brightness;
		auto brightness = 100.0 * sum / max;
		auto result = static_cast<std::uint8_t>(brightness);
		assert(result <= 100);
		return result;
	}
}

void bulbs_group::prepare_status(bool force_get) {
	if(force_get || (!m_send_time && clock::now() - m_last_set > timeout)) {
		for(std::size_t i = 0; i < m_members.size(); ++i) {
			m_status[i] = m_members[i].get();
			m_to_set[i] = false;
		}
	}
}

void bulbs_group::send() {
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

void bulbs_group::send_status() {
	for(std::size_t i = 0; i < size(); ++i) {
		if(m_to_set[i]) {
			m_members[i].set(m_status[i]);
			m_to_set[i] = false;
		}
	}
	m_last_set = clock::now();
}

void bulbs_group::set(std::size_t index, std::uint8_t brightness) {
	m_status[index] = brightness;
	m_to_set[index] = true;
}
