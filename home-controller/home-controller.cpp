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
	constexpr auto wait_increment = std::chrono::seconds(2);
	constexpr auto max_wait_time = std::chrono::seconds(10);
	auto wait_time = std::chrono::seconds(0);
	while(true) {
		std::this_thread::sleep_for(wait_time);
		try {
			auto controller = std::make_unique<home::controller>(configuration_path);
			controller->start();
			return controller;
		}
		catch(std::exception &e) {
			wait_time += wait_increment;
			if(wait_time > max_wait_time) {
				throw;
			}
			log::log(std::string("Home controller failed to start with exception: ") + e.what() + ". Retrying.");
		}
	}
}

int main(int argc, const char * argv[]) {
	try {
		if(argc < 2) {
			throw std::runtime_error("No configuration path");
		}
		while(true) {
			log::log("Starting home controller");
			auto controller = start_controller(argv[1]);
			auto start_time = std::chrono::steady_clock::now();
			controller->wait();
			auto end_time = std::chrono::steady_clock::now();
			auto duration = end_time - start_time;
			auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
			log::log("Controller finished in " + std::to_string(seconds) + " seconds");
			constexpr auto timeout = std::chrono::seconds(5);
			if(duration < timeout) {
				throw std::runtime_error("Controller failed to run");
			}
			std::this_thread::sleep_for(timeout);
		}
		return 0;
	}
	catch(std::exception &e) {
		log::log(std::string("Home controller finished with exception: ") + e.what());
	}
	catch(...) {
		log::log("Home controller finished with unknown exception");
	}
	return 1;
}

