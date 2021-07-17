#include "item.h" // include items only to work without dependency

static const int WRAP_COLUMNS = 65;
static const int PREPAD = 15;

inline std::vector<std::string> word_wrap( const char* s, int columns=65, int prepad=0 )
{
	std::vector<std::string> v;
	std::string pad(prepad,' ');

	size_t l=strlen(s); if(l<=columns){ v.emplace_back(s); return v; }
	
	std::vector<std::string> vs = explode(s," \n"), bs(1);
	for( auto& v : vs )
	{
		if(bs.back().length()+v.length()>columns) bs.emplace_back(pad);
		if(!bs.back().empty()) bs.back() += " ";
		bs.back() += v;
	}
	return bs;
}

inline const char* join_word_wrap( const std::wstring& w, int columns=WRAP_COLUMNS, int prepad=PREPAD )
{
	return join( word_wrap( wtoa(w.c_str()), columns, prepad ), "\n");
}

bool item_build_t::exec()
{
	if(method==method_t::UNKNOWN){ MessageBoxA( nullptr, format( "method is unknown\n" ), aname(), MB_OK ); return false; }

	// build command
	if(!b.ssh&&!b.custom)
	{
		// junction is not allowed as dst
		if(dst.is_dir()&&dst.is_junction())
		{ MessageBoxA( nullptr, format( "dst=%s should not be a junction/link\n", dst.wtoa() ), aname(), MB_OK ); return false; }

		// src should exist when not using wildcard as src
		if(!b.use_include||!wcsstr(src.c_str(),L"*"))
		{ if(!src.exists()){ MessageBoxA( nullptr, format( "src=%s not exists\n", src.wtoa() ), aname(), MB_OK ); return false; } }
	}

	std::string status;
	status += format( "-------------------------------------------------\n" );
	status += format( " %s%s\n", method2str(method), format(".%s",aname()) );
	status += format( "-------------------------------------------------\n" );
	if(method==method_t::CUSTOM)
	{
		status += format( " %s\n", path(cmd).to_slash().wtoa() );
	}
	else
	{
		status += format( " Source:        %s\n", wtoa((method==method_t::RSYNC?src.cygwin():src.to_slash()).c_str()) );
		status += format( " Dest:          %s\n", wtoa((method==method_t::RSYNC?dst.cygwin():dst.to_slash()).c_str()) );
		if(!b.trivial_option) status += format( " Option:        %s\n", wtoa(opt.c_str()) );
	}

	if(b.use_include)
	{
		if(!nf.disp.empty()) status += format( " Include files: %s\n", join_word_wrap(nf.disp) );
	}
	else if(!b.no_exclude)
	{
		if(!xf.disp.empty()) status += format( " Exclude file:  %s\n", join_word_wrap(xf.disp) );
		if(!xd.disp.empty()) status += format( " Exclude dir:   %s\n", join_word_wrap(xd.disp) );
		if(!gf.disp.empty()) status += format( " Ignore file:   %s\n", join_word_wrap(gf.disp) );
		if(!gd.disp.empty()) status += format( " Ignore dir:    %s\n", join_word_wrap(gd.disp) );
	}

	// instance
	if(instance>1) status += format( " Instance  :    %d\n", instance );

	status += format( "-------------------------------------------------\n" );

	// log commands wet non-robocopy commands
	if(method!=method_t::ROBOCOPY||b.dry) printf( status.c_str() );
	
	// log commands
	if(b.dry)
	{
		if(b.instance) apply_instance(true);
		printf( "\n%s\n", wtoa(cmd.c_str()) );
		printf( "\n-------------------------------------------------\n\n" );
	}
	else if(method==method_t::COPY||method==method_t::MOVE)
	{
		bool b_move = method==method_t::MOVE;
		if(!dst.dir().parent().exists()) dst.dir().parent().mkdir();
		std::string s=format( "%s %s %s\n", src.wtoa(), b_move?">>":">", dst.wtoa() );

		if(src.is_dir()) printf( "%s%s\n", (b_move?src.move_dir(dst):src.copy_dir(dst))?"":"failed: ", s.c_str() );
		else
		{
			if(dst.is_dir()||dst.back()==L'\\') dst = dst.add_backslash()+src.name(); // convert dst dir to file path
			if(b_move) printf( "%s%s\n", src.move_file(dst)?"":"failed: ", s.c_str() );
			else if(!dst.exists()||src.mfiletime()!=dst.mfiletime()) printf( "%s%s\n", src.copy_file(dst)?"":"failed: ", s.c_str() );
		}
		printf("\n");
	}
	else // exec batch script
	{
		if(!dst.dir().parent().exists()) dst.dir().parent().mkdir();
		if((method==method_t::SZIP||method==method_t::ZIP)&&dst.exists()) dst.delete_file();

		// test crc for include only
		if(b.crc&&b.use_include&&method==method_t::RSYNC&&!src.is_dir())
		{
			path dst_file = dst.dir()+src.name();
			if(dst_file.exists() && src.crc32c()==dst_file.crc32c())
			{
				printf("crc-skipping %s\n\n",src.cygwin().wtoa());
				return true;
			}
		}

		// instancing
		if(b.instance&&!apply_instance(false)){ printf( "instancing failed, skipping to a next entry\n"); return false; }

		// real process
		PROCESS_INFORMATION pi = {NULL};
		STARTUPINFO si = {NULL};
		si.cb = sizeof(si);
		si.dwFlags=STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_SHOW;

		CreateProcessW( NULL, (LPWSTR)cmd.c_str(), NULL, NULL, FALSE, NULL, NULL, NULL, &si, &pi );
		WaitForSingleObject(pi.hProcess,INFINITE);		// wait for finish
		CloseHandle( pi.hProcess );   // must release handle
		CloseHandle( pi.hThread );    // must release handle

		// restore hidden system folder
		if(method==method_t::ROBOCOPY&&src.is_root_dir()&&!dst.is_root_dir()&&dst.is_dir()&&dst.is_hidden())
		{
			if(dst.is_system()) dst.set_system(false);
			if(dst.is_hidden()) dst.set_hidden(false);
		}
		printf("\n");
	}

	return true;
}

