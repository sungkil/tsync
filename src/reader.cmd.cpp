#include <gxut/gxfilesystem.h>
#include <gxut/gxstring.h>
#include <gxut/gxargparse.h>
#include <gxut/gxiniparser.h>
#include <gxut/gxmemory.h>
#include "main.h"

bool read_cmd( gx::argparse::parser_t& ap, std::vector<item_t>& items, path& src, std::vector<path>& dsts )
{
	for( auto& dst : dsts )
	{
		reader_t s; item_t* t=s.read( ap, "cmd", src, dst ); if(!t) return false;
		items.push_back(*t); safe_delete(t);
	}
	return true;
}

item_t* reader_t::read( gx::argparse::parser_t& parser, const std::string& sec, path& src, path& dst )
{
	this->src = src;

	// retrieve const key and value
	if(parser.exists("cmd"))		cmd = parser.get<>("c");
	if(parser.exists("robocopy"))	cmd = "robocopy"; // override cmd
	if(parser.exists("option"))		opt = parser.get<std::wstring>("option");
	if(parser.exists("port"))		port = parser.get<int>("port");
	if(parser.exists("dry"))		b.dry = true;
	if(parser.exists("shutdown"))	b.shutdown = true;
	if(parser.exists("crc"))		b.crc = true;
	if(parser.exists("nf"))			for(auto& s:explode_with_quotes(parser.get<std::wstring>("nf").c_str()))	nf.push_back(s);
	if(parser.exists("xf"))			for(auto& s:explode_with_quotes(parser.get<std::wstring>("xf").c_str()))	xf.push_back(s);
	if(parser.exists("xd"))			for(auto& s:explode_with_quotes(parser.get<std::wstring>("xd").c_str()))	xd.push_back(s);
	if(parser.exists("gf"))			for(auto& s:explode_with_quotes(parser.get<std::wstring>("gf").c_str()))	gf.push_back(s);
	if(parser.exists("gd"))			for(auto& s:explode_with_quotes(parser.get<std::wstring>("gd").c_str()))	gd.push_back(s);

	return build_item(sec.c_str(),dst);
}