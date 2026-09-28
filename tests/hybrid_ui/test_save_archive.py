"""Exercise the actual archive helper with disposable files and injected rename failure."""
from pathlib import Path
import subprocess
import tempfile
source = (Path(__file__).resolve().parents[2] / 'src/main_menu.cpp').read_text()
start = source.index('static bool archive_character_save(')
end = source.index('\nbool main_menu::load_character_tab', start)
helper = source[start:end].replace('namespace fs = std::filesystem;', 'namespace fs = test_fs;')
preamble = r'''
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cassert>
#include <iostream>
namespace fs = std::filesystem;
namespace test_fs {
using fs::path; using fs::directory_iterator; using fs::directory_entry;
using fs::is_regular_file; using fs::exists; using fs::create_directories;
int fail_after = -1;
void rename(const path &from, const path &to, std::error_code &ec) {
    if(fail_after == 0) { fail_after=-1; ec=std::make_error_code(std::errc::permission_denied); return; }
    if(fail_after > 0) --fail_after;
    fs::rename(from,to,ec);
}
}
#define _(text) text
template<typename... Args> void popup(const char *, Args&&...) {}
struct world_path { fs::path value; fs::path get_unrelative_path() const {return value;} };
struct WORLD { fs::path root; world_path folder_path() const {return {root};} };
struct save_t { std::string id; std::string base_path() const {return id;} };
void write(const fs::path &p, const std::string &value="fixture") { std::ofstream(p)<<value; }
std::string read(const fs::path &p) {std::ifstream f(p); return {std::istreambuf_iterator<char>(f),{}};}
'''
test = r'''
int main(int argc,char **argv) {
    assert(argc==2); fs::path root=argv[1]; fs::create_directories(root);
    WORLD world{root}; save_t hero{"hero"};
    write(root/"hero.sav","character"); write(root/"hero.pt","time");
    fs::create_directories(root/"hero.mm1"); write(root/"hero.mm1"/"memory","seen");
    write(root/"heroLong.sav","other"); write(root/"master.gsav","world");
    assert(archive_character_save(world,hero));
    auto first=root/"deleted_characters"/"hero-1";
    assert(!fs::exists(root/"hero.sav")); assert(read(first/"hero.sav")=="character");
    assert(read(first/"hero.mm1"/"memory")=="seen");
    assert(read(root/"heroLong.sav")=="other" && read(root/"master.gsav")=="world");
    std::cout<<"PASS character archive, map memory, other-save and shared-world preservation\n";
    write(root/"hero.sav","second"); assert(archive_character_save(world,hero));
    assert(read(root/"deleted_characters"/"hero-2"/"hero.sav")=="second");
    assert(read(first/"hero.sav")=="character");
    std::cout<<"PASS repeat removal keeps previous backup\n";
    assert(!archive_character_save(world,hero));
    std::cout<<"PASS missing primary is rejected\n";
    WORLD blocked{root/"blocked"}; fs::create_directory(blocked.root);
    write(blocked.root/"hero.sav"); write(blocked.root/"deleted_characters");
    assert(!archive_character_save(blocked,hero)); assert(fs::exists(blocked.root/"hero.sav"));
    std::cout<<"PASS backup-folder failure leaves save intact\n";
    write(root/"hero.sav","rollback"); write(root/"hero.pt","rollback-time");
    test_fs::fail_after=1; assert(!archive_character_save(world,hero));
    assert(read(root/"hero.sav")=="rollback" && read(root/"hero.pt")=="rollback-time");
    std::cout<<"PASS injected partial-move failure rolls back\n";
}
'''
with tempfile.TemporaryDirectory(prefix='astral-archive-test-') as temp:
    temp=Path(temp)
    cpp=temp/'test.cpp'; cpp.write_text(preamble+helper+test)
    subprocess.run(['g++','-std=c++17','-O0',str(cpp),'-o',str(temp/'test')],check=True)
    subprocess.run([str(temp/'test'),str(temp/'fixture')],check=True)
