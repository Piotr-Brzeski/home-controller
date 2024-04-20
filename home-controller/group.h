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
	void add(bulb* new_member) {
		m_members.push_back(new_member);
	}
	
	void toggle();
	void increase();
	void decrease();
	
private:
	std::vector<bulb*> m_members;
	
};

}
