//
//  ws_connection.cpp
//  home-controller
//
//  Created by Piotr Brzeski on 2023-11-30.
//  Copyright © 2023-2026 Brzeski.net. All rights reserved.
//

#include "ws_connection.h"
#include "exception.h"
#include <cpp-log/log.h>
#include <curl/curl.h>
#include <cassert>
#include <thread>

using namespace home;

ws_connection::ws_connection(std::string const& url, std::string const& access_token)
	: m_url(url)
{
	add_header(m_headers, "Authorization: Bearer " + access_token);
}

ws_connection::~ws_connection() {
	try {
		stop();
	}
	catch(...) {
	}
	if(m_ws != nullptr) {
		::curl_easy_cleanup(m_ws);
	}
	::curl_slist_free_all(m_headers);
}

void ws_connection::start(std::function<void(std::string const&)> message_callback) {
	if(m_ws_state.valid()) {
		throw exception("Websocket connection is already started");
	}
	connect();
	m_ws_state = std::async(std::launch::async, [message_callback, this](){
		auto message = std::string();
		auto buffer = std::array<char, 1024>();
		while(true) {
			try {
				std::size_t size = 0;
				::curl_ws_frame const* meta = nullptr;
				auto status = ::curl_ws_recv(m_ws, buffer.data(), buffer.size(), &size, &meta);
				while(status == CURLE_AGAIN) {
					if(!m_select.wait(m_descriptor).contains(m_descriptor)) {
						assert(false);
						return;
					}
					status = ::curl_ws_recv(m_ws, buffer.data(), buffer.size(), &size, &meta);
				}
				check(status);
				message.append(buffer.data(), size);
				if(meta->bytesleft == 0 && (meta->flags & CURLWS_CONT) == 0) {
					message_callback(message);
					message.clear();
				}
			}
			catch(...) {
				message.clear();
				reconnect();
			}
		}
	});
}

void ws_connection::stop() {
	if(m_ws_state.valid()) {
		m_select.wake();
		m_ws_state.get();
	}
}

void ws_connection::connect() {
	if(m_ws != nullptr) {
		::curl_easy_cleanup(m_ws);
	}
	m_ws = ::curl_easy_init();
	if(m_ws == nullptr) {
		throw exception("cURL fatal error");
	}
	::curl_easy_setopt(m_ws, CURLOPT_CONNECT_ONLY, 2L);
	configure(m_ws, m_headers);
	check(::curl_easy_setopt(m_ws, CURLOPT_URL, m_url.c_str()));
	auto status = send_request(m_ws);
	auto status_str = std::to_string(status);
	logger::log("Websocket connect " + m_url + " : [" + status_str + "]");
	if(status != 101) {
		throw exception("Websocket connect failed with status " + status_str);
	}
	m_descriptor = get_socket();
}

void ws_connection::reconnect() {
	constexpr auto max_delay = std::chrono::seconds(30);
	auto delay = std::chrono::seconds(1);
	while(true) {
		logger::log("Websocket connection lost, reconnecting in " + std::to_string(delay.count()) + "s");
		std::this_thread::sleep_for(delay);
		try {
			connect();
			return;
		}
		catch(exception& e) {
			logger::log(std::string("Websocket reconnect failed: ") + e.what());
			delay = std::min(delay * 2, max_delay);
		}
	}
}

int ws_connection::get_socket() {
	curl_socket_t sockfd = -1;
	check(::curl_easy_getinfo(m_ws, CURLINFO_ACTIVESOCKET, &sockfd));
	return sockfd;
}
