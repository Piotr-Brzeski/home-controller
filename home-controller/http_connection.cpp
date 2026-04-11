//
//  http_connection.cpp
//  home-controller
//
//  Created by Piotr Brzeski on 2023-10-31.
//  Copyright © 2023-2026 Brzeski.net. All rights reserved.
//

#include "http_connection.h"
#include "exception.h"
#include <cpp-log/log.h>
#include <curl/curl.h>

using namespace home;

namespace {

std::size_t write_cb(void* data, std::size_t size, std::size_t nmemb, void* context) {
	auto& result = *static_cast<std::string*>(context);
	auto data_size = size*nmemb;
	result.append(static_cast<char const*>(data), data_size);
	return data_size;
}

}

// MARK: HTTP GET

http_get::http_get(std::string const& access_token)
	: m_access_token(access_token)
{
	error_buffer()[0] = '\0';
	m_get = ::curl_easy_init();
	if(m_get == nullptr) {
		throw exception("cURL fatal error");
	}
	add_header(m_get_headers, "Authorization: Bearer " + access_token);
	configure(m_get, m_get_headers);
	check(::curl_easy_setopt(m_get, CURLOPT_WRITEFUNCTION, write_cb));
	check(::curl_easy_setopt(m_get, CURLOPT_WRITEDATA, &m_response));
}

http_get::~http_get() {
	::curl_easy_cleanup(m_get);
	::curl_slist_free_all(m_get_headers);
}

std::string const& http_get::get(std::string const& url) {
	if(url != m_get_url) {
		error_buffer()[0] = '\0';
		check(::curl_easy_setopt(m_get, CURLOPT_URL, url.c_str()));
		m_get_url = url;
	}
	m_response.clear();
	auto status = send_request(m_get);
	auto status_str = std::to_string(status);
	logger::log("HTTPS GET " + m_get_url + " : [" + status_str + "] " + m_response);
	if(status == 200) {
		return m_response;
	}
	throw exception("HTTP GET failed with status " + status_str);
}


// MARK: HTTP PATCH

http_patch::http_patch(std::string const& url, std::string const& access_token)
	: m_patch_url(url)
{
	error_buffer()[0] = '\0';
	m_patch = ::curl_easy_init();
	if(m_patch == nullptr) {
		throw exception("cURL fatal error");
	}
	add_header(m_patch_headers, "Authorization: Bearer " + access_token);
	add_header(m_patch_headers, "Content-Type: application/json");
	configure(m_patch, m_patch_headers);
	check(::curl_easy_setopt(m_patch, CURLOPT_URL, url.c_str()));
	check(::curl_easy_setopt(m_patch, CURLOPT_CUSTOMREQUEST, "PATCH"));
}

http_patch::http_patch(http_patch&& patch)
	: m_patch(patch.m_patch)
	, m_patch_headers(patch.m_patch_headers)
	, m_patch_url(std::move(patch.m_patch_url))
	, m_result(std::move(patch.m_result))
{
	patch.m_patch = nullptr;
	patch.m_patch_headers = nullptr;
}

http_patch::~http_patch() {
	try {
		if(m_result.valid()) {
			m_result.get();
		}
	}
	catch(...) {
	}
	::curl_easy_cleanup(m_patch);
	::curl_slist_free_all(m_patch_headers);
}

void http_patch::send(std::vector<std::string> patches) {
	if(m_result.valid()) {
		m_result.get();
	}
	m_result = std::async(std::launch::async, [patches = std::move(patches), this](){
		error_buffer()[0] = '\0';
		for(auto& patch : patches) {
			check(::curl_easy_setopt(m_patch, CURLOPT_POSTFIELDS, patch.c_str()));
			check(::curl_easy_setopt(m_patch, CURLOPT_POSTFIELDSIZE, patch.size()));
			auto status = send_request(m_patch);
			auto status_str = std::to_string(status);
			logger::log("HTTPS PATCH " + m_patch_url + " " + patch + " : [" + status_str + "]");
			if(status != 202) {
				throw exception("HTTP PATCH failed with status " + status_str);
			}
		}
	});
}
