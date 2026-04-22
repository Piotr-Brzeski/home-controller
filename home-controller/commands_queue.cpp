//
//  commands_queue.cpp
//  home-controller
//
//  Created by Piotr Brzeski on 2024-04-16.
//  Copyright © 2024-2026 Brzeski.net. All rights reserved.
//

#include "commands_queue.h"
#include <cpp-log/log.h>

using namespace home;

commands_queue::commands_queue() {
	m_thread = std::thread([this](){
		auto commands = std::vector<command>();
		auto lock = std::unique_lock(m_mutex);
		while(get(commands) || m_run) {
			lock.unlock();
			for(auto& cmd : commands) {
				try {
					cmd();
				}
				catch(std::exception& e) {
					logger::log(std::string("Command failed: ") + e.what());
				}
				catch(...) {
					logger::log("Command failed with unknown exception");
				}
			}
			lock.lock();
			if(m_run && m_commands.empty()) {
				if(m_next_wake_time) {
					m_condition.wait_until(lock, *m_next_wake_time);
				}
				else {
					m_condition.wait(lock);
				}
			}
		}
	});
}

commands_queue::~commands_queue() {
	{
		std::lock_guard lock(m_mutex);
		m_run = false;
		m_condition.notify_one();
	}
	m_thread.join();
}

void commands_queue::execute(command cmd) {
	std::lock_guard lock(m_mutex);
	m_commands.push_back(cmd);
	m_condition.notify_one();
}

void commands_queue::execute(void* id, command cmd) {
	std::lock_guard lock(m_mutex);
	m_commands.push_back(cmd);
	m_queue.erase(id);
	m_condition.notify_one();
}

void commands_queue::execute_and_set(void* id, command cmd, time_point time) {
	std::lock_guard lock(m_mutex);
	m_commands.push_back(cmd);
	m_queue[id] = {cmd, time};
	m_condition.notify_one();
}

/// Get commands to be executed now and set the next wake time
/// Must be called with m_mutex locked
bool commands_queue::get(std::vector<command>& commands) {
	commands.clear();
	std::swap(m_commands, commands);
	m_next_wake_time = std::nullopt;
	auto now = std::chrono::steady_clock::now();
	for(auto it = m_queue.begin(); it != m_queue.end();) {
		auto execution_time = it->second.time;
		if(execution_time > now) {
			if(m_next_wake_time) {
				m_next_wake_time = std::min(*m_next_wake_time, execution_time);
			}
			else {
				m_next_wake_time = execution_time;
			}
			++it;
		}
		else {
			commands.push_back(std::move(it->second.cmd));
			it = m_queue.erase(it);
		}
	}
	return !commands.empty();
}
