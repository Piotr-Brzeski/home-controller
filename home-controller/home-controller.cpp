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

int main(int argc, const char * argv[]) {
	try {
		if(argc < 2) {
			throw std::runtime_error("No configuration path.");
		}
		while(true) {
			constexpr auto timeout = std::chrono::seconds(5);
			auto start_time = std::chrono::steady_clock::now();
			auto controller = home::controller(argv[1]);
			controller.start();
			controller.wait();
			auto end_time = std::chrono::steady_clock::now();
			auto duration = end_time - start_time;
			if(duration < timeout) {
				auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
				throw std::runtime_error("Controller finished in " + std::to_string(seconds) + " seconds");
			}
		}
		return 0;
	}
	catch(std::exception &e) {
		log::log(std::string("Home controlled finished with excetion: ") + e.what());
	}
	catch(...) {
		log::log("Home controlled finished with unknown excetion");
	}
	return 1;
}

