//
//  commands_queue.h
//  home-controller
//
//  Created by Piotr Brzeski on 2024-04-16.
//  Copyright © 2024 Brzeski.net. All rights reserved.
//

#pragma once

#include <functional>
#include <chrono>
#include <vector>
#include <map>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <optional>

namespace home {

class commands_queue {
public:
	using command = std::function<void()>;
	using time_point = std::chrono::steady_clock::time_point;
	
	commands_queue();
	~commands_queue();
	
	void execute(command cmd);
	void execute(void* id, command cmd);
	void execute_and_set(void* id, command cmd, time_point time);
	
private:
	bool get(std::vector<command>& commands);
	
	struct command_with_time {
		command    cmd;
		time_point time;
	};
	
	bool                               m_run = true;
	std::vector<command>               m_commands;
	std::map<void*, command_with_time> m_queue;
	std::optional<time_point>          m_next_wake_time;
	std::mutex                         m_mutex;
	std::condition_variable            m_condition;
	std::thread                        m_thread;
};

}
