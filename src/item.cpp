#include <gxut/gxfilesystem.h>
#include <gxut/gxstring.h>
#include <gxut/gxargparse.h>
#include <gxut/gxiniparser.h>
#include <gxut/gxmemory.h>
#include "item.h"
#include "global.h"

void item_t::prebuild()
{
	build.name = name;
	build.method = method;
	build.src = src;
	build.dst = dst;
	build.cmd = get_command();
	build.opt = get_option();
	build.instance = instance;
	build.b.dry = global().b.dry||this->b.dry;
	build.b.crc = global().b.crc||this->b.crc;
	build.b.no_exclude = method==method_t::COPY||method==method_t::MOVE||is_custom();
	build.b.ssh = is_ssh();
	build.b.custom = is_custom();
	build.b.trivial_option = build.opt.empty() || has_trivial_option();
	build.b.use_include = use_include();
	build.b.instance = instance>0;

	// file/dir names
	build.nf.disp = include_file_names(true);	build.nf.run = include_file_names(false);
	build.xf.disp = exclude_file_names(true);	build.xf.run = exclude_file_names(false);
	build.xd.disp = exclude_dir_names(true);	build.xd.run = exclude_dir_names(false);
	build.gf.disp = ignore_file_names(true);	build.gf.run = ignore_file_names(false);
	build.gd.disp = ignore_dir_names(true);		build.gd.run = ignore_dir_names(false);
}

const wchar_t* item_t::get_names( names_t& v, const wchar_t* prefix, const wchar_t* postfix )
{
	if(v.empty()) return L"";
	std::wstring names;
	for( auto& s : v )
	{
		auto s1 = method==method_t::RSYNC?path(s).to_slash():path(s);
		names += format(L" %s%s%s", prefix,s1.auto_quote(), postfix );
	}
	return trim(names.c_str());
}

const wchar_t* item_t::get_option()
{
	std::wstring option1;
		
	if(method==method_t::ROBOCOPY)	option1 = L" /FFT /NDL /MIR /NP /R:0 /W:30";
	else if(method==method_t::SZIP)	option1 = L"-stl -bb3 -up0q0 -r -ssw -md48m -mx9 -mmt8"; // update option: https://sevenzip.osdn.jp/chm/cmdline/switches/update.htm
	else if(method==method_t::ZIP)	option1 = L"-stl -bb3 -up0q0 -r -mm=deflate -mx9 -mmt8"; // update option: https://sevenzip.osdn.jp/chm/cmdline/switches/update.htm
	else if(method==method_t::RSYNC)
	{
		option1 = use_include()?RSYNC_OPTION_DEFAULT_INCLUDE:RSYNC_OPTION_DEFAULT_LOCAL;
		if(is_ssh())			option1 += L" --chmod=ugo=rwX";				// windows should use /etc/fstab/
		if(is_synology())		option1 += L" --rsync-path=/usr/bin/rsync";	// fix with explicit rsync path to avoid connection refusal in Synology
		if(is_ssh()&&port!=22)	option1 += format(L" --rsh='ssh -p%d'", port );	// custom ssh port
	}

	return trim((this->opt.empty()?option1:option1+L" "+this->opt).c_str());
}

const wchar_t* item_t::get_command()
{
	if(method==method_t::ROBOCOPY)
	{
		return format( L"robocopy %s %s %s /XF %s /XD %s", src.auto_quote(), dst.auto_quote(), get_option(), exclude_file_names(), exclude_dir_names() );
	}
	else if(method==method_t::COPY||method==method_t::MOVE)
	{
		return format( L"%s /Y %s %s", method==method_t::COPY?L"copy":L"move", src.auto_quote(), dst.auto_quote() );
	}
	else if(method==method_t::CUSTOM)
	{
		return custom.c_str();
	}
	else if(method==method_t::RSYNC)
	{
		if(use_include())	return format( L"rsync %s %s --exclude=* %s %s", get_option(), include_file_names(), src_dir().cygwin().auto_quote(), dst.cygwin().auto_quote() );
		else 				return format( L"rsync %s %s %s %s %s %s %s",	 get_option(), ignore_file_names(), ignore_dir_names(), exclude_file_names(), exclude_dir_names(), src_dir().cygwin().auto_quote(), dst.cygwin().auto_quote() );
	}
	else if(method==method_t::SZIP||method==method_t::ZIP)
	{
		path ext = method==method_t::SZIP?".7z":".zip";
		path dst1 = dst.is_dir()||dst.back()==L'\\'?dst.add_backslash()+(src.is_dir()?src.dir_name():src.name()):dst;
		if(dst1.ext().empty()) dst1 = dst1+ext;
		// [CAUTION] - "src\\*" exclude src dir name in archive, which avoids scanning the parent dir (when using just "src")
		path src1 = src.is_dir()?(src.add_backslash()+L"*"):src;
		return format( L"%s u %s %s %s %s %s", SZIP_PATH, dst1.auto_quote(), src1.auto_quote(), get_option(), exclude_file_names(), exclude_dir_names() );
	}

	return L"";
}

void item_t::apply_name_filter( names_t& v )
{
	if(method==method_t::ROBOCOPY)
	{
		auto it=v.find(L"*esktop.ini"); if(it!=v.end()) v.erase(it);
		it=v.find(L"Desktop.ini"); if(it!=v.end()) v.erase(it);
		it=v.find(L"desktop.ini"); if(it!=v.end()) v.erase(it);
	}
}

// return src - sub
inline void diff_names( item_t::names_t& src, const item_t::names_t& sub )
{
	for( auto it=src.begin(); it!=src.end(); )
	{
		if(sub.find(*it)!=sub.end()) it=src.erase(it);
		else it++;
	}
}

item_t::names_t item_t::get_include_files()
{
	auto s=global().nf;
	s.insert(nf.begin(),nf.end());
	if(!src.is_dir()) s.insert(src.name().c_str());
	return s;
}

item_t::names_t item_t::get_exclude_files()
{
	names_t v=global().xf;
	v.insert(xf.begin(),xf.end());
	diff_names( v, get_include_files() ); // remove exclude when include exists
	return v;
}

item_t::names_t item_t::get_exclude_dirs()
{
	names_t v = global().xd;
	v.insert(xd.begin(),xd.end());
	return v;
}

item_t::names_t item_t::get_ignore_files()
{
	names_t v = global().gf; v.insert(gf.begin(),gf.end());
	diff_names( v, get_exclude_files() ); // remove ignore when exclude exists
	return v;
}

item_t::names_t item_t::get_ignore_dirs()
{
	names_t v = global().gd; v.insert(gd.begin(),gd.end());
	diff_names( v, get_exclude_dirs() ); // remove ignore when exclude exists
	return v;
}

const wchar_t* item_t::include_file_names( bool b_disp )
{
	names_t v;
	auto s = get_include_files();
	for( auto f : s )
	{
		if(f.empty()) continue; path p=path(f).to_slash(); const wchar_t* pc=p.c_str(); size_t l=p.size();
		if(!wcsstr(pc,L"/*")&&(src.add_backslash()+p).is_dir()) v.insert((p.add_slash()+"***").c_str()); // '***' includes all the subdirectories and files
		else if(l>4&&wcsstr(pc+l-4,L"/***"))	v.insert(pc);								// insert as is for *** pattern
		else if(l>3&&wcsstr(pc+l-3,L"/**"))		v.insert(str_replace(pc,L"/**",L"/***"));	// replace ** to ***
		else if(l>2&&wcsstr(pc+l-2,L"/*"))		v.insert(str_replace(pc,L"/*",L"/***"));	// replace ** to ***
		else if(l>2&&pc[l-1]!=L'*'&&wcschr(pc,L'/')) // add all the parents for them not to be excluded by exclude='*'
		{
			auto vc=explode_conservative(p.c_str(),L'/');
			std::wstring c;for(int j=0,jn=int(vc.size())-1;j<jn;j++){c+=vc[j];c+=L"/";if(vc[j][0])v.insert(c);}
			v.insert(pc);
		}
		else v.insert(pc); // insert as is for the others
	}
	return b_disp?str_replace(get_names(v),L"\\",L"/"):get_names(v,L"--include=");
}

const wchar_t* item_t::exclude_file_names( bool b_disp )
{
	bool br=b_disp||method==method_t::ROBOCOPY, b7=method==method_t::SZIP||method==method_t::ZIP;
	names_t v = get_exclude_files();
	if(!b_disp) for( auto& g : get_ignore_files() ) v.insert(g);
	apply_name_filter(v);
	return get_names( v, br?L"":b7?L"-xr!":L"--exclude=" );
}

const wchar_t* item_t::exclude_dir_names( bool b_disp )
{
	bool br=b_disp||method==method_t::ROBOCOPY, b7=method==method_t::SZIP||method==method_t::ZIP;
	names_t v = get_exclude_dirs();
	if(!b_disp) for( auto& g : get_ignore_dirs() ) v.insert(g);
	apply_name_filter(v);
	return get_names( v, br?L"":b7?L"-xr!":L"--exclude=" );
}

const wchar_t* item_t::ignore_file_names( bool b_disp )
{
	bool br=b_disp||method==method_t::ROBOCOPY, b7=method==method_t::SZIP||method==method_t::ZIP;
	names_t v = get_ignore_files();
	apply_name_filter(v);
	return get_names( v, br?L"":b7?L"-xr!":L"--filter='P ", br||b7?L"":L"'" ); 		// P === protect
}

const wchar_t* item_t::ignore_dir_names( bool b_disp )
{
	bool br=b_disp||method==method_t::ROBOCOPY, b7=method==method_t::SZIP||method==method_t::ZIP;
	names_t v = get_ignore_dirs();
	apply_name_filter(v);
	return get_names( v, br?L"":b7?L"-xr!":L"--filter='P ", br||b7?L"":L"'" );		// P === protect
}

void item_t::dump()
{
	printf("[%s]\n",wtoa(name.c_str()));
	printf("method = %d\n",method);
	printf("opt = %s\n",wtoa(opt.c_str()));
	printf("src = %s\n",wtoa(src.c_str()));
	printf("dst = %s\n",wtoa(dst.c_str()));
	printf("port = %d\n", port );
	printf("dry = %d\n", b.dry );
	printf("shutdown = %d\n", b.shutdown );
	printf("crc = %d\n", b.crc );
	printf("nf ="); for( auto& v : nf ) printf(" %s",wtoa(v.c_str())); printf("\n");
	printf("xf ="); for( auto& v : xf ) printf(" %s",wtoa(v.c_str())); printf("\n");
	printf("xd ="); for( auto& v : xd ) printf(" %s",wtoa(v.c_str())); printf("\n");
	printf("gf ="); for( auto& v : gf ) printf(" %s",wtoa(v.c_str())); printf("\n");
	printf("gd ="); for( auto& v : gd ) printf(" %s",wtoa(v.c_str())); printf("\n");
	printf("\n");
}
