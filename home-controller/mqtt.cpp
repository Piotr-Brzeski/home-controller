//
//  mqtt.cpp
//  home-controller
//
//  Created by Piotr Brzeski on 2023-12-17.
//  Copyright © 2023 Brzeski.net. All rights reserved.
//

#include "mqtt.h"
#include "exception.h"
#include <cpp-log/log.h>
#include <mosquitto.h>
#include <mutex>

using namespace home;

namespace {

std::mutex mutex;
int counter = 0;

void check(int result, char const* error_message) {
	if(result != MOSQ_ERR_SUCCESS) {
		throw exception(std::string(error_message) + ": " + ::mosquitto_strerror(result));
	}
}

constexpr int port = 1883;
constexpr int ping_interval = 10; //s

}

mosquitto_user::mosquitto_user() {
	auto lock = std::lock_guard(mutex);
	if(counter == 0) {
		check(::mosquitto_lib_init(), "mosquitto_lib_init failed");
	}
	++counter;
}

mosquitto_user::~mosquitto_user() {
	auto lock = std::lock_guard(mutex);
	if(--counter == 0) {
		::mosquitto_lib_cleanup();
	}
}

mqtt::mqtt() {
	m_mosq = ::mosquitto_new(nullptr, true, this);
	if(m_mosq == nullptr) {
		throw exception("mosquitto_new failed");
	}
}
mqtt::~mqtt() {
	unsubscribe();
	::mosquitto_destroy(m_mosq);
}

void mqtt::connect(std::string const& host) {
	if(m_connected) {
		throw exception("can not connect - already connected.");
	}
	auto res = ::mosquitto_connect(m_mosq, host.c_str(), port, ping_interval);
	check(res, "mosquitto_connect failed");
	m_connected = true;
}

void mqtt::publish(std::string const& channel, std::string const& message) {
	if(m_subscribed) {
		throw exception("can not publish - subscription is active.");
	}
	if(!m_connected) {
		throw exception("can not publish - not connected.");
	}
	execute([&, this](){ return ::mosquitto_publish(m_mosq, nullptr, channel.c_str(), static_cast<int>(message.size()), message.data(), 0, false); }, "mosquitto_publish failed");
	logger::log("MQTT publish [" + channel + "]: " + message);
}

std::string mqtt::get_message(std::string const& host, std::string const& channel) {
	auto init = mosquitto_user();
	auto message = std::string();
	auto res = ::mosquitto_subscribe_callback([](::mosquitto*, void* context, const ::mosquitto_message* msg){
		auto& message = *static_cast<std::string*>(context);
		message.append(static_cast<const char*>(msg->payload), msg->payloadlen);
		return 1;
	}, &message, channel.c_str(), 0, host.c_str(), port, nullptr, ping_interval, true, nullptr, nullptr, nullptr, nullptr);
	check(res, "mosquitto_subscribe_callback failed");
	logger::log("MQTT got message from " + host + " [" + channel + "]: " + message);
	return message;
}

void mqtt::subscribe(std::string const& channel, callback_t callback) {
	auto channels = std::vector<char*>(1, const_cast<char*>(channel.c_str()));
	subscribe(channels, callback);
}

void mqtt::subscribe(std::vector<std::string> const& channels, callback_t callback) {
	auto channel_names = std::vector<char*>();
	channel_names.reserve(channels.size());
	for(auto& channel_name: channels) {
		channel_names.push_back(const_cast<char*>(channel_name.c_str()));
	}
	subscribe(channel_names, callback);
}

void mqtt::unsubscribe() {
	if(m_subscribed) {
		// Ignore result for now
		::mosquitto_loop_stop(m_mosq, true);
		::mosquitto_message_callback_set(m_mosq, nullptr);
		m_subscription_callback = callback_t();
		m_subscribed = false;
	}
}

void mqtt::subscribe(std::vector<char*> const& channels, callback_t callback) {
	if(!m_connected) {
		throw exception("can not subscribe - not connected.");
	}
	if(m_subscribed) {
		throw exception("can not subscribe - subscription is active.");
	}
	if(channels.empty()) {
		throw exception("can not subscribe - channels list is empty.");
	}
	m_subscription_callback = callback;
	::mosquitto_message_callback_set(m_mosq, [](::mosquitto*, void* context, const ::mosquitto_message* msg){
		auto self = static_cast<mqtt*>(context);
		if(self != nullptr && self->m_subscription_callback) {
			auto channel = std::string(msg->topic);
			auto message = std::string(static_cast<const char*>(msg->payload), msg->payloadlen);
			logger::log("MQTT recv [" + channel + "]: " + message);
			self->m_subscription_callback(std::move(channel), std::move(message));
		}
	});
	execute([&, this](){ return ::mosquitto_subscribe_multiple(m_mosq, nullptr, static_cast<int>(channels.size()), channels.data(), 0, 0, nullptr); }, "mosquitto_subscribe_multiple failed");
	auto res = ::mosquitto_loop_start(m_mosq);
	check(res, "mosquitto_loop_start failed");
	m_subscribed = true;
}

void mqtt::execute(std::function<int()> operation, char const* error_message) {
	auto res = operation();
	if(res != MOSQ_ERR_SUCCESS) {
		if(m_connected) {
			res = ::mosquitto_reconnect(m_mosq);
			check(res, "mosquitto_reconnect failed");
			res = operation();
			check(res, error_message);
		}
	}
}
