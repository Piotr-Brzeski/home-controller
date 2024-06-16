//
//  switches_controller.cpp
//  home-controller
//
//  Created by Piotr Brzeski on 2024-06-15.
//  Copyright © 2024 Brzeski.net. All rights reserved.
//

#include "switches_controller.h"
#include "exception.h"
#include "json.h"
#include <cassert>

using namespace home;

void switches_controller::add(std::string const& name, callback_t toggle, callback_t increase, callback_t decrease) {
	assert(!name.empty());
	assert(toggle);
	assert(increase);
	assert(decrease);
	if(!m_callbacks.emplace(name, callbacks{toggle, increase, decrease}).second) {
		throw exception("Switch \"" + name + "\" is already configured.");
	}
}

void switches_controller::start() {
	if(m_callbacks.empty()) {
		return;
	}
	auto names = std::vector<std::string>();
	names.reserve(m_callbacks.size());
	for(auto& callback : m_callbacks) {
		names.push_back(callback.first);
	}
	subscribe(names, [this](std::string const& channel, std::string message) {
		try {
			auto it = m_callbacks.find(channel);
			if(it == m_callbacks.end()) {
				assert(false);
				return;
			}
			auto& actions = it->second;
			static auto const action_key = std::string("action");
			static auto const action_toggle = std::string("toggle");
			static auto const action_increase = std::string("brightness_up_click");
			static auto const action_decrease = std::string("brightness_down_click");
			auto event_json = json(std::move(message));
			auto action_value = event_json.get().get(action_key);
			// TODO: Handle (log) error
			assert(action_value);
			auto action = action_value->get_string();
			if(action == action_toggle) {
				actions.toggle();
			}
			else if(action == action_increase) {
				actions.increase();
			}
			else if(action == action_decrease) {
				actions.decrease();
			}
		}
		catch(...) {
			// TODO: Log error
			assert(false);
		}
	});
}
