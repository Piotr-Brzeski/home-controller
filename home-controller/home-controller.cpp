//
//  home-controller.cpp
//  home-controller
//
//  Created by Piotr Brzeski on 2023-05-27.
//  Copyright © 2023 Brzeski.net. All rights reserved.
//

#include "controller.h"
#include <cpp-log/log.h>
#include <chrono>
#include <thread>
#include <memory>

auto start_controller(const char* configuration_path) {
	auto wait_time = std::chrono::seconds(0);
	while(true) {
		if(wait_time.count() > 0) {
			logger::log("Home controller - wait " + std::to_string(wait_time.count()) + " seconds.");
			std::this_thread::sleep_for(wait_time);
		}
		try {
			auto controller = std::make_unique<home::controller>(configuration_path);
			controller->start();
			return controller;
		}
		catch(std::exception &e) {
			constexpr auto min_wait_time = std::chrono::seconds(5);
			constexpr auto max_wait_time = std::chrono::seconds(1000);
			if(wait_time.count() == 0) {
				wait_time = min_wait_time;
			}
			else {
				wait_time *= 2;
			}
			if(wait_time > max_wait_time) {
				throw;
			}
			logger::log(std::string("Home controller failed to start with exception: ") + e.what() + ". Retrying.");
		}
	}
}

int main(int argc, const char * argv[]) {
	auto log = logger::start(logger::cout());
	try {
		if(argc < 2) {
			throw std::runtime_error("No configuration path");
		}
		while(true) {
			logger::log("Starting home controller");
			auto controller = start_controller(argv[1]);
			auto start_time = std::chrono::steady_clock::now();
			controller->wait();
			auto end_time = std::chrono::steady_clock::now();
			auto duration = end_time - start_time;
			auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
			logger::log("Home controller finished in " + std::to_string(seconds) + " seconds");
			constexpr auto timeout = std::chrono::seconds(5);
			if(duration < timeout) {
				throw std::runtime_error("Home controller failed to run");
			}
			std::this_thread::sleep_for(timeout);
		}
		return 0;
	}
	catch(std::exception &e) {
		logger::log(std::string("Home controller finished with exception: ") + e.what());
	}
	catch(...) {
		logger::log("Home controller finished with unknown exception");
	}
	return 1;
}

