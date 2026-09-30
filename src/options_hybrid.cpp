#include "options_hybrid.h"

#if defined(TILES)

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstring>

#include "imgui/imgui.h"

#include "cata_imgui.h"
#include "output.h"
#include "string_formatter.h"
#include "translations.h"
#include "ui_hybrid_chrome.h"
#include "ui_hybrid_widgets.h"

namespace
{
namespace w = ui_hybrid_widgets;
namespace theme = ui_hybrid_chrome::theme;

std::string lower( std::string t )
{
    std::transform( t.begin(), t.end(), t.begin(), []( unsigned char c ) {
        return std::tolower( c );
    } );
    return t;
}
} // namespace

options_hybrid_view::options_hybrid_view( options_manager &mgr_,
        options_manager::options_container *world_, bool ingame_, bool world_only_ )
    : mgr( mgr_ ), global( mgr_.options ), world( world_ != nullptr ? *world_ : mgr_.options ),
      ingame( ingame_ ), world_only( world_only_ )
{
    for( size_t i = 0; i < mgr.pages_.size(); ++i ) {
        if( mgr.pages_[i].id_ == "world_default" ) {
            world_page = static_cast<int>( i );
        }
    }
    page = world_only && world_page >= 0 ? world_page : 0;
    // Groups start open: the whole page is browsable without extra clicks.
    for( const options_manager::Group &g : mgr.groups_ ) {
        groups_open.emplace( g.id_, true );
    }
}

options_manager::options_container &options_hybrid_view::container_for_page( int p )
{
    return ( ingame || world_only ) && p == world_page ? world : global;
}

bool options_hybrid_view::matches_filter( const options_manager::cOpt &opt ) const
{
    if( filter.empty() ) {
        return true;
    }
    const std::string needle = lower( filter );
    return lower( opt.getMenuText() ).find( needle ) != std::string::npos ||
           lower( opt.getTooltip() ).find( needle ) != std::string::npos ||
           lower( opt.getName() ).find( needle ) != std::string::npos;
}

void options_hybrid_view::draw_tabs()
{
    const float s = theme::scale();
    const ui_hybrid_chrome::theme::tokens &tk = theme::get();
    if( !world_only ) {
        for( int i = 0; i < static_cast<int>( mgr.pages_.size() ); ++i ) {
            if( i > 0 ) {
                ImGui::SameLine( 0.f, tk.xs * s );
            }
            const std::string label = ingame && i == world_page ? _( "Current world" ) :
                                      mgr.pages_[i].name_.translated();
            if( w::tab( label.c_str(), page == i ) ) {
                page = i;
                described.clear();
            }
        }
        ImGui::SameLine( 0.f, tk.lg * s );
    }
    // Search box, right of the tabs.
    char buf[128];
    std::snprintf( buf, sizeof( buf ), "%s", filter.c_str() );
    const float search_w = std::min( 320.f * s, ImGui::GetContentRegionAvail().x );
    ImGui::SetNextItemWidth( search_w );
    if( ImGui::InputTextWithHint( "##options_filter", _( "Search options…" ), buf, sizeof( buf ) ) ) {
        filter = buf;
    }
    if( w::probe::enabled() ) {
        w::probe::record( "input", "options_filter", ImGui::GetItemRectMin(), ImGui::GetItemRectMax() );
    }
    ImDrawList *draw = ImGui::GetWindowDrawList();
    const ImVec2 p = ImGui::GetCursorScreenPos();
    draw->AddRectFilled( ImVec2( p.x, p.y ), ImVec2( p.x + ImGui::GetContentRegionAvail().x, p.y + 1.f ),
                         tk.edge_quiet );
    ImGui::Dummy( ImVec2( 0.f, tk.xs * s ) );
}

void options_hybrid_view::draw_option_row( options_manager::cOpt &opt, bool enabled,
        const std::string &reason )
{
    const float s = theme::scale();
    const ui_hybrid_chrome::theme::tokens &tk = theme::get();
    const std::string type = opt.getType();
    const std::string name = opt.getName();
    ImGui::PushID( name.c_str() );
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex( 0 );
    ImGui::AlignTextToFramePadding();
    const float label_w = ImGui::GetContentRegionAvail().x;
    const std::string label = w::fit_text( opt.getMenuText(), label_w );
    ImGui::TextColored( enabled ? ui_hybrid_chrome::palette::text() : ui_hybrid_chrome::palette::text_muted(),
                        "%s", label.c_str() );
    const bool label_hovered = ImGui::IsItemHovered();
    if( w::probe::enabled() ) {
        w::probe::record( "option_label", opt.getMenuText(), ImGui::GetItemRectMin(), ImGui::GetItemRectMax() );
    }
    ImGui::TableSetColumnIndex( 1 );
    bool control_hovered = false;
    if( type == "bool" ) {
        bool v = opt.value_as<bool>();
        if( w::toggle( "##v", v, enabled, reason.empty() ? nullptr : reason.c_str() ) ) {
            opt.setValue( v ? "true" : "false" );
            dirty = true;
            edited = true;
        }
        control_hovered = ImGui::IsItemHovered( ImGuiHoveredFlags_AllowWhenDisabled );
        ImGui::SameLine( 0.f, tk.sm * s );
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored( ui_hybrid_chrome::palette::text_muted(), "%s", opt.getValueName().c_str() );
    } else if( type == "string_select" ) {
        const std::vector<options_manager::id_and_option> items = opt.getItems();
        std::vector<std::string> names;
        names.reserve( items.size() );
        int selected = -1;
        for( int i = 0; i < static_cast<int>( items.size() ); ++i ) {
            names.push_back( items[i].second.translated() );
            if( items[i].first == opt.getValue() ) {
                selected = i;
            }
        }
        const int chosen = w::dropdown( "##v", opt.getValueName(), names, selected, -1.f, enabled );
        control_hovered = ImGui::IsItemHovered( ImGuiHoveredFlags_AllowWhenDisabled );
        if( chosen >= 0 && chosen != selected ) {
            opt.setValue( items[chosen].first );
            dirty = true;
            edited = true;
        }
    } else if( type == "int_map" ) {
        std::vector<std::string> names;
        int selected = -1;
        const int cur = opt.value_as<int>();
        for( int i = 0; i < static_cast<int>( opt.mIntValues.size() ); ++i ) {
            names.push_back( opt.mIntValues[i].second.translated() );
            if( opt.mIntValues[i].first == cur ) {
                selected = i;
            }
        }
        const int chosen = w::dropdown( "##v", opt.getValueName(), names, selected, -1.f, enabled );
        control_hovered = ImGui::IsItemHovered( ImGuiHoveredFlags_AllowWhenDisabled );
        if( chosen >= 0 && chosen != selected ) {
            opt.setValue( opt.mIntValues[chosen].first );
            dirty = true;
            edited = true;
        }
    } else if( type == "int" ) {
        int v = opt.value_as<int>();
        if( !enabled ) {
            ImGui::BeginDisabled( true );
        }
        ImGui::SetNextItemWidth( -FLT_MIN );
        const std::string fmt = opt.format.empty() ? "%d" : opt.format;
        if( ImGui::SliderInt( "##v", &v, opt.iMin, opt.iMax, fmt.c_str(), ImGuiSliderFlags_AlwaysClamp ) ) {
            opt.setValue( v );
            dirty = true;
            edited = true;
        }
        if( !enabled ) {
            ImGui::EndDisabled();
        }
        control_hovered = ImGui::IsItemHovered( ImGuiHoveredFlags_AllowWhenDisabled );
    } else if( type == "float" ) {
        float v = opt.value_as<float>();
        if( !enabled ) {
            ImGui::BeginDisabled( true );
        }
        ImGui::SetNextItemWidth( -FLT_MIN );
        const std::string fmt = opt.format.empty() ? "%.2f" : opt.format;
        if( ImGui::SliderFloat( "##v", &v, opt.fMin, opt.fMax, fmt.c_str(), ImGuiSliderFlags_AlwaysClamp ) ) {
            if( opt.fStep > 0.f ) {
                v = std::round( v / opt.fStep ) * opt.fStep;
            }
            opt.setValue( v );
            dirty = true;
            edited = true;
        }
        if( !enabled ) {
            ImGui::EndDisabled();
        }
        control_hovered = ImGui::IsItemHovered( ImGuiHoveredFlags_AllowWhenDisabled );
    } else if( type == "string_input" ) {
        std::string &edit = text_edits[name];
        if( !ImGui::IsAnyItemActive() || edit.empty() ) {
            edit = opt.getValue();
        }
        char buf[512];
        std::snprintf( buf, sizeof( buf ), "%s", edit.c_str() );
        if( !enabled ) {
            ImGui::BeginDisabled( true );
        }
        ImGui::SetNextItemWidth( -FLT_MIN );
        const int max_len = opt.getMaxLength() > 0 ? std::min( opt.getMaxLength() + 1, 512 ) : 512;
        if( ImGui::InputText( "##v", buf, max_len ) ) {
            edit = buf;
        }
        if( ImGui::IsItemDeactivatedAfterEdit() ) {
            opt.setValue( edit );
            dirty = true;
            edited = true;
        }
        if( !enabled ) {
            ImGui::EndDisabled();
        }
        control_hovered = ImGui::IsItemHovered( ImGuiHoveredFlags_AllowWhenDisabled );
    } else {
        ImGui::TextColored( ui_hybrid_chrome::palette::text_muted(), "%s", opt.getValueName().c_str() );
        control_hovered = ImGui::IsItemHovered();
    }
    if( label_hovered || control_hovered ) {
        described = name;
        described_group.clear();
    }
    ImGui::PopID();
}

void options_hybrid_view::draw_page( float height )
{
    const float s = theme::scale();
    const ui_hybrid_chrome::theme::tokens &tk = theme::get();
    if( page < 0 || page >= static_cast<int>( mgr.pages_.size() ) ) {
        return;
    }
    const options_manager::Page &pg = mgr.pages_[page];
    options_manager::options_container &cont = container_for_page( page );
    ( void ) height;
    if( ingame && page == world_page ) {
        ImGui::PushTextWrapPos( ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x );
        ImGui::TextColored( ui_hybrid_chrome::palette::warning(), "%s",
                            _( "Note: some of these options may produce unexpected results if changed." ) );
        ImGui::PopTextWrapPos();
        ImGui::Dummy( ImVec2( 0.f, tk.xs * s ) );
    }
    bool table_open = false;
    const auto begin_table = [&]() {
        if( table_open ) {
            return;
        }
        table_open = ImGui::BeginTable( "##options_rows", 2, ImGuiTableFlags_SizingStretchProp |
                                        ImGuiTableFlags_RowBg | ImGuiTableFlags_NoBordersInBody );
        if( table_open ) {
            ImGui::TableSetupColumn( "label", ImGuiTableColumnFlags_WidthStretch, 0.5f );
            ImGui::TableSetupColumn( "value", ImGuiTableColumnFlags_WidthStretch, 0.5f );
        }
    };
    const auto end_table = [&]() {
        if( table_open ) {
            ImGui::EndTable();
            table_open = false;
        }
    };
    int shown = 0;
    for( const options_manager::PageItem &item : pg.items_ ) {
        if( item.type == options_manager::ItemType::BlankLine ) {
            continue;
        }
        if( item.type == options_manager::ItemType::GroupHeader ) {
            if( !filter.empty() ) {
                continue; // a search flattens the groups
            }
            end_table();
            const options_manager::Group &g = mgr.find_group( item.data );
            bool &open = groups_open[g.id_];
            ImGui::PushID( g.id_.c_str() );
            const w::row_state st;
            const w::row_result r = w::selectable_row( "group", g.name_.translated(), std::string(),
                                    open ? "chevron_down" : "chevron_right", nullptr, st, tk.row_compact );
            if( r.clicked ) {
                open = !open;
            }
            if( r.hovered ) {
                described.clear();
                described_group = g.id_;
            }
            ImGui::PopID();
            continue;
        }
        // Option
        if( !item.group.empty() && groups_open.count( item.group ) && !groups_open[item.group] &&
            filter.empty() ) {
            continue;
        }
        auto it = cont.find( item.data );
        if( it == cont.end() ) {
            continue;
        }
        options_manager::cOpt &opt = it->second;
        if( opt.is_hidden() || !matches_filter( opt ) ) {
            continue;
        }
        std::string reason;
        bool enabled = true;
        if( opt.hasPrerequisite() && !opt.checkPrerequisite() ) {
            enabled = false;
            reason = string_format( _( "Requires: %s" ), mgr.get_option( opt.getPrerequisite() ).getMenuText() );
        }
        begin_table();
        draw_option_row( opt, enabled, reason );
        ++shown;
    }
    end_table();
    if( shown == 0 ) {
        w::empty_state( _( "No options match" ), filter.empty() ? std::string() : _( "Try another search." ) );
    }
}

void options_hybrid_view::draw_description( float width, float height )
{
    const float s = theme::scale();
    const ui_hybrid_chrome::theme::tokens &tk = theme::get();
    if( !w::panel_begin( "options_desc", ImVec2( width, height ), true, ImGuiWindowFlags_NoScrollbar ) ) {
        w::panel_end();
        return;
    }
    if( !described.empty() ) {
        options_manager::options_container &cont = container_for_page( page );
        auto it = cont.find( described );
        if( it != cont.end() ) {
            const options_manager::cOpt &opt = it->second;
            w::push_font_section();
            ImGui::PushTextWrapPos( ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x );
            ImGui::TextColored( ui_hybrid_chrome::palette::accent(), "%s", opt.getMenuText().c_str() );
            ImGui::PopTextWrapPos();
            w::pop_font();
            ImGui::Dummy( ImVec2( 0.f, tk.xs * s ) );
            ImGui::PushTextWrapPos( ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x );
            ImGui::TextUnformatted( opt.getTooltip().c_str() );
            ImGui::Dummy( ImVec2( 0.f, tk.sm * s ) );
            ImGui::TextColored( ui_hybrid_chrome::palette::text_muted(), "%s", opt.getDefaultText().c_str() );
            if( opt.hasPrerequisite() ) {
                ImGui::Dummy( ImVec2( 0.f, tk.xs * s ) );
                ImGui::TextColored( ui_hybrid_chrome::palette::text_muted(), "%s",
                                    string_format( _( "Requires: %s" ),
                                                   mgr.get_option( opt.getPrerequisite() ).getMenuText() ).c_str() );
            }
            ImGui::PopTextWrapPos();
        }
    } else if( !described_group.empty() ) {
        const options_manager::Group &g = mgr.find_group( described_group );
        w::push_font_section();
        ImGui::TextColored( ui_hybrid_chrome::palette::accent(), "%s", g.name_.translated().c_str() );
        w::pop_font();
        ImGui::PushTextWrapPos( ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x );
        ImGui::TextUnformatted( g.tooltip_.translated().c_str() );
        ImGui::PopTextWrapPos();
    } else {
        ImGui::TextColored( ui_hybrid_chrome::palette::text_muted(), "%s",
                            _( "Point at an option to read what it does." ) );
    }
    w::panel_end();
}

void options_hybrid_view::draw( float footer_logical )
{
    const float s = theme::scale();
    const ui_hybrid_chrome::theme::tokens &tk = theme::get();
    draw_tabs();
    const float height = std::max( 80.f * s, ImGui::GetContentRegionAvail().y - footer_logical * s );
    const float total_w = ImGui::GetContentRegionAvail().x;
    const float desc_w = std::clamp( total_w * 0.3f, 220.f * s, 520.f * s );
    const float gap = tk.lg * s;
    // The page scrolls (long pages need it); the description panel does not.
    ImGui::BeginChild( "options_left", ImVec2( total_w - desc_w - gap, height ), ImGuiChildFlags_None,
                       ImGuiWindowFlags_NoBackground );
    draw_page( height );
    ImGui::EndChild();
    ImGui::SameLine( 0.f, gap );
    draw_description( desc_w, height );
}

#endif // TILES
