//
//  http_connection.h
//  home-controller
//
//  Created by Piotr Brzeski on 2023-10-31.
//  Copyright © 2023-2026 Brzeski.net. All rights reserved.
//

#pragma once

#include "curl_connection.h"
#include <future>
#include <vector>

namespace home {

class http_get : public curl_connection {
public:
	http_get(std::string const& access_token);
	~http_get();
	
	http_get(http_get const&) = delete;
	http_get(http_get&&) = delete;
	
	std::string const& get(std::string const& url);
	
	std::string const& access_token() const {
		return m_access_token;
	}
	
private:
	void*             m_get = nullptr;
	::curl_slist*     m_get_headers = nullptr;
	std::string       m_response;
	std::string       m_get_url;
	std::string const m_access_token;

};

class http_patch : public curl_connection {
public:
	http_patch(std::string const& url, std::string const& access_token);
	http_patch(http_patch&&);
	~http_patch();
	
	http_patch(http_patch const&) = delete;
	
	void send(std::vector<std::string> patches);
	
private:
	void*             m_patch = nullptr;
	::curl_slist*     m_patch_headers = nullptr;
	std::string       m_patch_url;
	std::future<void> m_result;
};

}
