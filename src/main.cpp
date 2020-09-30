#include <gxut/gxfilesystem.h>
#include <gxut/gxstring.h>
#include <gxut/gxargparse.h>
#include <gxut/gxiniparser.h>
#include <gxut/gxmemory.h>
#include "main.h"

void init_argparser( gx::argparse::parser_t& ap )
{
	ap.copyright( "Sungkil Lee", 2011 );
	ap.header( "%s (exTensive SYNC) is a backup frontend to multiple backends\nincluding rsync, robocopy, 7zip, zip, copy, move, and custom.\n", ap.name() );
	ap.add_argument( "src" ).help( "{*.ini|*.tsync|*.toml|*.lua} file or src file/dir" );
	ap.add_argument( "dst" ).set_optional().help( "optional dst dirs/files (for cmd-based sync)" );
	ap.add_option( "b", "robocopy" ).help( "use robocopy; equivalent to --cmd=robocopy" );
	ap.add_option( "s", "shutdown" ).help( "shutdown the system in the end" );
	ap.add_option( "d", "dry" ).help( "dry run without execution" );
	ap.add_option( "c", "cmd" ).subarg("str").help( "rsync (default), robocopy, 7zip, zip (for command line only)" );
	ap.add_option( "crc" ).help( "check crc for file sync (not for directory sync)" );
	ap.add_option( "o", "option" ).subarg("str").help( "additional user arguments/options" );
	ap.add_option( "p", "port" ).subarg("int").help( "port for rsync/SSH (e.g., 22)" );
	ap.add_option( "n", "include" ).subarg("str").help( "include files/directories" );
	ap.add_option( "xf", "exclude_file" ).subarg("str").help( "exclude files" );
	ap.add_option( "xd", "exclude_dir" ).subarg("str").help( "exclude directories" );
	ap.add_option( "gf", "ignore_file" ).subarg("str").help( "ignore files" );
	ap.add_option( "gd", "ignore_dir" ).subarg("str").help( "ignore directories" );
	//ap.footer( "Use proper *** patterns for include\nExample: --include={subdir}/***\n" );
}

int wmain( int argc, wchar_t* argv[] )
{
	setbuf(stdout,0);setbuf(stderr,0); // no need to have fflush(stdout/stderr)

	gx::argparse::parser_t ap;
	init_argparser(ap);
	if(!ap.parse( argc, argv )) return EXIT_FAILURE;

	// read essential cmd arguments
	path src;
	if(ap.exists("src"))
	{
		src=ap.get<path>("src"); if(!src.empty()&&src.is_relative()) src=src.absolute();
		if(src.ext().empty()&&!src.exists()){for(auto e:INI_EXTENSIONS) if((src+L"."+e).exists()){ src=src+L"."+e; break; } }
	}
	if(!src.exists()){ printf( "error: %s not exists\n", src.wtoa() ); return EXIT_FAILURE; }
	
	// build multiple dst list
	std::vector<path> dsts;
	if(ap.exists("dst"))
	{
		std::vector<path> v={ ap.get<path>("dst").c_str() };
		for( auto& o : ap.others() ) v.emplace_back(o.c_str());
		for( auto& d : v )
		{
			if(!d.empty()&&d.is_relative()) d=d.absolute();
			dsts.push_back(d);
		}
	}

	// global states from cmd
	bool b_shutdown = ap.exists( "s" );
	bool b_dry = ap.exists( "d" );
	bool b_crc = ap.exists( "crc" );

	// read global tsync.ini and override with cmd options
	if(!global().read_ini( b_dry, b_shutdown, b_crc )) return EXIT_FAILURE;

	// read cmd or ini, and build items for run
	std::vector<item_t> items;
	if(!dsts.empty())
	{
		if(!read_cmd(ap,items,src,dsts)) return EXIT_FAILURE;
	}
	else if(ALL_EXTENSIONS.find(src.ext())!=ALL_EXTENSIONS.end())
	{
		if(INI_EXTENSIONS.find(src.ext())!=INI_EXTENSIONS.end()){	if(!read_ini(src,items))	return EXIT_FAILURE; }
		else if(src.ext()=="toml"){									if(!read_toml(src,items))	return EXIT_FAILURE; }
		else if(src.ext()=="lua"){									if(!read_lua(src,items))	return EXIT_FAILURE; }
	}

	// execution
	for( auto& t : items ) if(!t.build.exec()) return EXIT_FAILURE;

	// shutdown after all sync
	if(global().b.shutdown)	_wsystem( L"shutdown /s /f /t 0" );
	else if( !os::console::has_parent() ) system( "pause" ); // if directly executed from the explorer, then pause

	return EXIT_SUCCESS;
}
