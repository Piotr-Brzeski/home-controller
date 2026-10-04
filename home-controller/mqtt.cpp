//
//  mqtt.cpp
//  home-controller
//
//  Created by Piotr Brzeski on 2023-12-17.
//  Copyright © 2023-2026 Brzeski.net. All rights reserved.
//

#include "mqtt.h"
#include "exception.h"
#include <cpp-log/log.h>
#include <mosquitto.h>
#include <mutex>
#include <cassert>

using namespace home;

namespace {

std::mutex mutex;
int counter = 0;

void check(int result, char const* error_message) {
	if(result != MOSQ_ERR_SUCCESS) {
		throw exception(std::string(error_message) + ": " + ::mosquitto_strerror(result));
	}
}

std::vector<char*> c_strings(std::vector<std::string> const& strings) {
	auto result = std::vector<char*>();
	result.reserve(strings.size());
	for(auto& str : strings) {
		result.push_back(const_cast<char*>(str.c_str()));
	}
	return result;
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
	::mosquitto_reconnect_delay_set(m_mosq, 1, 30, true);
	::mosquitto_connect_callback_set(m_mosq, [](::mosquitto*, void* context, int rc){
		auto self = static_cast<mqtt*>(context);
		if(rc == 0) {
			logger::log("MQTT connected");
			auto lock = std::lock_guard(self->m_subscription_mutex);
			if(self->m_subscribed) {
				auto res = self->internal_subscribe();
				if(res != MOSQ_ERR_SUCCESS) {
					logger::log(std::string("MQTT resubscribe failed: ") + ::mosquitto_strerror(res));
				}
			}
		}
		else {
			logger::log(std::string("MQTT connection failed: ") + ::mosquitto_connack_string(rc));
		}
	});
	::mosquitto_disconnect_callback_set(m_mosq, [](::mosquitto*, void*, int rc){
		if(rc != 0) {
			logger::log("MQTT connection lost (reason=" + std::to_string(rc) + ")");
		}
	});
	::mosquitto_message_callback_set(m_mosq, [](::mosquitto*, void* context, const ::mosquitto_message* msg){
		auto self = static_cast<mqtt*>(context);
		auto callback = callback_t();
		{
			auto lock = std::lock_guard(self->m_subscription_mutex);
			callback = self->m_subscription_callback;
		}
		if(callback) {
			auto channel = std::string(msg->topic);
			auto message = std::string(static_cast<const char*>(msg->payload), msg->payloadlen);
			logger::log("MQTT recv [" + channel + "]: " + message);
			callback(std::move(channel), std::move(message));
		}
	});
}

mqtt::~mqtt() {
	disconnect();
	::mosquitto_destroy(m_mosq);
}

void mqtt::connect(std::string const& host) {
	if(m_connected) {
		throw exception("can not connect - already connected.");
	}
	logger::log("MQTT connect to " + host);
	auto res = ::mosquitto_connect(m_mosq, host.c_str(), port, ping_interval);
	check(res, "mosquitto_connect failed");
	// Network loop sends keepalive pings and reconnects automatically
	res = ::mosquitto_loop_start(m_mosq);
	if(res != MOSQ_ERR_SUCCESS) {
		::mosquitto_disconnect(m_mosq);
	}
	check(res, "mosquitto_loop_start failed");
	m_connected = true;
}

/// Stops the network loop - no callback is running when it returns
void mqtt::disconnect() {
	if(m_connected) {
		// Ignore results for now
		::mosquitto_disconnect(m_mosq);
		::mosquitto_loop_stop(m_mosq, false);
		m_connected = false;
	}
}

void mqtt::publish(std::string const& channel, std::string const& message) {
	if(!m_connected) {
		throw exception("can not publish - not connected.");
	}
	auto res = ::mosquitto_publish(m_mosq, nullptr, channel.c_str(), static_cast<int>(message.size()), message.data(), 0, false);
	check(res, "mosquitto_publish failed");
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
	auto lock = std::lock_guard(m_subscription_mutex);
	if(m_subscribed) {
		auto channels = c_strings(m_subscription_channels);
		// Ignore result for now
		::mosquitto_unsubscribe_multiple(m_mosq, nullptr, static_cast<int>(channels.size()), channels.data(), nullptr);
		m_subscription_callback = callback_t();
		m_subscription_channels.clear();
		m_subscribed = false;
	}
}

void mqtt::subscribe(std::vector<char*> const& channels, callback_t callback) {
	if(!m_connected) {
		throw exception("can not subscribe - not connected.");
	}
	if(channels.empty()) {
		throw exception("can not subscribe - channels list is empty.");
	}
	auto lock = std::lock_guard(m_subscription_mutex);
	if(m_subscribed) {
		throw exception("can not subscribe - subscription is active.");
	}
	m_subscription_callback = callback;
	m_subscription_channels.assign(channels.begin(), channels.end());
	m_subscribed = true;
	auto res = internal_subscribe();
	if(res == MOSQ_ERR_NO_CONN) {
		// Connect callback subscribes once the connection is established
		logger::log("MQTT not connected yet - subscription postponed");
	}
	else if(res != MOSQ_ERR_SUCCESS) {
		m_subscription_callback = callback_t();
		m_subscription_channels.clear();
		m_subscribed = false;
		check(res, "mosquitto_subscribe_multiple failed");
	}
}

/// Must be called with m_subscription_mutex locked
int mqtt::internal_subscribe() {
	assert(!m_subscription_channels.empty());
	auto channels = c_strings(m_subscription_channels);
	auto res = ::mosquitto_subscribe_multiple(m_mosq, nullptr, static_cast<int>(channels.size()), channels.data(), 0, 0, nullptr);
	if(res == MOSQ_ERR_SUCCESS) {
		auto channel_names = std::string();
		for(auto& channel : m_subscription_channels) {
			if(!channel_names.empty()) {
				channel_names += ", ";
			}
			channel_names += channel;
		}
		logger::log("MQTT subscribed to " + std::to_string(channels.size()) + " channels: " + channel_names);
	}
	return res;
}
