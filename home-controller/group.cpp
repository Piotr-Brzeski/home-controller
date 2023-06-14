//
//  group.cpp
//  home-controller
//
//  Created by Piotr Brzeski on 2023-06-13.
//  Copyright © 2023 Brzeski.net. All rights reserved.
//

#include "group.h"
#include <algorithm>

using namespace home;

void group::toggle() {
	bool is_on = std::ranges::any_of(m_members, [](auto& m){ return m.brightness() != 0; });
	std::ranges::for_each(m_members, [is_on](auto& m){ m.set(!is_on); });
}

void group::increase() {
	auto member = m_members.begin();
	auto min_brightness = member->brightness();
	for(auto it = member + 1; it != m_members.end(); ++it) {
		auto brightness = it->brightness();
		if(brightness < min_brightness) {
			min_brightness = brightness;
			member = it;
		}
	}
	member->increase();
}

void group::decrease() {
	auto member = m_members.rbegin();
	auto max_brightness = member->brightness();
	for(auto it = member + 1; it != m_members.rend(); ++it) {
		auto brightness = it->brightness();
		if(brightness > max_brightness) {
			max_brightness = brightness;
			member = it;
		}
	}
	member->decrease();
}
