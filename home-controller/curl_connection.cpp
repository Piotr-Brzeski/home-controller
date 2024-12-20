//
//  curl_connection.cpp
//  home-controller
//
//  Created by Piotr Brzeski on 2023-11-30.
//  Copyright © 2023-2024 brzeski.net. All rights reserved.
//

#include "curl_connection.h"
#include "exception.h"
#include <curl-ws/curl.h>
#include <mutex>

using namespace home;

namespace {

std::mutex mutex;
int counter = 0;

}

curl_connection::curl_connection() {
	auto lock = std::lock_guard(mutex);
	if(counter == 0) {
		error_buffer()[0] = '\0';
		check(::curl_global_init(CURL_GLOBAL_ALL));
	}
	++counter;
}

curl_connection::~curl_connection() {
	auto lock = std::lock_guard(mutex);
	if(--counter == 0) {
		::curl_global_cleanup();
	}
}

char* curl_connection::error_buffer() {
	thread_local char buffer[CURL_ERROR_SIZE];
	return buffer;
}

void curl_connection::check(int int_result) {
	auto result = static_cast<CURLcode>(int_result);
	if(result == CURLE_OK) {
		return;
	}
	auto error_message = "cURL error " + std::to_string(result) + ": ";
	if(error_buffer()[0] == '\0') {
		error_message += ::curl_easy_strerror(result);
	}
	else {
		error_message += error_buffer();
		if(error_message.back() == '\n') {
			error_message.pop_back();
		}
	}
	throw exception(error_message);
}

void curl_connection::add_header(::curl_slist*& headers, std::string const& header) {
	auto new_headers = ::curl_slist_append(headers, header.c_str());
	if(new_headers == nullptr) {
		throw exception("cURL add header failed");
	}
	headers = new_headers;
}

void curl_connection::configure(CURL* curl, ::curl_slist* headers) {
	error_buffer()[0] = '\0';
	check(::curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer()));
	// Allow self-signed certificates
	check(::curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0));
	// Allow any hostname
	check(::curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0));
	check(::curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers));
}

long curl_connection::send_request(CURL* curl) {
	error_buffer()[0] = '\0';
	check(::curl_easy_perform(curl));
	long status = 0;
	check(::curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status));
	return status;
}
