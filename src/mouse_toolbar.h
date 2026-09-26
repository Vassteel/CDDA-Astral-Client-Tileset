#pragma once
#ifndef CATA_SRC_MOUSE_TOOLBAR_H
#define CATA_SRC_MOUSE_TOOLBAR_H

#include <optional>

#include "action.h"

/**
 * Phase C v1: compact always-visible (toggleable) ImGui toolbar for mouse play.
 * Clickable buttons dispatch existing ACTION_* handlers. Keyboard bindings are
 * unchanged. Shown only during normal play (DEFAULTMODE) when the option is on.
 */
namespace mouse_toolbar
{

/** Ensure the toolbar UI adaptor exists when the option is enabled. */
void ensure_shown();

/** Tear down the toolbar (e.g. leaving the world). */
void hide();

/** True while get_player_input is in its DEFAULTMODE wait loop. */
void set_default_mode_wait( bool waiting );

/**
 * If a toolbar button was clicked, return that action_id and clear the pending
 * slot. Called from handle_action before waiting for keyboard/mouse input.
 */
std::optional<action_id> take_pending_action();

/** True if a toolbar button was clicked and is waiting to be dispatched. */
bool has_pending_action();

} // namespace mouse_toolbar

#endif // CATA_SRC_MOUSE_TOOLBAR_H
