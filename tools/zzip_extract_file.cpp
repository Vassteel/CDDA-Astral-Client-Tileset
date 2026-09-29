// One-shot: extract a .sav entry from .sav.zzip to a plain file.
#include "zzip.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

int main( int argc, char **argv )
{
    if( argc != 3 ) {
        std::cerr << "usage: zzip_extract_file <file.sav.zzip> <out.sav>\n";
        return 2;
    }
    std::filesystem::path zpath = argv[1];
    std::filesystem::path opath = argv[2];
    std::optional<zzip> z = zzip::load( zpath );
    if( !z ) {
        std::cerr << "zzip::load failed for " << zpath << "\n";
        return 1;
    }
    std::filesystem::path entry = opath.filename();
    // Prefer entry matching basename of out path; also try without path quirks
    std::vector<std::byte> content = z->get_file( entry );
    if( content.empty() ) {
        // try common CDDA hashed name from zzip path stem
        std::filesystem::path alt = zpath.filename();
        // strip .zzip then if ends with .sav keep
        std::string name = alt.string();
        if( name.size() > 5 && name.substr( name.size() - 5 ) == ".zzip" ) {
            name = name.substr( 0, name.size() - 5 );
        }
        content = z->get_file( name );
        entry = name;
    }
    if( content.empty() ) {
        std::cerr << "get_file empty for entry " << entry << "\n";
        return 1;
    }
    std::ofstream out( opath, std::ios::binary );
    if( !out ) {
        std::cerr << "cannot write " << opath << "\n";
        return 1;
    }
    out.write( reinterpret_cast<const char *>( content.data() ),
               static_cast<std::streamsize>( content.size() ) );
    std::cout << "extracted " << entry << " bytes " << content.size()
              << " -> " << opath << "\n";
    return 0;
}
