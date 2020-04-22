#pragma once
#include "item.h"
#include "global.h"

struct section_t
{
	std::string		name;				// section name
	std::string		cmd;				// rsync, robocopy, ....
	std::wstring	custom;				// custom command line
	std::wstring	opt;				// additional custom option
	path			src;
	path			dst;
	int				port = 0;			// SSH port
	int				instance = 0;		// instances for incremental backup
	struct { bool dry=false, shutdown=false, crc=false; } b;

	std::vector<std::wstring> nf, xf, xd, gf, gd;	// include, exclude, ignore
	std::map<std::string,std::wstring> others;

	// member functions
	bool empty() const { return name.empty(); }
	void clear(){ name.clear(); cmd.clear(); opt.clear(); src.clear(); dst.clear(); port=0; b.dry=false; b.shutdown=false; b.crc=false; }
	item_t* build_item();

	// read by multiple sources
	std::vector<item_t*> read( INIParser& parser, const std::string& sec );
	item_t* read( gx::argparse::parser_t& parser, const std::string& sec, path& src, path& dst );
};

bool read_cmd( gx::argparse::parser_t& ap, std::vector<item_t>& items, path& src, path& dst );
bool read_ini( path ini_path, std::vector<item_t>& items );