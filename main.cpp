#include <iostream>
#include <list>

#include "vp_parser.h"

int main(int argc, char** argv)
{
	std::list<vp_index*> vp_list;

	for (int i = 1; i < argc; ++i) {
		vp_index* new_idx = new vp_index();
		if (!new_idx->parse(argv[i])) {
			std::cerr << "Error parsing " << argv[i] << std::endl;
			delete new_idx;
		} else {
			vp_list.push_back(new_idx);
		}
		std::cout << new_idx->print_index_listing() << std::endl;
	}

	// Cleanup
	for (auto* idx : vp_list) {
		delete idx;
	}

	return 0;
}
