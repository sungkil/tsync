#pragma once
#include <gxut/gxfilesystem.h>
#include <gxut/gxstring.h>
#include <gxut/gxargparse.h>
#include <gxut/gxiniparser.h>
#include <gxut/gxmemory.h>

static const path		SZIP_PATH_FULL = L"C:\\Program Files\\7-Zip\\7z.exe";
static const path		SZIP_PATH = SZIP_PATH_FULL.exists()?SZIP_PATH_FULL.auto_quote():SZIP_PATH_FULL.name().auto_quote();
static const wchar_t*	RSYNC_OPTION_DEFAULT_INCLUDE = L"-rtDvzLOm"; // L copy symlinks as referent dir/files, -m: --prune-empty-dirs
static const wchar_t*	RSYNC_OPTION_DEFAULT_LOCAL = L"-rtDvzL --delete --delete-excluded"; // default local option; -O: ignore dir timestamps
static const wchar_t*	RSYNC_OPTION_EXCLUDE = L" --delete --delete-excluded";
static std::set<path>	ALL_EXTENSIONS = { L"ini", L"tsync", L"toml", L"lua" };
static std::set<path>	INI_EXTENSIONS = { L"ini", L"tsync" };
static const char*		INI_EXT_FILTER(){ static std::string s; if(s.empty()){ for(auto& e:INI_EXTENSIONS)s+=format("|*.%s",wtoa(e.c_str()));s=s.substr(1,s.length()-1); } return s.c_str(); }
static const int		MAX_INSTANCE = 64;

enum class method_t { UNKNOWN=0, RSYNC=1, ROBOCOPY=2, SZIP=3, ZIP=4, COPY=5, MOVE=6, CUSTOM=7 };

// common item types
struct base_item_t
{
	std::wstring		name;
	method_t			method;
	path				src;
	path				dst;
	std::wstring		opt;
	int					instance;

	const char* aname(){ return wtoa(name.c_str()); }
};

struct item_build_t : public base_item_t
{
	std::wstring	cmd;
	struct { bool dry, crc, no_exclude, ssh, custom, trivial_option, use_include, instance; } b;
	struct { std::wstring run, disp; } nf, xf, xd, gf, gd;
	
	bool exec();
	bool apply_instance( bool b_dry );
};

struct item_t : public base_item_t
{
	using names_t = std::set<std::wstring>;
	
	std::wstring	custom;			// custom cmdline; method should be CUSTOM
	item_build_t	build;			// built items
	int				port=0;			// ssh port
	int				instance=0;		// instances for incremental backup
	
	// misc. flags
	struct { bool dry=false, shutdown=false, crc=false; } b;

	names_t nf;		// include files (excluding all the others)
	names_t xf;		// exclude files/dirs
	names_t xd;		// exclude dirs
	names_t gf;		// ignore files/dirs
	names_t gd;		// ignore dirs
	
	item_t( const wchar_t* name, method_t m=method_t::UNKNOWN, const wchar_t* src=L"", const wchar_t* dst=L"" ){ this->name=name; this->method=m; this->src=src; this->dst=dst; if(this->src.is_dir()) this->src=this->src.add_backslash(); }
	path src_dir(){ return src.is_dir()?src:src.dir(); }
	bool is_custom(){ return method==method_t::CUSTOM; }
	bool is_ssh(){ return src.is_ssh()||dst.is_ssh(); }
	bool is_synology(){ return src.is_synology()||dst.is_synology(); }
	bool has_trivial_option(){ if(opt.empty()) return true; if(method!=method_t::RSYNC) return false; std::wstring o=get_option(); return o.empty()||o==RSYNC_OPTION_DEFAULT_INCLUDE||o==RSYNC_OPTION_DEFAULT_LOCAL; }
	bool use_include(){ return method==method_t::RSYNC&&(!nf.empty()||(!is_ssh()&&!is_synology()&&!src.is_dir()&&src.exists())); }
	bool use_instance(){ return (method==method_t::RSYNC||method==method_t::ROBOCOPY)&&instance>0&&instance<=MAX_INSTANCE; }

	const wchar_t* get_names( names_t& v, const wchar_t* prefix=L"", const wchar_t* postfix=L"" );
	const wchar_t* get_option();
	const wchar_t* get_command();
	void apply_name_filter( names_t& v );

	names_t get_include_files();
	names_t get_exclude_files();
	names_t get_exclude_dirs();
	names_t get_ignore_files();
	names_t get_ignore_dirs();

	const wchar_t* include_file_names( bool b_disp=false );
	const wchar_t* exclude_file_names( bool b_disp=false );
	const wchar_t* exclude_dir_names( bool b_disp=false );
	const wchar_t* ignore_file_names( bool b_disp=false );
	const wchar_t* ignore_dir_names( bool b_disp=false );

	// for debugging
	bool operator==( const item_t& other ) const { return method==other.method && name==other.name && opt==other.opt && src==other.src && dst==other.dst && nf==other.nf && xf==other.xf && xd==other.xd && gf==other.gf && gd==other.gd && port==other.port && memcmp(&b,&other.b,sizeof(b))==0; }
	bool operator!=( const item_t& other ) const { return !operator==(other); }
	void dump();

	// prebuild items for recursive imported execution
	void prebuild();
};

inline method_t str2method( const char* str )
{
	std::string s=tolower(str);
	return s=="robocopy"?method_t::ROBOCOPY:
		s=="zip"?method_t::ZIP:s=="7zip"?method_t::SZIP:s=="7-zip"?method_t::SZIP:s=="7z"?method_t::SZIP:
		s=="copy"?method_t::COPY:s=="move"?method_t::MOVE:s=="rename"?method_t::MOVE:
		s=="custom"?method_t::CUSTOM:
		method_t::RSYNC;
}

inline const char* method2str( method_t cmd )
{
	return cmd==method_t::ROBOCOPY?"robocopy":
		cmd==method_t::SZIP?"7zip":
		cmd==method_t::ZIP?"zip":
		cmd==method_t::COPY?"copy":
		cmd==method_t::MOVE?"move":
		cmd==method_t::RSYNC?"rsync":
		cmd==method_t::CUSTOM?"custom":
		"UNKNOWN";
}
