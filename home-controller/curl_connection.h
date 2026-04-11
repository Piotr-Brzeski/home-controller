//
//  curl_connection.h
//  home-controller
//
//  Created by Piotr Brzeski on 2023-11-30.
//  Copyright © 2023-2026 Brzeski.net. All rights reserved.
//

#pragma once

#include <string>
#include <array>

extern "C" {
struct curl_slist;
}

namespace home {

class curl_connection {
public:
	curl_connection();
	~curl_connection();
	
protected:
	char* error_buffer();
	void check(int result);
	static void add_header(::curl_slist*& headers, std::string const& header);
	void configure(void* curl, ::curl_slist* headers);
	long send_request(void* curl);

private:
	std::array<char, 256> m_error_buffer;
};

}
