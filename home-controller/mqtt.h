//
//  mqtt.h
//  home-controller
//
//  Created by Piotr Brzeski on 2023-12-17.
//  Copyright © 2023 Brzeski.net. All rights reserved.
//

#pragma once

#include <functional>
#include <string>
#include <vector>

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
	void publish(std::string const& channel, std::string const& message);
	void subscribe(std::string const& channel, callback_t callback);
	void subscribe(std::vector<std::string> const& channels, callback_t callback);
	void unsubscribe();
	
private:
	void subscribe(std::vector<char*> const& channels, callback_t callback);
	void execute(std::function<int()> operation, char const* error_message);
	
	mosquitto* m_mosq = nullptr;
	callback_t m_subscription_callback;
	bool       m_connected = false;
	bool       m_subscribed = false;
};

}
