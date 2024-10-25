/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Prototypes for functions for game sequencing.
 *
 */


#ifndef _GAMESEQ_H
#define _GAMESEQ_H

#include "player.h"
#include "mission.h"
#include "object.h"
#include "wall.h"

#define SUPER_MISSILE       0
#define SUPER_SEEKER        1
#define SUPER_SMARTBOMB     2
#define SUPER_SHOCKWAVE     3

#ifdef SHAREWARE
#define Last_level			7			//the number of the very last level for shareware
#define Last_secret_level	0 			// No secret levels!
// #else
// extern int Last_level,Last_secret_level;	//set by mission code
#endif

// extern int Secret_level_table[MAX_SECRET_LEVELS_PER_MISSION];

#define LEVEL_NAME_LEN 36       //make sure this is multiple of 4!

// Current_level_num starts at 1 for the first level
// -1,-2,-3 are secret levels
// 0 means not a real level loaded
extern int Current_level_num, Next_level_num;
extern char Current_level_name[LEVEL_NAME_LEN];
extern obj_position Player_init[MAX_PLAYERS];


// This is the highest level the player has ever reached
extern int Player_highest_level;

//
// New game sequencing functions
//

// starts a new game on the given level
void StartNewGame(int start_level);

// starts the next level
void StartNewLevel(int level_num);

// Actually does the work to start new level
void StartNewLevelSub(int level_num, int page_in_textures, int secret_flag);

void InitPlayerObject();            //make sure player's object set up
void init_player_stats_game(ubyte pnum);      //clear all stats

// called when the player has finished a level
// if secret flag is true, advance to secret level, else next normal level
void PlayerFinishedLevel(int secret_flag);

// called when the player has died
void DoPlayerDead(void);

// load a level off disk. level numbers start at 1.
// Secret levels are -1,-2,-3
void LoadLevel(int level_num,int page_in_textures);

extern void gameseq_remove_unused_players();

extern void update_player_stats();

// from scores.c

extern void show_high_scores(int place);
extern void draw_high_scores(int place);
extern int add_player_to_high_scores(player *pp);
extern void input_name (int place);
extern int reset_high_scores();
extern void init_player_stats_level(int secret_flag);

void open_message_window(void);
void close_message_window(void);

// create flash for player appearance
extern void create_player_appearance_effect(object *player_obj);

// goto whatever secrect level is appropriate given the current level
//extern int goto_secret_level();

// reset stuff so game is semi-normal when playing from editor
void editor_reset_stuff_on_level();

// Show endlevel bonus scores
extern void DoEndLevelScoreGlitz(int network);
extern void DoBestRanksScoreGlitz(int level_num);

// Calculate scores and ranks based on saved performance data
extern int calculateRank(int level_num);
extern int truncateRanks(int rank);

extern void getLevelNameFromRankFile(int level_num, char* buffer);
 
// stuff for multiplayer
extern int NumNetPlayerPositions;

void bash_to_shield(int, char *);

typedef struct partimealgo {
	double movementTime;
	int toDoListIDs[MAX_OBJECTS + MAX_TRIGGERS];
	int toDoListTypes[MAX_OBJECTS + MAX_TRIGGERS];
	int toDoListSize;
	int doneListIDs[MAX_OBJECTS + MAX_TRIGGERS];
	int doneListTypes[MAX_OBJECTS + MAX_TRIGGERS];
	int doneListSize;
	int blackListIDs[MAX_OBJECTS + MAX_TRIGGERS];
	int blackListTypes[MAX_OBJECTS + MAX_TRIGGERS];
	int blackListSize;
	int doneWalls[MAX_WALLS];
	int doneWallsSize;
	// Remember the IDs of all walls in the level that are locked in some way, so we can see if any of our paths encounter them.
	int lockedWallIDs[MAX_WALLS];
	int lockedWallUnlockedBy[MAX_WALLS];
	int reactorWallIDs[MAX_WALLS];
	int reactorWallUnlockedBy[MAX_WALLS];
	int numLockedWalls;
	int numReactorWalls;
	int pathfinds; // Keeps track of pathfinding attempts in parTime calculator, so it can automatically stop after an unnecessary amount to avoid softlocks.
	int pathCompletable; // If the most recently created path can be done in its entirety. Will be 0 if it runs into something (typically a grate).
	vms_vector lastPosition; // Tracks the last place algo went to within the same segment.
	int matcenLives[MAX_ROBOT_CENTERS]; // We need to track how many times we trip matcens, since each one can only be tripped three times.
	// Time spent clearing matcens.
	double matcenTime;
	// Track the locations of energy centers for when we need to make a pit stop...
	int energyCenters[MAX_NUM_FUELCENS];
	int numEnergyCenters;
	// Variable to tell it when to refill its energy.
	fix simulatedEnergy;
	fix vulcanAmmo; // What it sounds like.
	// How much robot HP we've had to destroy to this point.
	double combatTime;
	fix energy_gained_per_pickup;
	// Info about the weapon algo currently has equipped.
	double dps;
	fix energy_used_per_shield;
	fix ammo_used_per_shield; // For when using vulcan.
	int heldWeapons[5]; // Which weapons algo has.
	int num_weapons; // The number of weapons algo has.
	int laser_level; // Use 1-4 for dual lasers, 5-8 for quad 1-4.
	int loops;
} __pack__ partimealgo;

extern partimealgo parTimeAlgo; // For all the par time algorithm stuff done in gameseq.c.

#endif /* _GAMESEQ_H */