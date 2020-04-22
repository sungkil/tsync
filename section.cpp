#include <gxut/gxfilesystem.h>
#include <gxut/gxstring.h>
#include <gxut/gxargparse.h>
#include <gxut/gxiniparser.h>
#include <gxut/gxmemory.h>
#include "main.h"

bool read_cmd( gx::argparse::parser_t& ap, std::vector<item_t>& items, path& src, path& dst )
{
	section_t s; item_t* t=s.read( ap, "cmd", src, dst ); if(!t) return false;
	items.push_back(*t); safe_delete(t);
	return true;
}

bool read_ini( path ini_path, std::vector<item_t>& items )
{
	// local global section
	INIParser parser( ini_path ); if(!parser.load()) return false;
	if(!global().read_local( parser )) return false;

	// other local items
	for( auto& sec : parser.get_section_list() )
	{
		if(_stricmp(sec.c_str(),"global")==0) continue;
		section_t s; std::vector<item_t*> t = s.read( parser, sec ); if(t.empty()) return false;
		for( auto* i : t ){ items.push_back(*i); safe_delete(i); }
	}

	return true;
}

std::vector<item_t*> import_ini( const char* sec, path ini_path )
{
	std::vector<item_t*> v;
	if(_stricmp(sec,"global")==0){ MessageBoxA( nullptr, format("%s(): import is not allowed in [global] section\n", __func__ ),"error", MB_OK ); return v; }
	if(!ini_path.exists()){ MessageBoxA( nullptr, format("%s(): %s not exists\n", __func__, ini_path.wtoa() ),"error", MB_OK ); return v; }
	if(INI_EXTENSIONS.find(ini_path.ext())==INI_EXTENSIONS.end()){ MessageBoxA( nullptr, format( "%s(): import_path(%s) supports only {%s}\n", __func__, ini_path.wtoa(), INI_EXT_WILDCARD ),"error", MB_OK ); return v; }

	global().push();
		std::vector<item_t> imported;
		if(!read_ini( ini_path, imported )) return v;
		for( auto i : imported ) v.push_back( new item_t(i) );
	global().pop();

	return v;
}

inline std::vector<std::wstring> explode_with_quotes( const wchar_t* src )
{
	wchar_t buff[_MAX_PATH], *b=buff;
	size_t len = wcslen(src);
	std::vector<std::wstring> v;
	bool b_quote=false;
	for( size_t k=0; k < len; k++ )
	{
		wchar_t w=src[k];
		if(w==L'\"'||w==L'\''){ b_quote=!b_quote; if(b_quote) continue; }
		else if(b_quote||(w!=' '&&w!='\t'&&w!='\n')){ *(b++) = w; continue; }
		b[0]=NULL; if(b!=buff) v.push_back(buff); b=buff;
	}
	b[0]=NULL; if(b!=buff) v.push_back(buff); b=buff;
	return v;
}

std::vector<item_t*> section_t::read( INIParser& parser, const std::string& sec )
{
	std::vector<item_t*> vi;
	if(!parser.section_exists(sec.c_str())) return vi;

	name = sec;
	path import_path;
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
		else if(key=="dst")				dst = value;
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

	vi.push_back(build_item());

	return vi;
}

item_t* section_t::read( gx::argparse::parser_t& parser, const std::string& sec, path& src, path& dst )
{
	name = sec;
	this->src = src;
	this->dst = dst;

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

	return build_item();
}

item_t* section_t::build_item()
{
	const char* name = this->name.c_str();
	method_t method = str2method(cmd.c_str());

	// global item
	bool this_is_global = _stricmp(name,"global")==0;

	// exception handling and preprocessing
	if(this_is_global)
	{
		if(!src.empty()){	printf( "[%s] src entry is not allowed in global section\n", name ); return nullptr; }
		if(!dst.empty()){	printf( "[%s] dst entry is not allowed in global section\n", name ); return nullptr; }
		if(!opt.empty()){	printf( "[%s] option is not allowed in global section\n", name ); return nullptr; }
		if(instance!=0){	printf( "[%s] instance is not allowed in global section\n", name ); return nullptr; }
	}
	else if(method!=method_t::CUSTOM)
	{
		if(src.empty()){		printf( "[%s] src entry not exists\n", name ); return nullptr; }
		if(dst.empty()){		printf( "[%s] dst entry not exists\n", name ); return nullptr; }
		if(!src.is_absolute()){	printf( "[%s] src should be an absolute path\n", name ); return nullptr; }
		if(!dst.is_absolute()){	printf( "[%s] dst should be an absolute path\n", name ); return nullptr; }
	}

	// check method
	if(method==method_t::RSYNC)
	{
		auto dv=dst.volume(); 
		if(!dst.is_unc()&&!dst.is_rsync()&&dv.exists()&&(dv.is_exfat()||dv.is_fat32()))
		{
			printf( "\nROBOCOPY is used for %s (on %s file system), where rsync has a problem with timestamps.\n", dst.wtoa(), wtoa(dv.filesystem.name) );
			method=method_t::ROBOCOPY;
		}
	}

	// get or create item
	item_t* t = this_is_global? &global() : new item_t(atow(name),method,src.c_str(),dst.c_str());

	// create additional items for exclude files/dirs
	if(method!=method_t::COPY&&method!=method_t::MOVE&&method!=method_t::CUSTOM)
	{
		t->nf.insert(nf.begin(),nf.end());
		t->xf.insert(xf.begin(),xf.end());
		t->xd.insert(xd.begin(),xd.end());
		t->gf.insert(gf.begin(),gf.end());
		t->gd.insert(gd.begin(),gd.end());
	}
	
	// post exception handling and preprocessing
	if(!this_is_global) // only for local sections
	{
		if(method!=method_t::RSYNC&&method!=method_t::COPY&&method!=method_t::MOVE&&method!=method_t::CUSTOM)
		{
			if(method!=method_t::SZIP&&method!=method_t::ZIP&&!t->src.is_dir()){ printf( "[%s] src should be a directory for %s\n", name, method2str(t->method) ); return nullptr; }
			if(!t->nf.empty()){ printf( "[%s] include_file supports only for rsync\n", name ); return nullptr; }
		}

		t->port = global().port;
		t->b.dry = global().b.dry;
		t->b.crc = global().b.crc;
		t->opt += trim( opt.c_str() ); // get additional user option

		// assign custom cmd
		if(method==method_t::CUSTOM) t->custom = custom;

		// incremental instancing
		if(instance!=0)
		{
			if(method!=method_t::RSYNC&&method!=method_t::ROBOCOPY){ printf( "[%s] instancing only supports rsync or robocopy\n", name ); return nullptr; }
			if(instance<0){	printf( "[%s] instance(%d) should be positive\n", name, instance ); return nullptr; }
			if(instance>MAX_INSTANCE){ printf( "[%s] instance(%d) should be < %d\n", name, instance, MAX_INSTANCE ); return nullptr; }
			if(dst.back()!=L'\\'&&dst.back()!=L'/'){ printf( "[%s] dst for instancing should be a directory\n", name ); return nullptr; }
			if(t->is_synology()||t->is_ssh()){ printf( "[%s] instancing does not support remote SSH\n", name ); return nullptr; }
			if(!wcsstr(dst.dir_name(),global_macro_t::get_timestamp())){ printf( "[%s] dst.dir_name() should include $date\n", name ); return nullptr; }
			if(!t->nf.empty()){ printf( "[%s] instance not supports inclusion\n", name ); return nullptr; }

			t->instance = instance;
		}
	}

	// override local options over global options
	t->b.dry = global().b.dry || this->b.dry;
	t->b.crc = global().b.crc || this->b.crc;
	if(port) t->port = port;

	// prebuild all
	if(!this_is_global) t->prebuild();

	return t;
}
