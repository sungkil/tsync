#include <gxut/gxfilesystem.h>
#include <gxut/gxstring.h>
#include <gxut/gxargparse.h>
#include <gxut/gxiniparser.h>
#include <gxut/gxmemory.h>
#include "main.h"

#pragma warning( push )
#pragma warning( disable: 4819 )	// supress waring for different encoding
#include <tomlplusplus/toml.hpp>
#include <string_view>
using namespace std::literals::string_view_literals;
#include <fstream>
#include <iostream>
#pragma warning( pop )

bool read_toml( path toml_path, std::vector<item_t>& items )
{
	printf("@@@@ not implemented yet; test only @@@@\n\n");

	auto config = toml::parse_file( toml_path.wtoa() );
	if (!config)
    {
        std::cerr
            << "Error parsing file '" << *config.error().source().path
            << "':\n" << config.error().description()
            << "\n  (" << config.error().source().begin << ")\n";
        return false;
    }


	std::string_view library_name = config["library"]["name2"].value_or(""sv);

	for (auto&& [k, v] : config)
	{
		printf( "[%s]\n", k.c_str() );
		v.visit([](auto& node) noexcept
		{
			for( auto&& [s,t] : *node.as_table())
			{
				auto v = t.value_or("test"sv);
				auto v2 = t.as_string();
				printf( "%s ** %s\n", s.c_str(), v2?v2->get().c_str():"no"  );

				if constexpr (toml::is_string<decltype(v2)>)
				{
					printf( "** %s\n", v2.as<std::string>()->get().c_str() );
				}

			}
		});
		printf("\n");
	}

	return false;
}