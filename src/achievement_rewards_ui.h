#pragma once
#ifndef CATA_SRC_ACHIEVEMENT_REWARDS_UI_H
#define CATA_SRC_ACHIEVEMENT_REWARDS_UI_H
class achievement;
namespace achievement_rewards_ui
{
void draw();
void process_actions();
void popup( const achievement & );
void release_gpu_resources();
}
#endif
