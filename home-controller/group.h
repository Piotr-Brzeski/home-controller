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

namespace home {

class group {
public:
	using bulb_get = std::function<std::uint8_t()>;
	using bulb_set = std::function<void(std::uint8_t)>;

	void add(bulb_get get, bulb_set set) {
		m_members.push_back({get, set});
	}
	
	void toggle();
	void increase();
	void decrease();
	
private:
	struct bulb_operations {
			bulb_get get;
			bulb_set set;
	};
	std::vector<bulb_operations> m_members;
	
};

}
