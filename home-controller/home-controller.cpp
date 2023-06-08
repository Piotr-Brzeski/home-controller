//
//  home-controller.cpp
//  home-controller
//
//  Created by Piotr Brzeski on 2023-05-27.
//  Copyright © 2023 Brzeski.net. All rights reserved.
//

#include "controller.h"
#include <iostream>
#include <future>

int main(int argc, const char * argv[]) {
	try {
		if(argc < 2) {
			throw std::runtime_error("No configuration path.");
		}
		auto controller = home::controller(argv[1]);
		controller.start();
		
		std::promise<void>().get_future().wait();
		return 0;
	}
	catch(std::exception &e) {
		std::cerr << "Exception: " << e.what() << std::endl;
	}
	catch(...) {
		std::cerr << "Exception." << std::endl;
	}
	return 1;
}

