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
 * Routines for EndGame, EndLevel, etc.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <time.h>

#include "inferno.h"
#include "game.h"
#include "key.h"
#include "object.h"
#include "physics.h"
#include "dxxerror.h"
#include "joy.h"
#include "iff.h"
#include "pcx.h"
#include "timer.h"
#include "render.h"
#include "laser.h"
#include "screens.h"
#include "textures.h"
#include "slew.h"
#include "gauges.h"
#include "texmap.h"
#include "3d.h"
#include "effects.h"
#include "menu.h"
#include "gameseg.h"
#include "wall.h"
#include "ai.h"
#include "hostage.h"
#include "fuelcen.h"
#include "switch.h"
#include "digi.h"
#include "gamesave.h"
#include "scores.h"
#include "u_mem.h"
#include "palette.h"
#include "morph.h"
#include "lighting.h"
#include "newdemo.h"
#include "titles.h"
#include "collide.h"
#include "weapon.h"
#include "sounds.h"
#include "args.h"
#include "gameseq.h"
#include "gamefont.h"
#include "newmenu.h"
#include "endlevel.h"
#include "playsave.h"
#include "ctype.h"
#include "multi.h"
#include "fireball.h"
#include "kconfig.h"
#include "config.h"
#include "robot.h"
#include "automap.h"
#include "cntrlcen.h"
#include "powerup.h"
#include "text.h"
#include "piggy.h"
#include "texmerge.h"
#include "paging.h"
#include "mission.h"
#include "state.h"
#include "songs.h"
#ifdef NETWORK
#include "multi.h"
#endif
#include "strutil.h"
#ifdef EDITOR
#include "editor/editor.h"
#endif
#ifdef OGL
#include "ogl_init.h"
#endif
#include "custom.h"
#include "byteswap.h"
#include "segment.h"
#include "gameseg.h"
#include "multibot.h"
#include "player.h"
#include "math.h"
#include "solarmap.h"

void init_player_stats_new_ship(ubyte pnum);
void copy_defaults_to_robot_all(void);
int AdvanceLevel(int secret_flag);
void StartLevel(int random);

//Current_level_num starts at 1 for the first level
//-1,-2,-3 are secret levels
//0 means not a real level loaded
int	Current_level_num=0,Next_level_num;
char	Current_level_name[LEVEL_NAME_LEN];

// #ifndef SHAREWARE
// int Last_level,Last_secret_level;
// #endif

// Global variables describing the player
int	N_players=1;	// Number of players ( >1 means a net game, eh?)
int 	Player_num=0;	// The player number who is on the console.
player			Players[MAX_PLAYERS];			// Misc player info
ranking Ranking; // Ranking system mod variables.
restartLevel RestartLevel;
obj_position	Player_init[MAX_PLAYERS];

// Global variables telling what sort of game we have
int NumNetPlayerPositions = -1;

extern fix ThisLevelTime;

// Extern from game.c to fix a bug in the cockpit!

extern int last_drawn_cockpit;
extern int Last_level_path_created;

void HUD_clear_messages(); // From hud.c


void verify_console_object()
{
	Assert( Player_num > -1 );
	Assert( Players[Player_num].objnum > -1 );
	ConsoleObject = &Objects[Players[Player_num].objnum];
	Assert( ConsoleObject->id==Player_num );
}

int count_number_of_robots()
{
	int robot_count;
	int i;

	robot_count = 0;
	for (i=0;i<=Highest_object_index;i++) {
		if (Objects[i].type == OBJ_ROBOT)
			robot_count++;
	}

	return robot_count;
}


int count_number_of_hostages()
{
	int count;
	int i;

	count = 0;
	for (i=0;i<=Highest_object_index;i++) {
		if (Objects[i].type == OBJ_HOSTAGE)
			count++;
	}

	return count;
}


void
gameseq_init_network_players()
{
	int i,k,j;

	// Initialize network player start locations and object numbers

	ConsoleObject = &Objects[0];
	k = 0;
	j = 0;
	for (i=0;i<=Highest_object_index;i++) {

		if (( Objects[i].type==OBJ_PLAYER )	|| (Objects[i].type == OBJ_GHOST) || (Objects[i].type == OBJ_COOP))
		{
#ifndef SHAREWARE
			if ( (!(Game_mode & GM_MULTI_COOP) && ((Objects[i].type == OBJ_PLAYER)||(Objects[i].type==OBJ_GHOST))) ||
	           ((Game_mode & GM_MULTI_COOP) && ((j == 0) || ( Objects[i].type==OBJ_COOP ) )) )
			{

				Objects[i].type=OBJ_PLAYER;
#endif
				Player_init[k].pos = Objects[i].pos;
				Player_init[k].orient = Objects[i].orient;
				Player_init[k].segnum = Objects[i].segnum;
				Players[k].objnum = i;
				Objects[i].id = k;
				k++;
#ifndef SHAREWARE
			}
			else
				obj_delete(i);
			j++;
#endif
		}
	}

	NumNetPlayerPositions = k;

	if (Game_mode & GM_MULTI) {
		// Ensure we have 8 starting locations, even if there aren't 8 in the file.  This makes observer mode work in all levels.
		for (; k < MAX_PLAYERS; k++) {
			i = obj_allocate();

			Objects[i].type = OBJ_PLAYER;

			Player_init[k].pos = Objects[k % NumNetPlayerPositions].pos;
			Player_init[k].orient = Objects[k % NumNetPlayerPositions].orient;
			Player_init[k].segnum = Objects[k % NumNetPlayerPositions].segnum;
			Players[k].objnum = i;
			Objects[i].id = k;

			Objects[i].attached_obj = -1;
		}

		NumNetPlayerPositions = MAX_PLAYERS;
	}
}

void gameseq_remove_unused_players()
{
	int i;

	// 'Remove' the unused players

#ifdef NETWORK
	if (Game_mode & GM_MULTI)
	{
		for (i=0; i < NumNetPlayerPositions; i++)
		{
			if ((i == 0 && Netgame.host_is_obs) || (!Players[i].connected) || (i >= N_players))
			{
				multi_make_player_ghost(i);
			}
		}
	}
	else
#endif
	{		// Note link to above if!!!
		for (i=1; i < NumNetPlayerPositions; i++)
		{
			obj_delete(Players[i].objnum);
		}
	}
}

fix StartingShields=INITIAL_SHIELDS;

// Setup player for new game
void init_player_stats_game(ubyte pnum)
{
	Players[pnum].score = 0;
	Players[pnum].last_score = 0;
	Players[pnum].lives = INITIAL_LIVES;
	Players[pnum].level = 1;
	Players[pnum].time_level = 0;
	Players[pnum].time_total = 0;
	Players[pnum].hours_level = 0;
	Players[pnum].hours_total = 0;
	Players[pnum].killer_objnum = -1;
	Players[pnum].net_killed_total = 0;
	Players[pnum].net_kills_total = 0;
	Players[pnum].num_kills_level = 0;
	Players[pnum].num_kills_total = 0;
	Players[pnum].num_robots_level = 0;
	Players[pnum].num_robots_total = 0;
	Players[pnum].KillGoalCount = 0;
	Players[pnum].hostages_rescued_total = 0;
	Players[pnum].hostages_level = 0;
	Players[pnum].hostages_total = 0;
	Players[pnum].laser_level = 0;
	Players[pnum].flags = 0;
	Players[pnum].shields_delta = 0;
	Players[pnum].shields_time = 0;

	init_player_stats_new_ship(pnum);

	if (pnum == Player_num && Game_mode & GM_MULTI)
		multi_send_ship_status();
}

void init_ammo_and_energy(void)
{
	if (Players[Player_num].energy < INITIAL_ENERGY)
		Players[Player_num].energy = INITIAL_ENERGY;
	if (Players[Player_num].shields < StartingShields)
		Players[Player_num].shields = StartingShields;

//	for (i=0; i<MAX_PRIMARY_WEAPONS; i++)
//		if (Players[Player_num].primary_ammo[i] < Default_primary_ammo_level[i])
//			Players[Player_num].primary_ammo[i] = Default_primary_ammo_level[i];

//	for (i=0; i<MAX_SECONDARY_WEAPONS; i++)
//		if (Players[Player_num].secondary_ammo[i] < Default_secondary_ammo_level[i])
//			Players[Player_num].secondary_ammo[i] = Default_secondary_ammo_level[i];
	if (Players[Player_num].secondary_ammo[0] < 2 + NDL - Difficulty_level)
		Players[Player_num].secondary_ammo[0] = 2 + NDL - Difficulty_level;
}

// Setup player for new level (After completion of previous level)
void init_player_stats_level(int secret_flag)
{
	(void)secret_flag;
	// int	i;

	Players[Player_num].last_score = Players[Player_num].score;

	Players[Player_num].level = Current_level_num;

#ifdef NETWORK
	if (!Network_rejoined)
#endif
		Players[Player_num].time_level = 0;	//Note link to above if !!!!!!

	init_ammo_and_energy();

	Players[Player_num].killer_objnum = -1;

	Players[Player_num].num_kills_level = 0;
	Players[Player_num].num_robots_level = count_number_of_robots();
	Players[Player_num].num_robots_total += Players[Player_num].num_robots_level;

	Players[Player_num].hostages_level = count_number_of_hostages();
	Players[Player_num].hostages_total += Players[Player_num].hostages_level;
	Players[Player_num].hostages_on_board = 0;

		Players[Player_num].flags &= (~KEY_BLUE);
		Players[Player_num].flags &= (~KEY_RED);
		Players[Player_num].flags &= (~KEY_GOLD);

	Players[Player_num].flags &= (~PLAYER_FLAGS_INVULNERABLE);
	Players[Player_num].flags &= (~PLAYER_FLAGS_CLOAKED);

		Players[Player_num].cloak_time = 0;
		Players[Player_num].invulnerable_time = 0;

		if ((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP))
			Players[Player_num].flags |= (KEY_BLUE | KEY_RED | KEY_GOLD);

	Player_is_dead = 0; // Added by RH
	Dead_player_camera = NULL;
	Players[Player_num].homing_object_dist = -F1_0; // Added by RH

	// properly init these cursed globals
	Next_flare_fire_time = Last_laser_fired_time = Next_laser_fire_time = Next_missile_fire_time = GameTime64;

	init_gauges();

	if (Game_mode & GM_MULTI)
		multi_send_ship_status();
}

// Setup player for a brand-new ship
void init_player_stats_new_ship(ubyte pnum)
{
	int i;

	if (pnum == Player_num)
	{
		if (Newdemo_state == ND_STATE_RECORDING)
		{
			newdemo_record_laser_level(Players[Player_num].laser_level, 0);
			newdemo_record_player_weapon(0, 0);
			newdemo_record_player_weapon(1, 0);
		}
		Global_laser_firing_count=0;
		Players[Player_num].primary_weapon = 0;
		Players[Player_num].secondary_weapon = 0;
		dead_player_end(); //player no longer dead
		Player_is_dead = 0;
		Player_exploded = 0;
		Player_eggs_dropped = 0;

		int delete_camera = 1; 
#ifdef NETWORK	
		if ((Game_mode & GM_MULTI) && (Netgame.SpawnStyle == SPAWN_STYLE_PREVIEW)) {	
			delete_camera = 0; 
		}	
#endif
		if(delete_camera)
			Dead_player_camera = 0;
	}

	Players[pnum].energy = INITIAL_ENERGY;
	Players[pnum].shields = StartingShields;
    Players[pnum].shields_certain = 1;
	Players[pnum].laser_level = 0;
	Players[pnum].killer_objnum = -1;
	Players[pnum].hostages_on_board = 0;
	for (i=0; i<MAX_PRIMARY_WEAPONS; i++)
		Players[pnum].primary_ammo[i] = 0;
	for (i=1; i<MAX_SECONDARY_WEAPONS; i++)
		Players[pnum].secondary_ammo[i] = 0;
	Players[pnum].secondary_ammo[0] = 2 + NDL - Difficulty_level;
	Players[pnum].primary_weapon_flags = HAS_LASER_FLAG;
	Players[pnum].secondary_weapon_flags = HAS_CONCUSSION_FLAG;
	Players[pnum].flags &= ~(PLAYER_FLAGS_QUAD_LASERS | PLAYER_FLAGS_CLOAKED | PLAYER_FLAGS_INVULNERABLE);
	Players[pnum].cloak_time = 0;
	Players[pnum].invulnerable_time = 0;
	Players[pnum].homing_object_dist = -F1_0;

	RespawningConcussions[pnum] = 0; 
	VulcanAmmoBoxesOnBoard[pnum] = 0;
	VulcanBoxAmmo[pnum] = 0;

	digi_kill_sound_linked_to_object(Players[pnum].objnum);
	
	if (pnum == Player_num && Game_mode & GM_MULTI)
		multi_send_ship_status();
}

#ifdef EDITOR

extern int game_handler(window *wind, d_event *event, void *data);

//reset stuff so game is semi-normal when playing from editor
void editor_reset_stuff_on_level()
{
	gameseq_init_network_players();
	init_player_stats_level(0);
	Viewer = ConsoleObject;
	ConsoleObject = Viewer = &Objects[Players[Player_num].objnum];
	ConsoleObject->id=Player_num;
	ConsoleObject->control_type = CT_FLYING;
	ConsoleObject->movement_type = MT_PHYSICS;
	Game_suspended = 0;
	verify_console_object();
	Control_center_destroyed = 0;
	if (Newdemo_state != ND_STATE_PLAYBACK)
		gameseq_remove_unused_players();
	init_cockpit();
	init_robots_for_level();
	init_ai_objects();
	init_morphs();
	init_all_matcens();
	init_player_stats_new_ship(Player_num);
	if (!Game_wind)
		Game_wind = window_create(&grd_curscreen->sc_canvas, 0, 0, SWIDTH, SHEIGHT, game_handler, NULL);
}
#endif

void reset_player_object();


//do whatever needs to be done when a player dies in multiplayer

void DoGameOver()
{
#ifndef SHAREWARE
	if (PLAYING_BUILTIN_MISSION)
#endif
		if (!cheats.enabled)
			scores_maybe_add_player(0);

	if (Game_wind)
		window_close(Game_wind);		// Exit out of game loop
}

//update various information about the player
void update_player_stats()
{
	Players[Player_num].time_level += FrameTime;	//the never-ending march of time...
	if ( Players[Player_num].time_level > i2f(3600) )	{
		Players[Player_num].time_level -= i2f(3600);
		Players[Player_num].hours_level++;
	}

	Players[Player_num].time_total += FrameTime;	//the never-ending march of time...
	if ( Players[Player_num].time_total > i2f(3600) )	{
		Players[Player_num].time_total -= i2f(3600);
		Players[Player_num].hours_total++;
	}
}

//go through this level and start any eclip sounds
void set_sound_sources()
{
	int segnum,sidenum;
	segment *seg;

	digi_init_sounds();		//clear old sounds

	for (seg=&Segments[0],segnum=0;segnum<=Highest_segment_index;seg++,segnum++)
		for (sidenum=0;sidenum<MAX_SIDES_PER_SEGMENT;sidenum++) {
			int tm,ec,sn;

			if ((tm=seg->sides[sidenum].tmap_num2) != 0)
				if ((ec=TmapInfo[tm&0x3fff].eclip_num)!=-1)
					if ((sn=Effects[ec].sound_num)!=-1) {
						vms_vector pnt;

						compute_center_point_on_side(&pnt,seg,sidenum);
						digi_link_sound_to_pos(sn,segnum,sidenum,&pnt,1, F1_0/2);

					}
		}

}


//fix flash_dist=i2f(1);
fix flash_dist=fl2f(.9);

//create flash for player appearance
void create_player_appearance_effect(object *player_obj)
{
	vms_vector pos;
	object *effect_obj;

#ifndef NDEBUG
	{
		int objnum = player_obj-Objects;
		if ( (objnum < 0) || (objnum > Highest_object_index) )
			Int3(); // See Rob, trying to track down weird network bug
	}
#endif

	if (player_obj == Viewer)
		vm_vec_scale_add(&pos, &player_obj->pos, &player_obj->orient.fvec, fixmul(player_obj->size,flash_dist));
	else
		pos = player_obj->pos;

	effect_obj = object_create_explosion(player_obj->segnum, &pos, player_obj->size, VCLIP_PLAYER_APPEARANCE );

	if (effect_obj) {
		effect_obj->orient = player_obj->orient;

		if ( Vclip[VCLIP_PLAYER_APPEARANCE].sound_num > -1 )
			digi_link_sound_to_object( Vclip[VCLIP_PLAYER_APPEARANCE].sound_num, effect_obj-Objects, 0, F1_0);
	}
}

//
// New Game sequencing functions
//

extern int descent_critical_error;

//get level filename. level numbers start at 1.  Secret levels are -1,-2,-3
char *get_level_file(int level_num)
{
#ifdef SHAREWARE
        {
                static char t[13];
                sprintf(t, "level%02d.sdl", level_num);
		return t;
        }
#else
        if (level_num<0)                //secret level
		return Secret_level_names[-level_num-1];
        else                                    //normal level
		return Level_names[level_num-1];
#endif
}

// routine to calculate the checksum of the segments.
void do_checksum_calc(ubyte *b, int len, unsigned int *s1, unsigned int *s2)
{

	while(len--) {
		*s1 += *b++;
		if (*s1 >= 255) *s1 -= 255;
		*s2 += *s1;
	}
}

ushort netmisc_calc_checksum()
{
	int i, j, k;
	unsigned int sum1,sum2;
	short s;
	int t;

	sum1 = sum2 = 0;
	for (i = 0; i < Highest_segment_index + 1; i++) {
		for (j = 0; j < MAX_SIDES_PER_SEGMENT; j++) {
			do_checksum_calc((unsigned char *)&(Segments[i].sides[j].type), 1, &sum1, &sum2);
			do_checksum_calc(&(Segments[i].sides[j].pad), 1, &sum1, &sum2);
			s = INTEL_SHORT(Segments[i].sides[j].wall_num);
			do_checksum_calc((ubyte *)&s, 2, &sum1, &sum2);
			s = INTEL_SHORT(Segments[i].sides[j].tmap_num);
			do_checksum_calc((ubyte *)&s, 2, &sum1, &sum2);
			s = INTEL_SHORT(Segments[i].sides[j].tmap_num2);
			do_checksum_calc((ubyte *)&s, 2, &sum1, &sum2);
			for (k = 0; k < 4; k++) {
				t = INTEL_INT(((int)Segments[i].sides[j].uvls[k].u));
				do_checksum_calc((ubyte *)&t, 4, &sum1, &sum2);
				t = INTEL_INT(((int)Segments[i].sides[j].uvls[k].v));
				do_checksum_calc((ubyte *)&t, 4, &sum1, &sum2);
				t = INTEL_INT(((int)Segments[i].sides[j].uvls[k].l));
				do_checksum_calc((ubyte *)&t, 4, &sum1, &sum2);
			}
			for (k = 0; k < 2; k++) {
				t = INTEL_INT(((int)Segments[i].sides[j].normals[k].x));
				do_checksum_calc((ubyte *)&t, 4, &sum1, &sum2);
				t = INTEL_INT(((int)Segments[i].sides[j].normals[k].y));
				do_checksum_calc((ubyte *)&t, 4, &sum1, &sum2);
				t = INTEL_INT(((int)Segments[i].sides[j].normals[k].z));
				do_checksum_calc((ubyte *)&t, 4, &sum1, &sum2);
			}
		}
		for (j = 0; j < MAX_SIDES_PER_SEGMENT; j++) {
			s = INTEL_SHORT(Segments[i].children[j]);
			do_checksum_calc((ubyte *)&s, 2, &sum1, &sum2);
		}
		for (j = 0; j < MAX_VERTICES_PER_SEGMENT; j++) {
			s = INTEL_SHORT(Segments[i].verts[j]);
			do_checksum_calc((ubyte *)&s, 2, &sum1, &sum2);
		}
		s = INTEL_SHORT(Segments[i].objects);
		do_checksum_calc((ubyte *)&s, 2, &sum1, &sum2);
		do_checksum_calc((unsigned char *)&(Segments[i].special), 1, &sum1, &sum2);
		do_checksum_calc((unsigned char *)&(Segments[i].matcen_num), 1, &sum1, &sum2);
		s = INTEL_SHORT(Segments[i].value);
		do_checksum_calc((ubyte *)&s, 2, &sum1, &sum2);
		t = INTEL_INT(((int)Segments[i].static_light));
		do_checksum_calc((ubyte *)&t, 4, &sum1, &sum2);
		s = INTEL_SHORT(0); // no matter if we need alignment on our platform, if we have editor we MUST consider this integer to get the same checksum as non-editor games calculate
		do_checksum_calc((ubyte *)&s, 2, &sum1, &sum2);
	}
	sum2 %= 255;
	return ((sum1<<8)+ sum2);
}

//load a level off disk. level numbers start at 1.  Secret levels are -1,-2,-3
void LoadLevel(int level_num,int page_in_textures)
{
	char *level_name;
	player save_player;

	save_player = Players[Player_num];

	Assert(level_num <= Last_level  && level_num >= Last_secret_level  && level_num != 0);

	level_name = get_level_file(level_num);

	if (!load_level(level_name))
		Current_level_num=level_num;

	gr_use_palette_table( "palette.256" );

	show_boxed_message(TXT_LOADING, 0);
#ifdef RELEASE
	timer_delay(F1_0);
#endif

#ifdef NETWORK
	my_segments_checksum = netmisc_calc_checksum();
#endif

	load_endlevel_data(level_num);
	
	load_custom_data(level_name);

#ifdef NETWORK
	reset_network_objects();
#endif

	Players[Player_num] = save_player;

	set_sound_sources();

	songs_play_level_song( Current_level_num, 0 );

	gr_palette_load(gr_palette);		//actually load the palette

	if ( page_in_textures ) {
		piggy_load_level_data();
#ifdef OGL
		ogl_cache_level_textures();
#endif
	}
}

//sets up Player_num & ConsoleObject
void InitPlayerObject()
{
	Assert(Player_num>=0 && Player_num<MAX_PLAYERS);

	if (Player_num != 0 )	{
		Players[0] = Players[Player_num];
		Player_num = 0;
	}

	Players[Player_num].objnum = 0;

	ConsoleObject = &Objects[Players[Player_num].objnum];

	ConsoleObject->type				= OBJ_PLAYER;
	ConsoleObject->id					= Player_num;
	ConsoleObject->control_type	= CT_FLYING;
	ConsoleObject->movement_type	= MT_PHYSICS;
}

int truncateRanks(int rank)
{
	int newRank = rank;
	if (rank == 2 || rank == 4)
		newRank = 3;
	if (rank == 5 || rank == 7)
		newRank = 6;
	if (rank == 8 || rank == 10)
		newRank = 9;
	if (rank == 11 || rank == 13)
		newRank = 12;
	return newRank;
}

int calculateRank(int level_num, int update_warm_start_status)
{
	int levelHostages = 0;
	int levelPoints = 0;
	double parTime = 0;
	int playerPoints = 0;
	double secondsTaken = 0;
	int playerHostages = 0;
	double hostagePoints = 0;
	double difficulty = 0;
	int deathCount = 0;
	double missedRngSpawn = 0;
	double rankPoints2 = 0;
	char buffer[256];
	char filename[256];
	sprintf(filename, "ranks/%s/%s/level%d.hi", Players[Player_num].callsign, Current_mission->filename, level_num); // Find file for the requested level.
	if (level_num > Current_mission->last_level)
		sprintf(filename, "ranks/%s/%s/levelS%d.hi", Players[Player_num].callsign, Current_mission->filename, level_num - Current_mission->last_level);
	PHYSFS_file* fp = PHYSFS_openRead(filename);
	if (fp == NULL)
		rankPoints2 = -10; // If no data exists, just assume level never played and set rankPoints in the range that returns N/A.
	else {
		PHYSFSX_getsTerminated(fp, buffer); // Fetch level data starting here. If a parameter isn't present in the file, it'll default to 0 in calculation.
		levelHostages = atoi(buffer);
		if (levelHostages < 0) // If level is unplayed, we know because the first (and only) value in the file will be -1, which is impossible for the hostage count.
			rankPoints2 = -10;
		else {
			PHYSFSX_getsTerminated(fp, buffer);
			levelPoints = atoi(buffer);
			PHYSFSX_getsTerminated(fp, buffer);
			parTime = atof(buffer);
			PHYSFSX_getsTerminated(fp, buffer); // Fetch player data starting here.
			playerPoints = atoi(buffer);
			PHYSFSX_getsTerminated(fp, buffer);
			secondsTaken = atof(buffer);
			PHYSFSX_getsTerminated(fp, buffer);
			playerHostages = atoi(buffer);
			PHYSFSX_getsTerminated(fp, buffer);
			difficulty = atof(buffer);
			PHYSFSX_getsTerminated(fp, buffer);
			deathCount = atoi(buffer);
			PHYSFSX_getsTerminated(fp, buffer);
			missedRngSpawn = atof(buffer);
			PHYSFSX_getsTerminated(fp, buffer);
			PHYSFSX_getsTerminated(fp, buffer);
			PHYSFSX_getsTerminated(fp, buffer);
			if (update_warm_start_status)
				Ranking.warmStart = atoi(buffer);
		}
	}
	PHYSFS_close(fp);
	double maxScore = levelPoints * 3;
	maxScore = (int)maxScore;
	double skillPoints = (int)(playerPoints * (difficulty / 4));
	double timePoints = (maxScore / 1.5) / pow(2, secondsTaken / parTime);
	if (secondsTaken < parTime)
		timePoints = (maxScore / 2.4) * (1 - (secondsTaken / parTime) * 0.2);
	if (!parTime)
		timePoints = 0;
	timePoints = (int)timePoints;
	hostagePoints = playerHostages * 2500 * ((difficulty + 8) / 12);
	if (playerHostages == levelHostages)
		hostagePoints *= 3;
	hostagePoints = round(hostagePoints); // Round this because I got 24999 hostage bonus once.
	double score = playerPoints + skillPoints + timePoints + missedRngSpawn + hostagePoints;
	maxScore += levelHostages * 7500;
	double deathPoints;
	if (deathCount == -1)
		deathPoints = ceil(maxScore / 12); // Round up instead of down for no damage bonus so score can't fall a point short and miss a rank.
	else
		deathPoints = -(maxScore * 0.4 - maxScore * (0.4 / pow(2, deathCount)));
	deathPoints = (int)deathPoints;
	score += deathPoints;
	if (rankPoints2 > -5)
		if (maxScore)
			rankPoints2 = (score / maxScore) * 12;
		else
			rankPoints2 = 13;
	Ranking.calculatedScore = score;
	if (rankPoints2 < -5)
		Ranking.rank = 0;
	if (rankPoints2 > -5)
		Ranking.rank = 1;
	if (rankPoints2 >= 0)
		Ranking.rank = (int)rankPoints2 + 2;
	if (!PlayerCfg.RankShowPlusMinus)
		Ranking.rank = truncateRanks(Ranking.rank);
	return Ranking.rank;
}

void getLevelNameFromRankFile(int level_num, char* buffer)
{
	char filename[256];
	sprintf(filename, "ranks/%s/%s/level%d.hi", Players[Player_num].callsign, Current_mission->filename, level_num); // Find file for the requested level.
	if (level_num > Current_mission->last_level)
		sprintf(filename, "ranks/%s/%s/levelS%d.hi", Players[Player_num].callsign, Current_mission->filename, level_num - Current_mission->last_level);
	PHYSFS_file* fp = PHYSFS_openRead(filename);
	if (fp == NULL)
		sprintf(buffer, "???");
	else {
		for (int i = 0; i < 10; i++)
			PHYSFSX_getsTerminated(fp, buffer); // Get a line ten times because the tenth line has the level name.
	}
	PHYSFS_close(fp);
}

//starts a new game on the given level
void StartNewGame(int start_level)
{	
	RestartLevel.restarts = 0;
	Ranking.warmStart = 0;
	PHYSFS_file* fp;
	char filename[256];
	int i = 1;
	char buffer[256];
	sprintf(buffer, "ranks/%s/%s", Players[Player_num].callsign, Current_mission->filename);
	PHYSFS_mkdir(buffer);
	while (i <= Current_mission->last_level - Current_mission->last_secret_level) {
		sprintf(filename, "ranks/%s/%s/level%d.hi", Players[Player_num].callsign, Current_mission->filename, i);
		if (i > Current_mission->last_level)
			sprintf(filename, "ranks/%s/%s/levelS%d.hi", Players[Player_num].callsign, Current_mission->filename, i - Current_mission->last_level);
		fp = PHYSFS_openRead(filename);
		if (fp == NULL) { // If this level's rank data file doesn't exist, create it now so it can be written to on the rank screen.
			fp = PHYSFS_openWrite(filename);
			PHYSFSX_printf(fp, "-1\n");
			PHYSFSX_printf(fp, "\n");
			PHYSFSX_printf(fp, "\n");
			PHYSFSX_printf(fp, "\n");
			PHYSFSX_printf(fp, "\n");
			PHYSFSX_printf(fp, "\n");
			PHYSFSX_printf(fp, "\n");
			PHYSFSX_printf(fp, "\n");
			PHYSFSX_printf(fp, "\n");
			PHYSFSX_printf(fp, "\n");
			PHYSFSX_printf(fp, "???");
		}
		PHYSFS_close(fp);
		i++;
	}
	
	state_quick_item = -1;	// for first blind save, pick slot to save in

	Game_mode = GM_NORMAL;

	Next_level_num = 0;

	InitPlayerObject();					//make sure player's object set up

	init_player_stats_game(Player_num);		//clear all stats

	N_players = 1;

	RestartLevel.updateRestartStuff = 1; // So the player doesn't restart with 0 in every field.
	StartNewLevel(start_level);

	Players[Player_num].starting_level = start_level;		// Mark where they started

	game_disable_cheats();
}

// Arne's code, except for the image pos/size and fromBestRanksButton stuff.
int endlevel_current_rank;
extern grs_bitmap nm_background1;
grs_bitmap transparent;
int endlevel_handler(newmenu* menu, d_event* event, void* userdata) {
	if (!Ranking.quickload) {
		switch (event->type) {
		case EVENT_WINDOW_DRAW:
			gr_palette_load(gr_palette);
			show_fullscr(&nm_background1);
			grs_bitmap* bm = RankBitmaps[endlevel_current_rank];
			int x = grd_curscreen->sc_w * 0.4;
			int y = grd_curscreen->sc_h * 0.725;
			int h = grd_curscreen->sc_h * 0.1125;
			if (!endlevel_current_rank || endlevel_current_rank == 14)
				h *= 1.0806; // Make E-rank bigger to compensate for the tilt, and X too because it's just small lol.
			int w = 3 * h;
			if (!endlevel_current_rank || endlevel_current_rank == 2 || endlevel_current_rank == 5 || endlevel_current_rank == 8 || endlevel_current_rank == 11 || endlevel_current_rank == 13 || endlevel_current_rank == 14)
				x += w * 0.138888889; // Push the image right if it doesn't have a plus or minus, otherwise it won't be centered.
			ogl_ubitmapm_cs(x, y, w, h, bm, -1, F1_0);
			grs_bitmap oldbackground = nm_background1;
			nm_background1 = transparent;
			int ret = newmenu_draw(newmenu_get_window(menu), menu);
			transparent = nm_background1;
			nm_background1 = oldbackground;
			return ret;
		}
	}
	if (Ranking.fromBestRanksButton) {
		switch (event->type) {
		case EVENT_NEWMENU_SELECTED:
			if (!do_difficulty_menu())
				return 1;
			if (Ranking.startingLevel > Current_mission->last_level)
				StartNewGame(Current_mission->last_level - Ranking.startingLevel);
			else
				StartNewGame(Ranking.startingLevel);
			break;
		}
	}
	return 0;
}


void DoEndLevelScoreGlitz(int network)
{
	if (Ranking.level_time == 0)
		Ranking.level_time = Players[Player_num].hours_level * 3600 + (double)Players[Player_num].time_level / 65536; // Failsafe for if this isn't updated.
	RestartLevel.restartsCache = RestartLevel.restarts;
	RestartLevel.restarts = 0;
	RestartLevel.isResults = 1;

	int level_points, skill_points, death_points, shield_points, energy_points, time_points, hostage_points, all_hostage_points, endgame_points;
	double skill_points2, missed_rng_drops, hostage_points2;
	char	all_hostage_text[64];
	char	endgame_text[64];
#define N_GLITZITEMS 12
	char				m_str[N_GLITZITEMS][31];
	newmenu_item	m[N_GLITZITEMS + 1];
	int				i, c;
	char				title[128];
	int				is_last_level;
	int				mine_level;

	//	Compute level player is on, deal with secret levels (negative numbers)
	mine_level = Players[Player_num].level;
	if (mine_level < 0)
		mine_level *= -(Last_level / N_secret_levels);

	level_points = Players[Player_num].score - Players[Player_num].last_score;
	Ranking.rankScore = level_points - Ranking.excludePoints;

	skill_points = 0, skill_points2 = 0;
	if (Difficulty_level == 1) {
		skill_points2 = Ranking.rankScore / 4;
	}
	if (Difficulty_level > 1) {
		skill_points = level_points * (Difficulty_level - 1) / 2;
		skill_points2 = Ranking.rankScore * ((double)Difficulty_level / 4);
	}
	skill_points -= skill_points % 100;
	skill_points2 = (int)skill_points2;

	shield_points = f2i(Players[Player_num].shields) * 10 * (Difficulty_level + 1);
	energy_points = f2i(Players[Player_num].energy) * 5 * (Difficulty_level + 1);
	time_points = (Ranking.maxScore / 1.5) / pow(2, Ranking.level_time / Ranking.parTime);
	if (Ranking.level_time < Ranking.parTime)
		time_points = (Ranking.maxScore / 2.4) * (1 - (Ranking.level_time / Ranking.parTime) * 0.2);
	Ranking.maxScore += Players[Player_num].hostages_level * 7500;
	hostage_points = Players[Player_num].hostages_on_board * 500 * (Difficulty_level + 1);
	hostage_points2 = Players[Player_num].hostages_on_board * 2500 * (((double)Difficulty_level + 8) / 12);

	all_hostage_text[0] = 0;
	endgame_text[0] = 0;

	if (Players[Player_num].hostages_on_board == Players[Player_num].hostages_level) {
		all_hostage_points = Players[Player_num].hostages_on_board * 1000 * (Difficulty_level + 1);
		hostage_points2 *= 3;
	}
	else
		all_hostage_points = 0;
	hostage_points2 = round(hostage_points2); // Round this because I got 24999 hostage bonus once.

	if (!cheats.enabled && !(Game_mode & GM_MULTI) && (Players[Player_num].lives) && (Current_level_num == Last_level)) {		//player has finished the game!
		endgame_points = Players[Player_num].lives * 10000;
		is_last_level = 1;
	}
	else
		endgame_points = is_last_level = 0;
	if (!cheats.enabled)
		add_bonus_points_to_score(shield_points + energy_points + skill_points + hostage_points + all_hostage_points + endgame_points);
	death_points = -(Ranking.maxScore * 0.4 - Ranking.maxScore * (0.4 / pow(2, Ranking.deathCount)));
	if (Ranking.noDamage)
		death_points = ceil(Ranking.maxScore / 12); // Round up instead of down for no damage bonus so score can't fall a point short and miss a rank.
	Ranking.missedRngSpawn *= ((double)Difficulty_level + 4) / 4; // Add would-be skill bonus into the penalty for ignored random offspring. This makes ignoring them on high difficulties more consistent and punishing.
	missed_rng_drops = Ranking.missedRngSpawn;
	Ranking.rankScore += skill_points2 + time_points + hostage_points2 + death_points + missed_rng_drops;

	int minutes = Ranking.level_time / 60;
	double seconds = Ranking.level_time - minutes * 60;
	int parMinutes = Ranking.parTime / 60;
	double parSeconds = Ranking.parTime - parMinutes * 60;
	char* diffname = 0;
	char timeText[256];
	char parTimeString[256];
	c = 0;
	if (Difficulty_level == 0)
		diffname = "Trainee";
	if (Difficulty_level == 1)
		diffname = "Rookie";
	if (Difficulty_level == 2)
		diffname = "Hotshot";
	if (Difficulty_level == 3)
		diffname = "Ace";
	if (Difficulty_level == 4)
		diffname = "Insane";
	if (Ranking.quickload == 0) { // Only print the modded result screen if the player didn't quickload. Print the vanilla one otherwise, because quickloading into a level causes missing crucial mod info.
		if (seconds < 10 || seconds == 60)
			sprintf(timeText, "%i:0%.3f", minutes, seconds);
		else
			sprintf(timeText, "%i:%.3f", minutes, seconds);
		if (parSeconds < 10 || parSeconds == 60)
			sprintf(parTimeString, "%i:0%.0f", parMinutes, parSeconds);
		else
			sprintf(parTimeString, "%i:%.0f", parMinutes, parSeconds);
		sprintf(m_str[c++], "Level score\t%.0f", level_points - Ranking.excludePoints);
		sprintf(m_str[c++], "Time: %s/%s\t%i", timeText, parTimeString, time_points);
		sprintf(m_str[c++], "Hostages: %i/%i\t%.0f", Players[Player_num].hostages_on_board, Players[Player_num].hostages_level, hostage_points2);
		sprintf(m_str[c++], "Skill: %s\t%.0f", diffname, skill_points2);
		if (Ranking.noDamage)
			sprintf(m_str[c++], "No damage\t%i", death_points);
		else
			sprintf(m_str[c++], "Deaths: %.0f\t%i", Ranking.deathCount, death_points);
		if (Ranking.missedRngSpawn < 0)
			sprintf(m_str[c++], "Missed RNG spawn: \t%.0f\n", Ranking.missedRngSpawn);
		else
			strcpy(m_str[c++], "\n");
		if (Ranking.warmStart)
			sprintf(m_str[c++], "Total score\t* %0.0f", Ranking.rankScore);
		else
			sprintf(m_str[c++], "Total score\t%0.0f", Ranking.rankScore);

		double rankPoints = (Ranking.rankScore / Ranking.maxScore) * 12;
		if (Ranking.maxScore == 0)
			rankPoints = 13;
		int rank = 0;
		if (rankPoints >= 0)
			rank = (int)rankPoints + 1;
		if (!PlayerCfg.RankShowPlusMinus)
			rank = truncateRanks(rank + 1) - 1;
		endlevel_current_rank = rank;
		if (cheats.enabled) {
			strcpy(m_str[c++], "\n\n\n");
			sprintf(m_str[c++], "   Cheated, no save!   "); // Don't show vanilla score when cheating, as players already know it'll always be zero.
		}
		else {
			PHYSFS_File* fp;
			PHYSFS_File* temp;
			char filename[256];
			char temp_filename[256];
			sprintf(filename, "ranks/%s/%s/level%i.hi", Players[Player_num].callsign, Current_mission->filename, Current_level_num);
			if (Current_level_num < 0)
				sprintf(filename, "ranks/%s/%s/levelS%i.hi", Players[Player_num].callsign, Current_mission->filename, Current_level_num * -1);
			sprintf(temp_filename, "ranks/%s/%s/temp.hi", Players[Player_num].callsign, Current_mission->filename);
			fp = PHYSFS_openRead(filename);
			if (fp != NULL) {
				if (Current_level_num > 0) {
					calculateRank(Current_level_num, 0);
				}
				else {
					calculateRank(Current_mission->last_level - Current_level_num, 0);
				}
				if (Ranking.rankScore > Ranking.calculatedScore || Ranking.rank == 0) {
					time_t timeOfScore = time(NULL);
					temp = PHYSFS_openWrite(temp_filename);
					PHYSFSX_printf(temp, "%i\n", Players[Player_num].hostages_level);
					PHYSFSX_printf(temp, "%.0f\n", (Ranking.maxScore - Players[Player_num].hostages_level * 7500) / 3);
					PHYSFSX_printf(temp, "%.0f\n", Ranking.parTime);
					PHYSFSX_printf(temp, "%.0f\n", level_points - Ranking.excludePoints);
					PHYSFSX_printf(temp, "%.3f\n", Ranking.level_time);
					PHYSFSX_printf(temp, "%i\n", Players[Player_num].hostages_on_board);
					PHYSFSX_printf(temp, "%i\n", Difficulty_level);
					if (Ranking.noDamage)
						PHYSFSX_printf(temp, "%i\n", -1);
					else
						PHYSFSX_printf(temp, "%.0f\n", Ranking.deathCount);
					PHYSFSX_printf(temp, "%.0f\n", Ranking.missedRngSpawn);
					PHYSFSX_printf(temp, "%s\n", Current_level_name);
					PHYSFSX_printf(temp, "%s", ctime(&timeOfScore));
					PHYSFSX_printf(temp, "%i", Ranking.warmStart);
					PHYSFS_close(temp);
					PHYSFS_close(fp);
					PHYSFS_delete(filename);
					PHYSFSX_rename(temp_filename, filename);
					if (Ranking.rank > 0)
						sprintf(m_str[c++], "New record!");
				}
				PHYSFS_close(fp);
				if (Ranking.rankScore > Ranking.calculatedScore && Ranking.rank > 0)
					strcpy(m_str[c++], "\n\n");
				else
					strcpy(m_str[c++], "\n\n\n");
			}
			else {
				sprintf(m_str[c++], "Saving error. Replay level.\n\n\n");
				PHYSFS_close(fp);
			}
			sprintf(m_str[c++], "Vanilla score:\t      %i", Players[Player_num].score); // Show players' base game score at the end of each level, so they can still compete with it when using the mod.
		}
	}
	else {
		if (cheats.enabled) { // Zero out all the bonuses when cheating while the mod's off.
			shield_points = 0;
			energy_points = 0;
			hostage_points = 0;
			skill_points = 0;
			all_hostage_points = 0;
			endgame_points = 0;
		}
		sprintf(m_str[c++], "%s%i", TXT_SHIELD_BONUS, shield_points);
		sprintf(m_str[c++], "%s%i", TXT_ENERGY_BONUS, energy_points);
		sprintf(m_str[c++], "%s%i", TXT_HOSTAGE_BONUS, hostage_points);
		sprintf(m_str[c++], "%s%i\n", TXT_SKILL_BONUS, skill_points);
		if (!cheats.enabled && (Players[Player_num].hostages_on_board == Players[Player_num].hostages_level))
			sprintf(all_hostage_text, "%s%i\n", TXT_FULL_RESCUE_BONUS, all_hostage_points);
		if (!cheats.enabled && !(Game_mode & GM_MULTI) && (Players[Player_num].lives) && (Current_level_num == Last_level))		//player has finished the game!
			sprintf(endgame_text, "%s%i\n", TXT_SHIP_BONUS, endgame_points);
		sprintf(m_str[c++], "%s%i\n", TXT_TOTAL_BONUS, shield_points + energy_points + hostage_points + skill_points + all_hostage_points + endgame_points);
		sprintf(m_str[c++], "%s%i", TXT_TOTAL_SCORE, Players[Player_num].score);
	}

	for (i = 0; i < c; i++) {
		m[i].type = NM_TYPE_TEXT;
		m[i].text = m_str[i];
	}

	// m[c].type = NM_TYPE_MENU;	m[c++].text = "Ok";

	if (!Ranking.quickload) {
		if (Current_level_num < 0)
			sprintf(title, "%s %i %s\n %s %s \n Press R to restart level", TXT_SECRET_LEVEL, -Current_level_num, TXT_COMPLETE, Current_level_name, TXT_DESTROYED);
		else
			sprintf(title, "%s %i %s\n%s %s \n Press R to restart level", TXT_LEVEL, Current_level_num, TXT_COMPLETE, Current_level_name, TXT_DESTROYED);
	}
	else {
		if (Current_level_num < 0)
			sprintf(title, "%s %i %s\n %s %s \n", TXT_SECRET_LEVEL, -Current_level_num, TXT_COMPLETE, Current_level_name, TXT_DESTROYED);
		else
			sprintf(title, "%s %i %s\n%s %s \n", TXT_LEVEL, Current_level_num, TXT_COMPLETE, Current_level_name, TXT_DESTROYED);
	}

		Assert(c <= N_GLITZITEMS);

		gr_init_bitmap_alloc(&transparent, BM_LINEAR, 0, 0, 1, 1, 1);
		transparent.bm_data[0] = 255;
		transparent.bm_flags |= BM_FLAG_TRANSPARENT;

#ifdef NETWORK
		if (network && (Game_mode & GM_NETWORK))
			newmenu_do2(NULL, title, c, m, multi_endlevel_poll1, NULL, 0, Menu_pcx_name);
		else {
#endif	// Note link!
			if (!strlen(Current_mission->filename) && PlayerCfg.UsePsxSolarmap) {
				if (Current_level_num < 8)
					newmenu_do2(NULL, title, c, m, endlevel_handler, NULL, 0, "map01.pcx");
				if ((Current_level_num > 7 && Current_level_num < 18) || Current_level_num == -1)
					newmenu_do2(NULL, title, c, m, endlevel_handler, NULL, 0, "map02.pcx");
				if (Current_level_num > 17 || (Current_level_num == -2 || Current_level_num == -3))
					newmenu_do2(NULL, title, c, m, endlevel_handler, NULL, 0, "map03.pcx");
			}
			else
				newmenu_do2(NULL, title, c, m, endlevel_handler, NULL, 0, STARS_BACKGROUND);
		}
		newmenu_free_background();
}

void DoBestRanksScoreGlitz(int level_num)
{
#define N_GLITZITEMS 12
	char				m_str[N_GLITZITEMS][31];
	newmenu_item	m[N_GLITZITEMS + 1];
	int				i, c;
	char				title[128];
	int				is_last_level = 0;
	int levelHostages = 0;
	int levelPoints = 0;
	double parTime;
	int playerPoints = 0;
	double secondsTaken = 0;
	int playerHostages = 0;
	double hostagePoints = 0;
	double difficulty = 0;
	int deathCount = 0;
	double missedRngSpawn = 0;
	char buffer[256];
	char filename[256];
	char levelName[256];
	char timeOfScore[256];
	int warmStart;
	Ranking.quickload = 0; // Set this to 0 so the rank image loads if the player quickloaded on their last played level.
	Ranking.fromBestRanksButton = 1; // So exiting a level and immediately going back into one via best ranks menu doesn't cause a loop.
	sprintf(filename, "ranks/%s/%s/level%d.hi", Players[Player_num].callsign, Current_mission->filename, level_num); // Find file for the requested level.
	if (level_num > Current_mission->last_level)
		sprintf(filename, "ranks/%s/%s/levelS%d.hi", Players[Player_num].callsign, Current_mission->filename, level_num - Current_mission->last_level);
	PHYSFS_file* fp = PHYSFS_openRead(filename);
	if (fp == NULL)
		return;
	else {
		PHYSFSX_getsTerminated(fp, buffer); // Fetch level data starting here. If a parameter isn't present in the file, it'll default to 0 in calculation.
		levelHostages = atoi(buffer);
		if (levelHostages < 0) // If level is unplayed, we know because the first value in the file will be -1, which is impossible for the hostage count.
			return;
		else {
			PHYSFSX_getsTerminated(fp, buffer);
			levelPoints = atoi(buffer);
			PHYSFSX_getsTerminated(fp, buffer);
			parTime = atof(buffer);
			PHYSFSX_getsTerminated(fp, buffer); // Fetch player data starting here.
			playerPoints = atoi(buffer);
			PHYSFSX_getsTerminated(fp, buffer);
			secondsTaken = atof(buffer);
			PHYSFSX_getsTerminated(fp, buffer);
			playerHostages = atoi(buffer);
			PHYSFSX_getsTerminated(fp, buffer);
			difficulty = atof(buffer);
			PHYSFSX_getsTerminated(fp, buffer);
			deathCount = atoi(buffer);
			PHYSFSX_getsTerminated(fp, buffer);
			missedRngSpawn = atof(buffer);
			PHYSFSX_getsTerminated(fp, buffer);
			sprintf(levelName, buffer);
			PHYSFSX_getsTerminated(fp, buffer);
			sprintf(timeOfScore, buffer);
			PHYSFSX_getsTerminated(fp, buffer);
			warmStart = atoi(buffer);
		}
	}
	PHYSFS_close(fp);
	double maxScore = levelPoints * 3;
	maxScore = (int)maxScore;
	double skillPoints = playerPoints * (difficulty / 4);
	skillPoints = (int)skillPoints;
	double timePoints = (maxScore / 1.5) / pow(2, secondsTaken / parTime);
	if (secondsTaken < parTime)
		timePoints = (maxScore / 2.4) * (1 - (secondsTaken / parTime) * 0.2);
	if (!parTime)
		timePoints = 0;
	timePoints = (int)timePoints;
	hostagePoints = playerHostages * 2500 * ((difficulty + 8) / 12);
	if (playerHostages == levelHostages)
		hostagePoints *= 3;
	hostagePoints = round(hostagePoints); // Round this because I got 24999 hostage bonus once.
	double score = playerPoints + skillPoints + timePoints + missedRngSpawn + hostagePoints;
	maxScore += levelHostages * 7500;
	double deathPoints;
	if (deathCount == -1)
		deathPoints = ceil(maxScore / 12); // Round up instead of down for no damage bonus so score can't fall a point short and miss a rank.
	else
		deathPoints = -(maxScore * 0.4 - maxScore * (0.4 / pow(2, deathCount)));
	deathPoints = (int)deathPoints;
	score += deathPoints;

	int minutes = secondsTaken / 60;
	double seconds = secondsTaken - minutes * 60;
	int parMinutes = parTime / 60;
	double parSeconds = parTime - parMinutes * 60;
	char* diffname = 0;
	char timeText[256];
	char parTimeText[256];
	c = 0;
	if (difficulty == 0)
		diffname = "Trainee";
	if (difficulty == 1)
		diffname = "Rookie";
	if (difficulty == 2)
		diffname = "Hotshot";
	if (difficulty == 3)
		diffname = "Ace";
	if (difficulty == 4)
		diffname = "Insane";

	if (seconds < 10 || seconds == 60)
		sprintf(timeText, "%i:0%.3f", minutes, seconds);
	else
		sprintf(timeText, "%i:%.3f", minutes, seconds);
	if (parSeconds < 10 || parSeconds == 60)
		sprintf(parTimeText, "%i:0%.0f", parMinutes, parSeconds);
	else
		sprintf(parTimeText, "%i:%.0f", parMinutes, parSeconds);
	sprintf(m_str[c++], "Level score\t%i", playerPoints);
	sprintf(m_str[c++], "Time: %s/%s\t%.0f", timeText, parTimeText, timePoints);
	sprintf(m_str[c++], "Hostages: %i/%i\t%0.f", playerHostages, levelHostages, hostagePoints);
	sprintf(m_str[c++], "Skill: %s\t%.0f", diffname, skillPoints);
	if (deathCount > -1)
		sprintf(m_str[c++], "Deaths: %i\t%0.f", deathCount, deathPoints);
	else
		sprintf(m_str[c++], "No damage\t%.0f", deathPoints);
	if (missedRngSpawn < 0)
		sprintf(m_str[c++], "Missed RNG spawn: \t%.0f\n", missedRngSpawn);
	else
		strcpy(m_str[c++], "\n");
	if (warmStart)
		sprintf(m_str[c++], "Total score\t* %0.0f", score);
	else
		sprintf(m_str[c++], "Total score\t%0.0f", score);
		double rankPoints = (score / maxScore) * 12;
	if (maxScore == 0)
		rankPoints = 13;
	int rank = 0;
	if (rankPoints >= 0)
		rank = (int)rankPoints + 1;
	if (!PlayerCfg.RankShowPlusMinus)
		rank = truncateRanks(rank + 1) - 1;
	endlevel_current_rank = rank;
	strcpy(m_str[c++], "\n\n\n");
	sprintf(m_str[c++], "Set on %s", timeOfScore);

	for (i = 0; i < c; i++) {
		m[i].type = NM_TYPE_TEXT;
		m[i].text = m_str[i];
	}

	// m[c].type = NM_TYPE_MENU;	m[c++].text = "Ok";

	if (level_num > Current_mission->last_level)
		sprintf(title, "%s on %s\nlevel S%i: %s\nEnter plays, esc returns", Players[Player_num].callsign, Current_mission->mission_name, (Current_mission->last_level - level_num) * -1, levelName);
	else
		sprintf(title, "%s on %s\nlevel %i: %s\nEnter plays, esc returns", Players[Player_num].callsign, Current_mission->mission_name, level_num, levelName);

	Assert(c <= N_GLITZITEMS);

	gr_init_bitmap_alloc(&transparent, BM_LINEAR, 0, 0, 1, 1, 1);
	transparent.bm_data[0] = 255;
	transparent.bm_flags |= BM_FLAG_TRANSPARENT;

	Ranking.startingLevel = level_num;
	if (!strlen(Current_mission->filename) && PlayerCfg.UsePsxSolarmap) {
		if (level_num < 8)
			newmenu_do2(NULL, title, c, m, endlevel_handler, NULL, 0, "map01.pcx");
		if ((level_num > 7 && level_num < 18) || level_num == 28)
			newmenu_do2(NULL, title, c, m, endlevel_handler, NULL, 0, "map02.pcx");
		if (level_num > 17 && level_num != 28)
			newmenu_do2(NULL, title, c, m, endlevel_handler, NULL, 0, "map03.pcx");
	}
	else
		newmenu_do2(NULL, title, c, m, endlevel_handler, NULL, 0, STARS_BACKGROUND);
	newmenu_free_background();

	gr_free_bitmap_data(&transparent);
}

int draw_rock(newmenu *menu, d_event *event, grs_bitmap *background)
{
	menu = menu;

	switch (event->type)
	{
		case EVENT_WINDOW_DRAW:
			gr_set_current_canvas(NULL);
			show_fullscr(background);
			break;
			
		default:
			break;
	}
	
	return 0;
}

void do_screen_message(char *fmt, ...)
{
	va_list arglist;
	grs_bitmap background;
	char msg[1024];
	
	if (Game_mode & GM_MULTI)
		return;
	
	gr_init_bitmap_data(&background);
	if (pcx_read_bitmap(Menu_pcx_name, &background, BM_LINEAR, gr_palette) != PCX_ERROR_NONE)
		return;

	gr_palette_load(gr_palette);

	va_start(arglist, fmt);
	vsprintf(msg, fmt, arglist);
	va_end(arglist);
	
	nm_messagebox1(NULL, (int (*)(newmenu *, d_event *, void *))draw_rock, &background, 1, TXT_OK, msg);
	gr_free_bitmap_data(&background);
}

//called when the player has finished a level
void PlayerFinishedLevel(int secret_flag)
{
	int	rval;
	int 	was_multi = 0;

	if (Game_wind)
		window_set_visible(Game_wind, 0);

	//credit the player for hostages
	Players[Player_num].hostages_rescued_total += Players[Player_num].hostages_on_board;

#ifndef SHAREWARE
	if (!(Game_mode & GM_MULTI) && (secret_flag)) {
		newmenu_item	m[1];

		m[0].type = NM_TYPE_TEXT;
		m[0].text = " ";			//TXT_SECRET_EXIT;

		newmenu_do2(NULL, TXT_SECRET_EXIT, 1, m, NULL, NULL, 0, Menu_pcx_name);
	}
#endif

// -- mk mk mk -- used to be here -- mk mk mk --

#ifdef NETWORK
	if (Game_mode & GM_NETWORK)
	{
		if (secret_flag)
			Players[Player_num].connected = CONNECT_FOUND_SECRET; // Finished and went to secret level
		else
			Players[Player_num].connected = CONNECT_WAITING; // Finished but did not die
	}

#endif
	last_drawn_cockpit = -1;

	if (Current_level_num == Last_level) {
#ifdef NETWORK
		if ((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP))
		{
			was_multi = 1;
			multi_endlevel_score();
			rval = AdvanceLevel(secret_flag);				//now go on to the next one (if one)
		}
		else
#endif
		{	// Note link to above else!
			DoEndLevelScoreGlitz(0);		//give bonuses
			rval = AdvanceLevel(secret_flag);				//now go on to the next one (if one)
		}
	} else {
#ifdef NETWORK
		if (Game_mode & GM_MULTI)
			multi_endlevel_score();
		else
#endif	// Note link!!
			DoEndLevelScoreGlitz(0);		//give bonuses
		rval = AdvanceLevel(secret_flag);				//now go on to the next one (if one)
	}

	if (!was_multi && rval) {
#ifndef SHAREWARE
		if (PLAYING_BUILTIN_MISSION)
#endif
			if( !cheats.enabled)
				scores_maybe_add_player(0);
		if (Game_wind)
			window_close(Game_wind);		// Exit out of game loop
	}
	else if (rval && Game_wind)
		window_close(Game_wind);

	if (Game_wind)
		window_set_visible(Game_wind, 1);
	reset_time();
}


int checkForWarmStart()
{
	// First, check for too much shields, energy, concussion missiles, vulcan ammo or laser level. Also check for quads.
	if (f2fl(Players[Player_num].shields) > 100 || f2fl(Players[Player_num].energy) > 100 || Players[Player_num].secondary_ammo[CONCUSSION_INDEX] > 7 - Difficulty_level || Players[Player_num].primary_ammo[VULCAN_INDEX] || Players[Player_num].laser_level || Players[Player_num].flags & PLAYER_FLAGS_QUAD_LASERS)
		return 1;
	// Next, check for the presence of any weapon besides lasers.
	for (int i = 1; i < 5; i++)
		if (Players[Player_num].primary_weapon_flags & HAS_FLAG(i))
			return 1;
	// Last, check for the presence of any missiles besides concussions.
	for (int i = 1; i < 5; i++)
		if (Players[Player_num].secondary_ammo[i])
			return 1;
	return 0; // Didn't find anything, not a warm start.
}

//from which level each do you get to each secret level
// int Secret_level_table[MAX_SECRET_LEVELS_PER_MISSION];

//called to go to the next level (if there is one)
//if secret_flag is true, advance to secret level, else next normal one
//	Return true if game over.
int AdvanceLevel(int secret_flag)
{
	Control_center_destroyed = 0;
	Ranking.freezeTimer = 0;

	#ifdef EDITOR
	if (Current_level_num == 0)
	{
		return 1;		//not a real level
	}
	#endif

	key_flush();

#ifdef NETWORK
	if (Game_mode & GM_MULTI)
	{
		int result;
		result = multi_endlevel(&secret_flag); // Wait for other players to reach this point
		if (result) // failed to sync
		{
			return (Current_level_num == Last_level);
		}
	}
#endif

	key_flush();

	if (RestartLevel.isResults == 2)
		StartNewLevel(Current_level_num);
	else {
		RestartLevel.updateRestartStuff = 1; // Assume we can update this struct upon starting a level. The game will tell us not to if we're restarting the current level from the result screen.
		if (Current_level_num == Last_level) {		//player has finished the game!

		if ((Newdemo_state == ND_STATE_RECORDING) || (Newdemo_state == ND_STATE_PAUSED))
			newdemo_stop_recording(0);

			do_end_briefing_screens(Ending_text_filename);

			return 1;

		}
		else {

			Next_level_num = Current_level_num + 1;		//assume go to next normal level

			if (secret_flag) {			//go to secret level instead
				int i;

				for (i = 0; i < -Last_secret_level; i++)
					if (Secret_level_table[i] == Current_level_num) {
						Next_level_num = -(i + 1);
						break;
					}
				Assert(i < -Last_secret_level);		//couldn't find which secret level
			}

			if (Current_level_num < 0) {			//on secret level, where to go?

				Assert(!secret_flag);				//shouldn't be going to secret level
				Assert(Current_level_num <= -1 && Current_level_num >= Last_secret_level);

				Next_level_num = Secret_level_table[(-Current_level_num) - 1] + 1;
			}

			if (PlayerCfg.UsePsxSolarmap)
				solarmap_show(Next_level_num);

			StartNewLevel(Next_level_num);
			if (checkForWarmStart()) // Don't consider the player to be warm starting unless they actually have more than the default loadout.
				Ranking.warmStart = 1;
		}
	}

	key_flush();

	return 0;
}

//called when the player has died
void DoPlayerDead()
{
	int cycle_window_vis = 1;
#ifdef NETWORK
	if ( (Game_mode & GM_MULTI) && (Netgame.SpawnStyle == SPAWN_STYLE_PREVIEW))  {
		cycle_window_vis = 0; 
	}
#endif

	reset_auto_select();

	if (Game_wind && cycle_window_vis)
		window_set_visible(Game_wind, 0);

	reset_palette_add();

	gr_palette_load (gr_palette);

	dead_player_end();		//terminate death sequence (if playing)

	#ifdef EDITOR
	if (Game_mode == GM_EDITOR) {			//test mine, not real level
		object * player = &Objects[Players[Player_num].objnum];
		//nm_messagebox( "You're Dead!", 1, "Continue", "Not a real game, though." );
		if (Game_wind)
			window_set_visible(Game_wind, 1);
		load_level("gamesave.lvl");
		init_player_stats_new_ship(Player_num);
		player->flags &= ~OF_SHOULD_BE_DEAD;
		StartLevel(0);
		return;
	}
	#endif

#ifdef NETWORK
	if ( Game_mode&GM_MULTI )
	{
		multi_do_death(Players[Player_num].objnum);
	}
	else
#endif
	{				//Note link to above else!
		Players[Player_num].lives--;
		if (Players[Player_num].lives == 0)
		{
			DoGameOver();
			return;
		}
	}

	if ( Control_center_destroyed ) {
		int	rval;

		//clear out stuff so no bonus
		Players[Player_num].hostages_on_board = 0;
		Players[Player_num].energy = 0;
		Players[Player_num].shields = 0;
#ifdef NETWORK
		Players[Player_num].connected = CONNECT_DIED_IN_MINE;
#endif

		do_screen_message(TXT_DIED_IN_MINE); // Give them some indication of what happened

		if (Current_level_num == Last_level) {
#ifdef NETWORK
			if ((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP))
			{
				multi_endlevel_score();
				rval = AdvanceLevel(0);			//if finished, go on to next level
			}
			else
#endif
			{			// Note link to above else!
				rval = AdvanceLevel(0);			//if finished, go on to next level
				DoEndLevelScoreGlitz(0);
			}
			init_player_stats_new_ship(Player_num);
			last_drawn_cockpit = -1;
		} else {
#ifdef NETWORK
			if (Game_mode & GM_MULTI)
				multi_endlevel_score();
			else
#endif
				DoEndLevelScoreGlitz(0);		// Note above link!
			rval = AdvanceLevel(0);			//if finished, go on to next level
			if (RestartLevel.updateRestartStuff) {
				// Set restart stuff here so dying during self destruct correctly sets it for the next level. If this isn't done, the player can restart to get their "incinerated" stuff back.
				RestartLevel.primary_weapon = 0;
				RestartLevel.secondary_weapon = 0;
				RestartLevel.flags &= ~(PLAYER_FLAGS_QUAD_LASERS | PLAYER_FLAGS_CLOAKED | PLAYER_FLAGS_INVULNERABLE);
				RestartLevel.energy = INITIAL_ENERGY;
				RestartLevel.shields = StartingShields;
				RestartLevel.lives = Players[Player_num].lives;
				RestartLevel.laser_level = 0;
				RestartLevel.primary_weapon_flags = HAS_LASER_FLAG;
				RestartLevel.secondary_weapon_flags = HAS_CONCUSSION_FLAG;
				for (int i = 0; i < MAX_PRIMARY_WEAPONS; i++)
					RestartLevel.primary_ammo[i] = 0;
				for (int i = 0; i < MAX_SECONDARY_WEAPONS; i++)
					RestartLevel.secondary_ammo[i] = 0;
				RestartLevel.secondary_ammo[0] = 2 + NDL - Difficulty_level;
				Ranking.warmStart = 0; // Don't count next level as a warm start, since the player just lost all their stuff in self destruct.
				init_player_stats_new_ship(Player_num);
			}
			last_drawn_cockpit = -1;
		}

		if (rval) {
#ifndef SHAREWARE
			if (PLAYING_BUILTIN_MISSION)
#endif
				if (!cheats.enabled)
					scores_maybe_add_player(0);
			if (Game_wind)
				window_close(Game_wind);		// Exit out of game loop
		}
	} else {
		init_player_stats_new_ship(Player_num);
		StartLevel(1);
	}

	if (Game_wind  && cycle_window_vis)
		window_set_visible(Game_wind, 1);
	reset_time();
}

//called when the player is starting a new level for normal game mode and restore state
void StartNewLevelSub(int level_num, int page_in_textures, int secret_flag)
{
	/*
	 * This flag is present for compatibility with D2X.  Set it to zero
	 * so the optimizer deletes all reference to it.
	 */
	secret_flag = 0;
	if (!(Game_mode & GM_MULTI)) {
		last_drawn_cockpit = -1;
	}

	if (GameArg.GameLogSplit) {
		// Include the mission name, level number and timestamp in the log filename
		char log_filename[PATH_MAX];
		time_t now = time(NULL);
		struct tm *t = localtime(&now);
		snprintf(log_filename, SDL_arraysize(log_filename), "gamelog-%s-%d-%04d%02d%02d-%02d%02d%02d.txt",
			Current_mission_filename, level_num, t->tm_year + 1900, t->tm_mon, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);

		con_switch_log(log_filename);
	}

	if (Newdemo_state == ND_STATE_PAUSED)
		Newdemo_state = ND_STATE_RECORDING;

	if (Newdemo_state == ND_STATE_RECORDING) {
		newdemo_set_new_level(level_num);
		newdemo_record_start_frame(FrameTime );
	} 

	LoadLevel(level_num, page_in_textures);

	Assert(Current_level_num == level_num);	//make sure level set right

	gameseq_init_network_players(); // Initialize the Players array for
											  // this level

#ifdef NETWORK
	if (Game_mode & GM_NETWORK)
	{
		if(multi_level_sync()) // After calling this, Player_num is set
		{
			songs_play_song( SONG_TITLE, 1 ); // level song already plays but we fail to start level...
			return;
		}

		// Reset the timestamp when we can send another ship status packet (for observer mode)
		Next_ship_status_time = GameTime64;

		if (imulti_new_game)
		{
			for (int i=0;i<MAX_PLAYERS;i++)
			{
				init_player_stats_new_ship(i);
			}
		}
	}
#endif

	HUD_clear_messages();

	automap_clear_visited();

	init_player_stats_level(secret_flag);

	gr_use_palette_table( "palette.256" );
	gr_palette_load(gr_palette);

#ifndef SHAREWARE
#ifdef NETWORK
	if ((Game_mode & GM_MULTI_COOP) && Network_rejoined)
	{
		int i;
		for (i = 0; i < N_players; i++)
			Players[i].flags |= Netgame.player_flags[i];
	}
#endif
#endif

	Viewer = &Objects[Players[Player_num].objnum];

#ifdef NETWORK
	if (Game_mode & GM_MULTI)
	{
		multi_prep_level(); // Removes robots from level if necessary
	}
#endif

	gameseq_remove_unused_players();

	Game_suspended = 0;

	Control_center_destroyed = 0;
	Ranking.freezeTimer = 0;

	init_cockpit();
	init_robots_for_level();
	init_ai_objects();
	init_morphs();
	init_all_matcens();
	reset_palette_add();

	if (!(Game_mode & GM_MULTI) && !cheats.enabled)
		set_highest_level(Current_level_num);

	reset_special_effects();
	init_exploding_walls();

#ifdef NETWORK
	if (Network_rejoined == 1)
	{
		Network_rejoined = 0;
		StartLevel(1);
	}
	else
#endif
		StartLevel(0);		// Note link to above if!

	copy_defaults_to_robot_all();
	init_controlcen_for_level();

	reset_respawnable_bots();

	set_homing_update_rate(Game_mode & GM_MULTI ? Netgame.HomingUpdateRate : 25);

	//	Say player can use FLASH cheat to mark path to exit.
	Last_level_path_created = -1;

	// Initialise for palette_restore()
	if (!((Game_mode & GM_MULTI) && (Newdemo_state != ND_STATE_PLAYBACK)))
		palette_save();

	int autodemo_enabled_for_game_mode =
		(!(Game_mode & GM_MULTI) && PlayerCfg.AutoDemoSp) ||
		((Game_mode & GM_MULTI) && PlayerCfg.AutoDemoMp);
	if (autodemo_enabled_for_game_mode && Newdemo_state != ND_STATE_RECORDING) {
		newdemo_start_recording(1);
	}

	if (!Game_wind)
		game();
}

#ifdef NETWORK
extern char PowerupsInMine[MAX_POWERUP_TYPES], MaxPowerupsAllowed[MAX_POWERUP_TYPES];
#endif
void bash_to_shield (int i,char *s)
{
#ifdef NETWORK
	int type=Objects[i].id;
#endif

#ifdef NETWORK
	PowerupsInMine[type]=MaxPowerupsAllowed[type]=0;
#endif

	Objects[i].id = POW_SHIELD_BOOST;
	Objects[i].rtype.vclip_info.vclip_num = Powerup_info[Objects[i].id].vclip_num;
	Objects[i].rtype.vclip_info.frametime = Vclip[Objects[i].rtype.vclip_info.vclip_num].frame_time;
	Objects[i].size = Powerup_info[POW_SHIELD_BOOST].size;
}

// - - - - - - - - - - START OF PAR TIME ALGORITHM STUFF - - - - - - - - - - \\

// Account for custom ship properties.
#define SHIP_MOVE_SPEED (((double)Player_ship->max_thrust / ((double)Player_ship->mass * (double)Player_ship->drag) * (1 - (double)Player_ship->drag / (double)f1_0)) * pow(f1_0, 2))
// 2500 ammo. There may already be an existing macro related to the ammo cap but I couldn't find one.
#define STARTING_VULCAN_AMMO 2500 * f1_0
#define OBJECTIVE_TYPE_INVALID 0
#define OBJECTIVE_TYPE_OBJECT 1
#define OBJECTIVE_TYPE_TRIGGER 2
#define OBJECTIVE_TYPE_ENERGY 3
#define OBJECTIVE_TYPE_WALL 4

int find_connecting_side(int from, int to) // Sirius' function, but I made it take ints instead of point segs for easier use (also the old '94 function "find_connect_side" already did it with point segs anyway).
{
	for (int side = 0; side < MAX_SIDES_PER_SEGMENT; side++)
		if (Segments[from].children[side] == to)
			return side;
	// This shouldn't happen if consecutive nodes from a valid path were passed in
	Int3();
	return -1;
}

typedef struct
{
	int type;
	int ID;
} partime_objective;

typedef struct
{
	int wallID;
	partime_objective unlockedBy;
} partime_locked_wall_info;

typedef struct
{
	double movementTime;
	partime_objective toDoList[MAX_OBJECTS + MAX_WALLS];
	int toDoListSize;
	partime_objective doneList[MAX_OBJECTS + MAX_WALLS];
	int doneListSize;
	partime_objective blackList[MAX_OBJECTS + MAX_WALLS];
	int blackListSize;
	int doneWalls[MAX_WALLS];
	int doneWallsSize;
	// Remember the IDs of all walls in the level that are locked in some way, so we can see if any of our paths encounter them.
	partime_locked_wall_info lockedWalls[MAX_WALLS];
	partime_locked_wall_info reactorWalls[MAX_WALLS];
	int numLockedWalls;
	int numReactorWalls;
	vms_vector lastPosition; // Tracks the last place algo went to within the same segment.
	int matcenLives[MAX_ROBOT_CENTERS]; // We need to track how many times we trip matcens, since each one can only be tripped three times.
	// Time spent clearing matcens.
	double matcenTime;
	fix vulcanAmmo; // What it sounds like.
	// How much robot HP we've had to destroy to this point.
	double combatTime;
	// Info about the weapon algo currently has equipped.
	double ammo_usage; // For when using vulcan.
	double heldWeapons[21]; // Which weapons algo has.
	int laser_level; // It's possible to make things work without this, but just tracking laser level directly makes things a lot easier.
	double pathObstructionTime; // Amount of time spent dealing with walls or matcens on the way to an objective (basically an Abyss 1.0 hotfix for the 32k HP wall let's be real lol).
	double shortestPathObstructionTime;
	int hasQuads;
	int segnum; // What segment Algo is in.
	ubyte isSegmentAccessible[MAX_SEGMENTS];
	ubyte segmentVisited[MAX_SEGMENTS];
} partime_calc_state;

double calculate_combat_time_wall(partime_calc_state* state, int wall_num, int pathFinal) // Tell algo to use the weapon that's fastest for the destructible wall in the way.
{ // I was originally gonna ignore this since hostage doors added negligible time, but then thanks to Devil's Heart, I learned that they can have absurd HP! :D
	double thisWeaponCombatTime = -1; // How much time does this wall take to destroy with the current weapon?
	double lowestCombatTime = -1; // Track the time of the fastest weapon so far.
	double ammoUsed = 0; // Same thing but vulcan.
	int topWeapon; // So when depleting energy/ammo, the right one is depleted. Also so the console shows the right weapon.
	double damage;
	double fire_rate;
	double ammo_usage;
	double splash_radius;
	double wall_health;
	for (int n = 0; n < 21; n++)
		if (!state->heldWeapons[n]) {
			double gunpoints = 2;
			if (!(n > LASER_ID_L4) && !state->hasQuads)
				gunpoints = 4;
			if (n == VULCAN_ID)
				gunpoints = 1;
			if (n == SPREADFIRE_ID)
				gunpoints = 3;
			damage = f2fl(Weapon_info[n].strength[Difficulty_level]) * gunpoints;
			fire_rate = (double)f1_0 / Weapon_info[n].fire_wait;
			if (!(n > LASER_ID_L4)) // For some reason, the game always uses laser 1's weapon data, even though other levels have a different fire rate and energy usage in theirs.
				fire_rate = (double)f1_0 / Weapon_info[0].fire_wait;
			ammo_usage = f2fl(Weapon_info[n].ammo_usage) * 13; // The 13 is to scale with the ammo counter.
			splash_radius = f2fl(Weapon_info[n].damage_radius);
			wall_health = f2fl(Walls[wall_num].hps + 1); // We do +1 to account for walls still being alive at exactly 0 HP.
			// Assume accuracy is always 100% for walls. They're big and don't move lol.
			int shots = ceil(wall_health / damage); // Split time and energy into shots to reflect how players really fire. A 30 HP robot will take two laser 1 shots to kill, not one and a half.
			if (f2fl(state->vulcanAmmo) >= shots * ammo_usage * f1_0) // Make sure we have enough ammo for this wall before using vulcan.
				thisWeaponCombatTime = shots / fire_rate;
			else
				thisWeaponCombatTime = INFINITY; // Make vulcan's time infinite so algo won't use it without ammo.
			if (thisWeaponCombatTime <= lowestCombatTime || lowestCombatTime == -1) { // If it should be used, update algo's weapon stats to the new one's for use in combat time calculation.
				lowestCombatTime = thisWeaponCombatTime;
				ammoUsed = shots * ammo_usage * f1_0;
				topWeapon = n;
			}
		}
	if (pathFinal) { // Only announce we destroyed the wall (or drain energy/ammo) if we actually did, and aren't just simulating doing so when picking a path.
		if (topWeapon == VULCAN_ID)
			state->vulcanAmmo -= ammoUsed * f1_0;
		if (!(topWeapon > LASER_ID_L4)) {
			if (!state->hasQuads)
				printf("Took %.3fs to fight wall %i with quad laser %i.\n", lowestCombatTime, wall_num, topWeapon + 1);
			else
				printf("Took %.3fs to fight wall %i with laser %i.\n", lowestCombatTime, wall_num, topWeapon + 1);
		}
		if (topWeapon == VULCAN_ID)
			printf("Took %.3fs to fight wall %i with vulcan, now at %.0f vulcan ammo\n", lowestCombatTime, wall_num, f2fl(state->vulcanAmmo));
		if (topWeapon == SPREADFIRE_ID)
			printf("Took %.3fs to fight wall %i with spreadfire\n", lowestCombatTime, wall_num);
		if (topWeapon == PLASMA_ID)
			printf("Took %.3fs to fight wall %i with plasma\n", lowestCombatTime, wall_num);
		if (topWeapon == FUSION_ID)
			printf("Took %.3fs to fight wall %i with fusion\n", lowestCombatTime, wall_num);
	}
	return lowestCombatTime + 1; // Give an extra second per wall to wait for the explosion to go down. Flying through it causes great damage.
}

double calculate_weapon_accuracy(partime_calc_state* state, weapon_info* weapon_info, int weapon_id, object* obj, robot_info* robInfo, int isObject)
{
	// Here we use various aspects of the combat situation to estimate what percentage of shots the player will hit.
	// First initialize weapon and enemy stuff. This is gonna be a long section.
	// Everything will be doubles due to the variables' involvement in division equations.

	if (weapon_id <= LASER_ID_L4)
		weapon_id = 0; // In D1, all laser levels use level 1's stats... for some reason.
	double projectile_speed = f2fl(Weapon_info[weapon_id].speed[Difficulty_level]);
	double player_size = f2fl(ConsoleObject->size);
	double projectile_size = 1;
	if (weapon_info->render_type)
		if (weapon_info->render_type == 2)
			projectile_size = f2fl(Polygon_models[weapon_info->model_num].rad) / f2fl(weapon_info->po_len_to_width_ratio);
		else
			projectile_size = f2fl(weapon_info->blob_size);
	double enemy_runs = 0;
	double enemy_health = f2fl(robInfo->strength);
	double enemy_max_speed = f2fl(robInfo->max_speed[Difficulty_level]);
	double enemy_evade_speed = robInfo->evade_speed[Difficulty_level] * 32;
	if (enemy_evade_speed > enemy_max_speed)
		enemy_evade_speed *= 0.75;
	if (isObject) {
		if (obj->ctype.ai_info.behavior == AIB_RUN_FROM)
			enemy_runs = 1;
		if (obj->type == OBJ_CNTRLCEN) { // For some reason reactors have a speed of 120??? They don't actually move tho so mark it down as 0.
			enemy_max_speed = 0;
			enemy_evade_speed = 0;
		}
	}
	double enemy_size = f2fl(Polygon_models[robInfo->model_num].rad);
	double enemy_weapon_speed = f2fl(Weapon_info[robInfo->weapon_type].speed[Difficulty_level]);
	double enemy_weapon_size = 1;
	if (Weapon_info[robInfo->weapon_type].render_type)
		if (Weapon_info[robInfo->weapon_type].render_type == 2)
			enemy_weapon_size = f2fl(Polygon_models[Weapon_info[robInfo->weapon_type].model_num].rad) / f2fl(Weapon_info[robInfo->weapon_type].po_len_to_width_ratio);
		else
			enemy_weapon_size = f2fl(Weapon_info[robInfo->weapon_type].blob_size);
	double enemy_attack_type = robInfo->attack_type;
	double enemy_weapon_homing_flag = Weapon_info[robInfo->weapon_type].homing_flag;

	// Next, find the "optimal distance" for fighting the given enemy with the given weapon. This is the distance where the enemy's fire can be dodged off of pure reaction time, without any prediction.
	// Once the player's ship can start moving 250ms (avg human reaction time) after the enemy shoots, and get far enough out of the way for the enemy's shots to miss, it's at the optimal distance.
	// Any closer, and the player is put in too much danger. Any further, and the player faces potential accuracy loss due to the enemy having more time to dodge themselves.
	double optimal_distance;
	if (enemy_attack_type) // In the case of enemies that don't shoot at you, the optimal distance depends on their speed, as generally you wanna stand further back the quicker they can approach you.
		optimal_distance = enemy_max_speed / 4; // The /4 is in reference to the 250ms benchmark from earlier. When they start charging you, you've gotta react and start backing up.
	else
		optimal_distance = (((player_size + enemy_weapon_size * F1_0) / SHIP_MOVE_SPEED) + 0.25) * enemy_weapon_speed;
	if (enemy_runs) // We don't want snipe robots to use this, as they actually shoot things.
		optimal_distance = 80; // In the case of enemies that run from you, we'll use the maximum enemy dodge distance because it returns healthy chase time values.

	// Next, figure out how well the enemy will dodge a player attack of this weapon coming from the optimal distance away, then base accuracy off of that.
	// For simplicity, we assume enemies face longways and dodge sideways relative to player rotation, and that the player is shooting at the middle of the target from directly ahead.
	// The amount of distance required to move is based off of hard coded gunpoints on the ship, as well as the radius of the player projectile, so we'll have to set values per weapon ID.
	// For spreading weapons, the offset technically depends on which projectile we're talking about, but we'll set it to that of the middle one's starting point for now, then account for decreasing accuracy over distance later.
	double projectile_offsets[21] = { 2.2, 2.2, 2.2, 2.2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2.2, 2.2, 0, 0, 0, 0, 0, 0 };
	// Quad lasers have a wider offset, making them a little harder to dodge. Account for this.
	// This makes their accuracy worse against small enemies than reality due to the offset of the inner lasers being ignored, but this is a rare occurence.
	if (!state->hasQuads) {
		projectile_offsets[LASER_ID_L1] *= 1.5;
		projectile_offsets[LASER_ID_L2] *= 1.5;
		projectile_offsets[LASER_ID_L3] *= 1.5;
		projectile_offsets[LASER_ID_L4] *= 1.5;
	}
	double dodge_distance = projectile_offsets[weapon_id] + projectile_size + enemy_size;

	// Now we reduce accuracy over distance for spreading weapons. Since that's also hard coded, we can just apply enemy size based multipliers per weapon ID.
	// At a certain distance, projectiles for Vulcan and Spreadfire will start missing from drifting so far off course.
	// For Vulcan we scale accuracy by the probability of a shot landing at a given distance (since the spread is random), and for Spreadfire/Helix we use a binary outcome.
	double accuracy_multiplier = 1;
	if (weapon_id == VULCAN_ID)
		if (optimal_distance / 32 > enemy_size + projectile_size) // This is the distance where Vulcan's accuracy dropoff starts.
			accuracy_multiplier *= (enemy_size + projectile_size) / (optimal_distance / 32);
	if (weapon_id == SPREADFIRE_ID)
		if (optimal_distance / 16 > enemy_size + projectile_size) // Divisor is gotten because Spreadfire projectiles move one unit outward for every 16 units forward.
			accuracy_multiplier /= 3;
	// If the enemy is small enough to fit between projectiles, only one can hit at a time, so halve accuracy.
	// This makes Algo unlikely to use something like lasers against sidearm modula, which is good because it obeys real world expectations.
	if (enemy_size < projectile_offsets[weapon_id] - projectile_size)
		accuracy_multiplier *= 0.5;

	double accuracy;
	if ((enemy_runs && enemy_max_speed) || (!enemy_runs && enemy_evade_speed))
		if (optimal_distance > 80) // Enemies can't dodge something until it's within 80 units of them (Source: ai.c line 830), but we can't cap optimal_distance itself at that because it'll make spread penalty too lenient.
			if (enemy_runs)
				accuracy = ((dodge_distance / enemy_max_speed) / (80 / projectile_speed));
			else
				accuracy = ((dodge_distance / enemy_evade_speed) / (80 / projectile_speed));
		else
			if (enemy_runs)
				accuracy = ((dodge_distance / enemy_max_speed) / (optimal_distance / projectile_speed));
			else
				accuracy = ((dodge_distance / enemy_evade_speed) / (optimal_distance / projectile_speed));
	else
		return accuracy_multiplier; // If the enemy doesn't move in the relevant way, guarantee all hits (assuming efficient size).
	if (accuracy * accuracy_multiplier > 1) // Accuracy can't be greater than 100%.
		return 1;
	else if (weapon_info->homing_flag)
		return (0.5 + accuracy / 2) * accuracy_multiplier; // Buff player accuracy if their weapon has homing.
	else
		return accuracy * accuracy_multiplier;
	// Enemies evade less and less linearly as their health goes down (Source: ai.c line 997), so we SHOULD return an average of 100% and the starting acc always instead of just with homing...
	// Gonna be honest though, I'm actually not sure about that HP part for D2, and high difficulty times are too harsh if we do it that way. Also the average thing is a lazy approach anyway.
}

double calculate_combat_time(partime_calc_state* state, object* obj, robot_info* robInfo, int isObject, int isMatcen) // Tell algo to use the weapon that's fastest for the current enemy.
{
	double thisWeaponCombatTime = -1; // How much time does this enemy take to kill with the current weapon?
	double lowestCombatTime = -1; // Track the time of the fastest weapon so far.
	double ammoUsed = 0; // Same thing but vulcan.
	int topWeapon = -1; // So when depleting energy/ammo, the right one is depleted. Also so the console shows the right weapon.
	double offspringHealth; // So multipliers done to offspring don't bleed into their parents' values.
	double accuracy; // Players are NOT perfect, and it's usually not their fault. We need to account for this if we want all par times to be reachable.
	double topAccuracy; // The percentage shown on the debug console.
	double adjustedRobotHealthNoAccuracy;
	// Weapon values converted to a format human beings in 2024 can understand.
	for (int n = 0; n < 21; n++)
		if (!state->heldWeapons[n]) {
			weapon_info* weapon_info = &Weapon_info[n];
			double gunpoints = 2;
			if (!(n > LASER_ID_L4) && !state->hasQuads) // Account for increased damage of quads.
				gunpoints = 4;
			if (n == VULCAN_ID)
				gunpoints = 1;
			if (n == SPREADFIRE_ID)
				gunpoints = 3;
			double damage = f2fl(weapon_info->strength[Difficulty_level]) * gunpoints;
			double fire_rate = (double)f1_0 / weapon_info->fire_wait;
			double ammo_usage = f2fl(weapon_info->ammo_usage) * 13; // The 13 is to scale with the ammo counter.
			double splash_radius = f2fl(weapon_info->damage_radius);
			double enemy_health = f2fl(robInfo->strength + 1); // We do +1 to account for robots still being alive at exactly 0 HP.
			double enemy_size = f2fl(Polygon_models[robInfo->model_num].rad);
			if (!(n > LASER_ID_L4)) // For some reason, the game always uses laser 1's weapon data, even though other levels have a different fire rate and energy usage in theirs.
				fire_rate = (double)f1_0 / Weapon_info[0].fire_wait;
			if (isObject)
				if (obj->type == OBJ_CNTRLCEN) {
					if (n == FUSION_ID)
						damage *= 2; // Fusion's damage is doubled against reactors in Redux.
					enemy_health = f2fl(obj->shields + 1);
					enemy_size = f2fl(obj->size);
				}
			double adjustedRobotHealth = enemy_health;
			adjustedRobotHealth /= (splash_radius - enemy_size) / splash_radius >= 0 ? 1 + (splash_radius - enemy_size) / splash_radius : 1; // Divide the health value of the enemy instead of increasing damage when accounting for splash damage, since we'll potentially have multiple damage values.
			adjustedRobotHealthNoAccuracy = adjustedRobotHealth;
			adjustedRobotHealth /= calculate_weapon_accuracy(state, weapon_info, n, obj, robInfo, isObject);
			accuracy = adjustedRobotHealthNoAccuracy / adjustedRobotHealth;
			int shots = round(ceil(adjustedRobotHealthNoAccuracy / damage) / accuracy); // Split time and energy into shots to reflect how players really fire. A 30 HP robot will take two laser 1 shots to kill, not one and a half.
			if (f2fl(state->vulcanAmmo) >= shots * ammo_usage * f1_0) // Make sure we have enough ammo for this robot before using vulcan.
				thisWeaponCombatTime = shots / fire_rate;
			else
				thisWeaponCombatTime = INFINITY; // Make vulcan's/gauss' time infinite so algo won't use it without ammo.
			if (thisWeaponCombatTime <= lowestCombatTime || lowestCombatTime == -1) { // If it should be used, update algo's weapon stats to the new one's for use in combat time calculation.
				lowestCombatTime = thisWeaponCombatTime;
				state->ammo_usage = shots * ammo_usage;
				ammoUsed = ammo_usage * shots * f1_0;
				topWeapon = n;
				topAccuracy = accuracy * 100;
			}
		}
	if (topWeapon == VULCAN_ID)
		state->vulcanAmmo -= ammoUsed * f1_0;
	if (isMatcen) {
		// Now account for RNG ammo drops from matcen bots and their robot spawn.
		if (robInfo->contains_type == OBJ_POWERUP && robInfo->contains_id == POW_VULCAN_AMMO)
			state->ammo_usage -= f2fl(((double)robInfo->contains_count * ((double)robInfo->contains_prob / 16)) * (STARTING_VULCAN_AMMO / 2));
		if (robInfo->contains_type == OBJ_ROBOT) {
			if (Robot_info[robInfo->contains_id].contains_type == OBJ_POWERUP && Robot_info[robInfo->contains_id].contains_id == POW_VULCAN_AMMO)
				state->ammo_usage -= f2fl(((double)Robot_info[robInfo->contains_id].contains_count * ((double)Robot_info[robInfo->contains_id].contains_prob / 16)) * (STARTING_VULCAN_AMMO / 2));
		}
	}
	else if (isObject) {
		if (!(topWeapon > LASER_ID_L4)) {
			if (!state->hasQuads)
				printf("Took %.3fs to fight robot type %i with quad laser %i, %.2f accuracy\n", lowestCombatTime, obj->id, topWeapon + 1, topAccuracy);
			else
				printf("Took %.3fs to fight robot type %i with laser %i, %.2f accuracy\n", lowestCombatTime, obj->id, topWeapon + 1, topAccuracy);
		}
		if (topWeapon == VULCAN_ID)
			printf("Took %.3fs to fight robot type %i with vulcan, %.2f accuracy, now at %.0f vulcan ammo\n", lowestCombatTime, obj->id, topAccuracy, f2fl(state->vulcanAmmo));
		if (topWeapon == SPREADFIRE_ID)
			printf("Took %.3fs to fight robot type %i with spreadfire, %.2f accuracy\n", lowestCombatTime, obj->id, topAccuracy);
		if (topWeapon == PLASMA_ID)
			printf("Took %.3fs to fight robot type %i with plasma, %.2f accuracy\n", lowestCombatTime, obj->id, topAccuracy);
		if (topWeapon == FUSION_ID)
			printf("Took %.3fs to fight robot type %i with fusion, %.2f accuracy\n", lowestCombatTime, obj->id, topAccuracy);
	}
	return lowestCombatTime;
}

int getObjectiveSegnum(partime_objective objective)
{
	if (objective.type == OBJECTIVE_TYPE_OBJECT)
		return Objects[objective.ID].segnum;
	if (objective.type == OBJECTIVE_TYPE_TRIGGER || objective.type == OBJECTIVE_TYPE_WALL)
		return Walls[objective.ID].segnum;
	printf("Warning: Par time is going to an undefined segment!\n");
	return -1;
}

vms_vector getObjectivePosition(partime_objective objective)
{
	if (objective.type == OBJECTIVE_TYPE_OBJECT)
		return Objects[objective.ID].pos;
	vms_vector segmentCenter;
	compute_segment_center(&segmentCenter, &Segments[getObjectiveSegnum(objective)]);
	return segmentCenter;
}

int robotHasKey(object* obj) // Should cover all the shinanegans level authors can do with key holders, in one neat convenient spot.
{
	if (obj->type == OBJ_ROBOT && obj->contains_type == OBJ_POWERUP && (obj->contains_id == POW_KEY_BLUE || obj->contains_id == POW_KEY_GOLD || obj->contains_id == POW_KEY_RED))
		return obj->contains_id; // This specific robot contains a key.
	if (obj->type == OBJ_ROBOT && obj->contains_type == -1 && Robot_info[obj->id].contains_type == OBJ_POWERUP && (Robot_info[obj->id].contains_id == POW_KEY_BLUE || Robot_info[obj->id].contains_id == POW_KEY_GOLD || Robot_info[obj->id].contains_id == POW_KEY_RED))
		return Robot_info[obj->id].contains_id; // This robot type is hard coded to contain a key, and this specific robot contains nothing different.
	if (obj->contains_type == OBJ_ROBOT && Robot_info[obj->contains_id].contains_type == OBJ_POWERUP && (Robot_info[obj->contains_id].contains_id == POW_KEY_BLUE || Robot_info[obj->contains_id].contains_id == POW_KEY_GOLD || Robot_info[obj->contains_id].contains_id == POW_KEY_RED))
		return Robot_info[obj->contains_id].contains_id; // This specific robot contains a robot whose type is hard coded to contain a key.
	return 0; // This robot does not contain a key.
}

void robotHasPowerup(partime_calc_state* state, int robotID, double weight) {
	robot_info* robInfo = &Robot_info[robotID];
	int weapon_id;
	int i;
	if (robInfo->contains_type == OBJ_POWERUP) {
		weapon_id = 0;
		if (robInfo->contains_id == POW_VULCAN_WEAPON)
			weapon_id = VULCAN_ID;
		if (robInfo->contains_id == POW_SPREADFIRE_WEAPON)
			weapon_id = SPREADFIRE_ID;
		if (robInfo->contains_id == POW_PLASMA_WEAPON)
			weapon_id = PLASMA_ID;
		if (robInfo->contains_id == POW_FUSION_WEAPON)
			weapon_id = FUSION_ID;
		if (weapon_id) {
			state->heldWeapons[weapon_id] *= (1 - weight) + (pow(1 - ((double)robInfo->contains_prob / 16), robInfo->contains_count) * weight);
			if (state->heldWeapons[weapon_id] <= 0.0625)
				state->heldWeapons[weapon_id] = 0;
		}
		else {
			if (robInfo->contains_id == POW_LASER) {
				for (i = 0; i < round(robInfo->contains_count * (robInfo->contains_prob / 16)); i++) {
					if (state->laser_level < LASER_ID_L4)
						state->laser_level++;
					state->heldWeapons[state->laser_level] = 0;
				}
			}
			if (robInfo->contains_id == POW_QUAD_FIRE) {
				state->hasQuads *= (1 - weight) + (pow(1 - ((double)robInfo->contains_prob / 16), robInfo->contains_count) * weight);
				if (state->hasQuads <= 0.0625)
					state->hasQuads = 0;
			}
			if (robInfo->contains_id == POW_VULCAN_AMMO)
				state->vulcanAmmo += (STARTING_VULCAN_AMMO / 2) * round(robInfo->contains_count * (robInfo->contains_prob / 16));
		}
	}
	if (robInfo->contains_type == OBJ_ROBOT) {
		robInfo = &Robot_info[Robot_info[robotID].contains_id];
		weapon_id = 0;
		if (robInfo->contains_id == POW_VULCAN_WEAPON)
			weapon_id = VULCAN_ID;
		if (robInfo->contains_id == POW_SPREADFIRE_WEAPON)
			weapon_id = SPREADFIRE_ID;
		if (robInfo->contains_id == POW_PLASMA_WEAPON)
			weapon_id = PLASMA_ID;
		if (robInfo->contains_id == POW_FUSION_WEAPON)
			weapon_id = FUSION_ID;
		if (weapon_id) {
			state->heldWeapons[weapon_id] *= (1 - weight) + (pow(1 - ((double)robInfo->contains_prob / 16), robInfo->contains_count) * weight);
			if (state->heldWeapons[weapon_id] <= 0.0625)
				state->heldWeapons[weapon_id] = 0;
		}
		else {
			if (robInfo->contains_id == POW_LASER) {
				for (i = 0; i < round(robInfo->contains_count * (robInfo->contains_prob / 16)); i++) {
					if (state->laser_level < LASER_ID_L4)
						state->laser_level++;
					state->heldWeapons[state->laser_level] = 0;
				}
			}
			if (robInfo->contains_id == POW_QUAD_FIRE) {
				state->hasQuads *= (1 - weight) + (pow(1 - ((double)robInfo->contains_prob / 16), robInfo->contains_count) * weight);
				if (state->hasQuads <= 0.0625)
					state->hasQuads = 0;
			}
			if (robInfo->contains_id == POW_VULCAN_AMMO)
				state->vulcanAmmo += (STARTING_VULCAN_AMMO / 2) * round(robInfo->contains_count * (robInfo->contains_prob / 16));
		}
	}
}

int findKeyObjectID(partime_calc_state* state, int keyType, int dontCheckAccessibility)
{
	int powerupID;
	switch (keyType)
	{
	case KEY_BLUE:
		powerupID = POW_KEY_BLUE;
		break;
	case KEY_GOLD:
		powerupID = POW_KEY_GOLD;
		break;
	case KEY_RED:
		powerupID = POW_KEY_RED;
		break;
	default:
		// That's not a key!
		Int3();
		return -1;
	}

	for (int i = 0; i <= Highest_object_index; i++) {
		if ((Objects[i].type == OBJ_POWERUP && Objects[i].id == powerupID) || robotHasKey(&Objects[i]) == powerupID)
			if (dontCheckAccessibility || state->isSegmentAccessible[Objects[i].segnum]) // Make sure the key or the robot that contains it can be physically flown to by the player.
				return i;
	}

	return -1; // Not found
}

int findConnectedWallNum(int wall_num)
{
	wall* wall = &Walls[wall_num];
	segment* connectedSegment = &Segments[Segments[wall->segnum].children[wall->sidenum]];
	if (connectedSegment == NULL)
		return -1;

	int connectedSide = find_connect_side(&Segments[wall->segnum], connectedSegment);
	if (connectedSide >= 0)
		return connectedSegment->sides[connectedSide].wall_num;

	return -1;
}

int findTriggerWallForWall(int wallID)
{
	wall* thisWall = &Walls[wallID];
	wall* connectedWall = NULL;
	int connectedWallID = findConnectedWallNum(wallID);
	if (connectedWallID >= 0)
		connectedWall = &Walls[connectedWallID];

	// Keep in mind: This doesn't guarantee the NEAREST trigger for the locked wall, but that'll prooooobably be fine.
	for (int otherWallID = 0; otherWallID < Num_walls; otherWallID++) {
		if (otherWallID == wallID || Walls[otherWallID].trigger == -1)
			continue;
		trigger* t = &Triggers[Walls[otherWallID].trigger];
		for (short link_num = 0; link_num < t->num_links; link_num++) {
			// Does the trigger target point at this wall's segment/side?
			if (t->seg[link_num] == thisWall->segnum && t->side[link_num] == thisWall->sidenum) {
				return otherWallID;
			}
			// Or that of the other side of this wall, if any?
			if (connectedWall && t->seg[link_num] == connectedWall->segnum && t->side[link_num] == connectedWall->sidenum) {
				return otherWallID;
			}
		}
	}

	return -1;
}

int findReactorObjectID()
{
	for (int i = 0; i <= Highest_object_index; i++) // First, look for bosses.
		if (Objects[i].type == OBJ_ROBOT && Robot_info[Objects[i].id].boss_flag != 0)
			return i;
	for (int i = 0; i <= Highest_object_index; i++) // If none are found, look for reactors.
		if (Objects[i].type == OBJ_CNTRLCEN)
			return i;
	return -1;
}

void removeLockedWallFromList(int index)
{
	for (int i = index; i < Ranking.numCurrentlyLockedWalls; i++) {
		Ranking.currentlyLockedWalls[i] = Ranking.currentlyLockedWalls[i + 1];
		Ranking.parTimeUnlockIDs[i] = Ranking.parTimeUnlockIDs[i + 1];
		Ranking.parTimeUnlockTypes[i] = Ranking.parTimeUnlockTypes[i + 1];
	}
	Ranking.numCurrentlyLockedWalls--;
}

void addObjectiveToList(partime_objective* list, int* listSize, partime_objective objective, int isDoneList)
{
	int i;
	// First make sure it's not already in there
	for (i = 0; i < *listSize; i++)
		if (list[i].type == objective.type && list[i].ID == objective.ID)
			return;

	if (*listSize >= MAX_OBJECTS + MAX_WALLS)
		Int3(); // We're out of space in the list; this shouldn't happen.

	list[*listSize].type = objective.type;
	list[*listSize].ID = objective.ID;
	(*listSize)++;

	if (isDoneList) {
		for (i = 0; i < Ranking.numCurrentlyLockedWalls; i++) {
			if (objective.type == Ranking.parTimeUnlockTypes[i] && objective.ID == Ranking.parTimeUnlockIDs[i]) {
				removeLockedWallFromList(i);
				i--; // Decrement the index so we don't skip over a consecutive unlock.
			}
		}
	}
}

void initLockedWalls(partime_calc_state* state, int removeUnlockableWalls)
{
	int i;
	partime_locked_wall_info* wallInfo = &state->lockedWalls[state->numLockedWalls];
	partime_locked_wall_info* reactorInfo = &state->reactorWalls[state->numReactorWalls];
	state->numLockedWalls = 0;
	for (i = 0; i < Num_walls; i++) {
		// In D2, closed walls can't be "locked" as in being unlockable, but we still need to consider them locked so the other stuff we copied from D2 works. That being said, you won't see closed walls be considered on line 2283 like they are in D2.
		if ((Walls[i].type == WALL_DOOR && (Walls[i].keys == KEY_BLUE || Walls[i].keys == KEY_GOLD || Walls[i].keys == KEY_RED)) || Walls[i].flags & WALL_DOOR_LOCKED || Walls[i].type == WALL_CLOSED) {
			partime_locked_wall_info* wallInfo = &state->lockedWalls[state->numLockedWalls];
			partime_locked_wall_info* reactorInfo = &state->reactorWalls[state->numReactorWalls];
			state->numLockedWalls++;
			wallInfo->wallID = i;

			// Is it opened by a key?
			if (Walls[i].type == WALL_DOOR && (Walls[i].keys == KEY_BLUE || Walls[i].keys == KEY_GOLD || Walls[i].keys == KEY_RED)) {
				wallInfo->unlockedBy.ID = findKeyObjectID(state, Walls[i].keys, removeUnlockableWalls);
				if (wallInfo->unlockedBy.ID == -1) { // If a colored door has no key, don't assign a type or ID to it.
					wallInfo->unlockedBy.type = OBJECTIVE_TYPE_INVALID;
					wallInfo->unlockedBy.ID = 0;
				}
				else
					wallInfo->unlockedBy.type = OBJECTIVE_TYPE_OBJECT;
				continue;
			}

			if (Walls[i].flags & WALL_DOOR_LOCKED) {
				// ...or is it opened by a trigger?
				int unlockWall = findTriggerWallForWall(i);
				if (unlockWall != -1) {
					wallInfo->unlockedBy.type = OBJECTIVE_TYPE_TRIGGER;
					wallInfo->unlockedBy.ID = unlockWall;
					continue;
				}

				// So doors opened by the reactor exploding are handled correctly.
				for (short link_num = 0; link_num < ControlCenterTriggers.num_links; link_num++) {
					if (ControlCenterTriggers.seg[link_num] == Walls[i].segnum && ControlCenterTriggers.side[link_num] == Walls[i].sidenum) {
						state->numLockedWalls--; // Nevermind about adding it here.
						state->numReactorWalls++; // We're adding it here instead.
						reactorInfo->wallID = i;
						reactorInfo->unlockedBy.type = 1;
						for (int o = 0; o <= Highest_object_index; o++)
							if (Objects[o].type == OBJ_CNTRLCEN)
								reactorInfo->unlockedBy.ID = o;
						break;
					}
				}
			}
		}
	}
	Ranking.numCurrentlyLockedWalls = state->numLockedWalls + state->numReactorWalls;
	for (i = 0; i < state->numLockedWalls; i++) {
		Ranking.currentlyLockedWalls[i] = state->lockedWalls[i].wallID;
		Ranking.parTimeUnlockIDs[i] = state->lockedWalls[i].unlockedBy.ID;
		Ranking.parTimeUnlockTypes[i] = state->lockedWalls[i].unlockedBy.type;
		// Don't add an unlock objective if it's a robot holding a key. Robots are already added anyway. (Shoutout to The Pit from LOTW for having a boss hold a key or I would never have noticed this happening.)
		// Also don't add anything with an invalid objective type. That just makes things more confusing to debug.
		if (state->lockedWalls[i].unlockedBy.type && !(state->lockedWalls[i].unlockedBy.type == OBJECTIVE_TYPE_OBJECT && Objects[state->lockedWalls[i].unlockedBy.ID].type == OBJ_ROBOT)) // Don't add an unlock objective if it's a robot holding a key. Robots are already added anyway. (Shoutout to The Pit from LOTW for having a boss hold a key or I would never have noticed this happening.)
			addObjectiveToList(state->toDoList, &state->toDoListSize, state->lockedWalls[i].unlockedBy, 0); // Add every key and unlock trigger to the to-do list, ignoring duplicates.
	}
	for (i = state->numLockedWalls; i < Ranking.numCurrentlyLockedWalls; i++) { // Now add reactor walls.
		Ranking.currentlyLockedWalls[i] = state->reactorWalls[i - state->numLockedWalls].wallID;
		Ranking.parTimeUnlockIDs[i] = state->reactorWalls[i - state->numLockedWalls].unlockedBy.ID;
		Ranking.parTimeUnlockTypes[i] = state->reactorWalls[i - state->numLockedWalls].unlockedBy.type;
	}
	// Now we have to iterate again because the unlocked side of one-sided locked walls need to be tested for an unlock ID, and that can only be done AFTER the rest of the unlocks have been added.
	for (i = 0; i < Num_walls; i++) {
		if (Walls[i].type == WALL_DOOR) { // Only doors can be shot from the inside. I wager that shooting the inside of a grate won't accomplish much.
			for (int w = 0; w < state->numLockedWalls; w++) {
				if (state->lockedWalls[w].wallID == i) {
					if (state->lockedWalls[w].unlockedBy.type) // If an unlock type never got assigned for this locked door, its neighboring wall could be what unlocks it (D1 S2).
						continue; // If one was though, that's not the case.
					int adjacent_wall_num = findConnectedWallNum(i);
					int skip = 0;
					for (int w2 = 0; w2 < state->numLockedWalls; w2++)
						if (state->lockedWalls[w2].wallID == adjacent_wall_num) {
							if (!state->lockedWalls[w2].unlockedBy.type) // Make sure it can be unlocked. If neither side can be unlocked then there's no point going through with this step.
								skip = 1;
							break;
						}
					if (!skip) {
						// Now make it the unlock ID for the locked side. Note: Due to the level design, these will probably be inaccessible objectives.
						// Wall type objectives function identically to trigger types, except Algo must have the required key before being able to mark them as done (like in Beeblebrox Mine).
						state->lockedWalls[w].unlockedBy.type = OBJECTIVE_TYPE_WALL;
						state->lockedWalls[w].unlockedBy.ID = adjacent_wall_num; // Wall type objectives are also identified by their wall number rather than their segment number, so we can read the keys associated with the wall.
						for (int w = 0; w < Ranking.numCurrentlyLockedWalls; w++) // Update the Ranking array to match.
							if (Ranking.currentlyLockedWalls[w] == i) {
								Ranking.parTimeUnlockTypes[w] = OBJECTIVE_TYPE_WALL;
								Ranking.parTimeUnlockIDs[w] = adjacent_wall_num;
								break;
							}
					}
				}
			}
		}
		// Note: As a side effect of this, the back sides of spawn doors and exits (if they're unlocked) will be added as inaccessible objectives, but this will almost always add negligible time to par.
	}
	// For the segment accessibility pass, we're gonna remove the unlockable walls so it can get through stuff like key doors.
	// We have to do this iteration separately, because if we don't, the part of marking things as done that removes locked walls from Ranked.currentlyLockedWalls will be removing them from a list that's still populating.
	if (removeUnlockableWalls) {
		for (i = 0; i < state->numLockedWalls; i++) // Only move something if it has a type, so we don't ignore grates as well.
			if (state->lockedWalls[i].unlockedBy.type)
				addObjectiveToList(state->doneList, &state->doneListSize, state->lockedWalls[i].unlockedBy, 1);
		if (state->numReactorWalls) // If needed, also add the reactor to done list so Algo can access reactor closets on its first run.
			addObjectiveToList(state->doneList, &state->doneListSize, state->reactorWalls[0].unlockedBy, 1);
		for (i = 0; i < state->numReactorWalls; i++) { // Remove any locked walls from the list that are the other side of a reactor wall. Algo went inside the D1 S1 reactor closet and couldn't get back out.
			int adjacent_wall_num = findConnectedWallNum(state->reactorWalls[i].wallID);
			for (int w = 0; w < Ranking.numCurrentlyLockedWalls; w++)
				if (Ranking.currentlyLockedWalls[w] == adjacent_wall_num)
					removeLockedWallFromList(w);
		}
	}
}

void removeObjectiveFromList(partime_objective* list, int* listSize, partime_objective objective)
{
	if (*listSize == 0)
		return;

	int offset = 0;
	for (offset = 0; offset < *listSize; offset++)
		if (list[offset].type == objective.type && list[offset].ID == objective.ID)
			break;
	if (offset + 1 < *listSize)
		memmove(list + offset, list + offset + 1, (*listSize - offset - 1) * sizeof(partime_objective));
	(*listSize)--;
}

// Find a path from a start segment to an objective.
// A lot of this is copied from the mark_player_path_to_segment function in game.c.
short create_path_partime(int start_seg, int target_seg, point_seg** path_start, int* path_count, partime_calc_state* state, partime_objective objective)
{
	object* objp = ConsoleObject;
	short player_path_length = 0;

	if (state->isSegmentAccessible[target_seg])
		create_path_points(objp, start_seg, target_seg, Point_segs_free_ptr, &player_path_length, MAX_POINT_SEGS, 0, 0, -1, objective.type, objective.ID, 0);
	else
		create_path_points(objp, start_seg, target_seg, Point_segs_free_ptr, &player_path_length, MAX_POINT_SEGS, 0, 0, -1, objective.type, objective.ID, 1);

	*path_start = Point_segs_free_ptr;
	*path_count = player_path_length;

	if (Point_segs[player_path_length - 1].segnum != target_seg)
		return 0;

	return player_path_length;
}

int find_reactor_wall_partime(partime_calc_state* state, point_seg* path, int path_count)
{
	for (int i = 0; i < path_count - 1; i++) {
		int sideNum = find_connecting_side(path[i].segnum, path[i + 1].segnum);
		side* side = &Segments[path[i].segnum].sides[sideNum];
		if (side->wall_num < 0)
			// No wall here, so it's definitely not locked.
			continue;

		// Is this wall unlocked by the reactor/boss dying?
		for (int w = 0; w < state->numReactorWalls; w++)
			if (side->wall_num == state->reactorWalls[w].wallID)
				return w;
	}
	return -1;
}

double calculate_path_length_partime(partime_calc_state* state, point_seg* path, int path_count, partime_objective objective, int path_final)
{
	// Find length of path in units and return it.
	// Note: technically we should be using f2fl on the result of vm_vec_dist, but since the
	// multipliers are baked into the constants in calculateParTime already, maybe it's better to
	// leave it for now.
	double pathLength = 0;
	state->pathObstructionTime = 0;
	if (path_count > 1) {
		for (int i = 0; i < path_count - 1; i++) {
			pathLength += vm_vec_dist(&path[i].point, &path[i + 1].point);
			// For objects, once we reach the target segment we move to the object to "pick it up".
			// Note: For now, this applies to robots, too.
			// Now, we account for the time it'd take to fight walls on the path (Abyss 1.0 par time 32k HP wall hotfix lol). Originally I accounted for matcen fight time as well, but that change did more harm than good.
			int wall_num = Segments[path[i].segnum].sides[find_connecting_side(path[i].segnum, path[i + 1].segnum)].wall_num;
			if (wall_num > -1) {
				if (Walls[wall_num].type == WALL_BLASTABLE) {
					int skip = 0; // So it skips incrementing this path's time for a wall already marked done.
					for (int n = 0; n < state->doneWallsSize; n++)
						if (state->doneWalls[n] == wall_num)
							skip = 1; // If algo's already destroyed this wall, don't account for it.
					if (!skip)
						state->pathObstructionTime += (calculate_combat_time_wall(state, wall_num, 0));
				}
			}
		}
		if (objective.type == OBJECTIVE_TYPE_OBJECT)
			pathLength += vm_vec_dist(&path[path_count - 1].point, &Objects[objective.ID].pos);
		// Paths to unreachable triggers will remain incomplete, but that's alright. They should never be unreachable anyway.
	}
	// Objective is in the same segment as the player. If it's an object, we still move to it.
	else if (objective.type == OBJECTIVE_TYPE_OBJECT)
		pathLength = vm_vec_dist(&state->lastPosition, &Objects[objective.ID].pos);
	state->pathObstructionTime *= SHIP_MOVE_SPEED; // Convert time to distance before adding.
	pathLength += state->pathObstructionTime;
	return pathLength; // We still need pathLength, despite now adding to movementTime directly, because individual paths need compared. Also fuelcen trip logic. You'll understand why if you look there.
}

int thisWallUnlocked(int wall_num, int currentObjectiveType, int currentObjectiveID, int typeThreeCheck)
{
	int unlocked = 1;
	for (int i = 0; i < Ranking.numCurrentlyLockedWalls; i++)
		if (Ranking.currentlyLockedWalls[i] == wall_num) // Let Algo through anyway if the wall is transparent and we're headed toward an unlock we don't have to go directly to (EG shooting through grate at unlocked side of door like S2).
			unlocked = (currentObjectiveType == OBJECTIVE_TYPE_WALL && check_transparency(&Segments[Walls[wall_num].segnum], Walls[wall_num].sidenum));
	if (!typeThreeCheck) { // If we're deciding whether to continue in find_nearest_objective_partime, we only want the wall facing Algo to be checked so wall type objectives don't always fail.
		// Also check the other side, so Algo doesn't get stuck in the milk closet on Vertigo 19. Oh, or others I guess.
		wall_num = findConnectedWallNum(wall_num);
		for (int i = 0; i < Ranking.numCurrentlyLockedWalls; i++)
			if (Ranking.currentlyLockedWalls[i] == wall_num)
				unlocked = (currentObjectiveType == OBJECTIVE_TYPE_WALL && check_transparency(&Segments[Walls[wall_num].segnum], Walls[wall_num].sidenum));
	}
	return unlocked;
}

partime_objective find_nearest_objective_partime(partime_calc_state* state, int start_seg, partime_objective* objectiveList, int objectiveListSize, point_seg** path_start, int* path_count, double* path_length)
{
	double pathLength;
	double shortestPathLength = -1;
	partime_objective nearestObjective;
	partime_objective objective;
	int i;
	int nearestSegnum = state->segnum;
	int shortestDistance;
	int distance;
	int objectiveSegnum;
	short player_path_length;

	vms_vector start;
	compute_segment_center(&start, &Segments[start_seg]);

	for (i = 0; i < objectiveListSize; i++) {
		objective = objectiveList[i];
		objectiveSegnum = getObjectiveSegnum(objective);
		// Draw a path as far as we can to the objective, avoiding currently locked doors. If we don't make it all the way, ignore any closed walls. Primarily for shooting through grates, but prevents a softlock on actual uncompletable levels.
		player_path_length = create_path_partime(start_seg, objectiveSegnum, path_start, path_count, state, objective);
		// If we're shooting the unlockable side of a one-sided locked wall, make sure we have the keys needed to unlock it first.
		// Also CAN ignore this if it's a transparent door but I'm not too worried about this.
		if (objective.type == OBJECTIVE_TYPE_WALL && !thisWallUnlocked(objective.ID, OBJECTIVE_TYPE_WALL, objective.ID, 1))
			continue;
		if (!player_path_length)
			continue;
		pathLength = calculate_path_length_partime(state, *path_start, *path_count, objective, 0);
		if (pathLength < shortestPathLength || shortestPathLength < 0) {
			shortestPathLength = pathLength;
			nearestObjective = objective;
			state->shortestPathObstructionTime = state->pathObstructionTime; // So the wrong amount isn't subtracted when it's time to add the real path length to movement time.
		}
	}

	// Did we find a legal objective? Return that.
	if (shortestPathLength >= 0) {
		Ranking.parTimeStateSegnum = -1;
		objectiveSegnum = getObjectiveSegnum(nearestObjective);
		// Regenerate the path since we may have checked something else in the meantime.
		player_path_length = create_path_partime(start_seg, objectiveSegnum, path_start, path_count, state, nearestObjective);
		*path_length = calculate_path_length_partime(state, *path_start, *path_count, nearestObjective, 1);
		// Let's record that we travelled to this segment from this direction.
		// Algo will only count each segment-direction combination once to avoid inflating par times when it likely does huge unoptimal backtracking, due to the nature of the "nearest neighbor" approach.
		// Levels that loop back on themselves are susceptible to having par times that are too low, but this is rare and usually non-fatal to rank possibility.
		for (i = 1; i < player_path_length - 1; i++)
			state->segmentVisited[Point_segs[i].segnum] = 1;
		// Now we need to find out where to place Algo for accessible objectives.
		// In the case of phasing through locked walls to get certain objectives, set it before the first transparent one. In the case of going into places that are too small, set it before that.
		int wall_num;
		int side_num;
		for (i = 0; i < player_path_length - 1; i++) {
			side_num = find_connecting_side(Point_segs[i].segnum, Point_segs[i + 1].segnum);
			wall_num = Segments[Point_segs[i].segnum].sides[side_num].wall_num;
			for (int w = 0; w < Ranking.numCurrentlyLockedWalls; w++) {
				if (Ranking.currentlyLockedWalls[w] == wall_num)
					if ((nearestObjective.type == OBJECTIVE_TYPE_WALL || !state->isSegmentAccessible[objectiveSegnum]) && check_transparency(&Segments[Walls[wall_num].segnum], Walls[wall_num].sidenum))
						if (Ranking.parTimeStateSegnum == -1)
							Ranking.parTimeStateSegnum = Walls[wall_num].segnum;
			}
			if (Ranking.parTimeSideSizes[Point_segs[i].segnum][side_num] < ConsoleObject->size * 2 && (nearestObjective.type == OBJECTIVE_TYPE_WALL || !state->isSegmentAccessible[objectiveSegnum]))
				if (Ranking.parTimeStateSegnum == -1)
					Ranking.parTimeStateSegnum = Point_segs[i].segnum;
			if (Ranking.parTimeStateSegnum > -1)
				break; // We found where to put Algo. No need to go further.
		}
		if (Ranking.parTimeStateSegnum == -1) {
			state->segnum = objectiveSegnum;
			state->lastPosition = getObjectivePosition(nearestObjective);
			return nearestObjective;
		}
		else {
			state->segnum = Ranking.parTimeStateSegnum;
			vms_vector segmentCenter;
			compute_segment_center(&segmentCenter, &Segments[state->segnum]);
			state->lastPosition = segmentCenter;
			return nearestObjective;
		}
	}
	else {
		// No reachable objectives in list.
		partime_objective emptyResult = { OBJECTIVE_TYPE_INVALID, 0 };
		return emptyResult;
	}
}

int getMatcenSegnum(int matcen_num)
{
	for (int i = 0; i < Highest_segment_index; i++)
		if (Segments[i].matcen_num == matcen_num)
			return i;
}

void check_for_walls_and_matcens_partime(partime_calc_state* state, point_seg* path, int path_count)
{ 
	int wall_num;
	int side_num;
	int adjacent_wall_num;
	// Find out if we have to fight a blastable wall on the way.
	int thisWallDestroyed = 0;
	for (int i = 0; i < path_count - 1; i++) {
		wall_num = Segments[path[i].segnum].sides[find_connecting_side(path[i].segnum, path[i + 1].segnum)].wall_num;
		adjacent_wall_num = Segments[path[i + 1].segnum].sides[find_connecting_side(path[i + 1].segnum, path[i].segnum)].wall_num;
		if (wall_num > -1) {
			if (Walls[wall_num].type == WALL_BLASTABLE) {
				for (int n = 0; n < state->doneWallsSize; n++) {
					if (state->doneWalls[n] == wall_num) {
						thisWallDestroyed = 1;
						break; // If algo's already destroyed this wall, don't destroy it again.
					}
				}
				if (!thisWallDestroyed) {
					// For algorithmic simplicity, wall fight time is counted toward movement time, not combat time. Sorry not sorry.
					state->doneWalls[state->doneWallsSize] = wall_num;
					state->doneWallsSize++;
					state->doneWalls[state->doneWallsSize] = adjacent_wall_num; // Mark the other side of the wall as done too, before algo gets to it. Only the hps of the side we're coming from applies, and it destroys both sides.
					state->doneWallsSize++;
				}
			}
		}
	}
	// How much time and energy does it take to handle the matcens along the way? Let's find out!
	if (Num_robot_centers > 0) { // Don't bother constantly scanning the path for matcens on levels with no matcens.
		for (int i = 0; i < path_count - 1; i++) { // Repeat this loop for every pair of segments on the path. I'm gonna comment every step, because either this whole process is confusing, or I'm just having a brain fog night.
			side_num = find_connecting_side(path[i].segnum, path[i + 1].segnum); // Find the side both segments share.
			wall_num = Segments[path[i].segnum].sides[side_num].wall_num; // Get its wall number.
			if (wall_num > -1) { // If that wall number is valid...
				if (Walls[wall_num].trigger > -1) { // If this wall has a trigger...
					if (Triggers[Walls[wall_num].trigger].flags & TRIGGER_MATCEN) { // If this trigger is a matcen type...
						double matcenTime = 0;
						double totalMatcenTime = 0;
						for (int c = 0; c < Triggers[Walls[wall_num].trigger].num_links; c++) { // Repeat this loop for every segment linked to this trigger.
							if (Segments[Triggers[Walls[wall_num].trigger].seg[c]].special == SEGMENT_IS_ROBOTMAKER) { // Check them to see if they're matcens. 
								segment* segp = &Segments[Triggers[Walls[wall_num].trigger].seg[c]]; // Whenever one is, set this variable as a shortcut so we don't have to put that long string of text every time.
								if (RobotCenters[segp->matcen_num].robot_flags[0] != 0 && state->matcenLives[segp->matcen_num] > 0) { // If the matcen has robots in it, and isn't dead or on cooldown, consider it triggered...
									uint	flags;
									sbyte	legal_types[32];		//	32 bits in a word, the width of robot_flags.
									int	num_types, robot_index;
									robot_index = 0;
									num_types = 0;
									flags = RobotCenters[segp->matcen_num].robot_flags[0];
									while (flags) {
										if (flags & 1)
											legal_types[num_types++] = robot_index;
										flags >>= 1;
										robot_index++;
									}
									// Find the average fight time for the robots in this matcen and multiply that by the spawn count on this difficulty.
									int n;
									double totalRobotTime = 0;
									double totalAmmoUsage = 0;
									double averageRobotTime = 0;
									for (n = 0; n < num_types; n++) {
										robot_info* robInfo = &Robot_info[legal_types[n]];
										if (legal_types[n] != 10) { // Don't consider matcen gophers. They run.
											totalRobotTime += calculate_combat_time(state, NULL, robInfo, 0, 1);
											if (robInfo->contains_type == OBJ_ROBOT) {
												totalRobotTime += calculate_combat_time(state, NULL, &Robot_info[robInfo->contains_id], 0, 1) * round((robInfo->contains_count * (robInfo->contains_prob / 16)));
												robotHasPowerup(state, robInfo->contains_id, (double)(1 / num_types));
											}
											else
												robotHasPowerup(state, legal_types[n], (double)(1 / num_types));
										}
										totalAmmoUsage += state->ammo_usage;
									}
									averageRobotTime = totalRobotTime / num_types;
									matcenTime += averageRobotTime * (Difficulty_level + 3);
									state->matcenLives[segp->matcen_num]--;
									state->vulcanAmmo -= ((totalAmmoUsage / num_types) * (f1_0 * (Difficulty_level + 3))); // and ammo, as those also change per matcen.
									if (matcenTime > 0)
										printf("Fought matcen %i at segment %i; lives left: %i\n", segp->matcen_num, getMatcenSegnum(segp->matcen_num), state->matcenLives[segp->matcen_num]);
									totalMatcenTime += averageRobotTime; // Add up the average fight times of each link so we can add them to the minimum time later.
								}
							}
						}
						// There's a minimum time for all matcen robots spawned on this path to be killed.
						if (matcenTime > 0 && matcenTime < 3.5 * (Difficulty_level + 2) + totalMatcenTime) {
							matcenTime = 3.5 * (Difficulty_level + 2) + totalMatcenTime;
							printf("Total fight time: %.3fs\n", matcenTime);
							state->combatTime += matcenTime;
							state->matcenTime += matcenTime;
						}
					}
				}
			}
		}
	}
}

void respond_to_objective_partime(partime_calc_state* state, partime_objective objective)
{
	int weapon_id;
	int i;
	if (objective.type == OBJECTIVE_TYPE_OBJECT) { // We don't fight triggers.
		object* obj = &Objects[objective.ID];
		if (obj->type == OBJ_POWERUP) {
			weapon_id = 0;
			if (obj->id == POW_VULCAN_WEAPON)
				weapon_id = VULCAN_ID;
			if (obj->id == POW_SPREADFIRE_WEAPON)
				weapon_id = SPREADFIRE_ID;
			if (obj->id == POW_PLASMA_WEAPON)
				weapon_id = PLASMA_ID;
			if (obj->id == POW_FUSION_WEAPON)
				weapon_id = FUSION_ID;
			if (weapon_id) { // If the powerup we got is a weapon, sets Algo's chance of not having it to 0.
				if (weapon_id == VULCAN_ID)
					state->vulcanAmmo += STARTING_VULCAN_AMMO / 2;
				state->heldWeapons[weapon_id] = 0;
			}
			else {
				if (obj->id == POW_LASER) {
					if (state->laser_level < LASER_ID_L4)
						state->laser_level++;
					state->heldWeapons[state->laser_level] = 0;
				}
				if (obj->id == POW_QUAD_FIRE)
					state->hasQuads = 0;
			}
		}
		if (obj->type == OBJ_ROBOT || obj->type == OBJ_CNTRLCEN) { // We don't fight keys, hostages or weapons.
			robot_info* robInfo = &Robot_info[obj->id];
			double combatTime = 0;
			double fightTime;
			if (obj->type == OBJ_CNTRLCEN)
				combatTime += calculate_combat_time(state, obj, robInfo, 1, 0);
			else {
				combatTime += calculate_combat_time(state, obj, robInfo, 1, 0);
				if (obj->contains_type == OBJ_ROBOT && obj->contains_count) {
					robInfo = &Robot_info[obj->contains_id];
					fightTime = calculate_combat_time(state, obj, robInfo, 0, 0) * obj->contains_count;
					combatTime += fightTime;
					printf("Took %.3fs to fight %i of robot type %i\n", fightTime, obj->contains_count, obj->contains_id);
				}
				else if (robInfo->contains_type == OBJ_ROBOT) {
					int assumedOffSpringCount = round(((double)robInfo->contains_count * ((double)robInfo->contains_prob / 16)));
					fightTime = calculate_combat_time(state, obj, &Robot_info[robInfo->contains_id], 0, 0) * assumedOffSpringCount;
					combatTime += fightTime;
					printf("Took %.3fs to fight %i of robot type %i\n", fightTime, assumedOffSpringCount, robInfo->contains_id);
				}
			}
			double teleportDistance = 0;
			short Boss_path_length = 0;
			state->combatTime += combatTime;
			if (robInfo->boss_flag > 0) { // Bosses have special abilities that take additional time to counteract. Boss levels are unfair without this.
				int num_teleports = combatTime / 2; // Bosses teleport two seconds after you start damaging them, meaning you can only get two seconds of damage in at a time before they move away.
				for (i = 0; i < Num_boss_teleport_segs; i++) { // Now we measure the distance between every possible pair of points the boss can teleport between.
					for (int n = 0; n < Num_boss_teleport_segs; n++) {
						create_path_points(obj, Boss_teleport_segs[i], Boss_teleport_segs[n], Point_segs_free_ptr, &Boss_path_length, MAX_POINT_SEGS, 0, 0, -1, 0, obj->id, 1); // Assume inaccesibility here so invalid paths don't get super long and drive up teleport time.
						for (int c = 0; c < Boss_path_length - 1; c++)
							teleportDistance += vm_vec_dist(&Point_segs[c].point, &Point_segs[c + 1].point);
					}
				}
				double teleportTime = ((teleportDistance / pow(Num_boss_teleport_segs, 2)) * num_teleports) / SHIP_MOVE_SPEED; // Account for the average teleport distance, not highest.
				state->movementTime += teleportTime;
				printf("Teleport time: %.3fs\n", teleportTime);
			}
			if (obj->contains_type == OBJ_POWERUP) {
				weapon_id = 0;
				if (obj->contains_id == POW_VULCAN_WEAPON)
					weapon_id = VULCAN_ID;
				if (obj->contains_id == POW_SPREADFIRE_WEAPON)
					weapon_id = SPREADFIRE_ID;
				if (obj->contains_id == POW_PLASMA_WEAPON)
					weapon_id = PLASMA_ID;
				if (obj->contains_id == POW_FUSION_WEAPON)
					weapon_id = FUSION_ID;
				if (weapon_id) { // If the powerup we got is a weapon, sets Algo's chance of not having it to 0.
					if (weapon_id == VULCAN_ID)
						state->vulcanAmmo += STARTING_VULCAN_AMMO / 2;
					state->heldWeapons[weapon_id] = 0;
				}
				else {
					if (obj->contains_id == POW_LASER) {
						for (i = 0; i < obj->contains_count; i++) {
							if (state->laser_level < LASER_ID_L4)
								state->laser_level++;
							state->heldWeapons[state->laser_level] = 0;
						}
					}
					if (obj->contains_id == POW_QUAD_FIRE)
						state->hasQuads = 0;
					if (obj->contains_id == POW_VULCAN_AMMO)
						state->vulcanAmmo += (STARTING_VULCAN_AMMO / 2) * obj->contains_count;
				}
			}
			else
				robotHasPowerup(state, obj->id, 1);
		}
	}
}

int getParTimeWeaponID(int index)
{
	int weaponIDs[5] = { 0, VULCAN_ID, SPREADFIRE_ID, PLASMA_ID, FUSION_ID };
		weaponIDs[0] = Players[Player_num].laser_level;
	return weaponIDs[index];
}

int determineSegmentAccessibility(partime_calc_state* state, int segnum)
{
	object* objp = ConsoleObject;
	short player_path_length = 0;
	create_path_points(objp, objp->segnum, segnum, Point_segs_free_ptr, &player_path_length, MAX_POINT_SEGS, 0, 0, -1, -1, 0, 0);
	if (Point_segs[player_path_length - 1].segnum != segnum) // The segment is inaccessible if Algo doesn't end up at it when given the rules established by initLockedWalls.
		return 0;
	return 1;
}

void calculateParTime() // Here is where we have an algorithm run a simulated path through a level to determine how long the player should take, both flying around and fighting robots.
{ // January 2024 me would crap himself if he saw this actually working lol.
	partime_calc_state state = { 0 }; // Initialize the algorithm's state. We'll call it Algo for short.
	fix64 start_timer_value, end_timer_value; // For tracking how long this algorithm takes to run.
	state.movementTime = 0; // Variable to track how much distance it's travelled.
	state.combatTime = 0; // Variable to track how much fighting it's done.
	// Now clear its checklists.
	state.toDoListSize = 0;
	state.doneListSize = 0;
	state.blackListSize = 0;
	state.segnum = ConsoleObject->segnum; // Start Algo off where the player spawns.
	state.lastPosition = ConsoleObject->pos; // Both in segnum and in coordinates. (Shoutout to Maximum level 17's quads being at spawn for letting me catch this.)
	int lastSegnum = ConsoleObject->segnum; // So the printf showing paths to and from segments works.
	int i;
	int j;
	Ranking.parTimeLoops = 0; // How many times the pathmaking process has repeated. This determines what toDoList is populated with, to make sure things are gone to in the right order.
	double pathLength; // Store create_path_partime's result in pathLength to compare to current shortest.
	double matcenTime = 0; // Debug variable to see how much time matcens are adding to the par time.
	point_seg* path_start; // The current path we are looking at (this is a pointer into somewhere in Point_segs).
	int path_count; // The number of segments in the path we're looking at.
	state.doneWallsSize = 0;
	// The values for each held weapon go by weapon ID, and are set based on how likely it is Algo doesn't have the weapon. It's only allowed to use a certain ID's weapon if its index is 0.
	// They start at 1, getting influenced by robot drop chances (automatically set to 0 if a weapon is picked up directly). Once it reaches a certain decimal value (1/16 chance), it automatically sets to 0.
	state.heldWeapons[0] = 0;
	for (i = 1; i < 21; i++)
		state.heldWeapons[i] = 1;
	// Quads work the same.
	state.hasQuads = 1;
	state.laser_level = 0;
	state.vulcanAmmo = 0;
	state.matcenTime = 0;
	
	// Calculate start time.
	timer_update();
	start_timer_value = timer_query();
	
	// Populate the locked walls list.
	initLockedWalls(&state, 1);
		
	for (i = 0; i <= Highest_segment_index; i++) { // Iterate through every side of every segment, measuring their sizes.
		for (int s = 0; s < 6; s++) {
			if (Segments[i].children[s] > -1) { // Don't measure closed sides. We can't go through them anyway.
				// Measure the distance between all of that side's verts to determine whether we can fit. ai_door_is_openable will use them later to disallow passage if we can't.
				int a = vm_vec_dist(&Vertices[Segments[i].verts[Side_to_verts[s][0]]], &Vertices[Segments[i].verts[Side_to_verts[s][1]]]);
				int b = vm_vec_dist(&Vertices[Segments[i].verts[Side_to_verts[s][1]]], &Vertices[Segments[i].verts[Side_to_verts[s][2]]]);
				int c = vm_vec_dist(&Vertices[Segments[i].verts[Side_to_verts[s][2]]], &Vertices[Segments[i].verts[Side_to_verts[s][3]]]);
				int d = vm_vec_dist(&Vertices[Segments[i].verts[Side_to_verts[s][3]]], &Vertices[Segments[i].verts[Side_to_verts[s][0]]]);
				int min_x = max(a, c);
				int min_y = max(b, d);
				Ranking.parTimeSideSizes[i][s] = min(min_x, min_y); // Thanks to ProxyOne for helping with the calculations.
			}
			else
				Ranking.parTimeSideSizes[i][s] = ConsoleObject->size * 2; // If a side is closed, mark it down as big enough.
		}
		state.segmentVisited[i] = 0;
	}
	for (i = 0; i <= Highest_segment_index; i++) // Lay out the map for where the "inaccessible" territory is so we can mark objectives within it as such.
		state.isSegmentAccessible[i] = determineSegmentAccessibility(&state, i);
		
	initLockedWalls(&state, 0);
	state.doneListSize = 0; // So locked walls can be removed correctly.
		
	// Initialize all matcens to 3 lives.
	for (i = 0; i < Num_robot_centers; i++)
		state.matcenLives[i] = 3;
		
	while (Ranking.parTimeLoops < 4) {
		// Collect our objectives at this stage...
		if (Ranking.parTimeLoops == 0) {
			for (i = 0; i <= Highest_object_index; i++) { // Populate the to-do list with all robots, hostages, weapons, and laser powerups. Ignore robots not worth over zero, as the player isn't gonna go for those. This should never happen, but it's just a failsafe.
				if ((Objects[i].type == OBJ_ROBOT && Robot_info[Objects[i].id].score_value > 0 && !Robot_info[Objects[i].id].boss_flag) || Objects[i].type == OBJ_HOSTAGE || (Objects[i].type == OBJ_POWERUP && (Objects[i].id == POW_EXTRA_LIFE || Objects[i].id == POW_LASER || Objects[i].id == POW_QUAD_FIRE || Objects[i].id == POW_VULCAN_WEAPON || Objects[i].id == POW_SPREADFIRE_WEAPON || Objects[i].id == POW_PLASMA_WEAPON || Objects[i].id == POW_FUSION_WEAPON))) {
					partime_objective objective = { OBJECTIVE_TYPE_OBJECT, i };
					addObjectiveToList(state.toDoList, &state.toDoListSize, objective, 0);
				}
			}
			for (i = 0; i < state.toDoListSize;) { // Now we go through and blacklist anything behind a reactor wall.
				partime_objective objective = { state.toDoList[i].type , state.toDoList[i].ID }; // Save a snapshot of what this index currently is, so the list shifting doesn't cause the wrong thing to be added.
				create_path_partime(ConsoleObject->segnum, getObjectiveSegnum(objective), &path_start, &path_count, &state, objective);
				int lockedWallID = find_reactor_wall_partime(&state, path_start, path_count);
				if (lockedWallID > -1) {
					removeObjectiveFromList(state.toDoList, &state.toDoListSize, objective);
					addObjectiveToList(state.blackList, &state.blackListSize, objective, 0);
				}
				else
					i++;
			}
		}
		if (Ranking.parTimeLoops == 1) {
			int levelHasReactor = 0;
			for (i = 0; i <= Highest_object_index; i++) { // Populate the to-do list with all reactors and bosses.
				if (Objects[i].type == OBJ_CNTRLCEN) {
					partime_objective objective = { OBJECTIVE_TYPE_OBJECT, i };
					addObjectiveToList(state.toDoList, &state.toDoListSize, objective, 0);
					levelHasReactor = 1;
				}
			}
			if (!levelHasReactor) {
				int bossScore;
				int highestBossScore = 0;
				int targetedBossID = -1;
				for (i = 0; i <= Highest_object_index; i++) {
					if (Objects[i].type == OBJ_ROBOT && Robot_info[Objects[i].id].boss_flag) { // Look at every boss, adding only the highest point value.
						// Killing any boss in levels with multiple kills ALL of them, only giving points for the one directly killed, so the player needs to target the highest-scoring one for the best rank. Give them the time needed for that one.
						// This means players may have to replay levels a bunch to find which boss gives most out of custom bosses/values, but I don't thinks that's a cause for great concern.
						bossScore = Robot_info[Objects[i].id].score_value;
						if (Objects[i].contains_type == OBJ_ROBOT && ((Objects[i].id != Robot_info[Objects[i].id].contains_id) || (Objects[i].id != Robot_info[Objects[i].contains_id].contains_id))) // So points in infinite robot drop loops aren't counted past the parent bot.
							bossScore += Robot_info[Objects[i].contains_id].score_value * Objects[i].contains_count;
						if (Objects[i].contains_type == OBJ_POWERUP && Objects[i].contains_id == POW_EXTRA_LIFE)
							bossScore += Objects[i].contains_count * 10000;
						state.combatTime += 6.2; // Each boss has its own deathroll that lasts six seconds one at a time.
						if (bossScore > highestBossScore) {
							highestBossScore = bossScore;
							targetedBossID = i;
						}
					}
				}
				if (targetedBossID > -1) { // If a level doesn't have a reactor OR boss, don't add the non-existent boss to the to-do list.
					partime_objective objective = { OBJECTIVE_TYPE_OBJECT, targetedBossID };
					addObjectiveToList(state.toDoList, &state.toDoListSize, objective, 0);
					// Let's hope no one makes a boss be worth negative points.
				}
			}
		}
		if (Ranking.parTimeLoops == 2) {
			int blackListSize = state.blackListSize; // We need a version that stays at the initial value, since state.blackListSize is actively decreased by the code below.
			for (i = 0; i < blackListSize; i++) { // Put the stuff we blacklisted earlier back on the list now that the reactor/boss is dead.
				partime_objective objective = { state.blackList[0].type, state.blackList[0].ID };
				removeObjectiveFromList(state.blackList, &state.blackListSize, objective);
				addObjectiveToList(state.toDoList, &state.toDoListSize, objective, 0);
			}
		}
		if (Ranking.parTimeLoops == 3) // Put the exit on the list.
			for (i = 0; i <= Num_triggers; i++)
				if (Triggers[i].flags == TRIGGER_EXIT || Triggers[i].flags == TRIGGER_SECRET_EXIT)
					for (j = 0; j <= Num_walls; j++)
						if (Walls[j].trigger == i) {
							partime_objective objective = { OBJECTIVE_TYPE_TRIGGER, j };
							addObjectiveToList(state.toDoList, &state.toDoListSize, objective, 0);
							i = Num_triggers + 1; // Only add one exit.
						}
			
		while (state.toDoListSize > 0) {
			// Find which object on the to-do list is the closest, ignoring the reactor/boss if it's not the only thing left.
			partime_objective nearestObjective =
				find_nearest_objective_partime(&state, state.segnum, state.toDoList, state.toDoListSize, &path_start, &path_count, &pathLength);
			
			if (nearestObjective.type == OBJECTIVE_TYPE_INVALID) {
				// This should only happen if there are no reachable objectives left in the list.
				// If that happens, we're done with this phase.
				break;
			}
			
			// Mark this objective as done.
			removeObjectiveFromList(state.toDoList, &state.toDoListSize, nearestObjective);
			addObjectiveToList(state.doneList, &state.doneListSize, nearestObjective, 1);
			if (Ranking.parTimeLoops == 1) // If we just added the reactor to the done list, remove any locked walls neighboring a reactor wall. They should be open too.
				for (i = 0; i < state.numReactorWalls; i++) {
					int adjacent_wall_num = findConnectedWallNum(state.reactorWalls[i].wallID);
					for (int w = 0; w < Ranking.numCurrentlyLockedWalls; w++)
						if (Ranking.currentlyLockedWalls[w] == adjacent_wall_num)
							removeLockedWallFromList(w);
				}
			
			// Track resource consumption and robot HP destroyed.
			// If there's no path and we're doing straight line distance, we have no idea what we'd
			// be crossing through, so tracking resources for the path would be meaningless.
			// We can still check the objective itself, though.
			int hasThisObjective = 0; // If the next object is a weapon/laser level/quads, and algo already has it/is maxed out, skip it. We don't wanna waste time getting redundant powerups.
			if (nearestObjective.type == OBJECTIVE_TYPE_OBJECT && Objects[nearestObjective.ID].type == OBJ_POWERUP) { // I'm splitting up the if conditions this time.
				if (!state.isSegmentAccessible[Objects[nearestObjective.ID].segnum]) // Don't go to a powerup that's inaccessible. We have to touch powerups directly to collect them.
					hasThisObjective = 1;
				int weaponIDs[5] = { 0, VULCAN_ID, SPREADFIRE_ID, PLASMA_ID, FUSION_ID };
				for (int n = 1; n < 5; n++)
					if (Objects[nearestObjective.ID].id == n + 12 && !state.heldWeapons[weaponIDs[n]])
						hasThisObjective = 1;
				if (Objects[nearestObjective.ID].id == POW_LASER && !state.heldWeapons[LASER_ID_L4])
					hasThisObjective = 1;
				if (Objects[nearestObjective.ID].id == POW_QUAD_FIRE && !state.hasQuads)
					hasThisObjective = 1;
				if ((Objects[nearestObjective.ID].id == POW_VULCAN_WEAPON && !state.heldWeapons[VULCAN_ID]) && state.vulcanAmmo == STARTING_VULCAN_AMMO * 4)
					hasThisObjective = 1;
			}
			if (!hasThisObjective) {
				int nearestObjectiveSegnum = getObjectiveSegnum(nearestObjective);
				printf("Path from segment %i to %i: %.3fs\n", lastSegnum, nearestObjectiveSegnum, pathLength / SHIP_MOVE_SPEED);
				respond_to_objective_partime(&state, nearestObjective);
				if (path_start != NULL)
					check_for_walls_and_matcens_partime(&state, path_start, path_count);
				// Cap algo's energy and ammo like the player's.
				if (state.vulcanAmmo > STARTING_VULCAN_AMMO * 4)
					state.vulcanAmmo = STARTING_VULCAN_AMMO * 4;
				// Now move ourselves to the objective for the next pathfinding iteration, unless the objective wasn't reachable with just flight, in which case move ourselves as far as we COULD fly.
				state.movementTime += ((pathLength - state.shortestPathObstructionTime) / SHIP_MOVE_SPEED);
				lastSegnum = state.segnum;
			}
			else
				state.segnum = lastSegnum; // find_nearest_objective_partime just tried to set Algo's segnum to something, but it shouldn't be in this case, so force it back.
		}
		Ranking.parTimeLoops++;
	}
		
	// Calculate end time.
	timer_update();
	end_timer_value = timer_query();
	printf("Par time: %.3fs (%.3f movement, %.3f combat) Matcen time: %.3fs\nCalculation time: %.3fs\n",
		state.movementTime + state.combatTime,
		state.movementTime,
		state.combatTime,
		state.matcenTime,
		f2fl(end_timer_value - start_timer_value));
	
	// Par time is rounded up to the nearest five seconds so it looks better / legible on the result screen, leaves room for the time bonus, and looks like a human set it.
		Ranking.parTime = 5 * (1 + (int)(state.movementTime + state.combatTime) / 5);
	// Also because Doom did five second increments.
	// Par time will vary based on difficulty, so the player will always have to go fast for a high time bonus, even on lower difficulties.
}

// - - - - - - - - - -  END OF PAR TIME ALGORITHM STUFF  - - - - - - - - - - \\

//called when the player is starting a new level for normal game model
void StartNewLevel(int level_num)
{
	hide_menus();

	GameTime64 = 0;
	ThisLevelTime=0;
	
	Ranking.deathCount = 0;
	Ranking.excludePoints = 0;
	Ranking.rankScore = 0;
	Ranking.missedRngSpawn = 0;
	Ranking.quickload = 0;
	Ranking.level_time = 0; // Set this to 0 despite it going unused until set to time_level, so we can save a variable when telling the in-game timer which time variable to display.
	Ranking.fromBestRanksButton = 0; // So the result screen knows it's not just viewing record details.
	Ranking.noDamage = 1;
	if (RestartLevel.updateRestartStuff) {
		RestartLevel.primary_weapon = Players[Player_num].primary_weapon;
		RestartLevel.secondary_weapon = Players[Player_num].secondary_weapon;
		RestartLevel.flags = Players[Player_num].flags;
		RestartLevel.energy = Players[Player_num].energy;
		RestartLevel.shields = Players[Player_num].shields;
		RestartLevel.lives = Players[Player_num].lives;
		RestartLevel.laser_level = Players[Player_num].laser_level;
		RestartLevel.primary_weapon_flags = Players[Player_num].primary_weapon_flags;
		RestartLevel.secondary_weapon_flags = Players[Player_num].secondary_weapon_flags;
		for (int i = 0; i < MAX_PRIMARY_WEAPONS; i++)
			RestartLevel.primary_ammo[i] = Players[Player_num].primary_ammo[i];
		for (int i = 0; i < MAX_SECONDARY_WEAPONS; i++)
			RestartLevel.secondary_ammo[i] = Players[Player_num].secondary_ammo[i];
	}
	
	StartNewLevelSub(level_num, 1, 0);

	RestartLevel.isResults = 0;
	int i, isRankable = 0; // We need to check if this level is beatable, since some secret levels in D2 are meant to be part of a base one.
	int highestBossScore = 0;
	int bossScore;
	Ranking.maxScore = 0;
	for (i = 0; i <= Highest_object_index; i++) {
		if (Objects[i].type == OBJ_ROBOT && Robot_info[Objects[i].id].boss_flag) { // Look at every boss, adding only the highest point value.
			bossScore = Robot_info[Objects[i].id].score_value;
			if (Objects[i].contains_type == OBJ_ROBOT && ((Objects[i].id != Robot_info[Objects[i].id].contains_id) || (Objects[i].id != Robot_info[Objects[i].contains_id].contains_id))) // So points in infinite robot drop loops aren't counted past the parent bot.
				bossScore += Robot_info[Objects[i].contains_id].score_value * Objects[i].contains_count;
			if (Objects[i].contains_type == OBJ_POWERUP && Objects[i].contains_id == POW_EXTRA_LIFE)
				bossScore += Objects[i].contains_count * 10000;
			if (bossScore > highestBossScore) {
				highestBossScore = bossScore;
				Ranking.maxScore = Robot_info[Objects[i].id].score_value;
			}
		}
	}
	for (i = 0; i <= Highest_object_index; i++) {
		if (Objects[i].type == OBJ_ROBOT) {
			if (!Robot_info[Objects[i].id].boss_flag)
				Ranking.maxScore += Robot_info[Objects[i].id].score_value;
			if (Objects[i].contains_type == OBJ_ROBOT && ((Objects[i].id != Robot_info[Objects[i].id].contains_id) || (Objects[i].id != Robot_info[Objects[i].contains_id].contains_id))) // So points in infinite robot drop loops aren't counted past the parent bot.
				Ranking.maxScore += Robot_info[Objects[i].contains_id].score_value * Objects[i].contains_count;
			if (Objects[i].contains_type == OBJ_POWERUP && Objects[i].contains_id == POW_EXTRA_LIFE)
				Ranking.maxScore += Objects[i].contains_count * 10000;
		}
		else if (Robot_info[Objects[i].id].boss_flag)
			isRankable = 1; // A boss is present, this level is beatable.
		if (Objects[i].type == OBJ_CNTRLCEN) {
			Ranking.maxScore += CONTROL_CEN_SCORE;
			isRankable = 1; // A reactor is present, this level is beatable.
		}
		if (Objects[i].type == OBJ_HOSTAGE)
			Ranking.maxScore += HOSTAGE_SCORE;
		if (Objects[i].type == OBJ_POWERUP && Objects[i].id == POW_EXTRA_LIFE)
			Ranking.maxScore += 10000;
	}
	Ranking.maxScore = (int)(Ranking.maxScore * 3); // This is not the final max score. Max score is still 7500 higher per hostage, but that's added last second so the time bonus can be weighted properly.
	for (i = 0; i <= Num_triggers; i++) {
		if (Triggers[i].flags & TRIGGER_EXIT || Triggers[i].flags & TRIGGER_SECRET_EXIT)
			isRankable = 1; // An exit is present, this level is beatable. Technically the level could still be unbeatable because the exit could be behind unreachable, but who would put an exit there?
	}
	if (!RestartLevel.restarts) // Don't calculate par time if we're restarting. We already have that information and it's not changing. This will reduce restart load times slightly.
		calculateParTime();
	if (!isRankable) { // If this level is not beatable, mark the level as beaten with zero points and an X-rank, so the mission can have an aggregate rank.
		PHYSFS_File* temp;
		char filename[256];
		char temp_filename[256];
		if (Current_level_num > 0)
			sprintf(filename, "ranks/%s/%s/level%i.hi", Players[Player_num].callsign, Current_mission->filename, Current_level_num);
		else
			sprintf(filename, "ranks/%s/%s/levelS%i.hi", Players[Player_num].callsign, Current_mission->filename, Current_level_num * -1);
		sprintf(temp_filename, "ranks/%s/%s/temp.hi", Players[Player_num].callsign, Current_mission->filename);
		time_t timeOfScore = time(NULL);
		temp = PHYSFS_openWrite(temp_filename);
		PHYSFSX_printf(temp, "0\n");
		PHYSFSX_printf(temp, "0\n");
		PHYSFSX_printf(temp, "0\n");
		PHYSFSX_printf(temp, "0\n");
		PHYSFSX_printf(temp, "0\n");
		PHYSFSX_printf(temp, "0\n");
		PHYSFSX_printf(temp, "%i\n", Difficulty_level);
		PHYSFSX_printf(temp, "-1\n");
		PHYSFSX_printf(temp, "0\n");
		PHYSFSX_printf(temp, "%s\n", Current_level_name);
		PHYSFSX_printf(temp, "%s\n", ctime(&timeOfScore));
		PHYSFSX_printf(temp, "0");
		PHYSFS_close(temp);
		PHYSFS_delete(filename);
		PHYSFSX_rename(temp_filename, filename);
	}
	else {
		char message[256];
		if (RestartLevel.restarts) {
			if (RestartLevel.restarts == 1)
				sprintf(message, "1 restart and counting...");
			else
				sprintf(message, "%i restarts and counting...", RestartLevel.restarts);
			powerup_basic(-10, -10, -10, 0, message);
		}
	}
	Ranking.alreadyBeaten = 0;
	if (level_num > 0) {
		if (calculateRank(level_num, 0) > 0)
			Ranking.alreadyBeaten = 1;
	}
	else {
		if (calculateRank(Current_mission->last_level - level_num, 0) > 0)
			Ranking.alreadyBeaten = 1;
	}
}

int previewed_spawn_point = 0; 

//initialize the player object position & orientation (at start of game, or new ship)
void InitPlayerPosition(int random)
{
	int NewPlayer=0;

	if (is_observer())
		NewPlayer = 0;
	else if (! ((Game_mode & GM_MULTI) && !(Game_mode&GM_MULTI_COOP)) ) // If not deathmatch
		NewPlayer = Player_num;
#ifdef NETWORK	
	else if ((Game_mode & GM_MULTI) && (Netgame.SpawnStyle == SPAWN_STYLE_PREVIEW)  && Dead_player_camera != NULL)
		NewPlayer = previewed_spawn_point; 
#endif
	else if (random == 1)
	{
		int i, trys=0;
		fix closest_dist = 0x7ffffff, dist;

		timer_update();
		d_srand((fix)timer_query());
		do {
			trys++;
			NewPlayer = d_rand() % NumNetPlayerPositions;

			closest_dist = 0x7fffffff;

			for (i=0; i<N_players; i++ )	{
				if ( (i!=Player_num) && (Objects[Players[i].objnum].type == OBJ_PLAYER) )	{
					dist = find_connected_distance(&Objects[Players[i].objnum].pos, Objects[Players[i].objnum].segnum, &Player_init[NewPlayer].pos, Player_init[NewPlayer].segnum, 15, WID_FLY_FLAG ); // Used to be 5, search up to 15 segments
					if ( (dist < closest_dist) && (dist >= 0) )	{
						closest_dist = dist;
					}
				}
			}
		} while ( (closest_dist<i2f(15*20)) && (trys<MAX_PLAYERS*2) );
	}
	else
	{
		// If deathmatch and not random, positions were already determined by sync packet
		reset_player_object();
		reset_cruise();
		return;
	}

	Assert(NewPlayer >= 0);
	Assert(NewPlayer < NumNetPlayerPositions);
	ConsoleObject->pos = Player_init[NewPlayer].pos;
	ConsoleObject->orient = Player_init[NewPlayer].orient;
#ifdef NETWORK	
	if ((Game_mode & GM_MULTI) && (Netgame.SpawnStyle == SPAWN_STYLE_PREVIEW) && Dead_player_camera != NULL) {
		ConsoleObject->orient = Dead_player_camera->orient;  
		Dead_player_camera = NULL; 
	}

	if (is_observer()) {
		ConsoleObject->pos = Objects[Players[Current_obs_player].objnum].pos;
		ConsoleObject->orient = Objects[Players[Current_obs_player].objnum].orient;
		obj_relink_all();
	} else
#endif	
		obj_relink(ConsoleObject-Objects,Player_init[NewPlayer].segnum);
	reset_player_object();
	reset_cruise();
}

//	-----------------------------------------------------------------------------------------------------
//	Initialize default parameters for one robot, copying from Robot_info to *objp.
//	What about setting size!?  Where does that come from?
void copy_defaults_to_robot(object *objp)
{
	robot_info	*robptr;
	int			objid;

	Assert(objp->type == OBJ_ROBOT);
	objid = objp->id;
	Assert(objid < N_robot_types);

	robptr = &Robot_info[objid];

	objp->shields = robptr->strength;

}

//	-----------------------------------------------------------------------------------------------------
//	Copy all values from the robot info structure to all instances of robots.
//	This allows us to change bitmaps.tbl and have these changes manifested in existing robots.
//	This function should be called at level load time.
void copy_defaults_to_robot_all(void)
{
	int	i;

	for (i=0; i<=Highest_object_index; i++)
		if (Objects[i].type == OBJ_ROBOT)
			copy_defaults_to_robot(&Objects[i]);
}

int	Do_appearance_effect=0;

//	-----------------------------------------------------------------------------------------------------
//called when the player is starting a level (new game or new ship)
void StartLevel(int random)
{
	Assert(!Player_is_dead);

	InitPlayerPosition(random);

	verify_console_object();

	ConsoleObject->control_type	= CT_FLYING;
	ConsoleObject->movement_type	= MT_PHYSICS;

	// create_player_appearance_effect(ConsoleObject);
	Do_appearance_effect = 1;

	if (Game_mode & GM_MULTI)
	{
		if ((Game_mode & GM_MULTI_COOP) || (Game_mode & GM_MULTI_ROBOTS))
			multi_send_score();
	 	multi_send_reappear();
		multi_do_protocol_frame(1, 1);
	}
	else // in Singleplayer, after we died ...
	{
		disable_matcens(); // ... disable matcens and ...
		clear_transient_objects(0); // ... clear all transient objects.
	}

	ai_reset_all_paths();
	ai_init_boss_for_ship();

	reset_rear_view();
	Auto_fire_fusion_cannon_time = 0;
	Fusion_charge = 0;

	if (!(Game_mode & GM_MULTI)) // stuff for Singleplayer only
	{

	}
}


