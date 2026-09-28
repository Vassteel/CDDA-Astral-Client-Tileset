#pragma once
#ifndef CATA_SRC_PROGRESSION_UI_H
#define CATA_SRC_PROGRESSION_UI_H

#include <string>

namespace progression_ui
{
// Handles the two progression entry topics; other dialogue topics return false.
bool show( const std::string &topic );
}

#endif
