#pragma once
#include "item.h"
#include "reader.h"
#include "global.h"

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