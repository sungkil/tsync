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
	ap.add_argument( "src" ).help( "*.ini/*.tsync file or src file/directory (for cmd-based sync)" );
	ap.add_argument( "dst" ).set_optional().help( "(optional) dst file/directory (for cmd-based sync)" );
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

void pause( const wchar_t* src_name )
{
	// if directly executed from the explorer, then pause
	wchar_t ctbuff[4096]; GetConsoleTitleW(ctbuff,MAX_PATH);
	path console_title = path(ctbuff).canonical();

	if(console_title==path::module_path())	system( "pause" );
	else if(console_title==src_name)		system( "pause" );
}

int wmain( int argc, wchar_t* argv[] )
{
	setbuf(stdout,0);setbuf(stderr,0); // no need to have fflush(stdout/stderr)

	gx::argparse::parser_t ap;
	init_argparser(ap);
	if(!ap.parse( argc, argv )) return EXIT_FAILURE;

	// read essential cmd arguments
	path src; if(ap.exists("src")){ src=ap.get<path>("src"); if(!src.empty()&&src.is_relative()) src=src.absolute(); }
	path dst; if(ap.exists("dst")){ dst=ap.get<path>("dst"); if(!dst.empty()&&dst.is_relative()) dst=dst.absolute(); }

	// exception handling on src_path
	if(!src.empty())
	{
		if(src.ext().empty()&&!src.exists())
		{ for( auto e : INI_EXTENSIONS ) if((src+L"."+e).exists()){ src=src+L"."+e; break; } }
		if(!src.exists()){ printf( "error: %s not exists\n", src.wtoa() ); return EXIT_FAILURE; }
	}

	// global states from cmd
	bool b_shutdown = ap.exists( "s" );
	bool b_dry = ap.exists( "d" );
	bool b_crc = ap.exists( "crc" );

	// read global tsync.ini and override with cmd options
	if(!global().read_ini( b_dry, b_shutdown, b_crc )) return EXIT_FAILURE;

	// read cmd or ini, and build items for run
	std::vector<item_t> items;
	if(!dst.empty()){ if(!read_cmd(ap,items,src,dst)) return EXIT_FAILURE; }
	else if(INI_EXTENSIONS.find(src.ext())!=INI_EXTENSIONS.end()){ if(!read_ini(src,items)) return EXIT_FAILURE; }

	// execution
	for( auto& t : items ) if(!t.build.exec()) return EXIT_FAILURE;

	// shutdown after all sync
	if(global().b.shutdown)	_wsystem( L"shutdown /s /f /t 0" );
	else					pause( src.name() ); // if directly executed from the explorer, then pause

	return EXIT_SUCCESS;
}
