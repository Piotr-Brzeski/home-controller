//
//  dirigera_system.h
//  home-controller
//
//  Created by Piotr Brzeski on 2024-11-30.
//  Copyright © 2024 Brzeski.net. All rights reserved.
//

#include "system_base.h"
#include "http_connection.h"
#include "ws_connection.h"
#include <map>

namespace home {

struct dirigera_config {
	std::string address;
	std::string access_token;
};

class dirigera_system: public system_base {
public:
	dirigera_system(dirigera_config const& configuration);
	dirigera_system(std::string const& address, std::string const& access_token);
	
	device_type get_device_type(std::string const& name) const override;
	void start(std::vector<std::string> const& names) override;
	void ping(std::string const& name) override;
	void set(std::string const& name, std::uint8_t brightness) override;
	
private:
	void enumerate_devices();
	void call(std::string const& channel, std::string message);
	
	// name => id
	std::map<std::string, std::string> m_outlets;
	std::map<std::string, http_patch>  m_patch_connections;
	std::string const                  m_uri;
	http_get                           m_get_connection;
	ws_connection                      m_ws;

};

}
