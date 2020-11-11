#pragma once
#include "item.h"

struct global_item_t : public item_t
{
	item_t first;

	global_item_t();
	void push();
	void pop();
	bool read_ini( bool b_dry, bool b_shutdown, bool b_crc );
	bool read_local( ini::parser_t& parser );
};

inline global_item_t& global(){ static global_item_t g; return g; }

struct global_macro_t
{
	std::map<std::wstring,std::wstring> data;

	global_macro_t()
	{
		data[L"$date"] = get_timestamp(); // pre-defined macro
	}

	bool apply_to( ini::entry_t* e )
	{
		if(!wcschr(e->value.c_str(),L'$')) return false;
		bool b=false;
		for( auto& [key,value] : data )
		{
			if(!wcsstr(e->value.c_str(),key.c_str())) continue;
			e->value = str_replace(e->value.c_str(),key.c_str(),value.c_str());
			b = true;
		}
		return b;
	}

	static inline const wchar_t* get_timestamp()
	{
		static std::wstring tstamp; if(!tstamp.empty()) return tstamp.c_str();
		FILETIME f=::now(); FileTimeToLocalFileTime(&f,&f);
		std::wstring now = atow(path::timestamp(f));
		tstamp = now.substr(0,4)+L"-"+now.substr(4,2)+L"-"+now.substr(6,2);
		return tstamp.c_str();
	}
};