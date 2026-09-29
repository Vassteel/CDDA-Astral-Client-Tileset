// One-shot: update (or create) a .sav.zzip entry from a plain .sav file.
#include "zzip.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

int main( int argc, char **argv )
{
    if( argc != 3 ) {
        std::cerr << "usage: zzip_update_file <file.sav.zzip> <file.sav>\n";
        return 2;
    }
    std::filesystem::path zpath = argv[1];
    std::filesystem::path spath = argv[2];
    std::ifstream in( spath, std::ios::binary );
    if( !in ) {
        std::cerr << "cannot read " << spath << "\n";
        return 1;
    }
    std::stringstream buf;
    buf << in.rdbuf();
    std::string content = buf.str();
    std::optional<zzip> z = zzip::load( zpath );
    if( !z ) {
        std::cerr << "zzip::load failed for " << zpath << "\n";
        return 1;
    }
    std::filesystem::path entry = spath.filename();
    if( !z->add_file( entry, content ) ) {
        std::cerr << "add_file failed\n";
        return 1;
    }
    std::filesystem::path tmp = zpath;
    tmp += ".tmp";
    if( !z->compact_to( tmp, 2.0 ) ) {
        std::cerr << "compact_to failed\n";
        return 1;
    }
    z.reset();
    std::error_code ec;
    std::filesystem::rename( tmp, zpath, ec );
    if( ec ) {
        std::cerr << "rename failed: " << ec.message() << "\n";
        return 1;
    }
    std::cout << "updated " << zpath << " entry " << entry << " bytes " << content.size() << "\n";
    return 0;
}
