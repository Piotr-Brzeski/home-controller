//
//  group.h
//  home-controller
//
//  Created by Piotr Brzeski on 2023-06-13.
//  Copyright © 2023 Brzeski.net. All rights reserved.
//

#pragma once

#include <functional>
#include <vector>
#include <cstdint>

namespace home {

class group {
public:
	struct member {
		std::function<std::uint8_t()> const brightness;
		std::function<void(bool)> const     set;
		std::function<void()> const         increase;
		std::function<void()> const         decrease;
	};
	
	void add(member&& new_member) {
		m_members.push_back(std::move(new_member));
	}
	
	void toggle();
	void increase();
	void decrease();
	
private:
	std::vector<member> m_members;
	
};

}
