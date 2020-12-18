#pragma once
#include <regex>

static const wchar_t* raw_delim = L" _-(.)";
static const wchar_t* delim = L"\\(?|\\)?|\\.?|_?|-?";
static std::wstring pattern = format( L"((%s)(\\d{4}|\\d{2})(%s)(\\d{1,2})(%s)(\\d{1,2})(%s)(%s))", delim, delim, delim, delim, delim );

struct date_t
{
	std::wstring raw_match;
	std::wstring match;
	int y=0,m=0,d=0;
	bool left = false;
	bool right = false;
	bool empty() const { return y==0||match.empty(); }
	int as_int() const { return d+m*100+y*10000; }
};

inline bool is_valid_year( int y ){ static SYSTEMTIME n=FileTimeToSystemTime(now()); return y>2010&&y<=n.wYear; }
inline bool is_valid_month( int m ){ return m>=1&&m<13; }
inline bool is_valid_day( int m, int d )
{
	if(d<1||d>31) return false;
	if(m==2&&d>29) return false;
	if((m==4||m==6||m==9||m==11)&&d>30) return false;
	return true;
}

bool is_valid_year( std::wstring y ){ return is_valid_year( _wtoi(y.c_str()) ); }
bool is_valid_month( std::wstring m ){ return is_valid_month( _wtoi(m.c_str()) ); }
bool is_valid_day( std::wstring m, std::wstring d ){ return is_valid_day( _wtoi(m.c_str()), _wtoi(d.c_str()) ); }

date_t match_to_date( std::wcmatch& r, bool b_debug=false )
{
	static SYSTEMTIME snow = FileTimeToSystemTime(now());
	date_t d; if(r.size()<10) return d;

	if(b_debug)
	{
		uint k=0;
		for( auto& s : r )
			printf("[%02d] %s\n",k++,wtoa(s.str().c_str()));
	}
	
	std::wstring y=r[3].str();	// year
	std::wstring m=r[5].str();	// month
	std::wstring a=r[7].str();	// day

	// convert 6-digit (2-2-2 as 4-2) to 2-2-2
	if(y.length()==4&&!is_valid_year(y))
	{
		if(b_debug) wprintf(L"%s(): %s-%s-%s >> ", atow(__func__), y.c_str(), m.c_str(), a.c_str() );
		a = m.size()<2?m+a:m;
		m = y.substr(2,2);
		y = y.substr(0,2);
		if(b_debug) wprintf(L"%s-%s-%s\n\n", y.c_str(), m.c_str(), a.c_str() );
	}
	else if(y.length()==2&&!is_valid_year(y)&&m.length()==1&&a.length()==1) // e.g., 1030
	{
		a = m+a;
		m = y;
		y = itow(snow.wYear);
	}

	d.y = _wtoi(y.c_str());	if(d.y>9&&d.y<100) d.y += 2000; // year
	d.m = _wtoi(m.c_str());	// month
	d.d = _wtoi(a.c_str());	// day

	if(!is_valid_year(d.y))		return date_t();
	if(!is_valid_month(d.m))	return date_t();
	if(!is_valid_day(d.m,d.d))	return date_t();

	d.raw_match = r[0].str();
	d.match = r[1].str();
	
	return d;
}

date_t find_date_in_path( path src, bool b_debug=false )
{
	date_t d;

	// build 2 patterns for front and back patterns
	std::vector<std::wstring> patterns =
	{
		std::wstring(L"^")+pattern,		// front pattern
		pattern+std::wstring(L"$"),		// back pattern
		std::wstring(L"IMG_")+pattern,	// image pattern
		std::wstring(L"PXL_")+pattern,	// image pattern
		std::wstring(L"VID_")+pattern,	// video pattern
		std::wstring(L"Screenshot_")+pattern,	// capture pattern
	};
	
	for( const auto& exp : patterns )
	{
		std::wcmatch r;
		std::regex_search( src.c_str(), r, std::wregex(exp) ); if(r.empty()) continue;
		date_t t = match_to_date(r,b_debug); if(t.empty()) continue;
		if(!d.empty()&&t.match.size()<d.match.size()) continue;

		d = t;
		d.left = exp.front()==L'^';
		d.right = exp.back()==L'$';
	}

	return d;
}