//
//  system_base.h
//  home-controller
//
//  Created by Piotr Brzeski on 2024-04-14.
//

#pragma once

#include <string>
#include <cstdint>
#include <functional>

namespace home {

enum class device_type { bulb, none };

class system_base {
public:
	using callback = std::function<void(std::string const&, std::uint8_t)>;
	
	virtual ~system_base() {
	}
	void set(callback callback) {
		m_callback = callback;
	}
	
	virtual device_type get_device_type(std::string const& name) const = 0;
	virtual void start(std::vector<std::string> const& names) = 0;
	virtual void ping(std::string const& name) = 0;
	virtual void set(std::string const& name, std::uint8_t brightness) = 0;
	
protected:
	callback m_callback;
};

} // namespace home

