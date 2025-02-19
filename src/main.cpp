/*
*   Calvin Neo
*   Copyright (C) 2016  Calvin Neo <calvinneo@calvinneo.com>
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License along
*   with this program; if not, write to the Free Software Foundation, Inc.,
*   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <iostream>  
#include <sstream>
#include <fstream>
#include <ctime>
#include <stdio.h>
#include <chrono>
#include "target/codegen.h"
#include "../for90std/for90std.h"
#include "develop.h"
#include "getopt2.h"

using namespace std; 

inline uint64_t get_current_ms() {
	using namespace std::chrono;
	time_point<system_clock, milliseconds> timepoint_now = time_point_cast<milliseconds>(system_clock::now());;
	auto tmp = duration_cast<milliseconds>(timepoint_now.time_since_epoch());
	std::time_t timestamp = tmp.count();
	return (uint64_t)timestamp;
}

int main(int argc, char* argv[], char* env[])
{
	int opt;
	std::string code;
	int print_tree = (int)false;
	struct option opts[] = { 
		{ "fortran", optional_argument, nullptr, 'F' },
		{ "file", required_argument, nullptr, 'v' },
		{ "debug", no_argument, nullptr, 'd' },
		{ "tree", no_argument, &print_tree, true },
		{ 0, 0, 0, 0 } 
	};

	while ((opt = getopt_long(argc, argv, "df:F::p", opts, nullptr)) != -1) {
		if (opt == 'f')
		{
			get_context().parse_config.hasfile = true;
			fstream f;
			f.open(optarg, ios::in);
			code = std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
			f.close();
		}
		else if (opt == 'F') {
			// for90
			if (strcmp(optarg, "77") == 0)
			{
				get_context().parse_config.for90 = false;
			}
			get_context().parse_config.for90 = true;
		}
		else if (opt == 'd') {
			// debug
			get_context().parse_config.isdebug = true;
		}
		else if (opt == 'C') {
			// use c style
			get_context().parse_config.usefor = false;
		}
	}
	get_context().parse_config.usefarray = true;
	if (get_context().parse_config.isdebug) {
		debug();
	}
	else if(get_context().parse_config.hasfile){
		uint64_t start_time = get_current_ms();
		do_trans(code);
		uint64_t end_time = get_current_ms();
		fprintf(stderr, "Cost time:%lld\n", end_time - start_time);
		cout << get_context().program_tree.get_what() << endl;
		// preorder(&program_tree);
	}
	else {

	}
	if (print_tree)
	{
		preorder(&get_context().program_tree);
	}
#ifdef _DEBUG
	system("pause");
#endif


	return 0;
}