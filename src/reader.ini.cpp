#include <gxut/gxfilesystem.h>
#include <gxut/gxstring.h>
#include <gxut/gxargparse.h>
#include <gxut/gxiniparser.h>
#include <gxut/gxmemory.h>
#include "main.h"

bool read_ini( path ini_path, std::vector<item_t>& items )
{
	// local global section
	INIParser parser( ini_path ); if(!parser.load()) return false;
	if(!global().read_local( parser )) return false;

	// other local items
	for( auto& sec : parser.get_section_list() )
	{
		if(_stricmp(sec.c_str(),"global")==0) continue;
		reader_t s; std::vector<item_t*> t = s.read( parser, sec ); if(t.empty()) return false;
		for( auto* i : t ){ items.push_back(*i); safe_delete(i); }
	}

	return true;
}

std::vector<item_t*> import_ini( const char* sec, path ini_path )
{
	std::vector<item_t*> v;
	if(_stricmp(sec,"global")==0){ MessageBoxA( nullptr, format("%s(): import is not allowed in [global] section\n", __func__ ),"error", MB_OK ); return v; }
	if(!ini_path.exists()){ MessageBoxA( nullptr, format("%s(): %s not exists\n", __func__, ini_path.wtoa() ),"error", MB_OK ); return v; }
	if(INI_EXTENSIONS.find(ini_path.ext())==INI_EXTENSIONS.end()){ MessageBoxA( nullptr, format( "%s(): import_path(%s) supports only {%s}\n", __func__, ini_path.wtoa(), INI_EXT_FILTER() ),"error", MB_OK ); return v; }

	global().push();
		std::vector<item_t> imported;
		if(!read_ini( ini_path, imported )) return v;
		for( auto i : imported ) v.push_back( new item_t(i) );
	global().pop();

	return v;
}

std::vector<item_t*> reader_t::read( INIParser& parser, const std::string& sec )
{
	std::vector<item_t*> vi;
	if(!parser.section_exists(sec.c_str())) return vi;

	path import_path;
	std::vector<path> dsts;
	for( auto* e : parser.get_entries( sec.c_str() ) )
	{
		std::string key0=e->key; if(key0.empty()) continue;
		std::string key=tolower(key0.c_str());

		// legacy support
		if(key=="include_file"||key=="include_files")	key="include";
		else if(key=="exclude_files")					key="exclude_file";
		else if(key=="exclude_dirs")					key="exclude_dir";
		else if(key=="ignore_files")					key="ignore_file";
		else if(key=="ignore_dirs")						key="ignore_dir";
		else if(key=="rsync_port")						key="port";
		
		// retrieve const key and value
		const char* k0	= key0.c_str();
		const char* k	= key.c_str();
		std::wstring value	= parser.get_value( sec.c_str(), k0 );

		if(key=="src")					src = value;
		else if(key=="dst")				for(auto& s:explode_with_quotes(value.c_str()))	dsts.push_back(s.c_str());
		else if(key=="cmd")				cmd = wtoa(value.c_str());
		else if(key=="custom"){			custom = value; cmd = "custom"; }
		else if(key=="option")			opt = value;
		else if(key=="port")			port = parser.get_int(sec.c_str(),k0);
		else if(key=="dry")				b.dry = parser.get_bool(sec.c_str(),k0);
		else if(key=="shutdown"){		if(parser.get_bool(sec.c_str(),k0)) global().b.shutdown = true; }
		else if(key=="crc")				b.crc = parser.get_bool(sec.c_str(),k0);
		else if(key=="include")			for(auto& s:explode_with_quotes(value.c_str()))	nf.push_back(s);
		else if(key=="exclude_file")	for(auto& s:explode_with_quotes(value.c_str()))	xf.push_back(s);
		else if(key=="exclude_dir")		for(auto& s:explode_with_quotes(value.c_str()))	xd.push_back(s);
		else if(key=="ignore_file")		for(auto& s:explode_with_quotes(value.c_str()))	gf.push_back(s);
		else if(key=="ignore_dir")		for(auto& s:explode_with_quotes(value.c_str()))	gd.push_back(s);
		else if(key=="instance")		instance = parser.get_int(sec.c_str(),k0);
		else if(key=="import")			import_path = path(value.c_str()).absolute(path(parser.get_path()).dir());
		else if(key.front()!='$')		// skip macros and error for others
		{
			printf( "section_t::%s(): [%s].%s is an unrecognized key\n",__func__,sec.c_str(),key0.c_str() );
			return vi;
		}
	}

	// directly return imported
	if(!import_path.empty()) return import_ini( sec.c_str(), import_path );

	// custom: error when being used wiht cmd together
	if(!custom.empty()&&cmd!="custom"){ printf( "section_t::read(%s): cmd cannot be used with custom\n",sec.c_str() ); return vi; }

	// assign global section cmd when not exists
	if(cmd.empty()&&parser.section_exists("global")&&parser.key_exists("global:cmd")) cmd = wtoa(parser.get_value("global:cmd"));

	// build multiple items for multiple dsts
	if(dsts.empty())						vi.push_back(build_item(sec.c_str(),path()));
	else if(dsts.size()==1)					vi.push_back(build_item(sec.c_str(),dsts.front()));
	else for(size_t k=0;k<dsts.size();k++)	vi.push_back(build_item(format("%s[%zd]",sec.c_str(),k),dsts[k]));

	return vi;
}