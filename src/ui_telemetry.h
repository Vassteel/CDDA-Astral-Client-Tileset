#pragma once
#ifndef CATA_SRC_UI_TELEMETRY_H
#define CATA_SRC_UI_TELEMETRY_H

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <utility>

// Local diagnostic breadcrumbs. No network transport or raw key/text capture.
namespace ui_telemetry
{
using fields = std::initializer_list<std::pair<std::string, std::string>>;
void initialize( const std::string &path, const std::string &version,
                 size_t max_bytes = 8 * 1024 * 1024 ) noexcept;
void shutdown() noexcept;
uint64_t record( const std::string &event, fields values = {} ) noexcept;
bool meaningful_action( const std::string &action );

// An unmatched begin identifies the last operation interrupted by a crash.
// "end" means the call returned, not that the gameplay operation succeeded.
class scope
{
    public:
        explicit scope( const std::string &event, fields values = {}, bool enabled = true );
        ~scope();
        scope( const scope & ) = delete;
        scope &operator=( const scope & ) = delete;
    private:
        std::string name;
        uint64_t begin_sequence = 0;
        int exceptions = 0;
};
}
#endif
