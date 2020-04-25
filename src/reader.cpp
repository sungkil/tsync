#include <gxut/gxfilesystem.h>
#include <gxut/gxstring.h>
#include <gxut/gxargparse.h>
#include <gxut/gxiniparser.h>
#include <gxut/gxmemory.h>
#include "main.h"

item_t* reader_t::build_item( const char* name, path dst )
{
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
	item_t* t;
	if(this_is_global) t = &global();
	else
	{
		t = new item_t(atow(name),method,src.c_str(),dst.c_str());
	}
	

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
