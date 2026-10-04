//
//  mqtt.h
//  home-controller
//
//  Created by Piotr Brzeski on 2023-12-17.
//  Copyright © 2023-2026 Brzeski.net. All rights reserved.
//

#pragma once

#include <functional>
#include <string>
#include <vector>
#include <mutex>

struct mosquitto;

namespace home {

class mosquitto_user {
public:
	mosquitto_user();
	~mosquitto_user();
};

class mqtt : public mosquitto_user {
public:
	mqtt();
	~mqtt();
	
	mqtt(mqtt const&) = delete;
	mqtt(mqtt&&) = delete;
	mqtt& operator=(mqtt const&) = delete;
	mqtt& operator=(mqtt&&) = delete;
	
	using callback_t = std::function<void(std::string channel, std::string message)>;
	
	static std::string get_message(std::string const& host, std::string const& channel);
	void connect(std::string const& host);
	void disconnect();
	void publish(std::string const& channel, std::string const& message);
	void subscribe(std::string const& channel, callback_t callback);
	void subscribe(std::vector<std::string> const& channels, callback_t callback);
	void unsubscribe();
	
private:
	void subscribe(std::vector<char*> const& channels, callback_t callback);
	int internal_subscribe();

	mosquitto*               m_mosq = nullptr;
	bool                     m_connected = false;
	// Subscription state is shared with the network loop thread
	std::mutex               m_subscription_mutex;
	callback_t               m_subscription_callback;
	std::vector<std::string> m_subscription_channels;
	bool                     m_subscribed = false;
};

}
