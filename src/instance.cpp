#include <Shlwapi.h>	// StrCmpLogicalW
#pragma comment(lib, "shlwapi")
#include "item.h" // include items only to work without dependency
#include "regex-date.h"
#include "main.h"

bool support_instance( method_t method, item_t* t, int instance, path dst, const char* name )
{
	bool b_dir = method==method_t::RSYNC||method==method_t::ROBOCOPY;
	bool b_file = method==method_t::SZIP||method==method_t::ZIP;

	if(!b_dir&&!b_file){ printf( "[%s] instancing only supports rsync, robocopy, 7zip, or zip\n", name ); return false; }
	if(b_dir)
	{
		if(dst.back()!=L'\\'&&dst.back()!=L'/'){ printf( "[%s] dst for instancing should be a directory\n", name ); return false; }
		if(!wcsstr(dst.dir_name(),global_macro_t::get_timestamp())){ printf( "[%s] dst.dir_name() should include $date\n", name ); return false; }
	}
	if(b_file)
	{
		if(dst.ext()!="7z"&&dst.ext()!=L"zip"){ printf( "[%s] dst extension for instancing should be 7z or zip\n", name ); return false; }
		if(!wcsstr(dst.name(),global_macro_t::get_timestamp())){ printf( "[%s] dst.name() should include $date\n", name ); return false; }
	}

	if(!t->nf.empty()){ printf( "[%s] instance not supports inclusion\n", name ); return false; }
	if(instance>MAX_INSTANCE){ printf( "[%s] instance(%d) should be < %d\n", name, instance, MAX_INSTANCE ); return false; }
	if(t->is_synology()||t->is_ssh()){ printf( "[%s] instancing does not support remote SSH\n", name ); return false; }

	return true;
}

bool apply_instance_dir( path dst, int instance, bool b_dry )
{
	auto sd = dst.dir().parent().subdirs( false );
	if(sd.size()<instance) return true;

	// sort the subdir list in decending order
	std::sort( sd.begin(), sd.end(), []( const path& a, const path& b )->bool
	{
		auto i = find_date_in_path(a.dir_name()).as_int();
		auto j = find_date_in_path(b.dir_name()).as_int();
		return i!=j?i>j:StrCmpLogicalW(a.c_str(),b.c_str())>0;
	});
	
	// delete old directories
	for( int kn=int(sd.size()), k=kn-1; k>=instance; k-- )
	{
		printf( "rmdir %s\n", sd[k].wtoa() );
		if(!b_dry) if(!sd[k].rmdir()) return false;
	}
	sd.resize(instance); // remove entries

	// rename recent directories if current not exists
	if(!dst.exists())
	{
		printf( "mv %s %s\n", sd.back().wtoa(), dst.wtoa() );
		if(!b_dry) sd.back().move_dir( dst );
	}

	printf("\n");
	return true;
}

bool apply_instance_file( path dst, int instance, bool b_dry )
{
	int thresh = instance;
	auto sf = dst.dir().scan(false);
	bool self_found = false; for( auto& f : sf ) if(f.name()==dst.name()) self_found = true;
	if(!self_found) thresh = thresh-1;  // current file already deleted

	if(sf.size()<thresh) return true;	// current file already deleted
	std::sort( sf.begin(), sf.end(), []( const path& a, const path& b )->bool
	{
		auto i = find_date_in_path(a.name(false)).as_int();
		auto j = find_date_in_path(b.name(false)).as_int();
		return i!=j?i>j:StrCmpLogicalW(a.c_str(),b.c_str())>0;
	});
	
	// delete old files
	for( int kn=int(sf.size()), k=kn-1; k>=thresh; k-- )
	{
		printf( "deleting %s\n", sf[k].wtoa() );
		if(!b_dry) if(!sf[k].rmfile()) false;
	}
	sf.resize(thresh); // remove entries

	printf("\n");
	return true;
}

bool item_build_t::apply_instance( bool b_dry )
{
	if(!b.instance) return false;
	if(method==method_t::RSYNC||method==method_t::ROBOCOPY) return apply_instance_dir(dst,instance,b_dry);
	if(method==method_t::SZIP||method==method_t::ZIP) return apply_instance_file(dst,instance,b_dry);
	
	return false; // nothing applied
}