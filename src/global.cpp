#include <gxut/gxfilesystem.h>
#include <gxut/gxstring.h>
#include <gxut/gxargparse.h>
#include <gxut/gxiniparser.h>
#include <gxut/gxmemory.h>
#include "main.h"

static std::deque<item_t> stack;

global_item_t::global_item_t():first(L"global"),item_t(L"global")
{
	port = 22;
	xf = { L"&#*", L"~$*", L"$~*", L"*humbs.db", L"*.dropbox*", L"*.cache*", L"*.bak", L"*.vc.db", L"*.synctex.gz" };
	xd = { L"&#*", L".vs", L".Syno*", L"*.dropbox*" };
	gf = { L"*esktop.ini", L"pagefile.sys" };
	gd = { L".sync", L"@eaDir", L"*ecycle*", L"*ECYCLE*", L"Recovery", L"*nicode_*e*", L"System*Volume*Information", L"lost*found", L".DS_Store*" };
		
	first = reinterpret_cast<item_t&>(*this); // keep the first state; this will be updated after global ini
}

void global_item_t::push()
{
	stack.push_back(*this);
	reinterpret_cast<item_t&>(*this) = first;
}

void global_item_t::pop()
{
	if(stack.empty()) return;
	reinterpret_cast<item_t&>(*this) = stack.back();
	stack.pop_back();
}

bool global_item_t::read_ini( bool b_dry, bool b_shutdown, bool b_crc )
{
	path global_ini_path;
	for( auto e : INI_EXTENSIONS )
	{
		global_ini_path = path::module_path().remove_ext()+L"."+e;
		if(global_ini_path.exists()) break;
	}

	if(global_ini_path.exists())
	{
		INIParser gp;	if(!gp.load(global_ini_path)) return false;
		reader_t g;		if(gp.section_exists("global")&&g.read(gp,"global").empty()) return false;
	}

	global().b.dry = global().b.dry || b_dry;
	global().b.crc = global().b.crc || b_crc;
	global().b.shutdown = global().b.shutdown || b_shutdown;
	global().first = global(); // store the initial state

	return true;
}

bool global_item_t::read_local( INIParser& parser )
{
	global_macro_t macro;

	if(parser.section_exists("global"))
	{
		reader_t g; if(g.read( parser, "global" ).empty()) return false;
		for( auto* e : parser.get_entries("global") ) // entries are already sorted by indices
		{
			macro.apply_to(e); // apply macros from earlier entries
			if(!e->key.empty()&&e->key[0]=='$') macro.data[atow(e->key.c_str())] = e->value;
		}
	}

	// apply macros
	for( auto& sec : parser.get_section_list() )
	{
		if(_stricmp(sec.c_str(),"global")==0) continue;
		for( auto* e : parser.get_entries(sec.c_str()) ) macro.apply_to(e);
	}

	return true;
}
