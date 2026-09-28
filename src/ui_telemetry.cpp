#include "ui_telemetry.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <locale>
#include <mutex>
#include <sstream>
#include <string_view>
#include <thread>

namespace ui_telemetry
{
namespace
{
using clock = std::chrono::steady_clock;
struct trace_state {
    std::mutex mutex;
    FILE *file = nullptr;
    std::string path;
    std::string session;
    size_t size = 0;
    size_t limit = 8 * 1024 * 1024;
    uint64_t sequence = 0;
    clock::time_point start;
};

trace_state &state()
{
    // UI/static destructors may run after normal logger shutdown.
    static trace_state *value = new trace_state;
    return *value;
}

std::string json_quote( std::string_view value )
{
    size_t length = std::min<size_t>( value.size(), 4096 );
    while( length < value.size() && length > 0 &&
           ( static_cast<unsigned char>( value[length] ) & 0xc0 ) == 0x80 ) {
        --length;
    }
    std::string result = "\"";
    const char *hex = "0123456789abcdef";
    for( const unsigned char ch : value.substr( 0, length ) ) {
        if( ch == '"' || ch == '\\' ) {
            result += '\\';
            result += ch;
        } else if( ch < 0x20 ) {
            result += "\\u00";
            result += hex[ch >> 4];
            result += hex[ch & 15];
        } else {
            result += ch;
        }
    }
    if( length < value.size() ) {
        result += "...";
    }
    return result + '"';
}

bool rotate( trace_state &s )
{
    if( s.file ) {
        std::fclose( s.file );
        s.file = nullptr;
    }
    std::error_code error;
    std::filesystem::remove( s.path + ".3", error );
    if( error ) {
        return false;
    }
    for( int i = 2; i >= 0; --i ) {
        const std::string old = s.path + ( i ? "." + std::to_string( i ) : "" );
        if( std::filesystem::exists( old, error ) ) {
            std::filesystem::rename( old, s.path + "." + std::to_string( i + 1 ), error );
        }
        if( error ) {
            return false;
        }
    }
    s.file = std::fopen( s.path.c_str(), "wb" );
    s.size = 0;
    return s.file != nullptr;
}
}

uint64_t record( const std::string &event, fields values ) noexcept
{
    try {
        trace_state &s = state();
        const std::lock_guard<std::mutex> lock( s.mutex );
        if( !s.file ) {
            return 0;
        }
        const auto epoch = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::system_clock::now().time_since_epoch() ).count();
        const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
                                 clock::now() - s.start ).count();
        const uint64_t seq = ++s.sequence;
        std::ostringstream out;
        out.imbue( std::locale::classic() );
        out << "{\"schema\":1,\"session\":" << json_quote( s.session ) << ",\"seq\":" << seq
            << ",\"unix_ms\":" << epoch << ",\"elapsed_us\":" << elapsed
            << ",\"thread\":\"" << std::this_thread::get_id() << "\",\"event\":" << json_quote( event )
            << ",\"fields\":{";
        bool first = true;
        size_t count = 0;
        for( const auto &value : values ) {
            if( count++ == 24 ) {
                break;
            }
            out << ( first ? "" : "," ) << json_quote( value.first ) << ':' << json_quote( value.second );
            first = false;
        }
        out << "}}\n";
        const std::string line = out.str();
        if( s.size && s.size + line.size() > s.limit && !rotate( s ) ) {
            return 0;
        }
        if( std::fwrite( line.data(), 1, line.size(), s.file ) != line.size() ||
            std::fflush( s.file ) != 0 ) {
            std::fclose( s.file );
            s.file = nullptr;
            return 0;
        }
        s.size += line.size();
        return seq;
    } catch( ... ) {
        // Diagnostics must never prevent the game from running.
        return 0;
    }
}

void initialize( const std::string &path, const std::string &version, size_t max_bytes ) noexcept
{
    try {
        const char *setting = std::getenv( "CDDA_UI_TELEMETRY" );
        if( setting && std::string_view( setting ) == "0" ) {
            return;
        }
        {
            trace_state &s = state();
            const std::lock_guard<std::mutex> lock( s.mutex );
            s.path = path;
            s.limit = std::max<size_t>( 512, max_bytes );
            s.sequence = 0;
            s.start = clock::now();
            s.session = std::to_string( std::chrono::system_clock::now().time_since_epoch().count() );
            const std::filesystem::path parent = std::filesystem::path( path ).parent_path();
            if( !parent.empty() ) {
                std::filesystem::create_directories( parent );
            }
            if( !rotate( s ) ) {
                return;
            }
        }
        record( "session.start", {{ "version", version }, { "limit_bytes", std::to_string( max_bytes ) }} );
    } catch( ... ) {
        return;
    }
}

void shutdown() noexcept
{
    record( "session.end" );
    try {
        trace_state &s = state();
        const std::lock_guard<std::mutex> lock( s.mutex );
        if( s.file ) {
            std::fclose( s.file );
            s.file = nullptr;
        }
    } catch( ... ) {
        return;
    }
}

bool meaningful_action( const std::string &action )
{
    return !action.empty() && action != "TIMEOUT" && action != "ANY_INPUT" &&
           action != "COORDINATE" && action != "MOUSE_MOVE" && action != "ERROR";
}

scope::scope( const std::string &event, fields values, bool enabled ) :
    name( event ), exceptions( std::uncaught_exceptions() )
{
    if( enabled ) {
        begin_sequence = record( name + ".begin", values );
    }
}

scope::~scope()
{
    if( begin_sequence ) {
        try {
            record( name + ".end", {{ "begin_seq", std::to_string( begin_sequence ) },
                { "unwinding", std::uncaught_exceptions() > exceptions ? "true" : "false" }} );
        } catch( ... ) {
            return;
        }
    }
}
}
