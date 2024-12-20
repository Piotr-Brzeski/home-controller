//
//  single_outlet.h
//  home-controller
//
//  Created by Piotr Brzeski on 2024-12-01.
//  Copyright © 2024 Brzeski.net. All rights reserved.
//

namespace home {

class single_outlet {
public:
//	using outlet_get = std::function<bool()>;
	using outlet_set = std::function<void(bool)>;
	
	single_outlet(outlet_set set)
		: m_set(set)
	{
	}
	
	~single_outlet() {
	}
	
	void enable() {
		m_set(true);
	}
	void disable() {
		m_set(false);
	}
	
private:
//	outlet_get m_get;
	outlet_set m_set;
};

}
