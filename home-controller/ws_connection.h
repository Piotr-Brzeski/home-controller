//
//  ws_connection.h
//  home-controller
//
//  Created by Piotr Brzeski on 2023-11-30.
//  Copyright © 2023 brzeski.net. All rights reserved.
//

#pragma once

#include "curl_connection.h"
#include <cpp-network/select.h>
#include <future>
#include <functional>

namespace home {

class ws_connection : public curl_connection {
public:
	ws_connection(std::string const& url, std::string const& access_token);
	~ws_connection();
	
	void start(std::function<void(std::string const&)> message_callback);
	void stop();
	
private:
	void connect();
	int get_socket();
	
	std::string       m_url;
	void*             m_ws = nullptr;
	::curl_slist*     m_headers = nullptr;
	std::future<void> m_ws_state;
	network::select   m_select;
	int               m_descriptor = -1;
};

}
