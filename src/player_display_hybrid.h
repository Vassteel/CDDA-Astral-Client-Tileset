#pragma once
#ifndef CATA_SRC_PLAYER_DISPLAY_HYBRID_H
#define CATA_SRC_PLAYER_DISPLAY_HYBRID_H

class Character;

/**
 * Soft-fork D Hybrid character sheet (ImGui charcoal/amber).
 * Preserves all vanilla info panels: stats, encumbrance, speed, skills,
 * traits, bionics, effects, proficiencies.
 */
void player_display_hybrid( Character &you, bool customize_character );

#endif // CATA_SRC_PLAYER_DISPLAY_HYBRID_H
