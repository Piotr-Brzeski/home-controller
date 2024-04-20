//
//  group.cpp
//  home-controller
//
//  Created by Piotr Brzeski on 2023-06-13.
//  Copyright © 2023-2024 Brzeski.net. All rights reserved.
//

#include "group.h"
#include <algorithm>

using namespace home;

void group::toggle() {
	bool is_on = std::ranges::any_of(m_members, [](auto& m){ return m->brightness() != 0; });
	auto new_brightness = is_on ? bulb::zero_brightness : bulb::max_brightness;
	std::ranges::for_each(m_members, [new_brightness](auto& m){ m->set(new_brightness); });
}

void group::increase() {
	auto member = m_members.begin();
	auto min_brightness = (*member)->brightness();
	for(auto it = member + 1; it != m_members.end(); ++it) {
		auto brightness = (*it)->brightness();
		if(brightness < min_brightness) {
			min_brightness = brightness;
			member = it;
		}
	}
	if(min_brightness < bulb::max_brightness) {
		(*member)->set(min_brightness + 1);
	}
}

void group::decrease() {
	auto member = m_members.rbegin();
	auto max_brightness = (*member)->brightness();
	for(auto it = member + 1; it != m_members.rend(); ++it) {
		auto brightness = (*it)->brightness();
		if(brightness > max_brightness) {
			max_brightness = brightness;
			member = it;
		}
	}
	if(max_brightness > bulb::zero_brightness) {
		(*member)->set(max_brightness - 1);
	}
}
