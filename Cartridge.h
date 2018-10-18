#pragma once
#include <string>
#include <vector>
#include "MBC.h"

using namespace std;

class Cartridge {

public:
	Cartridge(string filename);
	~Cartridge();

	struct Type {
		int mbc;
		bool
			rom = true,
			bandai_tama5,
			battery, ram,
			mmo1,
			rumble,
			timer,
			pocket_camera,
			huc3,
			huc1;
	};

	Type type = Type();
	string get_title();
	bool is_japanese();
	short get_size();
	string to_string();
	vector<char> rom;
	MBC mbc;

	Type get_type() {
		return type;
	}

private:
	void set_type();
};