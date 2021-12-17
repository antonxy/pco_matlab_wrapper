#include <iostream>
#include "pco_wrapper.hpp"

int main(int argc, char** argv) {
	std::cout << "Hello" << std::endl;
	try {
		PCOCamera cam;
		cam.open(0);
		cam.transfer_mip(1, 1, 10, 10);
		cam.close();
	}
	catch (const std::exception& ex) {
		std::cout << ex.what() << std::endl;
	}
}