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
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Routines for EndGame, EndLevel, etc.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !defined(_MSC_VER) && !defined(macintosh)
#include <unistd.h>
#endif
#include <time.h>
#include "inferno.h"
#include "game.h"
#include "player.h"
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
#ifdef NETWORK
#  include "multi.h"
#endif
#include "playsave.h"
#include "ctype.h"
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
#include "gamepal.h"
#include "movie.h"
#include "controls.h"
#include "credits.h"
#include "gamemine.h"
#ifdef EDITOR
#include "editor/editor.h"
#endif

#ifdef OGL
#include "ogl_init.h"
#endif

#include "strutil.h"
#include "rle.h"
#include "byteswap.h"
#include "segment.h"
#include "gameseg.h"
#include "multibot.h"
#include "math.h"
#include "object.h"


void StartNewLevelSecret(int level_num, int page_in_textures);
void InitPlayerPosition(int random_flag);
void DoEndGame(void);
void AdvanceLevel(int secret_flag);
void StartLevel(int random);
void filter_objects_from_level();





//Current_level_num starts at 1 for the first level
//-1,-2,-3 are secret levels
//0 means not a real level loaded
int	Current_level_num=0,Next_level_num;
char	Current_level_name[LEVEL_NAME_LEN];

// Global variables describing the player
int	N_players=1;	// Number of players ( >1 means a net game, eh?)
int 	Player_num=0;	// The player number who is on the console.
player	Players[MAX_PLAYERS+4];	// Misc player info
ranking Ranking; // Ranking system mod variables.
extern restartLevel RestartLevel;
obj_position	Player_init[MAX_PLAYERS];

// Global variables telling what sort of game we have
int NumNetPlayerPositions = -1;

extern fix ThisLevelTime;

// Extern from game.c to fix a bug in the cockpit!

extern int Last_level_path_created;

//	HUD_clear_messages external, declared in gauges.h
#ifndef _GAUGES_H
extern void HUD_clear_messages(); // From hud.c
#endif

//	Extra prototypes declared for the sake of LINT
void init_player_stats_new_ship(ubyte pnum);
void copy_defaults_to_robot_all(void);

int	Do_appearance_effect=0;

extern int Rear_view;

int	First_secret_visit = 1;

extern int descent_critical_error;

//--------------------------------------------------------------------
void verify_console_object()
{
	Assert( Player_num > -1 );
	Assert( Players[Player_num].objnum > -1 );
	ConsoleObject = &Objects[Players[Player_num].objnum];
	Assert( ConsoleObject->type==OBJ_PLAYER || (is_observer() && Player_num == OBSERVER_PLAYER_ID) );
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

//added 10/12/95: delete buddy bot if coop game.  Probably doesn't really belong here. -MT
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
			if ( (!(Game_mode & GM_MULTI_COOP) && ((Objects[i].type == OBJ_PLAYER)||(Objects[i].type==OBJ_GHOST))) ||
	           ((Game_mode & GM_MULTI_COOP) && ((j == 0) || ( Objects[i].type==OBJ_COOP ))) )
			{
				Objects[i].type=OBJ_PLAYER;
				Player_init[k].pos = Objects[i].pos;
				Player_init[k].orient = Objects[i].orient;
				Player_init[k].segnum = Objects[i].segnum;
				Players[k].objnum = i;
				Objects[i].id = k;
				k++;
			}
			else
				obj_delete(i);
			j++;
		}

		if ((Objects[i].type==OBJ_ROBOT) && (Robot_info[Objects[i].id].companion) && (Game_mode & GM_MULTI))
			obj_delete(i);		//kill the buddy in netgames

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

	if (pnum == Player_num)
		First_secret_visit = 1;

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

extern	ubyte	Last_afterburner_state;

// Setup player for new level (After completion of previous level)
void init_player_stats_level(int secret_flag)
{
	// int	i;

	Players[Player_num].last_score = Players[Player_num].score;

	Players[Player_num].level = Current_level_num;

#ifdef NETWORK
	if (!Network_rejoined) {
#endif
		Players[Player_num].time_level = 0;
		Players[Player_num].hours_level = 0;
#ifdef NETWORK
	}
#endif

	Players[Player_num].killer_objnum = -1;

	Players[Player_num].num_kills_level = 0;
	Players[Player_num].num_robots_level = count_number_of_robots();
	Players[Player_num].num_robots_total += Players[Player_num].num_robots_level;

	Players[Player_num].hostages_level = count_number_of_hostages();
	Players[Player_num].hostages_total += Players[Player_num].hostages_level;
	Players[Player_num].hostages_on_board = 0;

	if (!secret_flag) {
		init_ammo_and_energy();

		Players[Player_num].flags &= (~KEY_BLUE);
		Players[Player_num].flags &= (~KEY_RED);
		Players[Player_num].flags &= (~KEY_GOLD);

		Players[Player_num].flags &=   ~(PLAYER_FLAGS_INVULNERABLE |
													PLAYER_FLAGS_CLOAKED |
													PLAYER_FLAGS_MAP_ALL);

		Players[Player_num].cloak_time = 0;
		Players[Player_num].invulnerable_time = 0;

		if ((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP))
			Players[Player_num].flags |= (KEY_BLUE | KEY_RED | KEY_GOLD);
	}

	Player_is_dead = 0; // Added by RH
	Dead_player_camera = NULL;
	Players[Player_num].homing_object_dist = -F1_0; // Added by RH

	// properly init these cursed globals
	Next_flare_fire_time = Last_laser_fired_time = Next_laser_fire_time = Next_missile_fire_time = GameTime64;

	Controls.afterburner_state = 0;
	Last_afterburner_state = 0;

	digi_kill_sound_linked_to_object(Players[Player_num].objnum);

	init_gauges();

	Missile_viewer = NULL;

	if (Game_mode & GM_MULTI)
		multi_send_ship_status();
}

extern	void init_ai_for_ship(void);

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
		Players[Player_num].primary_weapon = 0;
		Players[Player_num].secondary_weapon = 0;
		for (i=0; i<MAX_PRIMARY_WEAPONS; i++)
			Primary_last_was_super[i] = 0;
		for (i=1; i<MAX_SECONDARY_WEAPONS; i++)
			Secondary_last_was_super[i] = 0;
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

		Global_laser_firing_count=0;
		Players[pnum].afterburner_charge = 0;
		Controls.afterburner_state = 0;
		Last_afterburner_state = 0;
		Missile_viewer=NULL; //reset missile camera if out there
		Missile_viewer_sig=-1;
		init_ai_for_ship();
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
	Players[pnum].flags &= ~( PLAYER_FLAGS_QUAD_LASERS | PLAYER_FLAGS_AFTERBURNER | PLAYER_FLAGS_CLOAKED | PLAYER_FLAGS_INVULNERABLE | PLAYER_FLAGS_MAP_ALL | PLAYER_FLAGS_CONVERTER | PLAYER_FLAGS_AMMO_RACK | PLAYER_FLAGS_HEADLIGHT | PLAYER_FLAGS_HEADLIGHT_ON | PLAYER_FLAGS_FLAG);
	Players[pnum].cloak_time = 0;
	Players[pnum].invulnerable_time = 0;
	Players[pnum].homing_object_dist = -F1_0; // Added by RH
	RespawningConcussions[pnum] = 0; 	
	VulcanAmmoBoxesOnBoard[pnum] = 0; 		
	VulcanBoxAmmo[pnum] = 0;

#ifdef NETWORK
	if(Game_mode & GM_MULTI && Netgame.BornWithBurner && !is_observer()) {
		Players[pnum].flags |= PLAYER_FLAGS_AFTERBURNER;
		if (pnum == Player_num)
			Players[pnum].afterburner_charge = f1_0;
	}
#endif
	digi_kill_sound_linked_to_object(Players[pnum].objnum);

	if (pnum == Player_num && Game_mode & GM_MULTI)
		multi_send_ship_status();
}

extern void init_stuck_objects(void);

#ifdef EDITOR

extern int Slide_segs_computed;

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
	init_controlcen_for_level();
	automap_clear_visited();
	init_stuck_objects();
	init_thief_for_level();

	Slide_segs_computed = 0;
	if (!Game_wind)
		Game_wind = window_create(&grd_curscreen->sc_canvas, 0, 0, SWIDTH, SHEIGHT, game_handler, NULL);
}
#endif

//do whatever needs to be done when a player dies in multiplayer

void DoGameOver()
{
//	nm_messagebox( TXT_GAME_OVER, 1, TXT_OK, "" );

	if (PLAYING_BUILTIN_MISSION)
		scores_maybe_add_player(0);

	if (Game_wind)
		window_close(Game_wind);		// Exit out of game loop
}

//update various information about the player
void update_player_stats()
{
	Players[Player_num].time_level += FrameTime;	//the never-ending march of time...
	if (Current_level_num < 0 && Ranking.freezeTimer == 0)
		Ranking.secretlevel_time += FrameTime;
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

//hack to not start object when loading level
extern int Dont_start_sound_objects;

//go through this level and start any eclip sounds
void set_sound_sources()
{
	int segnum,sidenum;
	segment *seg;

	digi_init_sounds();		//clear old sounds

	Dont_start_sound_objects = 1;

	for (seg=&Segments[0],segnum=0;segnum<=Highest_segment_index;seg++,segnum++)
		for (sidenum=0;sidenum<MAX_SIDES_PER_SEGMENT;sidenum++) {
			int tm,ec,sn;

			if (WALL_IS_DOORWAY(seg,sidenum) & WID_RENDER_FLAG)
				if ((((tm=seg->sides[sidenum].tmap_num2) != 0) && ((ec=TmapInfo[tm&0x3fff].eclip_num)!=-1)) || ((ec=TmapInfo[seg->sides[sidenum].tmap_num].eclip_num)!=-1))
					if ((sn=Effects[ec].sound_num)!=-1) {
						vms_vector pnt;
						int csegnum = seg->children[sidenum];

						//check for sound on other side of wall.  Don't add on
						//both walls if sound travels through wall.  If sound
						//does travel through wall, add sound for lower-numbered
						//segment.

						if (IS_CHILD(csegnum) && csegnum < segnum) {
							if (WALL_IS_DOORWAY(seg, sidenum) & (WID_FLY_FLAG+WID_RENDPAST_FLAG)) {
								segment *csegp;
								int csidenum;

								csegp = &Segments[seg->children[sidenum]];
								csidenum = find_connect_side(seg, csegp);

								if (csegp->sides[csidenum].tmap_num2 == seg->sides[sidenum].tmap_num2)
									continue;		//skip this one
							}
						}

						compute_center_point_on_side(&pnt,seg,sidenum);
						digi_link_sound_to_pos(sn,segnum,sidenum,&pnt,1, F1_0/2);
					}
		}

	Dont_start_sound_objects = 0;

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
		t = INTEL_INT(Segments[i].objects);
		do_checksum_calc((ubyte *)&t, 4, &sum1, &sum2);
	}
	sum2 %= 255;
	return ((sum1<<8)+ sum2);
}

void free_polygon_models();
void load_robot_replacements(char *level_name);
int read_hamfile();
extern int Robot_replacements_loaded;

// load just the hxm file
void load_level_robots(int level_num)
{
	char *level_name;

	Assert(level_num <= Last_level  && level_num >= Last_secret_level  && level_num != 0);

	if (level_num<0)		//secret level
		level_name = Secret_level_names[-level_num-1];
	else					//normal level
		level_name = Level_names[level_num-1];

	if (Robot_replacements_loaded) {
		int load_mission_ham();
		free_polygon_models();
		load_mission_ham();
		Robot_replacements_loaded = 0;
	}
	load_robot_replacements(level_name);
	Robot_replacements_loaded |= multi_change_weapon_info();
}

//load a level off disk. level numbers start at 1.  Secret levels are -1,-2,-3
void LoadLevel(int level_num,int page_in_textures)
{
	char *level_name;
	player save_player;
	int load_ret;

	save_player = Players[Player_num];

	if (Newdemo_state == ND_STATE_PLAYBACK && level_num == 0) {
		// Workaround for Redux bug. We're hoping that the demo was recorded in a one-level
		// mission; otherwise we don't have a good way to guess which one to load.
		level_num = 1;
		nm_messagebox(NULL, 1, TXT_OK, "This demo was recorded in\nan old version of D2X-Redux.\n"
			"Some game information\nmay be missing.");
	}

	Assert(level_num <= Last_level  && level_num >= Last_secret_level  && level_num != 0);

	if (level_num<0)		//secret level
		level_name = Secret_level_names[-level_num-1];
	else					//normal level
		level_name = Level_names[level_num-1];

	gr_set_current_canvas(NULL);
	gr_clear_canvas(BM_XRGB(0, 0, 0));		//so palette switching is less obvious

	load_ret = load_level(level_name);		//actually load the data from disk!

	if (load_ret)
		Error("Couldn't load level file <%s>, error = %d",level_name,load_ret);

	Current_level_num=level_num;

	load_palette(Current_level_palette,1,1);		//don't change screen

	show_boxed_message(TXT_LOADING, 0);
#ifdef RELEASE
	timer_delay(F1_0);
#endif

	load_endlevel_data(level_num);

	if (EMULATING_D1)
		load_d1_bitmap_replacements();
	else
		load_bitmap_replacements(level_name);

	load_level_robots(level_num);

	if ( page_in_textures ) {
		piggy_load_level_data();
#ifdef OGL
		ogl_cache_level_textures();
#endif
	}

#ifdef NETWORK
	my_segments_checksum = netmisc_calc_checksum();

	reset_network_objects();
#endif

	Players[Player_num] = save_player;

	set_sound_sources();

	songs_play_level_song( Current_level_num, 0 );

	gr_palette_load(gr_palette);		//actually load the palette

//	WIN(HideCursorW());
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

extern void init_seismic_disturbances(void);

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

int calculateProjectedRank()
{
	// This function, largely a modified version of calculateRank, is an experimental disabled feature where your end of level score is projected live throughout the level as opposed to your rank score. It can be enabled in gauges.c under hud_show_score.
	// It was gonna show alongside a live rank, but I decided against it for a multitude of reasons, including vague spoilers of levels' contents/requirements for new players, stress of constant point loss, and inconsistency with the remains counter.
	int rankPoints2 = -10;
	double maxScore = Ranking.maxScore - Ranking.num_thief_points * 3;
	maxScore = (int)maxScore;
	double skillPoints = Ranking.rankScore * (Difficulty_level / 4);
	skillPoints = (int)skillPoints;
	double timePoints = (maxScore / 1.5) / pow(2, f2fl(Players[Player_num].time_level) / Ranking.parTime);
	if (f2fl(Players[Player_num].time_level) < Ranking.parTime)
		timePoints = (maxScore / 2.4) * (1 - (f2fl(Players[Player_num].time_level) / Ranking.parTime) * 0.2);
	timePoints = (int)timePoints;
	int hostagePoints = Players[Player_num].hostages_on_board * 2500 * ((Difficulty_level + 8) / 12);
	if (Players[Player_num].hostages_on_board == Players[Player_num].hostages_level)
		hostagePoints *= 3;
	hostagePoints = round(hostagePoints); // Round this because I got 24999 hostage bonus once.
	double score = Ranking.rankScore + skillPoints + timePoints + Ranking.missedRngSpawn + hostagePoints;
	maxScore += Players[Player_num].hostages_level * 7500;
	double deathPoints = maxScore * 0.4 - maxScore * (0.4 / pow(2, Ranking.deathCount));
	deathPoints = (int)deathPoints;
	score -= deathPoints;
	if (rankPoints2 > -5) {
		rankPoints2 = (score / maxScore) * 12;
	}
	if (rankPoints2 > -5 && maxScore == 0)
		rankPoints2 = 12;
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
		if (levelHostages < 0) // If level is unplayed, we know because the first value in the file will be -1, which is impossible for the hostage count.
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
			Ranking.warmStart = atoi(buffer);
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
	timePoints = (int)timePoints;
	hostagePoints = playerHostages * 2500 * ((difficulty + 8) / 12);
	if (playerHostages == levelHostages)
		hostagePoints *= 3;
	hostagePoints = round(hostagePoints); // Round this because I got 24999 hostage bonus once.
	double score = playerPoints + skillPoints + timePoints + missedRngSpawn + hostagePoints;
	maxScore += levelHostages * 7500;
	double deathPoints = maxScore * 0.4 - maxScore * (0.4 / pow(2, deathCount));
	deathPoints = (int)deathPoints;
	score -= deathPoints;
	if (rankPoints2 > -5) {
		rankPoints2 = (score / maxScore) * 12;
	}
	if (rankPoints2 > -5 && maxScore == 0)
		rankPoints2 = 12;
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
	Ranking.cheated = 0;
	Ranking.secret_hostages_on_board = 0; // If we don't set this to zero here, then getting hostages in a secret level, quitting out and starting a new game will cause the next level to never give full rescue bonus.
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
			PHYSFSX_printf(fp, "???");
		}
		PHYSFS_close(fp);
		i++;
	}
	
	state_quick_item = -1;	// for first blind save, pick slot to save in

	Game_mode = GM_NORMAL;

	Next_level_num = 0;

	InitPlayerObject();				//make sure player's object set up

	init_player_stats_game(Player_num);		//clear all stats

	N_players = 1;

	RestartLevel.updateRestartStuff = 1; // So the player doesn't restart with 0 in every field.
	if (start_level < 0)
		StartNewLevelSecret(start_level, 0);
	else
		StartNewLevel(start_level);

	Players[Player_num].starting_level = start_level;		// Mark where they started

	game_disable_cheats();

	init_seismic_disturbances();
}

// Arne's code, except for the image pos/size and fromBestRanksButton stuff.
int endlevel_current_rank;
extern grs_bitmap nm_background1;
grs_bitmap transparent;
int endlevel_handler(newmenu* menu, d_event* event, void* userdata) {
	if (!((Current_level_num > 0 && Ranking.quickload) || (Current_level_num < 0 && Ranking.secretQuickload))) {
		switch (event->type) {
		case EVENT_WINDOW_DRAW:
			gr_palette_load(gr_palette);
			show_fullscr(&nm_background1);
			grs_bitmap* bm = RankBitmaps[endlevel_current_rank];
			int x = grd_curscreen->sc_w * 0.4;
			int y = grd_curscreen->sc_h * 0.725;
			int h = grd_curscreen->sc_h * 0.1125;
			if (!endlevel_current_rank)
				h *= 1.0806; // Make E-rank bigger to compensate for the tilt.
			int w = 3 * h;
			if (!endlevel_current_rank || endlevel_current_rank == 2 || endlevel_current_rank == 5 || endlevel_current_rank == 8 || endlevel_current_rank == 11 || endlevel_current_rank == 13)
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
			if (Ranking.startingLevel > Current_mission->last_level) {
				nm_messagebox(NULL, 1, "Ok", "Can't start on secret level!\nTry saving right before teleporter.");
				return 1;
			}
			else {
				if (!do_difficulty_menu()) {
					return 1;
				}
				StartNewGame(Ranking.startingLevel);
			}
			break;
		}
	}
	return 0;
}

//	-----------------------------------------------------------------------------
//	Does the bonus scoring.
//	Call with dead_flag = 1 if player died, but deserves some portion of bonus (only skill points), anyway.
void DoEndLevelScoreGlitz(int network)
{
	if (Ranking.level_time == 0)
		Ranking.level_time = (Players[Player_num].hours_level * 3600) + ((double)Players[Player_num].time_level / 65536); // Failsafe for if this isn't updated.
	Ranking.maxScore -= Ranking.num_thief_points * 3; // Subtract thieves from the max score right before result screen loads so the "remains" counter is correct in the level.
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
	if (Difficulty_level == 1)
		skill_points2 = Ranking.rankScore / 4;
	if (Difficulty_level > 1) {
		skill_points = level_points * (Difficulty_level / 4);
		skill_points2 = Ranking.rankScore * ((double)Difficulty_level / 4);
	}
	skill_points -= skill_points % 100;
	skill_points2 = (int)skill_points2;

	shield_points = f2i(Players[Player_num].shields) * 5 * mine_level;
	shield_points -= shield_points % 50;
	energy_points = f2i(Players[Player_num].energy) * 2 * mine_level;
	energy_points -= energy_points % 50;
	time_points = (Ranking.maxScore / 1.5) / pow(2, Ranking.level_time / Ranking.parTime);
	if (Ranking.level_time < Ranking.parTime)
		time_points = (Ranking.maxScore / 2.4) * (1 - (Ranking.level_time / Ranking.parTime) * 0.2);
	Ranking.maxScore += Players[Player_num].hostages_level * 7500;
	hostage_points = Players[Player_num].hostages_on_board * 500 * (Difficulty_level + 1);
	hostage_points2 = Players[Player_num].hostages_on_board * 2500 * (((double)Difficulty_level + 8) / 12);

	all_hostage_text[0] = 0;
	endgame_text[0] = 0;
	all_hostage_points = 0;

	if (Players[Player_num].hostages_on_board == Players[Player_num].hostages_level)
		all_hostage_points = Players[Player_num].hostages_on_board * 1000 * (Difficulty_level + 1);

	if (!cheats.enabled && !(Game_mode & GM_MULTI) && (Players[Player_num].lives) && (Current_level_num == Last_level)) {		//player has finished the game!
		endgame_points = Players[Player_num].lives * 10000;
		is_last_level = 1;
	}
	else
		endgame_points = is_last_level = 0;
	if (!cheats.enabled)
		add_bonus_points_to_score(shield_points + energy_points + skill_points + hostage_points + all_hostage_points + endgame_points);
	if (Players[Player_num].hostages_on_board - Ranking.secret_hostages_on_board == Players[Player_num].hostages_level) // For mod score, we subtract the secret level's hostages to avoid the hostage overshoot bug that voids full rescue bonus.
		hostage_points2 *= 3;
	hostage_points2 = round(hostage_points2); // Round this because I got 24999 hostage bonus once.
	death_points = (Ranking.maxScore * 0.4 - Ranking.maxScore * (0.4 / pow(2, Ranking.deathCount))) * -1;
	Ranking.missedRngSpawn *= ((double)Difficulty_level + 4) / 4; // Add would-be skill bonus into the penalty for ignored random offspring. This makes ignoring them on high difficulties more consistent and punishing.
	missed_rng_drops = Ranking.missedRngSpawn;
	Ranking.rankScore += skill_points2 + time_points + hostage_points2 + death_points + missed_rng_drops;

	int minutes = Ranking.level_time / 60;
	double seconds = Ranking.level_time - minutes * 60;
	int parMinutes = Ranking.parTime / 60;
	double parSeconds = Ranking.parTime - parMinutes * 60;
	char* diffname = 0;
	char timeText[256];
	char parTime[256];
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
			sprintf(parTime, "%i:0%.0f", parMinutes, parSeconds);
		else
			sprintf(parTime, "%i:%.0f", parMinutes, parSeconds);
		sprintf(m_str[c++], "Level score\t%.0f", level_points - Ranking.excludePoints);
		sprintf(m_str[c++], "Time: %s/%s\t%i", timeText, parTime, time_points);
		sprintf(m_str[c++], "Hostages: %i/%i\t%.0f", Players[Player_num].hostages_on_board, Players[Player_num].hostages_level, hostage_points2);
		sprintf(m_str[c++], "Skill: %s\t%.0f", diffname, skill_points2);
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
			rankPoints = 12;
		int rank = 0;
		if (rankPoints >= 0)
			rank = (int)rankPoints + 1;
		if (!PlayerCfg.RankShowPlusMinus)
			rank = truncateRanks(rank + 1) - 1;
		endlevel_current_rank = rank;
		if (Ranking.cheated) {
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
				calculateRank(Current_level_num, 0);
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

		if (!Ranking.quickload) 
			sprintf(title, "%s %i %s\n%s %s \n Press R to restart level", TXT_LEVEL, Current_level_num, TXT_COMPLETE, Current_level_name, TXT_DESTROYED);
		else
			sprintf(title, "%s %i %s\n%s %s \n", TXT_LEVEL, Current_level_num, TXT_COMPLETE, Current_level_name, TXT_DESTROYED);

		Assert(c <= N_GLITZITEMS);

		gr_init_bitmap_alloc(&transparent, BM_LINEAR, 0, 0, 1, 1, 1);
		transparent.bm_data[0] = 255;
		transparent.bm_flags |= BM_FLAG_TRANSPARENT;

#ifdef NETWORK
		if (network && (Game_mode & GM_NETWORK))
			newmenu_do2(NULL, title, c, m, multi_endlevel_poll1, NULL, 0, STARS_BACKGROUND);
		else
#endif
			// NOTE LINK TO ABOVE!!!
			newmenu_do2(NULL, title, c, m, endlevel_handler, NULL, 0, STARS_BACKGROUND);
		newmenu_free_background();
		
		gr_free_bitmap_data(&transparent);
}

void DoEndSecretLevelScoreGlitz()
{
	int level_points, skill_points, death_points, time_points;
	double missed_rng_drops, hostage_points;
#define N_GLITZITEMS 12
	char				m_str[N_GLITZITEMS][31];
	newmenu_item	m[N_GLITZITEMS + 1];
	int				i, c;
	char				title[128];
	int				is_last_level = 0;
	Ranking.secretlevel_time = Ranking.secretlevel_time / 65536;
	Ranking.secretMaxScore -= Ranking.num_secret_thief_points * 3; // Subtract thieves from the max score right before result screen loads so the "remains" counter is correct in the level.

	//	Compute level player is on, deal with secret levels (negative numbers)

	level_points = Players[Player_num].score - Ranking.secretlast_score;
	Ranking.secretRankScore = level_points - Ranking.secretExcludePoints;

	skill_points = (int)(Ranking.secretRankScore * ((double)Difficulty_level / 4));

	time_points = (Ranking.secretMaxScore / 1.5) / pow(2, Ranking.secretlevel_time / Ranking.secretParTime);
	if (Ranking.secretlevel_time < Ranking.secretParTime)
		time_points = (Ranking.secretMaxScore / 2.4) * (1 - (Ranking.secretlevel_time / Ranking.secretParTime) * 0.2);
	Ranking.secretMaxScore += Ranking.hostages_secret_level * 7500;
	hostage_points = Ranking.secret_hostages_on_board * 2500 * (((double)Difficulty_level + 8) / 12);
	if (Ranking.secret_hostages_on_board == Ranking.hostages_secret_level)
		hostage_points *= 3;
	hostage_points = round(hostage_points); // Round this because I got 24999 hostage bonus once.
	death_points = (Ranking.secretMaxScore * 0.4 - Ranking.secretMaxScore * (0.4 / pow(2, Ranking.secretDeathCount))) * -1;
	Ranking.secretMissedRngSpawn *= ((double)Difficulty_level + 4) / 4; // Add would-be skill bonus into the penalty for ignored random offspring. This makes ignoring them on high difficulties more consistent and punishing.
	missed_rng_drops = Ranking.secretMissedRngSpawn;
	Ranking.secretRankScore += skill_points + time_points + hostage_points + death_points + missed_rng_drops;

	int minutes = Ranking.secretlevel_time / 60;
	double seconds = Ranking.secretlevel_time - minutes * 60;
	int parMinutes = Ranking.secretParTime / 60;
	double parSeconds = Ranking.secretParTime - parMinutes * 60;
	char* diffname = 0;
	char timeText[256];
	char parTime[256];
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
	if (Ranking.secretQuickload == 0) { // Only print the modded result screen if the player didn't quickload. Print the vanilla one otherwise, because quickloading into a level causes missing crucial mod info.
		if (seconds < 10 || seconds == 60)
			sprintf(timeText, "%i:0%.3f", minutes, seconds);
		else
			sprintf(timeText, "%i:%.3f", minutes, seconds);
		if (parSeconds < 10 || parSeconds == 60)
			sprintf(parTime, "%i:0%.0f", parMinutes, parSeconds);
		else
			sprintf(parTime, "%i:%.0f", parMinutes, parSeconds);
		sprintf(m_str[c++], "Level score\t%.0f", level_points - Ranking.secretExcludePoints);
		sprintf(m_str[c++], "Time: %s/%s\t%i", timeText, parTime, time_points);
		sprintf(m_str[c++], "Hostages: %i/%.0f\t%.0f", Ranking.secret_hostages_on_board, Ranking.hostages_secret_level, hostage_points);
		sprintf(m_str[c++], "Skill: %s\t%i", diffname, skill_points);
		sprintf(m_str[c++], "Deaths: %.0f\t%i", Ranking.secretDeathCount, death_points);
		if (Ranking.secretMissedRngSpawn > 0)
			Ranking.secretMissedRngSpawn = 0; // Let's hope this penalty isn't actually needed on a secret level with a thief, or it won't function correctly.
		if (Ranking.secretMissedRngSpawn < 0)
			sprintf(m_str[c++], "Missed RNG spawn: \t%.0f\n", Ranking.secretMissedRngSpawn);
		else
			strcpy(m_str[c++], "\n");
		sprintf(m_str[c++], "Total score\t%0.0f", Ranking.secretRankScore);

		double rankPoints = (Ranking.secretRankScore / Ranking.secretMaxScore) * 12;
		if (Ranking.secretMaxScore == 0)
			rankPoints = 12;
		int rank = 0;
		if (rankPoints >= 0)
			rank = (int)rankPoints + 1;
		if (!PlayerCfg.RankShowPlusMinus)
			rank = truncateRanks(rank + 1) - 1;
   		endlevel_current_rank = rank;
		if (Ranking.cheated) {
			strcpy(m_str[c++], "\n\n\n");
			sprintf(m_str[c++], "   Cheated, no save!   "); // Don't show vanilla score when cheating, as players already know it'll always be zero.
		}
		else {
			PHYSFS_File* fp;
			PHYSFS_File* temp;
			char filename[256];
			char temp_filename[256];
			sprintf(filename, "ranks/%s/%s/levelS%i.hi", Players[Player_num].callsign, Current_mission->filename, Current_level_num * -1);
			sprintf(temp_filename, "ranks/%s/%s/temp.hi", Players[Player_num].callsign, Current_mission->filename);
			fp = PHYSFS_openRead(filename);
			if (!fp == NULL) {
				calculateRank(Current_mission->last_level - Current_level_num, 0);
				if (Ranking.secretRankScore > Ranking.calculatedScore || Ranking.rank == 0) {
					time_t timeOfScore = time(NULL);
					temp = PHYSFS_openWrite(temp_filename);
					PHYSFSX_printf(temp, "%.0f\n", Ranking.hostages_secret_level);
					PHYSFSX_printf(temp, "%.0f\n", (Ranking.secretMaxScore - Ranking.hostages_secret_level * 7500) / 3);
					PHYSFSX_printf(temp, "%.0f\n", Ranking.secretParTime);
					PHYSFSX_printf(temp, "%.0f\n", level_points - Ranking.secretExcludePoints);
					PHYSFSX_printf(temp, "%.3f\n", Ranking.secretlevel_time);
					PHYSFSX_printf(temp, "%i\n", Ranking.secret_hostages_on_board);
					PHYSFSX_printf(temp, "%i\n", Difficulty_level);
					PHYSFSX_printf(temp, "%.0f\n", Ranking.secretDeathCount);
					PHYSFSX_printf(temp, "%.0f\n", Ranking.secretMissedRngSpawn);
					PHYSFSX_printf(temp, "%s\n", Current_level_name);
					PHYSFSX_printf(temp, "%s", ctime(&timeOfScore));
					PHYSFSX_printf(temp, "%i", 0);
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

		for (i = 0; i < c; i++) {
			m[i].type = NM_TYPE_TEXT;
			m[i].text = m_str[i];
		}

		// m[c].type = NM_TYPE_MENU;	m[c++].text = "Ok";
		sprintf(title, "%s %i %s\n %s %s \n", TXT_SECRET_LEVEL, -Current_level_num, TXT_COMPLETE, Current_level_name, TXT_DESTROYED);

		Assert(c <= N_GLITZITEMS);

		gr_init_bitmap_alloc(&transparent, BM_LINEAR, 0, 0, 1, 1, 1);
		transparent.bm_data[0] = 255;
		transparent.bm_flags |= BM_FLAG_TRANSPARENT;

		newmenu_do2(NULL, title, c, m, endlevel_handler, NULL, 0, STARS_BACKGROUND);
		newmenu_free_background();
		
		gr_free_bitmap_data(&transparent);
}

void DoBestRanksScoreGlitz(int level_num)
{
#define N_GLITZITEMS 12
	char				m_str[N_GLITZITEMS][32];
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
		if (levelHostages < 0) // If level is unplayed, we know because the first (and only) value in the file will be -1, which is impossible for the hostage count.
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
	timePoints = (int)timePoints;
	hostagePoints = playerHostages * 2500 * ((difficulty + 8) / 12);
	if (playerHostages == levelHostages)
		hostagePoints *= 3;
	hostagePoints = round(hostagePoints); // Round this because I got 24999 hostage bonus once.
	double score = playerPoints + skillPoints + timePoints + missedRngSpawn + hostagePoints;
	maxScore += levelHostages * 7500;
	double deathPoints = maxScore * 0.4 - maxScore * (0.4 / pow(2, deathCount));
	deathPoints = (int)deathPoints;
	score -= deathPoints;

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
	if (deathPoints)
		sprintf(m_str[c++], "Deaths: %i\t%0.f", deathCount, -deathPoints);
	else
		sprintf(m_str[c++], "Deaths: %i\t%i", deathCount, 0);
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
		rankPoints = 12;
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
		sprintf(title, "%s on %s\nlevel S%i: %s\nEnter plays level, esc returns to title", Players[Player_num].callsign, Current_mission->mission_name, (Current_mission->last_level - level_num) * -1, levelName);
	else
		sprintf(title, "%s on %s\nlevel %i: %s\nEnter plays level, esc returns to title", Players[Player_num].callsign, Current_mission->mission_name, level_num, levelName);

	Assert(c <= N_GLITZITEMS);

	gr_init_bitmap_alloc(&transparent, BM_LINEAR, 0, 0, 1, 1, 1);
	transparent.bm_data[0] = 255;
	transparent.bm_flags |= BM_FLAG_TRANSPARENT;

	Ranking.startingLevel = level_num;
	newmenu_do2(NULL, title, c, m, endlevel_handler, NULL, 0, STARS_BACKGROUND);
	newmenu_free_background();
}

//	-----------------------------------------------------------------------------------------------------
//called when the player is starting a level (new game or new ship)
void StartSecretLevel()
{
	Assert(!Player_is_dead);

	InitPlayerPosition(0);

	verify_console_object();

	ConsoleObject->control_type	= CT_FLYING;
	ConsoleObject->movement_type	= MT_PHYSICS;

	// -- WHY? -- disable_matcens();
	clear_transient_objects(0);		//0 means leave proximity bombs

	// create_player_appearance_effect(ConsoleObject);
	Do_appearance_effect = 1;

	ai_reset_all_paths();
	// -- NO? -- reset_time();

	reset_rear_view();
	Auto_fire_fusion_cannon_time = 0;
	Fusion_charge = 0;
}

extern void set_pos_from_return_segment(void);

//	Returns true if secret level has been destroyed.
int p_secret_level_destroyed(void)
{
	if (First_secret_visit) {
		return 0;		//	Never been there, can't have been destroyed.
	} else {
		if (PHYSFSX_exists(SECRETC_FILENAME,0))
		{
			return 0;
		} else {
			return 1;
		}
	}
}

//	-----------------------------------------------------------------------------------------------------
#define TXT_SECRET_RETURN  "Returning to level %i", Entered_from_level
#define TXT_SECRET_ADVANCE "Base level destroyed.\nAdvancing to level %i", Entered_from_level+1

int draw_stars_bg(newmenu *menu, d_event *event, grs_bitmap *background)
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
	if (pcx_read_bitmap(STARS_BACKGROUND, &background, BM_LINEAR, gr_palette) != PCX_ERROR_NONE)
		return;

	gr_palette_load(gr_palette);

	va_start(arglist, fmt);
	vsprintf(msg, fmt, arglist);
	va_end(arglist);
	
	nm_messagebox1(NULL, (int (*)(newmenu *, d_event *, void *))draw_stars_bg, &background, 1, TXT_OK, msg);
	gr_free_bitmap_data(&background);
}

// - - - - - - - - - - START OF PAR TIME ALGORITHM STUFF - - - - - - - - - - \\

// Since f1_0 being an int causes rounding errors.
#define f1_0_double 65536.0f
// Account for custom ship properties.
#define SHIP_MOVE_SPEED (((double)Player_ship->max_thrust / ((double)Player_ship->mass * (double)Player_ship->drag) * (1 - (double)Player_ship->drag / f1_0_double)) * pow(f1_0_double, 2))
// 2500 ammo. There may already be an existing macro related to the ammo cap but I couldn't find one.
#define STARTING_VULCAN_AMMO (2500 * f1_0_double)
#define OBJECTIVE_TYPE_INVALID 0
#define OBJECTIVE_TYPE_OBJECT 1
#define OBJECTIVE_TYPE_TRIGGER 2
#define OBJECTIVE_TYPE_ENERGY 3
#define OBJECTIVE_TYPE_WALL 4

int find_connecting_side(int from, int to) // Sirius' function, but I made it take ints instead of point segs for easier use.
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
	partime_objective toDoList[MAX_OBJECTS + MAX_TRIGGERS + MAX_WALLS];
	int toDoListSize;
	partime_objective doneList[MAX_OBJECTS + MAX_TRIGGERS + MAX_WALLS];
	int doneListSize;
	partime_objective blackList[MAX_OBJECTS + MAX_TRIGGERS + MAX_WALLS];
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
	int matcenTriggers[MAX_TRIGGERS]; // We need to track how many times we trip triggers, since they can be set to only trip once.
	// Time spent clearing matcens.
	double matcenTime;
	// Track the locations of energy centers for when we need to make a pit stop...
	partime_objective energyCenters[MAX_NUM_FUELCENS];
	int numEnergyCenters;
	// Variable to tell it when to refill its energy.
	fix simulatedEnergy;
	fix vulcanAmmo; // What it sounds like.
	// How much robot HP we've had to destroy to this point.
	double combatTime;
	fix energy_gained_per_pickup;
	// Info about the weapon algo currently has equipped.
	double energy_usage;
	double ammo_usage; // For when using vulcan.
	int heldWeapons[9]; // Which weapons algo has.
	int num_weapons; // The number of weapons algo has.
	double pathObstructionTime; // Amount of time spent dealing with walls or matcens on the way to an objective (basically an Abyss 1.0 hotfix for the 32k HP wall let's be real lol).
	double shortestPathObstructionTime;
	int hasQuads;
	int segnum; // What segment Algo is in.
	int objectives; // How many objectives Algo has dealt with so far.
	int objectiveSegments[MAX_OBJECTS + MAX_TRIGGERS + MAX_WALLS];
	double objectiveEnergies[MAX_OBJECTS + MAX_TRIGGERS + MAX_WALLS];
	double energyTime;
	ubyte thiefKeys; // Keeps track of which keys have been held by a thief.
} partime_calc_state;

double calculate_combat_time_wall(partime_calc_state* state, int wall_num, int pathFinal) // Tell algo to use the weapon that's fastest for the destructible wall in the way.
{ // I was originally gonna ignore this since hostage doors added negligible time, but then thanks to Devil's Heart, I learned that they can have absurd HP! :D
	int weapon_id = 0; // Just a shortcut for the relevant index in algo's inventory.
	double thisWeaponCombatTime = -1; // How much time does this wall take to destroy with the current weapon?
	double lowestCombatTime = -1; // Track the time of the fastest weapon so far.
	double energyUsed = 0; // To calculate energy cost.
	double ammoUsed = 0; // Same thing but vulcan.
	int topWeapon; // So when depleting energy/ammo, the right one is depleted. Also so the console shows the right weapon.
	// Weapon values converted to a format human beings in 2024 can understand.
	double damage;
	double fire_rate;
	double energy_usage;
	double ammo_usage;
	double splash_radius;
	double wall_health;
	for (int n = 0; n < state->num_weapons; n++) {
		weapon_id = state->heldWeapons[n];
		double gunpoints = 2;
		if ((!(weapon_id > LASER_ID_L4) || weapon_id == LASER_ID_L5 || weapon_id == LASER_ID_L6) && state->hasQuads) { // Account for increased damage of quads.
			if (weapon_id > LASER_ID_L4)
				gunpoints = 4;
			else
				gunpoints = 3; // For some reason only quad 1-4 gets 25% damage reduction while quad 5-6 gets none.
		}
		if (weapon_id == VULCAN_ID || weapon_id == GAUSS_ID || weapon_id == OMEGA_ID)
			gunpoints = 1;
		if (weapon_id == SPREADFIRE_ID)
			gunpoints = 3;
		if (weapon_id == HELIX_ID)
			gunpoints = 5;
		damage = f2fl(Weapon_info[weapon_id].strength[Difficulty_level]) * gunpoints;
		fire_rate = (f1_0_double / Weapon_info[weapon_id].fire_wait);
		energy_usage = f2fl(Weapon_info[weapon_id].energy_usage);
		ammo_usage = f2fl(Weapon_info[weapon_id].ammo_usage) * 13; // The 13 is to scale with the ammo counter.
		splash_radius = f2fl(Weapon_info[weapon_id].damage_radius);
		wall_health = f2fl(Walls[wall_num].hps);
		if (weapon_id == FUSION_ID)
			energy_usage = 2; // Fusion's energy_usage field is 0, so we have to manually set it.
		else {  // Difficulty-based energy nerfs don't impact fusion.
			if (Difficulty_level == 0) // Trainee has 0.5x energy consumption.
				energy_usage *= 0.5;
			if (Difficulty_level == 1) // Rookie has 0.75x energy consumption.
				energy_usage *= 0.75;
		}
		// Assume accuracy is always 100% for walls. They're big and don't move lol.
		int shots = wall_health / damage + 1; // Split time and energy into shots to reflect how players really fire. A 30 HP robot will take two laser 1 shots to kill, not one and a half.
		if (f2fl(state->vulcanAmmo) >= shots * ammo_usage * f1_0) // Make sure we have enough ammo for this robot before using vulcan.
			thisWeaponCombatTime = shots / fire_rate;
		else
			thisWeaponCombatTime = INFINITY; // Make vulcan's/gauss' time infinite so algo won't use it without ammo.
		if (thisWeaponCombatTime < lowestCombatTime || lowestCombatTime == -1) { // If it should be used, update algo's weapon stats to the new one's for use in combat time calculation.
			lowestCombatTime = thisWeaponCombatTime;
			energyUsed = energy_usage * shots * f1_0;
			ammoUsed = ammo_usage * shots * f1_0;
			topWeapon = weapon_id;
		}
	}
	if (pathFinal) { // Only announce we destroyed the wall (or drain energy/ammo) if we actually did, and aren't just simulating doing so when picking a path.
		if (topWeapon == VULCAN_ID || topWeapon == GAUSS_ID)
			state->vulcanAmmo -= ammoUsed * f1_0;
		else {
			state->simulatedEnergy -= energyUsed;
		}
		if (!(topWeapon > LASER_ID_L4) || topWeapon == LASER_ID_L5 || topWeapon == LASER_ID_L6) {
			if (state->hasQuads) {
				if (!(topWeapon > LASER_ID_L4))
					printf("Took %.3fs to fight wall %i with quad laser %i\n", lowestCombatTime, wall_num, topWeapon + 1);
				else
					printf("Took %.3fs to fight wall %i with quad laser %i\n", lowestCombatTime, wall_num, topWeapon - 25);
			}
			else {
				if (!(topWeapon > LASER_ID_L4))
					printf("Took %.3fs to fight wall %i with laser %i\n", lowestCombatTime, wall_num, topWeapon + 1);
				else
					printf("Took %.3fs to fight wall %i with laser %i\n", lowestCombatTime, wall_num, topWeapon - 25);
			}
		}
		if (topWeapon == VULCAN_ID)
			printf("Took %.3fs to fight wall %i with vulcan\n", lowestCombatTime, wall_num);
		if (topWeapon == SPREADFIRE_ID)
			printf("Took %.3fs to fight wall %i with spreadfire\n", lowestCombatTime, wall_num);
		if (topWeapon == PLASMA_ID)
			printf("Took %.3fs to fight wall %i with plasma\n", lowestCombatTime, wall_num);
		if (topWeapon == FUSION_ID)
			printf("Took %.3fs to fight wall %i with fusion\n", lowestCombatTime, wall_num);
		if (topWeapon == GAUSS_ID)
			printf("Took %.3fs to fight wall %i with gauss\n", lowestCombatTime, wall_num);
		if (topWeapon == HELIX_ID)
			printf("Took %.3fs to fight wall %i with helix\n", lowestCombatTime, wall_num);
		if (topWeapon == PHOENIX_ID)
			printf("Took %.3fs to fight wall %i with phoenix\n", lowestCombatTime, wall_num);
		if (topWeapon == OMEGA_ID)
			printf("Took %.3fs to fight wall %i with omega\n", lowestCombatTime, wall_num);
	}
	return lowestCombatTime + 1; // Give an extra second per wall to wait for the explosion to go down. Flying through it causes great damage.
}

double calculate_weapon_accuracy(partime_calc_state* state, weapon_info* weapon_info, int weapon_id, object* obj, robot_info* robInfo, int robot_type)
{
	//return 1; // For if I ever want to disable this.
	// Here we use various aspects of the combat situation to estimate what percentage of shots the player will hit.
	// robot_type tells this function which data to look at to find the right enemy.
	// 0 means a parent robot, 1 means a robot dropped from the obj data, 2 means a robot dropped from the robot data, 3 means a matcen robot.

	if (weapon_info->homing_flag)
		return 1; // Generally, robots can't dodge homing stuff that you shoot.
	if (weapon_id == OMEGA_ID)
		return 1; // Omega always hits (within a certain distance but we'll fudge it). It'll still rarely get used due to the ultra low DPS Algo perceives it to have, but we can at least give it this, right?

	// First initialize weapon and enemy stuff. This is gonna be a long section.
	// Everything will be doubles due to the variables' involvement in division equations.

	double enemy_max_speed;
	double enemy_evade_speed;
	double enemy_health;
	double projectile_speed = f2fl(Weapon_info[weapon_id].speed[Difficulty_level]);
	double enemy_size;
	double player_size = f2fl(ConsoleObject->size);
	double projectile_size = 1;
	if (weapon_info->render_type) {
		if (weapon_info->render_type == 2)
			projectile_size = f2fl(Polygon_models[weapon_info->model_num].rad) / f2fl(weapon_info->po_len_to_width_ratio);
		else
			projectile_size = f2fl(weapon_info->blob_size);
	}
	double enemy_weapon_speed;
	double enemy_weapon_size;
	double enemy_attack_type;
	double enemy_weapon_homing_flag;
	double enemy_behavior;
	if (robot_type == 0) {
		enemy_health = f2fl(obj->shields);
		enemy_behavior = obj->ctype.ai_info.behavior;
		if (robInfo->thief)
			enemy_behavior = AIB_RUN_FROM; // We'll mark thieves down as running enemies, since that's typically what they do.
		if (obj->type == OBJ_CNTRLCEN) { // For some reason reactors have a speed of 120??? They don't actually move tho so mark it down as 0.
			enemy_max_speed = 0;
			enemy_evade_speed = 0;
		}
		else {
			enemy_max_speed = f2fl(robInfo->max_speed[Difficulty_level]);
			enemy_evade_speed = robInfo->evade_speed[Difficulty_level] * 32;
				if (enemy_evade_speed > enemy_max_speed)
					enemy_evade_speed *= 0.75;
		}
		enemy_size = f2fl(Polygon_models[robInfo->model_num].rad);
		enemy_weapon_speed = f2fl(Weapon_info[robInfo->weapon_type].speed[Difficulty_level]);
		enemy_weapon_size = 1;
		if (Weapon_info[robInfo->weapon_type].render_type) {
			if (Weapon_info[robInfo->weapon_type].render_type == 2)
				enemy_weapon_size = f2fl(Polygon_models[Weapon_info[robInfo->weapon_type].model_num].rad) / f2fl(Weapon_info[robInfo->weapon_type].po_len_to_width_ratio);
			else
				enemy_weapon_size = f2fl(Weapon_info[robInfo->weapon_type].blob_size);
		}
		enemy_attack_type = robInfo->attack_type;
		enemy_weapon_homing_flag = Weapon_info[robInfo->weapon_type].homing_flag;
	}
	if (robot_type == 1) {
		enemy_health = f2fl(Robot_info[obj->contains_id].strength);
		enemy_behavior = Robot_info[obj->contains_id].behavior;
		if (Robot_info[obj->contains_id].thief)
			enemy_behavior = AIB_RUN_FROM; // We'll mark thieves down as running enemies, since that's typically what they do.
		enemy_max_speed = f2fl(Robot_info[obj->contains_id].max_speed[Difficulty_level]);
		enemy_evade_speed = Robot_info[obj->contains_id].evade_speed[Difficulty_level] * 32;
		if (enemy_evade_speed > enemy_max_speed)
			enemy_evade_speed *= 0.75;
		enemy_size = f2fl(Polygon_models[Robot_info[obj->contains_id].model_num].rad);
		enemy_weapon_speed = f2fl(Weapon_info[Robot_info[obj->contains_id].weapon_type].speed[Difficulty_level]);
		enemy_weapon_size = 1;
		if (Weapon_info[Robot_info[obj->contains_id].weapon_type].render_type) {
			if (Weapon_info[Robot_info[obj->contains_id].weapon_type].render_type == 2)
				enemy_weapon_size = f2fl(Polygon_models[Weapon_info[Robot_info[obj->contains_id].weapon_type].model_num].rad) / f2fl(Weapon_info[Robot_info[obj->contains_id].weapon_type].po_len_to_width_ratio);
			else
				enemy_weapon_size = f2fl(Weapon_info[Robot_info[obj->contains_id].weapon_type].blob_size);
		}
		enemy_attack_type = Robot_info[obj->contains_id].attack_type;
		enemy_weapon_homing_flag = Weapon_info[Robot_info[obj->contains_id].weapon_type].homing_flag;
	}
	if (robot_type == 2) {
		enemy_health = f2fl(Robot_info[robInfo->contains_id].strength);
		enemy_behavior = Robot_info[robInfo->contains_id].behavior;
		if (Robot_info[robInfo->contains_id].thief)
			enemy_behavior = AIB_RUN_FROM; // We'll mark thieves down as running enemies, since that's typically what they do.
		enemy_max_speed = f2fl(Robot_info[robInfo->contains_id].max_speed[Difficulty_level]);
		enemy_evade_speed = Robot_info[robInfo->contains_id].evade_speed[Difficulty_level] * 32;
		if (enemy_evade_speed > enemy_max_speed)
			enemy_evade_speed *= 0.75;
		enemy_size = f2fl(Polygon_models[robInfo->model_num].rad);
		enemy_weapon_speed = f2fl(Weapon_info[Robot_info[robInfo->contains_id].weapon_type].speed[Difficulty_level]);
		enemy_weapon_size = 1;
		if (Weapon_info[Robot_info[robInfo->contains_id].weapon_type].render_type) {
			if (Weapon_info[Robot_info[robInfo->contains_id].weapon_type].render_type == 2)
				enemy_weapon_size = f2fl(Polygon_models[Weapon_info[Robot_info[robInfo->contains_id].weapon_type].model_num].rad) / f2fl(Weapon_info[Robot_info[robInfo->contains_id].weapon_type].po_len_to_width_ratio);
			else
				enemy_weapon_size = f2fl(Weapon_info[Robot_info[robInfo->contains_id].weapon_type].blob_size);
		}
		enemy_attack_type = Robot_info[robInfo->contains_id].attack_type;
		enemy_weapon_homing_flag = Weapon_info[Robot_info[robInfo->contains_id].weapon_type].homing_flag;
	}
	if (robot_type == 3) {
		enemy_health = f2fl(robInfo->strength);
		enemy_behavior = robInfo->behavior;
		if (robInfo->thief)
			enemy_behavior = AIB_RUN_FROM; // We'll mark thieves down as running enemies, since that's typically what they do.
		enemy_max_speed = f2fl(robInfo->max_speed[Difficulty_level]);
		enemy_evade_speed = robInfo->evade_speed[Difficulty_level] * 32;
		if (enemy_evade_speed > enemy_max_speed)
			enemy_evade_speed *= 0.75;
		enemy_size = f2fl(Polygon_models[robInfo->model_num].rad);
		enemy_weapon_speed = f2fl(Weapon_info[robInfo->weapon_type].speed[Difficulty_level]);
		enemy_weapon_size = 1;
		if (Weapon_info[robInfo->weapon_type].render_type) {
			if (Weapon_info[robInfo->weapon_type].render_type == 2)
				enemy_weapon_size = f2fl(Polygon_models[Weapon_info[robInfo->weapon_type].model_num].rad) / f2fl(Weapon_info[robInfo->weapon_type].po_len_to_width_ratio);
			else
				enemy_weapon_size = f2fl(Weapon_info[robInfo->weapon_type].blob_size);
		}
		enemy_attack_type = robInfo->attack_type;
		enemy_weapon_homing_flag = Weapon_info[robInfo->weapon_type].homing_flag;
	}
	
	// Next, find the "optimal distance" for fighting the given enemy with the given weapon. This is the distance where the enemy's fire can be dodged off of pure reaction time, without any prediction.
	// Once the player's ship can start moving 250ms (avg human reaction time) after the enemy shoots, and get far enough out of the way for the enemy's shots to miss, it's at the optimal distance.
	// Any closer, and the player is put in too much danger. Any further, and the player faces potential accuracy loss due to the enemy having more time to dodge themselves.
	double optimal_distance;
	if (enemy_attack_type) // In the case of enemies that don't shoot at you, the optimal distance depends on their speed, as generally you wanna stand further back the quicker they can approach you.
		optimal_distance = enemy_max_speed / 4; // The /4 is in reference to the 250ms benchmark from earlier. When they start charging you, you've gotta react and start backing up.
	else
		optimal_distance = (((player_size + enemy_weapon_size * F1_0) / SHIP_MOVE_SPEED) + 0.25) * enemy_weapon_speed;
	if (enemy_behavior == AIB_RUN_FROM) // We don't want snipe robots to use this, as they actually shoot things.
		optimal_distance = 80; // In the case of enemies that run from you, we'll use the maximum enemy dodge distance because it returns healthy chase time values.

	// Next, figure out how well the enemy will dodge a player attack of this weapon coming from the optimal distance away, then base accuracy off of that.
	// For simplicity, we assume enemies face longways and dodge sideways relative to player rotation, and that the player is shooting at the middle of the target from directly ahead.
	// The amount of distance required to move is based off of hard coded gunpoints on the ship, as well as the radius of the player projectile, so we'll have to set values per weapon ID.
	// For spreading weapons, the offset technically depends on which projectile we're talking about, but we'll set it to that of the middle one's starting point for now, then account for decreasing accuracy over distance later.
	double projectile_offsets[35] = { 2.2, 2.2, 2.2, 2.2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2.2, 2.2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2.2, 2.2, 0, 0, 2.2};
	// Quad lasers have a wider offset, making them a little harder to dodge. Account for this.
	// This makes their accuracy worse against small enemies than reality due to the offset of the inner lasers being ignored, but this is a rare occurence.
	if (state->hasQuads) {
		projectile_offsets[LASER_ID_L1] *= 1.5;
		projectile_offsets[LASER_ID_L2] *= 1.5;
		projectile_offsets[LASER_ID_L3] *= 1.5;
		projectile_offsets[LASER_ID_L4] *= 1.5;
		projectile_offsets[LASER_ID_L5] *= 1.5;
		projectile_offsets[LASER_ID_L6] *= 1.5;
	}
	double dodge_distance = projectile_offsets[weapon_id] + projectile_size + enemy_size;

	// Now we reduce accuracy over distance for spreading weapons. Since that's also hard coded, we can just apply enemy size based multipliers per weapon ID.
	// At a certain distance, projectiles for Vulcan and Spreadfire will start missing from drifting so far off course.
	// For Vulcan we scale accuracy by the probability of a shot landing at a given distance (since the spread is random), and for Spreadfire/Helix we use a binary outcome.
	double accuracy_multiplier = 1;
	if (weapon_id == VULCAN_ID) {
		if (optimal_distance / 32 > enemy_size + projectile_size) // This is the distance where Vulcan's accuracy dropoff starts.
			accuracy_multiplier *= (enemy_size + projectile_size) / (optimal_distance / 32);
	}
	if (weapon_id == GAUSS_ID) { // Gauss has 20% of Vulcan's spread... as if they thought it wasn't good enough or something. This is unlikely to ever influence the accuracy value but hey you never know.
		if (optimal_distance / 160 > enemy_size + projectile_size) // This is the distance where Gauss' accuracy dropoff starts.
			accuracy_multiplier *= (enemy_size + projectile_size) / (optimal_distance / 160);
	}
	if (weapon_id == SPREADFIRE_ID) {
		if (optimal_distance / 16 > enemy_size + projectile_size) // Divisor is gotten because Spreadfire projectiles move one unit outward for every 16 units forward.
			accuracy_multiplier /= 3;
	}
	if (weapon_id == HELIX_ID) {
		if (optimal_distance / 8 > enemy_size + projectile_size) // Outer Helix projectiles move one unit outward for every eight units forward.
			accuracy_multiplier *= 0.6;
		if (optimal_distance / 16 > enemy_size + projectile_size) // Middle-left and middle-right Helix projectiles are equal to Spreadfire's outer projectiles in sideways movement.
			accuracy_multiplier *= 0.2;
	}
	// If the enemy is small enough to fit between projectiles, only one can hit at a time, so halve accuracy.
	// This makes Algo unlikely to use something like lasers against sidearm modula, which is good because it obeys real world expectations.
	if (enemy_size < projectile_offsets[weapon_id] - projectile_size)
		accuracy_multiplier *= 0.5;

	double accuracy;
	if ((enemy_behavior == AIB_RUN_FROM && enemy_max_speed) || (enemy_behavior != AIB_RUN_FROM && enemy_evade_speed)) {
		if (optimal_distance > 80) { // Enemies can't dodge something until it's within 80 units of them (Source: ai.c line 830), but we can't cap optimal_distance itself at that because it'll make spread penalty too lenient.
			if (enemy_behavior == AIB_RUN_FROM)
				accuracy = ((dodge_distance / enemy_max_speed) / (80 / projectile_speed));
			else
				accuracy = ((dodge_distance / enemy_evade_speed) / (80 / projectile_speed));
		}
		else {
			if (enemy_behavior == AIB_RUN_FROM)
				accuracy = ((dodge_distance / enemy_max_speed) / (optimal_distance / projectile_speed));
			else
				accuracy = ((dodge_distance / enemy_evade_speed) / (optimal_distance / projectile_speed));
		}
	}
	else
		return accuracy_multiplier; // If the enemy doesn't move in the relevant way, guarantee all hits (assuming efficient size).
	if (accuracy > 1) // Accuracy can't be greater than 100%.
		return 1;
	else if (enemy_behavior == AIB_SNIPE || enemy_behavior == AIB_RUN_FROM)
		return accuracy * accuracy_multiplier;
	else
		return (0.5 + accuracy / 2) * accuracy_multiplier;
	// Enemies evade less and less linearly as their health goes down (Source: D1 ai.c line 997), so return an average of 100% and the starting acc, except for fleeing enemies, who we want to return very low values.
	// Gonna be honest, I'm actually not sure about that HP part for D2, but high difficulty times are way too easy if we don't have this so I'm keeping it anyway.
}

double calculate_combat_time(partime_calc_state* state, object* obj, robot_info* robInfo) // Tell algo to use the weapon that's fastest for the current enemy.
{
	int weapon_id = 0; // Just a shortcut for the relevant index in algo's inventory.
	double thisWeaponCombatTime = -1; // How much time does this enemy take to kill with the current weapon?
	double lowestCombatTime = -1; // Track the time of the fastest weapon so far.
	double energyUsed = 0; // To calculate energy cost.
	double ammoUsed = 0; // Same thing but vulcan.
	int topWeapon = -1; // So when depleting energy/ammo, the right one is depleted. Also so the console shows the right weapon.
	double offspringHealth; // So multipliers done to offspring don't bleed into their parents' values.
	double accuracy; // Players are NOT perfect, and it's usually not their fault. We need to account for this if we want all par times to be reachable.
	double topAccuracy; // The percentage shown on the debug console.
	double adjustedRobotHealthNoAccuracy;
	// Weapon values converted to a format human beings in 2024 can understand.
	for (int n = 0; n < state->num_weapons; n++) {
		weapon_id = state->heldWeapons[n];
		weapon_info* weapon_info = &Weapon_info[weapon_id];
		double gunpoints = 2;
		if ((!(weapon_id > LASER_ID_L4) || weapon_id == LASER_ID_L5 || weapon_id == LASER_ID_L6) && state->hasQuads) { // Account for increased damage of quads.
			if (weapon_id > LASER_ID_L4)
				gunpoints = 4;
			else
				gunpoints = 3; // For some reason only quad 1-4 gets 25% damage reduction while quad 5-6 gets none.
		}
		if (weapon_id == VULCAN_ID || weapon_id == GAUSS_ID || weapon_id == OMEGA_ID)
			gunpoints = 1;
		if (weapon_id == SPREADFIRE_ID)
			gunpoints = 3;
		if (weapon_id == HELIX_ID)
			gunpoints = 5;
		double damage = f2fl(weapon_info->strength[Difficulty_level]) * gunpoints;
		if (weapon_id == FUSION_ID && obj->type == OBJ_CNTRLCEN)
			damage *= 2; // Fusion's damage is doubled against reactors in Redux.
		double fire_rate = (f1_0_double / weapon_info->fire_wait);
		double energy_usage = f2fl(weapon_info->energy_usage);
		double ammo_usage = f2fl(weapon_info->ammo_usage) * 13; // The 13 is to scale with the ammo counter.
		double splash_radius = f2fl(weapon_info->damage_radius);
		double enemy_health = f2fl(obj->shields);
		double enemy_spawn_health = f2fl(Robot_info[obj->contains_id].strength);
		double enemy_size = f2fl(obj->size);
		// If we're fighting a boss that's immune to the weapon we're about to calculate, skip the weapon.
		if (robInfo->boss_flag) {
			if (weapon_id == GAUSS_ID)
				damage *= 1 - ((double)Difficulty_level * 0.1); // Damage of gauss on bosses goes down as difficulty goes up.
			if (Boss_invulnerable_energy[robInfo->boss_flag - BOSS_D2]) {
				if (!(weapon_id == VULCAN_ID || weapon_id == GAUSS_ID))
					continue;
				else
					ammo_usage = 0; // Give Algo infinite vulcan ammo so it doesn't softlock fighting a boss that's immune to energy weapons. Do remember that Algo doesn't have access to missiles!
			}
			if (Boss_invulnerable_matter[robInfo->boss_flag - BOSS_D2])
				if (weapon_id == VULCAN_ID || weapon_id == GAUSS_ID)
					continue;
		}
		if (weapon_id == FUSION_ID)
			energy_usage = 2; // Fusion's energy_usage field is 0, so we have to manually set it.
		else {  // Difficulty-based energy nerfs don't impact fusion.
			if (Difficulty_level == 0) // Trainee has 0.5x energy consumption.
				energy_usage *= 0.5;
			if (Difficulty_level == 1) // Rookie has 0.75x energy consumption.
				energy_usage *= 0.75;
		}
		double adjustedRobotHealth = enemy_health;
		adjustedRobotHealth /= (splash_radius - enemy_size) / splash_radius >= 0 ? 1 + (splash_radius - enemy_size) / splash_radius : 1; // Divide the health value of the enemy instead of increasing damage when accounting for splash damage, since we'll potentially have multiple damage values.
		adjustedRobotHealthNoAccuracy = adjustedRobotHealth;
		adjustedRobotHealth /= calculate_weapon_accuracy(&state, weapon_info, weapon_id, obj, robInfo, 0);
		enemy_spawn_health = f2fl(robInfo->strength);
		if (robInfo->thief)
			state->combatTime += 2.5; // To account for the death tantrum they throw when they get their comeuppance for stealing your stuff.
		if (obj->contains_type == OBJ_ROBOT) { // Now we account for robots guaranteed to drop from this robot, if any.
			enemy_size = f2fl(Polygon_models[Robot_info[obj->contains_id].model_num].rad);
			offspringHealth = enemy_spawn_health * obj->contains_count;
			offspringHealth /= (splash_radius - enemy_size) / splash_radius >= 0 ? 1 + (splash_radius - enemy_size) / splash_radius : 1;
			adjustedRobotHealthNoAccuracy += offspringHealth;
			offspringHealth /= calculate_weapon_accuracy(&state, weapon_info, weapon_id, obj, robInfo, 1);
			adjustedRobotHealth += offspringHealth;
		}
		else if (robInfo->contains_type == OBJ_ROBOT) { // Now account for robots that are hard coded to drop (EG spiders dropping spiderlings, obj stuff overwrites this).
			enemy_size = f2fl(Polygon_models[Robot_info[robInfo->contains_id].model_num].rad);
			offspringHealth = enemy_spawn_health * robInfo->contains_count;
			offspringHealth *= robInfo->contains_prob;
			offspringHealth /= 16;
			offspringHealth /= (splash_radius - enemy_size) / splash_radius >= 0 ? 1 + (splash_radius - enemy_size) / splash_radius : 1;
			adjustedRobotHealthNoAccuracy += offspringHealth;
			offspringHealth /= calculate_weapon_accuracy(&state, weapon_info, weapon_id, obj, robInfo, 2);
			adjustedRobotHealth += offspringHealth;
		}
		// I'm not going any deeper than this (two layers), because you can have theoretically infinite. I've only seen three layers once (D1 level 13), and never beyond that, which would be asking for trouble on multiple fronts.
		accuracy = adjustedRobotHealthNoAccuracy / adjustedRobotHealth;
		int shots = (adjustedRobotHealthNoAccuracy / damage + 1) / accuracy + 1; // Split time and energy into shots to reflect how players really fire. A 30 HP robot will take two laser 1 shots to kill, not one and a half.
		if (f2fl(state->vulcanAmmo) >= shots * ammo_usage * f1_0) // Make sure we have enough ammo for this robot before using vulcan.
			thisWeaponCombatTime = shots / fire_rate;
		else
			thisWeaponCombatTime = INFINITY; // Make vulcan's/gauss' time infinite so algo won't use it without ammo.
		if (thisWeaponCombatTime < lowestCombatTime || lowestCombatTime == -1) { // If it should be used, update algo's weapon stats to the new one's for use in combat time calculation.
			lowestCombatTime = thisWeaponCombatTime;
			energyUsed = energy_usage * shots * f1_0;
			ammoUsed = ammo_usage * shots * f1_0;
			topWeapon = weapon_id;
			topAccuracy = accuracy * 100;
		}
	}
	if (lowestCombatTime == -1)
		lowestCombatTime = 0; // Prevent a softlock if no primaries work on a given boss.
	if (topWeapon == VULCAN_ID || topWeapon == GAUSS_ID)
		state->vulcanAmmo -= ammoUsed * f1_0;
	else {
		state->simulatedEnergy -= energyUsed;
	}
	if (!(topWeapon > LASER_ID_L4) || topWeapon == LASER_ID_L5 || topWeapon == LASER_ID_L6) {
		if (state->hasQuads) {
			if (!(topWeapon > LASER_ID_L4))
				printf("Took %.3fs to fight robot type %i with quad laser %i, %.2f accuracy\n", lowestCombatTime, obj->id, topWeapon + 1, topAccuracy);
			else
				printf("Took %.3fs to fight robot type %i with quad laser %i, %.2f accuracy\n", lowestCombatTime, obj->id, topWeapon - 25, topAccuracy);
		}
		else {
			if (!(topWeapon > LASER_ID_L4))
				printf("Took %.3fs to fight robot type %i with laser %i, %.2f accuracy\n", lowestCombatTime, obj->id, topWeapon + 1, topAccuracy);
			else
				printf("Took %.3fs to fight robot type %i with laser %i, %.2f accuracy\n", lowestCombatTime, obj->id, topWeapon - 25, topAccuracy);
		}
	}
	if (topWeapon == VULCAN_ID)
		printf("Took %.3fs to fight robot type %i with vulcan, %.2f accuracy\n", lowestCombatTime, obj->id, topAccuracy);
	if (topWeapon == SPREADFIRE_ID)
		printf("Took %.3fs to fight robot type %i with spreadfire, %.2f accuracy\n", lowestCombatTime, obj->id, topAccuracy);
	if (topWeapon == PLASMA_ID)
		printf("Took %.3fs to fight robot type %i with plasma, %.2f accuracy\n", lowestCombatTime, obj->id, topAccuracy);
	if (topWeapon == FUSION_ID)
		printf("Took %.3fs to fight robot type %i with fusion, %.2f accuracy\n", lowestCombatTime, obj->id, topAccuracy);
	if (topWeapon == GAUSS_ID)
		printf("Took %.3fs to fight robot type %i with gauss, %.2f accuracy\n", lowestCombatTime, obj->id, topAccuracy);
	if (topWeapon == HELIX_ID)
		printf("Took %.3fs to fight robot type %i with helix, %.2f accuracy\n", lowestCombatTime, obj->id, topAccuracy);
	if (topWeapon == PHOENIX_ID)
		printf("Took %.3fs to fight robot type %i with phoenix, %.2f accuracy\n", lowestCombatTime, obj->id, topAccuracy);
	if (topWeapon == OMEGA_ID)
		printf("Took %.3fs to fight robot type %i with omega, %.2f accuracy\n", lowestCombatTime, obj->id, topAccuracy);
	return lowestCombatTime;
}

double calculate_combat_time_matcen(partime_calc_state* state, robot_info* robInfo) // We need a matcen version of this function, because matcens don't use the Objects struct.
{
	int weapon_id = 0; // Just a shortcut for the relevant index in algo's inventory.
	double thisWeaponMatcenTime = -1; // How much damage worth of shooting to we have to do for this enemy in this matcen?
	double lowestMatcenTime = -1; // Track the lowest amount so far.
	double accuracy; // Players are NOT perfect, and it's usually not their fault. We need to account for this if we want all par times to be reachable.
	double adjustedRobotHealthNoAccuracy;
	int topWeapon;
	// Weapon values converted to a format human beings in 2024 can understand.
	double enemy_size;
	for (int n = 0; n < state->num_weapons; n++) {
		weapon_id = state->heldWeapons[n];
		enemy_size = f2fl(Polygon_models[robInfo->model_num].rad);
		weapon_info* weapon_info = &Weapon_info[weapon_id];
		double damage = f2fl(weapon_info->strength[Difficulty_level]);
		double fire_rate = (f1_0_double / weapon_info->fire_wait);
		double energy_usage = f2fl(weapon_info->energy_usage);
		double ammo_usage = f2fl(weapon_info->ammo_usage) * 13; // The 13 is to scale with the ammo counter.
		double splash_radius = f2fl(weapon_info->damage_radius);
		double enemy_health = f2fl(robInfo->strength);
		double gunpoints = 2;
		if ((!(weapon_id > LASER_ID_L4) || weapon_id == LASER_ID_L5 || weapon_id == LASER_ID_L6) && state->hasQuads) { // Account for increased damage of quads.
			if (weapon_id > LASER_ID_L4)
				gunpoints = 4;
			else
				gunpoints = 3; // For some reason only quad 1-4 gets 25% damage reduction while quad 5-6 gets none.
		}
		if (weapon_id == VULCAN_ID || weapon_id == GAUSS_ID || weapon_id == OMEGA_ID)
			gunpoints = 1;
		if (weapon_id == SPREADFIRE_ID)
			gunpoints = 3;
		if (weapon_id == HELIX_ID)
			gunpoints = 5;
		if (weapon_id == FUSION_ID)
			energy_usage = 2; // Fusion's energy_usage field is 0, so we have to manually set it.
		else {  // Difficulty-based energy nerfs don't impact fusion.
			if (Difficulty_level == 0) // Trainee has 0.5x energy consumption.
				energy_usage *= 0.5;
			if (Difficulty_level == 1) // Rookie has 0.75x energy consumption.
				energy_usage *= 0.75;
		}
		double adjustedRobotHealth = enemy_health;
		adjustedRobotHealth /= (splash_radius - enemy_size) / splash_radius >= 0 ? 1 + (splash_radius - enemy_size) / splash_radius : 1;
		adjustedRobotHealthNoAccuracy = adjustedRobotHealth;
		adjustedRobotHealth /= calculate_weapon_accuracy(&state, weapon_info, weapon_id, NULL, robInfo, 3);
		double offspringHealth; // So multipliers done to offspring don't bleed into their parents' values.
		enemy_size = f2fl(Polygon_models[Robot_info[robInfo->contains_id].model_num].rad);
		if (robInfo->contains_type == OBJ_ROBOT) { // Now account for robots that are hard coded to drop (EG spiders dropping spiderlings, obj stuff overwrites this).
			offspringHealth = f2fl(Robot_info[robInfo->contains_id].strength);
			offspringHealth *= robInfo->contains_count * robInfo->contains_prob;
			offspringHealth /= 16;
			offspringHealth /= (splash_radius - enemy_size) / splash_radius >= 0 ? 1 + (splash_radius - enemy_size) / splash_radius : 1;
			adjustedRobotHealthNoAccuracy += offspringHealth;
			offspringHealth /= calculate_weapon_accuracy(&state, weapon_info, weapon_id, NULL, robInfo, 2);
			adjustedRobotHealth += offspringHealth;
		}
		accuracy = adjustedRobotHealthNoAccuracy / adjustedRobotHealth;
		int shots = (adjustedRobotHealthNoAccuracy / damage + 1) / accuracy + 1; // Split time and energy into shots to reflect how players really fire. A 30 HP robot will take two laser 1 shots to kill, not one and a half.
		if (f2fl(state->vulcanAmmo) >= shots * ammo_usage * f1_0 * (Difficulty_level + 3)) // Make sure we have enough ammo for the average of this matcen's robots, times the number of waves on this difficulty, before using it.
			thisWeaponMatcenTime = shots / fire_rate;
		else
			thisWeaponMatcenTime = INFINITY; // Make vulcan's/gauss' time infinite so algo won't use it without ammo.
		if (thisWeaponMatcenTime < lowestMatcenTime || lowestMatcenTime == -1) { // If it should be used, update algo's weapon stats to the new one's for use in combat time calculation.
			state->energy_usage = shots * energy_usage; // We need to calculate thess externally for the end of matcen calc.
			state->ammo_usage = shots * ammo_usage;
			lowestMatcenTime = thisWeaponMatcenTime;
			topWeapon = weapon_id;
		}
		// Now account for RNG energy/ammo drops from matcen bots and their robot spawn.
		if (robInfo->contains_type == OBJ_POWERUP && robInfo->contains_id == POW_ENERGY)
			state->energy_usage -= f2fl(((double)robInfo->contains_count * ((double)robInfo->contains_prob / 16)) * state->energy_gained_per_pickup);
		if (robInfo->contains_type == OBJ_POWERUP && robInfo->contains_id == POW_VULCAN_AMMO)
			state->ammo_usage -= f2fl(((double)robInfo->contains_count * ((double)robInfo->contains_prob / 16)) * state->energy_gained_per_pickup);
		if (robInfo->contains_type == OBJ_ROBOT) {
			if (Robot_info[robInfo->contains_id].contains_type == OBJ_POWERUP && Robot_info[robInfo->contains_id].contains_id == POW_ENERGY)
				state->energy_usage -= f2fl(((double)Robot_info[robInfo->contains_id].contains_count * ((double)Robot_info[robInfo->contains_id].contains_prob / 16)) * (STARTING_VULCAN_AMMO / 2));
			if (Robot_info[robInfo->contains_id].contains_type == OBJ_POWERUP && Robot_info[robInfo->contains_id].contains_id == POW_VULCAN_AMMO)
				state->ammo_usage -= f2fl(((double)Robot_info[robInfo->contains_id].contains_count * ((double)Robot_info[robInfo->contains_id].contains_prob / 16)) * (STARTING_VULCAN_AMMO / 2));
		}
	}
	return lowestMatcenTime;
}

int getObjectiveSegnum(partime_objective objective)
{
	if (objective.type == OBJECTIVE_TYPE_OBJECT)
		return Objects[objective.ID].segnum;
	if (objective.type == OBJECTIVE_TYPE_TRIGGER || objective.type == OBJECTIVE_TYPE_ENERGY)
		return objective.ID;
	if (objective.type == OBJECTIVE_TYPE_WALL)
		return Walls[objective.ID].segnum;

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

int findKeyObjectID(int keyType)
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
		if ((Objects[i].type == OBJ_POWERUP && Objects[i].id == powerupID) ||
			(Objects[i].type == OBJ_ROBOT && Objects[i].contains_type == OBJ_POWERUP && Objects[i].contains_id == powerupID) || (Objects[i].type == OBJ_ROBOT && Robot_info[Objects[i].id].contains_type == OBJ_POWERUP && Robot_info[Objects[i].id].contains_id == powerupID))
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

	if (*listSize >= MAX_OBJECTS + MAX_TRIGGERS + MAX_WALLS)
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

int find_connecting_wall(int wall_num)
{
	int adjacent_wall_num = -1;
	int adjacent_wall_segnum;
	int connecting_side;
	for (int c = 0; c < 6; c++) { // Find that neighboring wall.
		adjacent_wall_segnum = Segments[Walls[wall_num].segnum].children[c];
		connecting_side = find_connecting_side(Walls[wall_num].segnum, adjacent_wall_segnum);
		if (wall_num == Segments[Walls[wall_num].segnum].sides[connecting_side].wall_num) {
			connecting_side = find_connecting_side(adjacent_wall_segnum, Walls[wall_num].segnum); // We have to reverse the arguments to get the opposite wall.
			adjacent_wall_num = Segments[adjacent_wall_segnum].sides[connecting_side].wall_num;
		}
		if (adjacent_wall_num > -1)
			return adjacent_wall_num;
	}
}

void initLockedWalls(partime_calc_state* state)
{
	int i;
	partime_locked_wall_info* wallInfo = &state->lockedWalls[state->numLockedWalls];
	partime_locked_wall_info* reactorInfo = &state->reactorWalls[state->numReactorWalls];
	// If these are yes but the levels have no matching keys (or bots containing said keys), we'll consider it a joint level, then combine the par times of the current main and secret level.
	int blueDoorPending = 0;
	int yellowDoorPending = 0;
	int redDoorPending = 0;
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
				wallInfo->unlockedBy.type = OBJECTIVE_TYPE_OBJECT;
				wallInfo->unlockedBy.ID = findKeyObjectID(Walls[i].keys);
				if (Walls[i].keys == KEY_BLUE)
					blueDoorPending = 1;
				if (Walls[i].keys == KEY_GOLD)
					yellowDoorPending = 1;
				if (Walls[i].keys == KEY_RED)
					redDoorPending = 1;
				continue;
			}

			if (Walls[i].flags & WALL_DOOR_LOCKED) {
				// ...or is it opened by a trigger?
				int unlockWall = findTriggerWallForWall(i);
				if (unlockWall != -1) {
					wallInfo->unlockedBy.type = OBJECTIVE_TYPE_TRIGGER;
					wallInfo->unlockedBy.ID = Walls[unlockWall].segnum;
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
	Ranking.numCurrentlyLockedWalls = state->numLockedWalls;
	for (i = 0; i < state->numLockedWalls; i++) {
		Ranking.currentlyLockedWalls[i] = state->lockedWalls[i].wallID;
		Ranking.parTimeUnlockIDs[i] = state->lockedWalls[i].unlockedBy.ID;
		Ranking.parTimeUnlockTypes[i] = state->lockedWalls[i].unlockedBy.type;
		// Don't add an unlock objective if it's a robot holding a key. Robots are already added anyway. (Shoutout to The Pit from LOTW for having a boss hold a key or I would never have noticed this happening.)
		// Also don't add anything with an invalid objective type. That just makes things more confusing to debug.
		if (state->lockedWalls[i].unlockedBy.type && !(state->lockedWalls[i].unlockedBy.type == 1 && Objects[state->lockedWalls[i].unlockedBy.ID].type == OBJ_ROBOT)) // Don't add an unlock objective if it's a robot holding a key. Robots are already added anyway. (Shoutout to The Pit from LOTW for having a boss hold a key or I would never have noticed this happening.)
			addObjectiveToList(state->toDoList, &state->toDoListSize, state->lockedWalls[i].unlockedBy, 0); // Add every key and unlock trigger to the to-do list, ignoring duplicates.
	}
	// Now we have to iterate again because the unlocked side of one-sided locked walls need to be tested for an unlock ID, and that can only be done AFTER the rest of the unlocks have been added.
	for (i = 0; i < Num_walls; i++) {
		if (Walls[i].type == WALL_DOOR && ((Walls[i].keys == KEY_BLUE || Walls[i].keys == KEY_GOLD || Walls[i].keys == KEY_RED) || Walls[i].flags & WALL_DOOR_LOCKED)) {
			partime_locked_wall_info* wallInfo = &state->lockedWalls[state->numLockedWalls];
			partime_locked_wall_info* reactorInfo = &state->reactorWalls[state->numReactorWalls];
			wallInfo->wallID = i;
			for (int w = 0; w < state->numLockedWalls; w++) {
				if (state->lockedWalls[w].wallID == i) {
					if (state->lockedWalls[w].unlockedBy.type) // If an unlock type never got assigned for this locked door, its neighboring wall could be what unlocks it (D1 S2).
						continue; // If one was though, that's not the case.
					int adjacent_wall_num = find_connecting_wall(i);
					if (!(Walls[adjacent_wall_num].flags & WALL_DOOR_LOCKED)) { // Make sure it can be unlocked. If neither side can be unlocked then there's no point going through with this step.
						// Now make it the unlock ID for the locked side. Note: Due to the level design, these will probably be inaccessible objectives.
						// Wall type objectives function identically to trigger types, except Algo must have the required key before being able to mark them as done (like in Beeblebrox Mine).
						wallInfo->unlockedBy.type = OBJECTIVE_TYPE_WALL;
						wallInfo->unlockedBy.ID = adjacent_wall_num; // Wall type objectives are also identified by their wall number rather than their segment number, so we can read the keys associated with the wall.
						addObjectiveToList(state->toDoList, &state->toDoListSize, wallInfo->unlockedBy, 0);
						continue;
					}
				}
			}
		}
		// Note: As a side effect of this, the back sides of spawn doors and exits (if they're unlocked) will be added as inaccessible objectives, but this will almost always add negligible time to par.
	}
	// Now let's reiterate one last time through the locked walls list if we're on the first run. We've gotta move all the stuff with unlock IDs to done list so Algo ignores locked doors.
	// We have to do this iteration separately, because if we don't, the part of marking things as done that removes locked walls from Ranked.currentlyLockedWalls will be removing them from a list that's still populating.
	if (!Ranking.parTimeRuns) {
		for (i = 0; i < state->numLockedWalls; i++) // Only move something if it has a type, so we don't ignore grates as well.
			if (state->lockedWalls[i].unlockedBy.type)
				addObjectiveToList(state->doneList, &state->doneListSize, state->lockedWalls[i].unlockedBy, 1);
		if (state->numReactorWalls) // If needed, also add the reactor to done list so Algo can access reactor closets on its first run.
			addObjectiveToList(state->doneList, &state->doneListSize, state->reactorWalls[0].unlockedBy, 1);
		for (i = 0; i < state->numReactorWalls; i++) { // Remove any locked walls from the list that are the other side of a reactor wall. Algo went inside the D1 S1 reactor closet and couldn't get back out.
			int adjacent_wall_num = find_connecting_wall(state->reactorWalls[i].wallID);
			for (int w = 0; w < Ranking.numCurrentlyLockedWalls; w++)
				if (Ranking.currentlyLockedWalls[w] == adjacent_wall_num)
					removeLockedWallFromList(w);
		}
	}
	for (i = 0; i < Highest_object_index; i++) {
		if (blueDoorPending)
			if ((Objects[i].type == OBJ_POWERUP && Objects[i].id == POW_KEY_BLUE) || (Objects[i].type == OBJ_ROBOT && (Objects[i].contains_type == OBJ_POWERUP && Objects[i].contains_id == POW_KEY_BLUE)))
				blueDoorPending = 0;
		if (yellowDoorPending)
			if ((Objects[i].type == OBJ_POWERUP && Objects[i].id == POW_KEY_GOLD) || (Objects[i].type == OBJ_ROBOT && (Objects[i].contains_type == OBJ_POWERUP && Objects[i].contains_id == POW_KEY_GOLD)))
				yellowDoorPending = 0;
		if (redDoorPending)
			if ((Objects[i].type == OBJ_POWERUP && Objects[i].id == POW_KEY_RED) || (Objects[i].type == OBJ_ROBOT && (Objects[i].contains_type == OBJ_POWERUP && Objects[i].contains_id == POW_KEY_RED)))
				redDoorPending = 0;
	}
	if (blueDoorPending || yellowDoorPending || redDoorPending) {
		if (Current_level_num > 0)
			Ranking.mergeLevels = 1;
		else
			Ranking.secretMergeLevels = 1;
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
int create_path_partime(int start_seg, int target_seg, point_seg** path_start, int* path_count, partime_calc_state* state, partime_objective objective)
{
	object* objp = ConsoleObject;
	short player_path_length = 0;
	ConsoleObject->segnum = start_seg; // We're gonna teleport the player to every one of the starting segments, then put him back at spawn in time for the level to start.

	// With the previous system of determining whether a path was completable, if a grated off area connected one sectors of a level with an otherwise locked off one, it could be used to bypass a locked door, which could make par times impossible.
	// I thought this was rather rare, then Algo did it on D2 level 10 with wall 12/13 lol. Now it should actually be rare, and less likely to cause impossibility if it occurs.
	create_path_points(objp, objp->segnum, target_seg, Point_segs_free_ptr, &player_path_length, MAX_POINT_SEGS, 0, 0, -1, objective.type, objective.ID);

	*path_start = Point_segs_free_ptr;
	*path_count = player_path_length;

	if (!Ranking.parTimeRuns) {
		if (Point_segs[player_path_length - 1].segnum != target_seg) { // Don't consider this objective in the contest for shortest path if it isn't accessible. We handle those later.
			for (int i = 0; i < Ranking.numInaccessibleObjectives; i++)
				if (Ranking.inaccessibleObjectiveTypes[i] == objective.type && Ranking.inaccessibleObjectiveIDs[i] == objective.ID)
					return 0; // We already added this objective to the inaccessible list, no need to add it again. Before I added this check, level's took ages to load due to hundreds of inaccessibles.
			if (objective.type) {
				// If we're pathing to an inaccessible objective, set Algo's position to the closest accessible point. At the objective will softlock it.
				// Due to how taxing create_path_partime is (and because I'm lazy lmao), we'll use vm_vec_dist to determine this instead of actually measuring the path correctly.
				// This will return the incorrect segnum when it comes to looping pathways, but this is better than the other alternatives.
				// Those were not updating its position at all (caused really long retread paths), and not counting movement time for inaccessible objectives at all (caused potentially impossible par times).
				// The good part about this compromise is that when it's incorrect, it's in the upper direction, so par times will be easy at worst instead of impossible.
				int nearestSegnum;
				int shortestDistance = -1;
				int distance = -1;
				vms_vector start;
				vms_vector finish;
				compute_segment_center(&finish, &Segments[getObjectiveSegnum(objective)]);
				for (int p = 0; p < player_path_length; p++) {
					compute_segment_center(&start, &Segments[Point_segs[p].segnum]);
					distance = vm_vec_dist(&start, &finish);
					if (distance < shortestDistance || shortestDistance == -1) {
						shortestDistance = distance;
						nearestSegnum = Point_segs[p].segnum;
					}
				}
				Ranking.inaccessibleObjectiveLastSegnums[Ranking.numInaccessibleObjectives] = nearestSegnum;
				Ranking.inaccessibleObjectiveTypes[Ranking.numInaccessibleObjectives] = objective.type;
				Ranking.inaccessibleObjectiveIDs[Ranking.numInaccessibleObjectives] = objective.ID;
				Ranking.numInaccessibleObjectives++;
			}
			return 0;
		}
	}

	return 1;
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

double calculate_path_length_partime(partime_calc_state* state, point_seg* path, int path_count, partime_objective objective)
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
			// Now, we account for the time it'd take to fight walls on the path (Abyss 1.0 par time hotfix lol). Originally I accounted for matcen fight time as well, but the change did more harm than good.
			int wall_num = Segments[path[i].segnum].sides[find_connecting_side(path[i].segnum, path[i + 1].segnum)].wall_num;
			if (wall_num > -1) {
				if (Walls[wall_num].type == WALL_BLASTABLE) {
					int skip = 0; // So it skips incrementing this path's time for a wall already marked done.
					for (int n = 0; n < state->doneWallsSize; n++)
						if (state->doneWalls[n] == wall_num)
							skip = 1; // If algo's already destroyed this wall, don't account for it.
					if (!skip)
						state->pathObstructionTime += calculate_combat_time_wall(state, wall_num, 0);
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

int thisWallUnlocked(int wall_num, int currentObjectiveType, int currentObjectiveID)
{
	for (int i = 0; i < Ranking.numCurrentlyLockedWalls; i++)
		if (Ranking.currentlyLockedWalls[i] == wall_num)
			return 0;
	for (int link_num = 0; link_num < ControlCenterTriggers.num_links; link_num++) // Don't wanna move state to ai.c just to access reactor walls.
		if (ControlCenterTriggers.seg[link_num] == Walls[wall_num].segnum && ControlCenterTriggers.side[link_num] == Walls[wall_num].sidenum && Ranking.parTimeLoops < 2)
			return 0;
	return 1;
}

partime_objective find_nearest_objective_partime(partime_calc_state* state, int addUnlocksToObjectiveList,
	int start_seg, partime_objective* objectiveList, int objectiveListSize, point_seg** path_start, int* path_count, double* path_length)
{
	double pathLength;
	double shortestPathLength = -1;
	partime_objective nearestObjective;
	partime_objective objective;
	int i;

	vms_vector start;
	compute_segment_center(&start, &Segments[start_seg]);

	for (i = 0; i < objectiveListSize; i++) {
		objective = objectiveList[i];
		// Draw a path as far as we can to the objective, avoiding currently locked doors. If we don't make it all the way, ignore any closed walls. Primarily for shooting through grates, but prevents a softlock on actual uncompletable levels.
		if (objective.type == OBJECTIVE_TYPE_WALL && !thisWallUnlocked(objective.ID, -1, -1)) // If we're shooting the unlockable side of a one-sided locked wall, make sure we have the keys needed to unlock it first.
			continue;
		if (!create_path_partime(start_seg, getObjectiveSegnum(objective), path_start, path_count, state, objective))
			continue; // We can't reach this objective right now; find the next one.
		pathLength = calculate_path_length_partime(state, *path_start, *path_count, objective);
		if (pathLength < shortestPathLength || shortestPathLength < 0) {
			shortestPathLength = pathLength;
			nearestObjective = objective;
			state->shortestPathObstructionTime = state->pathObstructionTime; // So the wrong amount isn't subtracted when it's time to add the real path length to movement time.
		}
	}

	// Did we find a legal objective? Return that.
	if (shortestPathLength >= 0) {
		// Regenerate the path since we may have checked something else in the meantime.
		create_path_partime(start_seg, getObjectiveSegnum(nearestObjective), path_start, path_count, state, objective);
		*path_length = shortestPathLength;
		if (Ranking.parTimeRuns) // DON'T update segnum or lastPosition if we just pathed to an inaccessible objective. That would lock Algo in a cage!
			for (i = 0; i < Ranking.numInaccessibleObjectives; i++)
				if (Ranking.inaccessibleObjectiveTypes[i] == nearestObjective.type && Ranking.inaccessibleObjectiveIDs[i] == nearestObjective.ID) {
					state->segnum = Ranking.inaccessibleObjectiveLastSegnums[i];
					vms_vector segmentCenter;
					compute_segment_center(&segmentCenter, &Segments[state->segnum]);
					state->lastPosition = segmentCenter;
					return nearestObjective;
				}
		state->segnum = getObjectiveSegnum(nearestObjective);
		state->lastPosition = getObjectivePosition(nearestObjective);
		return nearestObjective;
	}
	else {
		// No reachable objectives in list.
		partime_objective emptyResult = { OBJECTIVE_TYPE_INVALID, 0 };
		return emptyResult;
	}
}

int do_we_have_this_weapon(partime_calc_state* state, int weapon_id)
{
	for (int n = 0; n < state->num_weapons; n++) {
		if (state->heldWeapons[n] == weapon_id)
			return 1;
	}
	return 0;
}

int getMatcenSegnum(int matcen_num)
{
	for (int i = 0; i < Highest_segment_index; i++)
		if (Segments[i].matcen_num == matcen_num)
			return i;
}

void check_for_walls_and_matcens_partime(partime_calc_state* state, point_seg* path, int path_count)
{
	// First, let's check if the ship can actually fit through this path. Thanks to Proxy for help with this part.
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
					state->combatTime += calculate_combat_time_wall(state, wall_num, 1);
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
					if (Triggers[Walls[wall_num].trigger].type & TT_MATCEN && !(state->matcenTriggers[Walls[wall_num].trigger] && Triggers[Walls[wall_num].trigger].type & TF_ONE_SHOT)) { // If this trigger is a matcen type... (and isn't a one time trigger that's been hit already)
						double matcenTime = 0;
						double totalMatcenTime = 0;
						for (int c = 0; c < Triggers[Walls[wall_num].trigger].num_links; c++) { // Repeat this loop for every segment linked to this trigger.
							if (Segments[Triggers[Walls[wall_num].trigger].seg[c]].special == SEGMENT_IS_ROBOTMAKER) { // Check them to see if they're matcens. 
								segment* segp = &Segments[Triggers[Walls[wall_num].trigger].seg[c]]; // Whenever one is, set this variable as a shortcut so we don't have to put that long string of text every time.
								if (RobotCenters[segp->matcen_num].robot_flags[0] != 0 && state->matcenLives[segp->matcen_num] > 0) { // If the matcen has robots in it, and isn't dead or on cooldown, consider it triggered...
									uint	flags;
									sbyte	legal_types[64];		//	64 bits, the width of robot_flags.
									int	num_types, robot_index, r;
									robot_index = 0;
									num_types = 0;
									for (r = 0; r < 2; r++) {
										robot_index = r * 32;
										flags = RobotCenters[segp->matcen_num].robot_flags[r];
										while (flags) {
											if (flags & 1)
												legal_types[num_types++] = robot_index;
											flags >>= 1;
											robot_index++;
										}
									}
									// Find the average fight time for the robots in this matcen and multiply that by the spawn count on this difficulty.
									int n;
									double totalRobotTime = 0;
									double totalEnergyUsage = 0;
									double totalAmmoUsage = 0;
									double averageRobotTime = 0;
									for (n = 0; n < num_types; n++) {
										robot_info* robInfo = &Robot_info[legal_types[n]];
										if (!(robInfo->behavior == AIB_RUN_FROM || robInfo->thief)) // Skip running bots and thieves.
											totalRobotTime += calculate_combat_time_matcen(state, robInfo);
										totalEnergyUsage += state->energy_usage;
										totalAmmoUsage += state->ammo_usage;
									}
									averageRobotTime = totalRobotTime / num_types;
									matcenTime += averageRobotTime * (Difficulty_level + 3);
									state->matcenLives[segp->matcen_num]--;
									state->matcenTriggers[Walls[wall_num].trigger]++; // Increment the number of times this specific trigger was hit, so one time triggers won't work after this, even if its matcen has lives left.
									state->simulatedEnergy -= (totalEnergyUsage / num_types) * (f1_0 * (Difficulty_level + 3)); // Do the same for energy
									state->vulcanAmmo -= ((totalAmmoUsage / num_types) * (f1_0 * (Difficulty_level + 3))) * f1_0; // and ammo, as those also change per matcen.
									if (matcenTime > 0)
										printf("Fought matcen %i at segment %i; lives left: %i\n", segp->matcen_num, getMatcenSegnum(segp->matcen_num), state->matcenLives[segp->matcen_num]);
									totalMatcenTime += averageRobotTime; // Add up the average fight times of each link so we can add them to the minimum time later.
								}
							}
						}
						// There's a minimum time for all matcen robots spawned on this path to be killed.
						if (matcenTime > 0 && matcenTime < 3.5 * (Difficulty_level + 2) + totalMatcenTime) {
							matcenTime = 3.5 * (Difficulty_level + 2) + totalMatcenTime;
							if (Ranking.parTimeRuns)
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

void update_energy_for_path_partime(partime_calc_state* state, point_seg* path, int path_count)
{
	// How much energy do we pick up while following this path?
	for (int i = 0; i < path_count; i++) {
		// DON'T set Algo's energy to 100 when it goes through a fuelcen. We'll be manually inserting visits to them in later, so we don't wanna double dip!
		// If there are energy powerups in this segment, collect them.
		for (int objNum = 0; objNum <= Highest_object_index; objNum++) { // This next if line's gonna be long. Basically making sure any of the weapons in the condition only give energy if we already have them.
			if (Objects[objNum].type == OBJ_POWERUP && (Objects[objNum].id == POW_ENERGY || Objects[objNum].id == POW_VULCAN_AMMO || (Objects[objNum].id == POW_VULCAN_WEAPON && do_we_have_this_weapon(state, VULCAN_ID)) || (Objects[objNum].id == POW_SPREADFIRE_WEAPON && do_we_have_this_weapon(state, SPREADFIRE_ID)) || (Objects[objNum].id == POW_PLASMA_WEAPON && do_we_have_this_weapon(state, PLASMA_ID)) || (Objects[objNum].id == POW_FUSION_WEAPON && do_we_have_this_weapon(state, FUSION_ID)) || (Objects[objNum].id == POW_LASER && state->heldWeapons[0] < LASER_ID_L4) || (Objects[objNum].id == POW_SUPER_LASER && state->heldWeapons[0] < LASER_ID_L6) || (Objects[objNum].id == POW_QUAD_FIRE && !state->hasQuads)) && Objects[objNum].segnum == path[i].segnum) {
				// ...make sure we didn't already get this one
				int thisSourceCollected = 0;
				for (int j = 0; j < state->doneListSize; j++)
					if (state->doneList[j].type == OBJECTIVE_TYPE_OBJECT && state->doneList[j].ID == objNum) {
						thisSourceCollected = 1;
						break;
					}
				if (!thisSourceCollected) {
					if (Objects[objNum].id == POW_VULCAN_AMMO || Objects[objNum].id == POW_VULCAN_WEAPON || Objects[objNum].id == POW_GAUSS_WEAPON)
						state->vulcanAmmo += STARTING_VULCAN_AMMO / 2;
					else
						state->simulatedEnergy += state->energy_gained_per_pickup;
					partime_objective energyObjective = { OBJECTIVE_TYPE_OBJECT, objNum };
					addObjectiveToList(state->doneList, &state->doneListSize, energyObjective, 1);
				}
			}
		}
	}
}

void update_energy_for_objective_partime(partime_calc_state* state, partime_objective objective)
{
	// How much energy does it take to complete this objective?
	if (objective.type == OBJECTIVE_TYPE_OBJECT) { // We don't fight triggers.
		object* obj = &Objects[objective.ID];
		if (obj->type == OBJ_POWERUP && (obj->id == POW_LASER || obj->id == POW_QUAD_FIRE || obj->id == POW_VULCAN_WEAPON || obj->id == POW_SPREADFIRE_WEAPON || obj->id == POW_PLASMA_WEAPON || obj->id == POW_FUSION_WEAPON || obj->id == POW_SUPER_LASER || obj->id == POW_GAUSS_WEAPON || obj->id == POW_HELIX_WEAPON || obj->id == POW_PHOENIX_WEAPON || obj->id == POW_OMEGA_WEAPON)) {
			int weapon_id = 0;
			if (obj->id == POW_VULCAN_WEAPON)
				weapon_id = VULCAN_ID;
			if (obj->id == POW_SPREADFIRE_WEAPON)
				weapon_id = SPREADFIRE_ID;
			if (obj->id == POW_PLASMA_WEAPON)
				weapon_id = PLASMA_ID;
			if (obj->id == POW_FUSION_WEAPON)
				weapon_id = FUSION_ID;
			if (obj->id == POW_GAUSS_WEAPON)
				weapon_id = GAUSS_ID;
			if (obj->id == POW_HELIX_WEAPON)
				weapon_id = HELIX_ID;
			if (obj->id == POW_PHOENIX_WEAPON)
				weapon_id = PHOENIX_ID;
			if (obj->id == POW_OMEGA_WEAPON)
				weapon_id = OMEGA_ID;
			if (weapon_id) { // If the powerup we got is a new weapon, add it to the list of weapons algo has.
				if (do_we_have_this_weapon(state, weapon_id)) { // Weapons you already have give energy/ammo.
					if (weapon_id == VULCAN_ID || weapon_id == GAUSS_ID)
						state->vulcanAmmo += STARTING_VULCAN_AMMO / 2;
					else
						state->simulatedEnergy += state->energy_gained_per_pickup * obj->contains_count;
				}
				else {
					state->heldWeapons[state->num_weapons] = weapon_id;
					state->num_weapons++;
					if (weapon_id == VULCAN_ID || weapon_id == GAUSS_ID)
						state->vulcanAmmo += STARTING_VULCAN_AMMO;
				}
			}
			else {
				if (obj->id == POW_LASER) {
					if (state->heldWeapons[0] < LASER_ID_L4)
						state->heldWeapons[0]++;
					else
						state->simulatedEnergy += state->energy_gained_per_pickup;
				}
				if (obj->id == POW_SUPER_LASER) {
					if (state->heldWeapons[0] < LASER_ID_L6) {
						if (state->heldWeapons[0] < LASER_ID_L5)
							state->heldWeapons[0] = LASER_ID_L5;
						else
							state->heldWeapons[0] = LASER_ID_L6;
					}
					else
						state->simulatedEnergy += state->energy_gained_per_pickup;
				}
				if (obj->id == POW_QUAD_FIRE) {
					if (!state->hasQuads)
						state->hasQuads = 1;
					else
						state->simulatedEnergy += state->energy_gained_per_pickup;
				}
			}
		}
		if (obj->type == OBJ_ROBOT || obj->type == OBJ_CNTRLCEN) { // We don't fight keys, hostages or weapons.
			robot_info* robInfo = &Robot_info[obj->id];
			double combatTime = calculate_combat_time(state, obj, robInfo);
			double highestTeleportDistance = -1;
			double teleportDistance = 0;
			short Boss_path_length = 0;
			double totalRobotTime = 0;
			double totalEnergyUsed = 0;
			double totalAmmoUsed = 0;
			double averageRobotTime = -1;
			int num_robot_types = 24; // How many robot types there are, which can probably be changed, but the one starting with a capital N doesn't work for some reason...
			int num_valid_robot_types = 0; // This one excludes bosses when computing average health.
			double teleportTime;
			state->combatTime += combatTime;
			if (robInfo->boss_flag > 0) { // Bosses have special abilities that take additional time to counteract. Boss levels are unfair without this.
				state->combatTime += 6; // The player must wait out potentially all of a six-second death roll. This could be merged into the movement time following the boss' death, but it's not that important.
				if (Boss_teleports[robInfo->boss_flag]) {
					int num_teleports = combatTime / 2; // Bosses teleport two seconds after you start damaging them, meaning you can only get two seconds of damage in at a time before they move away.
					for (int i = 0; i < Num_boss_teleport_segs; i++) { // Now we measure the distance between every possible pair of points the boss can teleport between, then add the furthest one for each teleport.
						for (int n = 0; n < Num_boss_teleport_segs; n++) {
							create_path_points(obj, Boss_teleport_segs[i], Boss_teleport_segs[n], Point_segs_free_ptr, &Boss_path_length, 100, 0, 0, -1, 0, obj->id);
							for (int c = 0; c < Boss_path_length - 1; c++)
								teleportDistance += vm_vec_dist(&Point_segs[c].point, &Point_segs[c + 1].point);
						}
					}
					teleportTime = (teleportDistance / pow(Num_boss_teleport_segs, 2)) * num_teleports; // Account for the average teleport distance, not highest.
					state->movementTime += teleportTime / SHIP_MOVE_SPEED;
					printf("Teleport time: %.3fs\n", teleportTime / SHIP_MOVE_SPEED);
				}
				// This commented code gives more time to account for bosses spawning robots, but it gave too much in general from my experience. Perhaps its inaccurate implementation was to blame, but I don't find doing it right to be worth the effort.
				//if (Boss_spews_bots_energy[robInfo->boss_flag - BOSS_D2] || Boss_spews_bots_matter[robInfo->boss_flag - BOSS_D2]) { // For right now, just give the bonus if the boss is capable of spawning bots at all. Perhaps eventually weapon use will factor into this.
					//for (int i = 0; i < num_robot_types; i++) { // Find the one that takes the longest to kill and add it for every five seconds it takes to kill the boss, including teleportTime.
						// More yapping: Could include potential time to travel to the spawned robots as well, but there's no way to really do that reliably.
						//if (!Robot_info[i].boss_flag - BOSS_D2) { // Bosses can't spawn other bosses lol.
							//totalRobotTime += calculate_combat_time_matcen(state, &Robot_info[i]); // Use the matcen version, as these robots are teleported in the same way matcen ones are.
							//totalEnergyUsed += state->energy_usage;
							//totalAmmoUsed += state->ammo_usage;
							//num_valid_robot_types++;
						//}
					//}
					//averageRobotTime = totalRobotTime / num_valid_robot_types;
					//int robotSpawns = (combatTime + teleportTime / SHIP_MOVE_SPEED) / (5 - (Boss_spew_more[robInfo->boss_flag - BOSS_D2] * 1.6666667)); // boss_spew_more being set makes enemies spawn at 1.5x the rate on average, so act like bots spawn every 3.33s instead of 5 in that case.
					//combatTime = averageRobotTime * robotSpawns;
					//state->combatTime += combatTime;
					//state->simulatedEnergy -= ((totalEnergyUsed / num_valid_robot_types) * robotSpawns) * f1_0; // Also account for energy used fighting them
					//state->vulcanAmmo -= ((totalAmmoUsed / num_valid_robot_types) * robotSpawns) * f1_0; // and ammo.
					//printf("Took %.3fs to deal with minions.\n", combatTime);
				//}
			}
			double addEnergy; // We have to do this to prevent data loss.
			if (obj->contains_type == OBJ_POWERUP && obj->contains_id == POW_ENERGY) {
				// If the robot is guaranteed to drop energy, give it to algo so it doesn't visit fuelcens more than needed.
				addEnergy = state->energy_gained_per_pickup* obj->contains_count;
				if (state->simulatedEnergy >= i2f(100)) // In D2, energy pickups have a chance to just not spawn if your energy is high enough. Account for that.
					addEnergy *= 0.5;
				else if (state->simulatedEnergy >= i2f(150))
					addEnergy *= 0.25;
				state->simulatedEnergy += addEnergy;
			}
			else if (Robot_info[obj->id].contains_type == OBJ_POWERUP && Robot_info[obj->id].contains_id == POW_ENERGY) { // Now account for RNG energy drops to throw Algo another bone, to make extra sure it doesn't have to go back to that crappy fuelcen again.
				addEnergy = (((double)robInfo->contains_count * (double)robInfo->contains_prob) / 16) * state->energy_gained_per_pickup;
				if (state->simulatedEnergy >= i2f(100))
					addEnergy *= 0.5;
				else if (state->simulatedEnergy >= i2f(150))
					addEnergy *= 0.25;
				state->simulatedEnergy += addEnergy;
			}
			if (obj->contains_type == OBJ_POWERUP && obj->contains_id == POW_VULCAN_AMMO) { // Now repeat with ammo.
				state->vulcanAmmo += (STARTING_VULCAN_AMMO / 2) * obj->contains_count;
			}
			else if (Robot_info[obj->id].contains_type == OBJ_POWERUP && Robot_info[obj->id].contains_id == POW_VULCAN_AMMO) {
				double addAmmo = (((double)robInfo->contains_count * (double)robInfo->contains_prob) / 16) * (STARTING_VULCAN_AMMO / 2); // We have to do this because data loss.
				state->vulcanAmmo += addAmmo;
			}
			if (obj->contains_type == OBJ_POWERUP && (obj->contains_id == POW_LASER || obj->contains_id == POW_QUAD_FIRE || obj->contains_id == POW_VULCAN_WEAPON || obj->contains_id == POW_SPREADFIRE_WEAPON || obj->contains_id == POW_PLASMA_WEAPON || obj->contains_id == POW_FUSION_WEAPON || obj->contains_id == POW_SUPER_LASER || obj->contains_id == POW_GAUSS_WEAPON || obj->contains_id == POW_HELIX_WEAPON || obj->contains_id == POW_PHOENIX_WEAPON || obj->contains_id == POW_OMEGA_WEAPON)) {
				int weapon_id = 0;
				if (obj->id == POW_VULCAN_WEAPON)
					weapon_id = VULCAN_ID;
				if (obj->id == POW_SPREADFIRE_WEAPON)
					weapon_id = SPREADFIRE_ID;
				if (obj->id == POW_PLASMA_WEAPON)
					weapon_id = PLASMA_ID;
				if (obj->id == POW_FUSION_WEAPON)
					weapon_id = FUSION_ID;
				if (obj->id == POW_GAUSS_WEAPON)
					weapon_id = GAUSS_ID;
				if (obj->id == POW_HELIX_WEAPON)
					weapon_id = HELIX_ID;
				if (obj->id == POW_PHOENIX_WEAPON)
					weapon_id = PHOENIX_ID;
				if (obj->id == POW_OMEGA_WEAPON)
					weapon_id = OMEGA_ID;
				if (weapon_id) { // If the powerup we got is a new weapon, add it to the list of weapons algo has.
					if (do_we_have_this_weapon(state, weapon_id)) { // Weapons you already have give energy/ammo.
						if (weapon_id == VULCAN_ID || weapon_id == GAUSS_ID)
							state->vulcanAmmo += STARTING_VULCAN_AMMO / 2;
						else
							state->simulatedEnergy += state->energy_gained_per_pickup * obj->contains_count;
					}
					else {
						state->heldWeapons[state->num_weapons] = weapon_id;
						state->num_weapons++;
						if (weapon_id == VULCAN_ID || weapon_id == GAUSS_ID)
							state->vulcanAmmo += STARTING_VULCAN_AMMO;
					}
				}
				else {
					if (obj->contains_id == POW_LASER) {
						if (state->heldWeapons[0] < LASER_ID_L4)
							state->heldWeapons[0]++;
						else
							state->simulatedEnergy += state->energy_gained_per_pickup * obj->contains_count;
					}
					if (obj->contains_id == POW_SUPER_LASER) {
						if (state->heldWeapons[0] < LASER_ID_L6) {
							if (state->heldWeapons[0] < LASER_ID_L5)
								state->heldWeapons[0] = LASER_ID_L5;
							else
								state->heldWeapons[0] = LASER_ID_L6;
						}
						else
							state->simulatedEnergy += state->energy_gained_per_pickup * obj->contains_count;
					}
					if (obj->contains_id == POW_QUAD_FIRE) {
						if (!state->hasQuads)
							state->hasQuads = 1;
						else
							state->simulatedEnergy += state->energy_gained_per_pickup * obj->contains_count;
					}
				}
			}
		}
	}
}

int getParTimeWeaponID(int index)
{
	int weaponIDs[10] = { 0, VULCAN_ID, SPREADFIRE_ID, PLASMA_ID, FUSION_ID, 0, GAUSS_ID, HELIX_ID, PHOENIX_ID, OMEGA_ID };
	if (Players[Player_num].laser_level < 5)
		weaponIDs[0] = Players[Player_num].laser_level;
	else
		weaponIDs[0] = Players[Player_num].laser_level + 25;
	return weaponIDs[index];
}

double findEnergyTime(partime_calc_state* state, partime_objective* objectiveList, int startIndex) // Props to Sirius for help with energy time.
{
	return 0; // Disabled for now.
	// This function is in charge of determining the mimimum time a player needs to refill their energy in a given level, then adding that to its par time.
	// Keep in mind this function isn't perfect lol. It assumes all fuelcens are accessible and unguarded at any time, and that the player follows Algo's exact actions, only refueling from and back to objective nodes.
	if (!state->numEnergyCenters)
		return 0; // This level has no fuelcens. Can't spend any time travelling to or refilling in one.
	int objectiveSegments[MAX_OBJECTS + MAX_TRIGGERS + MAX_WALLS];
	double objectiveEnergies[MAX_OBJECTS + MAX_TRIGGERS + MAX_WALLS];
	double objectiveFuelcenTripTimes[MAX_OBJECTS + MAX_TRIGGERS + MAX_WALLS]; // This array is in charge of tracking the travel time to and from the nearest fuelcen, starting at the segment of objective X.
	// With that, we don't have to do thousands of expensive pathfinding operations.
	double pathLength; // Store create_path_partime's result in pathLength to compare to current shortest.
	point_seg* path_start; // The current path we are looking at (this is a pointer into somewhere in Point_segs).
	int path_count; // The number of segments in the path we're looking at.
	double increaseEnergiesBy;
	for (int i = 0; i < state->objectives; i++) { // Now let's set our local arrays to match the official ones, filling in the trip times for all of the segments Algo visited.
		objectiveSegments[i] = state->objectiveSegments[i];
		objectiveEnergies[i] = state->objectiveEnergies[i];
		if (Segments[objectiveSegments[i]].special == SEGMENT_IS_FUELCEN) // No need to measure distance to a fuelcen if we're already at a fuelcen.
			objectiveFuelcenTripTimes[i] = 0;
		else {
			find_nearest_objective_partime(&state, 0, objectiveSegments[i], state->energyCenters, state->numEnergyCenters, &path_start, &path_count, &pathLength);
			objectiveFuelcenTripTimes[i] = (pathLength / SHIP_MOVE_SPEED) * 2; // Doing *2 here to account for the trip back, so it doesn't have to be done even more outside of this.
		}
	}
	double minTime = INFINITY;
	double energyTime = 0;
	int refuel = 0;
	for (int i = 0; i < state->objectives; i++)
		if (objectiveEnergies[i] <= 0)
			refuel = 1;
	if (!refuel)
		return 0; // we don't need to refuel
	for (int refillIndex = startIndex; refillIndex < state->objectives; refillIndex++) {
		if (objectiveEnergies[refillIndex] < 100) { // Only attempt a simulated refill where energy at the given point is low enough.
			increaseEnergiesBy = 100 - objectiveEnergies[refillIndex];
			// Cap the increase at 100 because player energy can't actually be negative. Also to handle super negative energy values as multiple required visits at the same objective (having to refill multiple times to defeat an ungodly beefy robot).
			if (increaseEnergiesBy > 100)
				increaseEnergiesBy = 100;
			for (int i = refillIndex; i < state->objectives; i++) {
				objectiveEnergies[i] += increaseEnergiesBy;
				if (objectiveEnergies[i] > 200)
					objectiveEnergies[i] = 200; // Energy can't be above 200 at any point.
			}
			energyTime = objectiveFuelcenTripTimes[refillIndex] + (increaseEnergiesBy / 25) + findEnergyTime(&state, objectiveList, refillIndex + 1); // increaseEnergiesBy / 25 is the time spent sitting in the fuelcen recharging.
			if (energyTime < minTime)
				minTime = energyTime;
		}
		else if (startIndex < state->objectives) // If it's not, skip ahead and try again as long as there's still stuff left.
			continue;
	}
	return minTime;
}

double calculateParTime() // Here is where we have an algorithm run a simulated path through a level to determine how long the player should take, both flying around and fighting robots.
{ // January 2024 me would crap himself if he saw this actually working lol.
	partime_calc_state state = { 0 }; // Initialize the algorithm's state. We'll call it Algo for short.
	Ranking.parTimeRuns = 0;
	Ranking.numInaccessibleObjectives = 0;
	for (int i = 0; i <= Highest_segment_index; i++) { // Iterate through every side of every segment, measuring their sizes.
		for (int s = 0; s < 6; s++) {
			if (Segments[i].children[s] > -1) { // Don't measure closed sides. We can't go through them anyway.
				// Measure the distance between all of that side's verts to determine whether we can fit. ai_door_is_openable will use them later to disallow passage if we can't.
				int a = vm_vec_dist(&Vertices[Segments[i].verts[Side_to_verts[s][0]]], &Vertices[Segments[i].verts[Side_to_verts[s][1]]]);
				int b = vm_vec_dist(&Vertices[Segments[i].verts[Side_to_verts[s][1]]], &Vertices[Segments[i].verts[Side_to_verts[s][2]]]);
				int c = vm_vec_dist(&Vertices[Segments[i].verts[Side_to_verts[s][2]]], &Vertices[Segments[i].verts[Side_to_verts[s][3]]]);
				int d = vm_vec_dist(&Vertices[Segments[i].verts[Side_to_verts[s][3]]], &Vertices[Segments[i].verts[Side_to_verts[s][0]]]);
				int min_x = max(a, c);
				int min_y = max(b, d);
				Ranking.parTimeSideSizes[i][s] = min(min_x, min_y);
			}
			else
				Ranking.parTimeSideSizes[i][s] = ConsoleObject->size * 2; // If a side is closed, mark it down as big enough.
		}
	}
	fix64 start_timer_value, end_timer_value; // For tracking how long this algorithm takes to run.
	while (Ranking.parTimeRuns < 2) {
		state.movementTime = 0; // Variable to track how much distance it's travelled.
		state.combatTime = 0; // Variable to track how much fighting it's done.
		// Now clear its checklists.
		state.toDoListSize = 0;
		state.doneListSize = 0;
		state.blackListSize = 0;
		int initialSegnum = ConsoleObject->segnum; // Version of segnum that stays at its initial value, to ensure the player is put in the right spot.
		state.segnum = initialSegnum; // Start Algo off where the player spawns.
		state.lastPosition = ConsoleObject->pos; // Both in segnum and in coordinates. (Shoutout to Maximum level 17's quads being at spawn for letting me catch this.)
		int lastSegnum = initialSegnum; // So the printf showing paths to and from segments works.
		int i;
		int j;
		Ranking.parTimeLoops = 0; // How many times the pathmaking process has repeated. This determines what toDoList is populated with, to make sure things are gone to in the right order.
		double pathLength; // Store create_path_partime's result in pathLength to compare to current shortest.
		double matcenTime = 0; // Debug variable to see how much time matcens are adding to the par time.
		point_seg* path_start; // The current path we are looking at (this is a pointer into somewhere in Point_segs).
		int path_count; // The number of segments in the path we're looking at.
		state.simulatedEnergy = Players[Player_num].energy; // Start with the player's energy, so fuelcen needs adapt to any extra energy they might have.
		state.vulcanAmmo = Players[Player_num].primary_ammo[1];
		state.doneWallsSize = 0;
		state.num_weapons = 1;
		state.heldWeapons[0] = 0;
		state.hasQuads = 0;
		state.thiefKeys = 0;
		state.matcenTime = 0;
		// Below is code that starts Algo off with the player's current primary loadout, but the idea of par times jumping around for the same level and difficulty was poorly received.
		//state.num_weapons = 0;
		//for (i = 0; i < 10; i++) {
			//if (Players[Player_num].primary_weapon_flags & HAS_FLAG(i)) {
				//if (!(i == 5)) { // Skip 5 since that's the super laser index, which is already being tracked in the normal laser one.
					//state.heldWeapons[state.num_weapons] = getParTimeWeaponID(i);
					//state.num_weapons++;
				//}
			//}
		//}
		//if (Players[Player_num].flags & PLAYER_FLAGS_QUAD_LASERS)
			//state.hasQuads = 1;
		// If calculating par time for a secret level, assume Algo already has everything it could possibly get from the base level, even if the player doesn't actually have them. 
		// This is to prevent players from setting a secret level's par time based on poor weapons, then leaving and grabbing good ones to return with an advantage.
		// This approach isn't perfect, and the perfect approach isn't possible without loading multiple levels at once and drastically increasing load times, so it'll have to do.
		//if (Current_level_num < 0) {
			//state.num_weapons = Ranking.parTimeNumWeapons;
			//for (i = 0; i < 9; i++)
				//state.heldWeapons[i] = Ranking.parTimeHeldWeapons[i];
			//state.hasQuads = Ranking.parTimeHasQuads;
		//}
		state.energy_gained_per_pickup = 3 * F1_0 + 3 * F1_0 * (NDL - Difficulty_level); // From pick_up_energy (powerup.c)
		if (!Difficulty_level)
			state.energy_gained_per_pickup = 27 * F1_0; // Trainee gives 27 energy per pickup in D2, as opposed to D1's 18.

		// Calculate start time.
		timer_update();
		start_timer_value = timer_query();

		// Populate the locked walls list.
		initLockedWalls(&state);

		// Initialize all matcens to 3 lives and guarantee them to be triggerable, unless it's Insane difficulty, then give them basically unlimited.
		for (i = 0; i < Num_robot_centers; i++) {
			if (Difficulty_level == 4)
				state.matcenLives[i] = 999;
			else
				state.matcenLives[i] = 3;
		}

		for (i = 0; i < Num_triggers; i++)
			state.matcenTriggers[i] = 0;

		// And energy stuff.
		for (i = 0; i < Highest_segment_index; i++)
			if (Segments[i].special == SEGMENT_IS_FUELCEN) {
				state.energyCenters[state.numEnergyCenters].type = OBJECTIVE_TYPE_ENERGY;
				state.energyCenters[state.numEnergyCenters].ID = i;
				state.numEnergyCenters++;
			}

		while (Ranking.parTimeLoops < 4) {
			// Collect our objectives at this stage...
			if (Ranking.parTimeLoops == 0) {
				for (i = 0; i <= Highest_object_index; i++) { // Populate the to-do list with all robots, hostages, weapons, and laser powerups. Ignore robots not worth over zero, as the player isn't gonna go for those. This should never happen, but it's just a failsafe. Also ignore any thieves that aren't carrying keys.
					if ((Objects[i].type == OBJ_ROBOT && Robot_info[Objects[i].id].score_value > 0 && !Robot_info[Objects[i].id].boss_flag && !(Robot_info[Objects[i].id].thief && !(Objects[i].contains_type == OBJ_POWERUP && (Objects[i].contains_id == POW_KEY_BLUE || Objects[i].contains_id == POW_KEY_GOLD || Objects[i].contains_id == POW_KEY_RED)))) || Objects[i].type == OBJ_HOSTAGE || (Objects[i].type == OBJ_POWERUP && (Objects[i].id == POW_EXTRA_LIFE || Objects[i].id == POW_LASER || Objects[i].id == POW_QUAD_FIRE || Objects[i].id == POW_VULCAN_WEAPON || Objects[i].id == POW_SPREADFIRE_WEAPON || Objects[i].id == POW_PLASMA_WEAPON || Objects[i].id == POW_FUSION_WEAPON || Objects[i].id == POW_SUPER_LASER || Objects[i].id == POW_GAUSS_WEAPON || Objects[i].id == POW_HELIX_WEAPON || Objects[i].id == POW_PHOENIX_WEAPON || Objects[i].id == POW_OMEGA_WEAPON))) {
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
						create_path_partime(ConsoleObject->segnum, getObjectiveSegnum(objective), &path_start, &path_count, &state, objective); // Pathfind to potentially update inaccessibleObjectives array, in case reactor is grated off.
						addObjectiveToList(state.toDoList, &state.toDoListSize, objective, 0);
						levelHasReactor = 1;
					}
				}
				if (!levelHasReactor) {
					int highestBossScore = 0;
					int targetedBossID = -1;
					for (i = 0; i <= Highest_object_index; i++) {
						if (Objects[i].type == OBJ_ROBOT && Robot_info[Objects[i].id].boss_flag) { // Look at every boss, adding only the highest point value.
							// Killing any boss in levels with multiple kills ALL of them, only giving points for the one directly killed, so the player needs to target the highest-scoring one for the best rank. Give them the time needed for that one.
							// This means players may have to replay levels a bunch to find which boss gives most out of custom bosses/values, but I don't thinks that's a cause for great concern.
							if (Robot_info[Objects[i].id].score_value > highestBossScore) {
								highestBossScore = Robot_info[Objects[i].id].score_value;
								targetedBossID = i;
								state.combatTime += 6; // Each boss has its own deathroll that lasts six seconds one at a time.
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
			if (Ranking.parTimeLoops == 3) { // Put the exit on the list.
				for (i = 0; i <= Num_triggers; i++) {
					if (Triggers[i].type == TT_EXIT || Triggers[i].type == TT_SECRET_EXIT) {
						for (j = 0; j <= Num_walls; j++) {
							if (Walls[j].trigger == i) {
								partime_objective objective = { OBJECTIVE_TYPE_TRIGGER, Walls[j].segnum };
								addObjectiveToList(state.toDoList, &state.toDoListSize, objective, 0);
								i = Num_triggers + 1; // Only add one exit.
							}
						}
					}
				}
			}

			while (state.toDoListSize > 0) {
				// Find which object on the to-do list is the closest, ignoring the reactor/boss if it's not the only thing left.
				partime_objective nearestObjective =
					find_nearest_objective_partime(&state, 1, state.segnum, state.toDoList, state.toDoListSize, &path_start, &path_count, &pathLength);

				if (nearestObjective.type == OBJECTIVE_TYPE_INVALID) {
					// This should only happen if there are no reachable objectives left in the list.
					// If that happens, we're done with this phase.
					break;
				}

				// Mark this objective as done.
				removeObjectiveFromList(state.toDoList, &state.toDoListSize, nearestObjective);
				addObjectiveToList(state.doneList, &state.doneListSize, nearestObjective, 1);
				if (Ranking.parTimeLoops == 1) { // If we just added the reactor to the done list, remove any locked walls neighboring a reactor wall. They should be open too.
					for (i = 0; i < state.numReactorWalls; i++) {
						int adjacent_wall_num = find_connecting_wall(state.reactorWalls[i].wallID);
						for (int w = 0; w < Ranking.numCurrentlyLockedWalls; w++)
							if (Ranking.currentlyLockedWalls[w] == adjacent_wall_num)
								removeLockedWallFromList(w);
					}
				}

				// Track resource consumption and robot HP destroyed.
				// If there's no path and we're doing straight line distance, we have no idea what we'd
				// be crossing through, so tracking resources for the path would be meaningless.
				// We can still check the objective itself, though.
				int hasThisObjective = 0; // If the next object is a weapon/laser level/quads, and algo already has it/is maxed out, skip it. We don't wanna waste time getting redundant powerups.
				if (nearestObjective.type == OBJECTIVE_TYPE_OBJECT && Objects[nearestObjective.ID].type == OBJ_POWERUP) { // I'm splitting up the if conditions this time.
					int weaponIDs[9] = { 0, VULCAN_ID, SPREADFIRE_ID, PLASMA_ID, FUSION_ID, GAUSS_ID, HELIX_ID, PHOENIX_ID, OMEGA_ID };
					int objectIDs[9] = { 0, POW_VULCAN_WEAPON, POW_SPREADFIRE_WEAPON, POW_PLASMA_WEAPON, POW_FUSION_WEAPON, POW_GAUSS_WEAPON, POW_HELIX_WEAPON, POW_PHOENIX_WEAPON, POW_OMEGA_WEAPON };
					for (int n = 1; n < 9; n++) {
						if (Objects[nearestObjective.ID].id == objectIDs[n] && do_we_have_this_weapon(&state, weaponIDs[n]))
							hasThisObjective = 1;
					}
					if (Objects[nearestObjective.ID].id == POW_LASER && state.heldWeapons[0] > LASER_ID_L3)
						hasThisObjective = 1;
					if (Objects[nearestObjective.ID].id == POW_SUPER_LASER && state.heldWeapons[0] == LASER_ID_L6)
						hasThisObjective = 1;
					if (Objects[nearestObjective.ID].id == POW_QUAD_FIRE && state.hasQuads)
						hasThisObjective = 1;
				}
				if (Objects[nearestObjective.ID].type == OBJ_ROBOT) // Only allow one thief to count toward par time per contained key color. (Fixes Bahagad Outbreak level 8.)
					if (Robot_info[Objects[nearestObjective.ID].id].thief)
						if (Objects[nearestObjective.ID].contains_type == OBJ_POWERUP) {
							if (Objects[nearestObjective.ID].contains_id == POW_KEY_BLUE)
								if (state.thiefKeys & KEY_BLUE)
									hasThisObjective = 1;
								else
									state.thiefKeys |= KEY_BLUE;
							if (Objects[nearestObjective.ID].contains_id == POW_KEY_GOLD)
								if (state.thiefKeys & KEY_GOLD)
									hasThisObjective = 1;
								else
									state.thiefKeys |= KEY_GOLD;
							if (Objects[nearestObjective.ID].contains_id == POW_KEY_RED)
								if (state.thiefKeys & KEY_RED)
									hasThisObjective = 1;
								else
									state.thiefKeys |= KEY_RED;
						}
				if (!hasThisObjective) {
					if (path_start != NULL) {
						check_for_walls_and_matcens_partime(&state, path_start, path_count);
						update_energy_for_path_partime(&state, path_start, path_count);
					}
					update_energy_for_objective_partime(&state, nearestObjective); // Do energy stuff.
					// Cap algo's energy and ammo like the player's.
					if (state.simulatedEnergy > MAX_ENERGY)
						state.simulatedEnergy = MAX_ENERGY;
					if (state.vulcanAmmo > STARTING_VULCAN_AMMO * 8)
						state.vulcanAmmo = STARTING_VULCAN_AMMO * 8;
					printf("Now at %.3f energy, %.0f vulcan ammo\n", f2fl(state.simulatedEnergy), f2fl(state.vulcanAmmo));

					int nearestObjectiveSegnum = getObjectiveSegnum(nearestObjective);
					printf("Path from segment %i to %i: %.3fs\n", lastSegnum, nearestObjectiveSegnum, pathLength / SHIP_MOVE_SPEED);
					// Now move ourselves to the objective for the next pathfinding iteration, unless the objective wasn't reachable with just flight, in which case move ourselves as far as we COULD fly.
					state.movementTime += (pathLength - state.shortestPathObstructionTime) / SHIP_MOVE_SPEED;
					lastSegnum = state.segnum;
					state.objectiveSegments[state.objectives] = state.segnum;
					state.objectiveEnergies[state.objectives] = f2fl(state.simulatedEnergy);
					state.objectives++;
				}
			}
			Ranking.parTimeLoops++;
		}
		ConsoleObject->segnum = initialSegnum;

		// Calculate end time.
		timer_update();
		end_timer_value = timer_query();
		Ranking.parTimeRuns++;
	}
	state.energyTime = findEnergyTime(&state, &state.toDoList, 0); // Time to calculate the minimum time spent going to fuelcens for the level.
	if (state.energyTime > state.combatTime)
		state.energyTime = state.combatTime; // Missions can abuse energy time by making the most powerful weapon's energy use absurdly high, so cap it.
	state.movementTime += state.energyTime; // Ultimately energy time is a subsect of movement time because we're, well, moving to and from the energy centers.
	printf("Par time: %.3fs (%.3f movement, %.3f combat) Matcen time: %.3fs, Fuelcen time: %.3fs\nCalculation time: %.3fs\n",
		state.movementTime + state.combatTime,
		state.movementTime,
		state.combatTime,
		state.matcenTime,
		state.energyTime,
		f2fl(end_timer_value - start_timer_value));

	// Store Algo's weapon info to use for secret levels predeterminately so players can't abuse their par times.
	//Ranking.parTimeNumWeapons = state.num_weapons;
	//for (i = 0; i < 10; i++)
		//Ranking.parTimeHeldWeapons[i] = state.heldWeapons[i];
	//Ranking.parTimeHasQuads = state.hasQuads;
	
	// Par time is rounded up to the nearest five seconds so it looks better/legible on the result screen, leaves room for the time bonus, and looks like a human set it.
	return 5 * (1 + (int)(state.movementTime + state.combatTime) / 5);
	// Also because Doom did five second increments.
	// Par time will vary based on difficulty, so the player will always have to go fast for a high time bonus, even on lower difficulties.
}

// - - - - - - - - - -  END OF PAR TIME ALGORITHM STUFF  - - - - - - - - - - \\

// called when the player is starting a new level for normal game mode and restore state
//	Need to deal with whether this is the first time coming to this level or not.  If not the
//	first time, instead of initializing various things, need to do a game restore for all the
//	robots, powerups, walls, doors, etc.
void StartNewLevelSecret(int level_num, int page_in_textures)
{
	ThisLevelTime = 0;

	last_drawn_cockpit = -1;

	if (Newdemo_state == ND_STATE_PAUSED)
		Newdemo_state = ND_STATE_RECORDING;

	if (Newdemo_state == ND_STATE_RECORDING) {
		newdemo_set_new_level(level_num);
		newdemo_record_start_frame(FrameTime);
	}
	else if (Newdemo_state != ND_STATE_PLAYBACK) {

		set_screen_mode(SCREEN_MENU);

		if (First_secret_visit) {
			do_screen_message(TXT_SECRET_EXIT);
		}
		else {
			if (PHYSFSX_exists(SECRETC_FILENAME, 0))
			{
				do_screen_message(TXT_SECRET_EXIT);
			}
			else {
				do_screen_message("Secret level already destroyed.\nAdvancing to level %i.", Current_level_num + 1);
			}
		}
	}

	LoadLevel(level_num, page_in_textures);

	Assert(Current_level_num == level_num); // make sure level set right

	gameseq_init_network_players(); // Initialize the Players array for this level

	HUD_clear_messages();

	automap_clear_visited();

	Viewer = &Objects[Players[Player_num].objnum];

	gameseq_remove_unused_players();

	Game_suspended = 0;

	Control_center_destroyed = 0;
	Ranking.freezeTimer = 0;

	init_cockpit();
	reset_palette_add();

	if (First_secret_visit || (Newdemo_state == ND_STATE_PLAYBACK)) {
		init_robots_for_level();
		init_ai_objects();
		init_smega_detonates();
		init_morphs();
		init_all_matcens();
		reset_special_effects();
		init_exploding_walls();
		StartSecretLevel();
	}
	else {
		if (PHYSFSX_exists(SECRETC_FILENAME, 0))
		{
			int	pw_save, sw_save;

			pw_save = Players[Player_num].primary_weapon;
			sw_save = Players[Player_num].secondary_weapon;
			state_restore_all(1, 1, SECRETC_FILENAME);
			Players[Player_num].primary_weapon = pw_save;
			Players[Player_num].secondary_weapon = sw_save;
			reset_special_effects();
			init_exploding_walls();
			StartSecretLevel();
			// -- No: This is only for returning to base level: set_pos_from_return_segment();
		}
		else {
			do_screen_message("Secret level already destroyed.\nAdvancing to level %i.", Current_level_num + 1);
			return;
		}
	}

	if (First_secret_visit) {
		copy_defaults_to_robot_all();
	}

	init_controlcen_for_level();

	// Say player can use FLASH cheat to mark path to exit.
	Last_level_path_created = -1;

	if (First_secret_visit) {
		Ranking.secretDeathCount = 0;
		Ranking.secretlevel_time = 0;
		Ranking.secretExcludePoints = 0;
		Ranking.secretRankScore = 0;
		Ranking.secretMaxScore = 0;
		Ranking.secretMissedRngSpawn = 0;
		Ranking.secretlast_score = Players[Player_num].score;
		Ranking.secret_hostages_on_board = 0;
		Ranking.secretQuickload = 0;
		Ranking.hostages_secret_level = 0;
		Ranking.fromBestRanksButton = 0; // We need this for starting secret levels too, since the normal start can be bypassed with a save.
		Ranking.num_secret_thief_points = 0;
		Ranking.secretMergeLevels = 0;

		int i;
		int isRankable = 0; // If the level doesn't have a reactor, boss or normal type exit, it can't be beaten and must be given special treatment.
		int highestBossScore = 0;
		for (i = 0; i <= Highest_object_index; i++) {
			if (Objects[i].type == OBJ_ROBOT && Robot_info[Objects[i].id].boss_flag) { // Look at every boss, adding only the highest point value.
				if (Robot_info[Objects[i].id].score_value > highestBossScore)
					highestBossScore = Robot_info[Objects[i].id].score_value;
			}
		}
		Ranking.secretMaxScore = highestBossScore;
		for (i = 0; i <= Highest_object_index; i++) {
			// It has been decided that thieves (and robots within them) will not count toward max score. They're just too annoying and unfun to kill, but will give you a considerable point advantage if you manage to take one down quickly.
			// However, we can't do that here. We have to let them slide for now so the "remains" counter stays accurate. Let's instead count them, then use that number to subtract the points when the level's over.
			if (Objects[i].type == OBJ_ROBOT && !Robot_info[Objects[i].id].boss_flag) { // Ignore bosses, we already decided which one to count before.
				Ranking.secretMaxScore += Robot_info[Objects[i].id].score_value;
				if (Robot_info[Objects[i].id].thief)
					Ranking.num_secret_thief_points += Robot_info[Objects[i].id].score_value;
				if (Objects[i].contains_type == OBJ_ROBOT && ((Objects[i].id != Robot_info[Objects[i].id].contains_id) || (Objects[i].id != Robot_info[Objects[i].contains_id].contains_id))) { // So points in infinite robot drop loops aren't counted past the parent bot.
					Ranking.secretMaxScore += Robot_info[Objects[i].contains_id].score_value * Objects[i].contains_count;
					if (Robot_info[Objects[i].contains_id].thief || Robot_info[Objects[i].id].thief) // If the parent is a thief, exclude it and its children. If the children are thieves, exclude just them.
						Ranking.num_secret_thief_points += Robot_info[Objects[i].contains_id].score_value * Objects[i].contains_count;
				}
				if (Objects[i].contains_type == OBJ_POWERUP && Objects[i].contains_id == POW_EXTRA_LIFE)
					Ranking.secretMaxScore += Objects[i].contains_count * 10000;
			}
			else if (Robot_info[Objects[i].id].boss_flag)
				isRankable = 1; // A boss is present, this level is beatable.
			if (Objects[i].type == OBJ_CNTRLCEN) {
				Ranking.secretMaxScore += CONTROL_CEN_SCORE;
				isRankable = 1; // A reactor is present, this level is beatable.
			}
			if (Objects[i].type == OBJ_HOSTAGE) {
				Ranking.secretMaxScore += HOSTAGE_SCORE;
				Ranking.hostages_secret_level++;
			}
			if (Objects[i].type == OBJ_POWERUP && Objects[i].id == POW_EXTRA_LIFE)
				Ranking.secretMaxScore += 10000;
		}
		Ranking.secretMaxScore = (int)(Ranking.secretMaxScore * 3);
		for (i = 0; i <= Num_triggers; i++) {
			if (Triggers[i].type == TT_EXIT)
				isRankable = 1; // A level-ending exit is present, this level is beatable. Technically the level could still be unbeatable because the exit could be behind unreachable, but who would put an exit there?
		}
		Ranking.secretAlreadyBeaten = 0;
		Ranking.secretParTime = calculateParTime();
		if (Ranking.mergeLevels)
			Ranking.parTime += Ranking.secretParTime;
		if (Ranking.secretMergeLevels)
			Ranking.secretParTime += Ranking.parTime;
		if (calculateRank(Current_mission->last_level - level_num, 0) > 0)
			Ranking.secretAlreadyBeaten = 1;
		if (!isRankable) { // If this level is not beatable, mark the level as beaten with zero points and an S-rank, so the mission can have an aggregate rank.
			PHYSFS_File* temp;
			char filename[256];
			char temp_filename[256];
			sprintf(filename, "ranks/%s/%s/levelS%i.hi", Players[Player_num].callsign, Current_mission->filename, Current_level_num * -1);
			sprintf(temp_filename, "ranks/%s/%s/temp.hi", Players[Player_num].callsign, Current_mission->filename);
			time_t timeOfScore = time(NULL);
			temp = PHYSFS_openWrite(temp_filename);
			PHYSFSX_printf(temp, "%i\n", Ranking.hostages_secret_level);
			PHYSFSX_printf(temp, "0");
			PHYSFSX_printf(temp, "0");
			PHYSFSX_printf(temp, "0");
			PHYSFSX_printf(temp, "0");
			PHYSFSX_printf(temp, "0");
			PHYSFSX_printf(temp, "%i\n", Difficulty_level);
			PHYSFSX_printf(temp, "0");
			PHYSFSX_printf(temp, "0");
			PHYSFSX_printf(temp, "%s\n", Current_level_name);
			PHYSFSX_printf(temp, "%s", ctime(&timeOfScore));
			PHYSFSX_printf(temp, "0");
			PHYSFS_close(temp);
			PHYSFS_delete(filename);
			PHYSFSX_rename(temp_filename, filename);
		}
	}
	First_secret_visit = 0;
}

int	Entered_from_level;

// ---------------------------------------------------------------------------------------------------------------
//	Called from switch.c when player is on a secret level and hits exit to return to base level.
void ExitSecretLevel(void)
{
	if (Newdemo_state == ND_STATE_PLAYBACK)
		return;

	if (Game_wind)
		window_set_visible(Game_wind, 0);

	if (!Control_center_destroyed)
		state_save_all(2, SECRETC_FILENAME, 0);
	else if (Ranking.secretQuickload == 0) // Don't show quickloaders a result screen for secret levels, as that's not in the vanilla game, and quickloaded levels have this mod disabled.
		DoEndSecretLevelScoreGlitz();

	if (PHYSFSX_exists(SECRETB_FILENAME,0))
	{
		int	pw_save, sw_save;

		do_screen_message(TXT_SECRET_RETURN);
		pw_save = Players[Player_num].primary_weapon;
		sw_save = Players[Player_num].secondary_weapon;
		state_restore_all(1, 1, SECRETB_FILENAME);
		Players[Player_num].primary_weapon = pw_save;
		Players[Player_num].secondary_weapon = sw_save;
	} else {
		// File doesn't exist, so can't return to base level.  Advance to next one.
		if (Entered_from_level == Last_level)
			DoEndGame();
		else {
			do_screen_message(TXT_SECRET_ADVANCE);
			StartNewLevel(Entered_from_level+1);
		}
	}

	if (Game_wind)
		window_set_visible(Game_wind, 1);
	reset_time();
	Ranking.rankScore = Players[Player_num].score - Players[Player_num].last_score - Ranking.excludePoints;
}

// ---------------------------------------------------------------------------------------------------------------
//	Set invulnerable_time and cloak_time in player struct to preserve amount of time left to
//	be invulnerable or cloaked.
void do_cloak_invul_secret_stuff(fix64 old_gametime)
{
	if (Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE) {
		fix64	time_used;

		time_used = old_gametime - Players[Player_num].invulnerable_time;
		Players[Player_num].invulnerable_time = GameTime64 - time_used;
	}

	if (Players[Player_num].flags & PLAYER_FLAGS_CLOAKED) {
		fix	time_used;

		time_used = old_gametime - Players[Player_num].cloak_time;
		Players[Player_num].cloak_time = GameTime64 - time_used;
	}
}

// ---------------------------------------------------------------------------------------------------------------
//	Called from switch.c when player passes through secret exit.  That means he was on a non-secret level and he
//	is passing to the secret level.
//	Do a savegame.
void EnterSecretLevel(void)
{
	int i;

	Assert(! (Game_mode & GM_MULTI) );

	if (Game_wind)
		window_set_visible(Game_wind, 0);

	digi_play_sample( SOUND_SECRET_EXIT, F1_0 );	// after above call which stops all sounds
	
	Entered_from_level = Current_level_num;

	Ranking.level_time = (Players[Player_num].hours_level * 3600) + Players[Player_num].time_level;
	if (Control_center_destroyed) {
		Ranking.level_time = (Players[Player_num].hours_level * 3600) + ((double)Players[Player_num].time_level / 65536);
		DoEndLevelScoreGlitz(0);
	}

	if (Newdemo_state != ND_STATE_PLAYBACK)
		state_save_all(1, NULL, 0);	//	Not between levels (ie, save all), IS a secret level, NO filename override

	//	Find secret level number to go to, stuff in Next_level_num.
	for (i=0; i<-Last_secret_level; i++)
		if (Secret_level_table[i]==Current_level_num) {
			Next_level_num = -(i+1);
			break;
		} else if (Secret_level_table[i] > Current_level_num) {	//	Allows multiple exits in same group.
			Next_level_num = -i;
			break;
		}

	if (! (i<-Last_secret_level))		//didn't find level, so must be last
		Next_level_num = Last_secret_level;

	// NMN 04/09/07  Do a REAL start level routine if we are playing a D1 level so we have
	//               briefings
	if (EMULATING_D1)
	{
		set_screen_mode(SCREEN_MENU);
		do_screen_message("Alternate Exit Found!\n\nProceeding to Secret Level!");
		StartNewLevel(Next_level_num);
	} else {
		if (RestartLevel.isResults == 2) // We need restart stuff here to make it work on result screens going into a secret level, so the player doesn't get spat out there anyway.
			StartNewLevel(Current_level_num);
		else
		   	StartNewLevelSecret(Next_level_num, 1);
	}
	// END NMN

	// do_cloak_invul_stuff();
	if (Game_wind)
		window_set_visible(Game_wind, 1);
	Ranking.fromBestRanksButton = 0; // So loading a save doesn't cause the next result screen to let you select difficulty.
	if (Ranking.secretQuickload == 1) {
		HUD_init_message_literal(HM_DEFAULT, "You quickloaded! Ranking system mod disabled for this level.");
		digi_play_sample(SOUND_BAD_SELECTION, F1_0);
	}
	reset_time();
}

//called when the player has finished a level
void PlayerFinishedLevel(int secret_flag)
{
	Assert(!secret_flag);

	if (Game_wind)
		window_set_visible(Game_wind, 0);

	//credit the player for hostages
	Players[Player_num].hostages_rescued_total += Players[Player_num].hostages_on_board;

	if (Game_mode & GM_NETWORK)
	{
		Players[Player_num].connected = CONNECT_WAITING; // Finished but did not die
	}

	last_drawn_cockpit = -1;

	AdvanceLevel(secret_flag);				//now go on to the next one (if one)

	if (Game_wind)
		window_set_visible(Game_wind, 1);
	reset_time();
}

#if defined(D2_OEM) || defined(COMPILATION)
#define MOVIE_REQUIRED 0
#else
#define MOVIE_REQUIRED 1
#endif

#ifdef D2_OEM
#define ENDMOVIE "endo"
#else
#define ENDMOVIE "end"
#endif

void show_order_form();

//called when the player has finished the last level
void DoEndGame(void)
{
	if ((Newdemo_state == ND_STATE_RECORDING) || (Newdemo_state == ND_STATE_PAUSED))
		newdemo_stop_recording(0);

	set_screen_mode( SCREEN_MENU );

	gr_set_current_canvas(NULL);

	key_flush();

	if (PLAYING_BUILTIN_MISSION && !(Game_mode & GM_MULTI))
	{ //only built-in mission, & not multi
		int played=MOVIE_NOT_PLAYED;	//default is not played

		init_subtitles(ENDMOVIE ".tex");	//ingore errors
		played = PlayMovie(ENDMOVIE,MOVIE_REQUIRED);
		close_subtitles();
		if (!played)
		{
			do_end_briefing_screens(Ending_text_filename);
		}
   }
   else if (!(Game_mode & GM_MULTI))    //not multi
   {
		char tname[FILENAME_LEN];

		do_end_briefing_screens (Ending_text_filename);

		//try doing special credits
		sprintf(tname,"%s.ctb",Current_mission_filename);
		credits_show(tname);
	}

	key_flush();

#ifdef NETWORK
	if (Game_mode & GM_MULTI)
		multi_endlevel_score();
	else if (Current_level_num != Last_level)
#endif
		// NOTE LINK TO ABOVE
		DoEndLevelScoreGlitz(0);

	if (PLAYING_BUILTIN_MISSION && !((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP))) {
		gr_set_current_canvas( NULL );
		gr_clear_canvas(BM_XRGB(0,0,0));
		load_palette(D2_DEFAULT_PALETTE,0,1);
		scores_maybe_add_player(0);
	}

	if (Game_wind)
		window_close(Game_wind);		// Exit out of game loop
}

int checkForWarmStart()
{
	// First, check for too much shields, energy, concussion missiles, vulcan ammo or laser level. Also check for quads.
	if (f2fl(Players[Player_num].shields) > 100 || f2fl(Players[Player_num].energy) > 100 || Players[Player_num].secondary_ammo[CONCUSSION_INDEX] > 7 - Difficulty_level || Players[Player_num].primary_ammo[VULCAN_INDEX] || Players[Player_num].laser_level || Players[Player_num].flags & PLAYER_FLAGS_QUAD_LASERS)
		return 1;
	// Next, check for the presence of any weapon besides lasers.
	for (int i = 1; i < 10; i++)
		if (Players[Player_num].primary_weapon_flags & HAS_FLAG(i))
			return 1;
	// Last, check for the presence of any missiles besides concussions.
	for (int i = 1; i < 10; i++)
		if (Players[Player_num].secondary_ammo[i])
			return 1;
	return 0; // Didn't find anything, not a warm start.
}

//called to go to the next level (if there is one)
//if secret_flag is true, advance to secret level, else next normal one
//	Return true if game over.
void AdvanceLevel(int secret_flag)
{
#ifdef NETWORK
	int result;
#endif

	Assert(!secret_flag);

	if (Current_level_num != Last_level) {
#ifdef NETWORK
		if (Game_mode & GM_MULTI)
			multi_endlevel_score();
#endif
	}
	if (!(Game_mode & GM_MULTI))
		// NOTE LINK TO ABOVE!!!
		DoEndLevelScoreGlitz(0);		//give bonuses

	Control_center_destroyed = 0;
	Ranking.freezeTimer = 0;

	#ifdef EDITOR
	if (Current_level_num == 0)
	{
		window_close(Game_wind);		//not a real level
	}
	#endif

#ifdef NETWORK
	if (Game_mode & GM_MULTI)	{
		result = multi_endlevel(&secret_flag); // Wait for other players to reach this point
		if (result) // failed to sync
		{
			if (Current_level_num == Last_level)		//player has finished the game!
				if (Game_wind)
					window_close(Game_wind);		// Exit out of game loop

			return;
		}
	}
#endif

	if (RestartLevel.isResults == 2)
		StartNewLevel(Current_level_num);
	else {
		RestartLevel.updateRestartStuff = 1; // Assume we can update this struct upon starting a level. The game will tell us not to if we're restarting the current level from the result screen.
		if (Current_level_num == Last_level) {		//player has finished the game!

			DoEndGame();

		}
		else {
			//NMN 04/08/07 If we are in a secret level and playing a D1
			// 	       level, then use Entered_from_level # instead
			if (Current_level_num < 0 && EMULATING_D1)
			{
				Next_level_num = Entered_from_level + 1;		//assume go to next normal level
			}
			else {
				Next_level_num = Current_level_num + 1;		//assume go to next normal level
			}
			// END NMN

			StartNewLevel(Next_level_num);
			if (checkForWarmStart()) // Don't consider the player to be warm starting unless they actually have more than the default loadout.
				Ranking.warmStart = 1;
		}
	}
}

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
		object * playerobj = &Objects[Players[Player_num].objnum];
		//nm_messagebox( "You're Dead!", 1, "Continue", "Not a real game, though." );
		if (Game_wind)
			window_set_visible(Game_wind, 1);
		load_level("gamesave.lvl");
		init_player_stats_new_ship(Player_num);
		playerobj->flags &= ~OF_SHOULD_BE_DEAD;
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

		//clear out stuff so no bonus
		Players[Player_num].hostages_on_board = 0;
		Ranking.secret_hostages_on_board = 0;
		Players[Player_num].energy = 0;
		Players[Player_num].shields = 0;
		Players[Player_num].connected = CONNECT_DIED_IN_MINE;

		do_screen_message(TXT_DIED_IN_MINE); // Give them some indication of what happened

		if (Current_level_num < 0) {
			if (!Ranking.secretQuickload) // Don't show quickloaders a result screen for secret levels, as that's not in the vanilla game, and quickloaded levels have this mod disabled.
				DoEndSecretLevelScoreGlitz();
			if (PHYSFSX_exists(SECRETB_FILENAME,0))
			{
				do_screen_message(TXT_SECRET_RETURN);
				state_restore_all(1, 2, SECRETB_FILENAME);			//	2 means you died
				Ranking.rankScore = Players[Player_num].score - Players[Player_num].last_score - Ranking.excludePoints;
				set_pos_from_return_segment();
				Players[Player_num].lives--;						//	re-lose the life, Players[Player_num].lives got written over in restore.
			} else {
				if (Entered_from_level == Last_level)
					DoEndGame();
				else {
					do_screen_message(TXT_SECRET_ADVANCE);
					StartNewLevel(Entered_from_level+1);
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
					RestartLevel.afterburner_charge = Players[Player_num].afterburner_charge;
					RestartLevel.omega_charge = Omega_charge;
					Ranking.warmStart = 0; // Don't count next level as a warm start, since the player just lost all their stuff in self destruct.
					init_player_stats_new_ship(Player_num);	//	New, MK, 05/29/96!, fix bug with dying in secret level, advance to next level, keep powerups!
				}
			}
		} else {

			AdvanceLevel(0);			//if finished, go on to next level

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
				RestartLevel.afterburner_charge = Players[Player_num].afterburner_charge;
				RestartLevel.omega_charge = Omega_charge;
				Ranking.warmStart = 0; // Don't count next level as a warm start, since the player just lost all their stuff in self destruct.
				init_player_stats_new_ship(Player_num);
			}
			last_drawn_cockpit = -1;
		}

	} else if (Current_level_num < 0) {
		if (PHYSFSX_exists(SECRETB_FILENAME,0))
		{
			do_screen_message(TXT_SECRET_RETURN);
			if (!Control_center_destroyed)
				state_save_all(2, SECRETC_FILENAME, 0);
			state_restore_all(1, 2, SECRETB_FILENAME);
			Ranking.rankScore = Players[Player_num].score - Players[Player_num].last_score - Ranking.excludePoints;
			set_pos_from_return_segment();
			Players[Player_num].lives--;						//	re-lose the life, Players[Player_num].lives got written over in restore.
		} else {
			do_screen_message(TXT_DIED_IN_MINE); // Give them some indication of what happened
			if (Entered_from_level == Last_level)
				DoEndGame();
			else {
				do_screen_message(TXT_SECRET_ADVANCE);
				StartNewLevel(Entered_from_level+1);
				init_player_stats_new_ship(Player_num);	//	New, MK, 05/29/96!, fix bug with dying in secret level, advance to next level, keep powerups!
			}
		}
	} else {
		init_player_stats_new_ship(Player_num);
		StartLevel(1);
	}

	digi_sync_sounds();

	if (Game_wind && cycle_window_vis)
		window_set_visible(Game_wind, 1);
	reset_time();
}

extern int BigWindowSwitch;

//called when the player is starting a new level for normal game mode and restore state
//	secret_flag set if came from a secret level
void StartNewLevelSub(int level_num, int page_in_textures, int secret_flag)
	{
	if (!(Game_mode & GM_MULTI)) {
		last_drawn_cockpit = -1;
	}
   BigWindowSwitch=0;

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

	LoadLevel(level_num,page_in_textures);

	Assert(Current_level_num == level_num);	//make sure level set right

	gameseq_init_network_players(); // Initialize the Players array for
											  // this level

	Viewer = &Objects[Players[Player_num].objnum];

	Assert(N_players <= NumNetPlayerPositions);
		//If this assert fails, there's not enough start positions

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
			for (int i = 0; i < MAX_PLAYERS; i++)
			{
				init_player_stats_new_ship(i);
			}
		}
	}
#endif

	HUD_clear_messages();

	automap_clear_visited();

	init_player_stats_level(secret_flag);

	load_palette(Current_level_palette,0,1);
	gr_palette_load(gr_palette);

#ifdef NETWORK
	if ((Game_mode & GM_MULTI_COOP) && Network_rejoined)
	{
		int i;
		for (i = 0; i < N_players; i++)
			Players[i].flags |= Netgame.player_flags[i];
	}

	if (Game_mode & GM_MULTI)
	{
		multi_prep_level(); // Removes robots from level if necessary
	}
#endif

	gameseq_remove_unused_players();

	Game_suspended = 0;

	Control_center_destroyed = 0;
	Ranking.freezeTimer = 0;

	set_screen_mode(SCREEN_GAME);

	init_cockpit();
	init_robots_for_level();
	init_ai_objects();
	init_smega_detonates();
	init_morphs();
	init_all_matcens();
	reset_palette_add();
	init_thief_for_level();
	init_stuck_objects();
	if (!(Game_mode & GM_MULTI))
		filter_objects_from_level();

	if (!(Game_mode & GM_MULTI) && !cheats.enabled)
		set_highest_level(Current_level_num);
	else
		read_player_file();		//get window sizes

	reset_special_effects();
	init_exploding_walls();

#ifdef OGL
	gr_remap_mono_fonts();
#endif


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
	// Also takes care of nm_draw_background() possibly being called
	if (!((Game_mode & GM_MULTI) && (Newdemo_state != ND_STATE_PLAYBACK)))
		full_palette_save();

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


void filter_objects_from_level()
 {
  int i;

  for (i=0;i<=Highest_object_index;i++)
	{
	 if (Objects[i].type==OBJ_POWERUP)
     if (Objects[i].id==POW_FLAG_RED || Objects[i].id==POW_FLAG_BLUE)
	   bash_to_shield (i,"Flag!!!!");
   }

 }

struct {
	int	level_num;
	char	movie_name[FILENAME_LEN];
} intro_movie[] = { 	{ 1,"pla"},
							{ 5,"plb"},
							{ 9,"plc"},
							{13,"pld"},
							{17,"ple"},
							{21,"plf"},
							{24,"plg"}};

#define NUM_INTRO_MOVIES (sizeof(intro_movie) / sizeof(*intro_movie))

extern int robot_movies;	//0 means none, 1 means lowres, 2 means hires
extern int intro_played;	//true if big intro movie played

void ShowLevelIntro(int level_num)
{
	//if shareware, show a briefing?

	if (!(Game_mode & GM_MULTI)) {
		int i;

		ubyte save_pal[sizeof(gr_palette)];

		memcpy(save_pal,gr_palette,sizeof(gr_palette));

		if (PLAYING_BUILTIN_MISSION) {

			if (is_SHAREWARE || is_MAC_SHARE)
			{
				if (level_num==1)
					do_briefing_screens (Briefing_text_filename, 1);
			}
			else if (is_D2_OEM)
			{
				if (level_num == 1 && !intro_played)
					do_briefing_screens(Briefing_text_filename, 1);
			}
			else // full version
			{
				for (i=0;i<NUM_INTRO_MOVIES;i++)
				{
					if (intro_movie[i].level_num == level_num)
					{
						Screen_mode = -1;
						PlayMovie(intro_movie[i].movie_name,MOVIE_REQUIRED);
						break;
					}
				}

				do_briefing_screens (Briefing_text_filename,level_num);
			}
		}
		else	//not the built-in mission (maybe d1, too).  check for add-on briefing
		{
			do_briefing_screens(Briefing_text_filename, level_num);
		}

		memcpy(gr_palette,save_pal,sizeof(gr_palette));
	}
}

//	---------------------------------------------------------------------------
//	If starting a level which appears in the Secret_level_table, then set First_secret_visit.
//	Reason: On this level, if player goes to a secret level, he will be going to a different
//	secret level than he's ever been to before.
//	Sets the global First_secret_visit if necessary.  Otherwise leaves it unchanged.
void maybe_set_first_secret_visit(int level_num)
{
	int	i;

	for (i=0; i<N_secret_levels; i++) {
		if (Secret_level_table[i] == level_num) {
			First_secret_visit = 1;
		}
	}
}

//called when the player is starting a new level for normal game model
//	secret_flag if came from a secret level
void StartNewLevel(int level_num)
{
	hide_menus();

	GameTime64 = 0;
	ThisLevelTime = 0;

	Ranking.deathCount = 0;
	Ranking.excludePoints = 0;
	Ranking.rankScore = 0;
	Ranking.missedRngSpawn = 0;
	Ranking.quickload = 0;
	Ranking.level_time = 0; // Set this to 0 despite it going unused until set to time_level, so we can save a variable when telling the in-game timer which time variable to display.
	Ranking.fromBestRanksButton = 0; // So the result screen knows it's not just viewing record details.
	Ranking.num_thief_points = 0;
	Ranking.mergeLevels = 0;
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
		RestartLevel.afterburner_charge = Players[Player_num].afterburner_charge;
		RestartLevel.omega_charge = Omega_charge;
	}

	if (level_num > 0) {
		maybe_set_first_secret_visit(level_num);
	}

	if (!RestartLevel.restarts && RestartLevel.isResults < 2) // Skip briefing for restarts.
		ShowLevelIntro(level_num);
	else
		First_secret_visit = 1; // Reset a secret level upon restarting its base level, for continuity's sake.

	StartNewLevelSub(level_num, 1, 0);

	RestartLevel.isResults = 0;

	// For D2, we have to recalculate max score every restart, or else restarting on levels with thieves will make it keep changing and mess up the scoring.
	int i;
	int isRankable = 0; // If the level doesn't have a reactor, boss or normal type exit, it can't be beaten and must be given special treatment.
	int highestBossScore = 0;
	for (i = 0; i <= Highest_object_index; i++) {
		if (Objects[i].type == OBJ_ROBOT && Robot_info[Objects[i].id].boss_flag) { // Look at every boss, adding only the highest point value.
			if (Robot_info[Objects[i].id].score_value > highestBossScore)
				highestBossScore = Robot_info[Objects[i].id].score_value;
		}
	}
	Ranking.maxScore = highestBossScore;
	for (i = 0; i <= Highest_object_index; i++) {
		// It has been decided that thieves (and robots within them) will not count toward max score (AKA they won't be required for S-ranks). They're just too annoying and unfun to kill, but will give you a considerable point advantage if you manage to take one down quickly.
		// However, we can't do that here. We have to let them slide for now so the "remains" counter stays accurate. Let's instead count them, then use that number to subtract the points when the level's over.
		if (Objects[i].type == OBJ_ROBOT) {
			if (!Robot_info[Objects[i].id].boss_flag) // Ignore bosses for score. We already decided which one to count before.
				Ranking.maxScore += Robot_info[Objects[i].id].score_value;
			if (Robot_info[Objects[i].id].thief)
				Ranking.num_thief_points += Robot_info[Objects[i].id].score_value;
			if (Objects[i].contains_type == OBJ_ROBOT && ((Objects[i].id != Robot_info[Objects[i].id].contains_id) || (Objects[i].id != Robot_info[Objects[i].contains_id].contains_id))) { // So points in infinite robot drop loops aren't counted past the parent bot.
				Ranking.maxScore += Robot_info[Objects[i].contains_id].score_value * Objects[i].contains_count;
				if (Robot_info[Objects[i].contains_id].thief || Robot_info[Objects[i].id].thief) // If the parent is a thief, exclude it and its children. If the children are thieves, exclude just them.
					Ranking.num_thief_points += Robot_info[Objects[i].contains_id].score_value * Objects[i].contains_count;
			}
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
	Ranking.maxScore = (int)(Ranking.maxScore * 3);
	for (i = 0; i <= Num_triggers; i++) {
		if (Triggers[i].type == TT_EXIT)
			isRankable = 1; // A level-ending exit is present, this level is beatable. Technically the level could still be unbeatable because the exit could be somewhere unreachable, but who would put an exit there?
	}
	if (RestartLevel.restarts) {
		const char message[256];
		if (RestartLevel.restarts == 1)
			sprintf(message, "1 restart and counting...");
		else
			sprintf(message, "%i restarts and counting...", RestartLevel.restarts);
		powerup_basic(-10, -10, -10, 0, message);
	}
	else {
		Ranking.parTime = calculateParTime();
		if (isRankable) { // If this level is not beatable, mark the level as beaten with zero points and an S-rank, so the mission can have an aggregate rank.
			PHYSFS_File* temp;
			char filename[256];
			char temp_filename[256];
			sprintf(filename, "ranks/%s/%s/level%i.hi", Players[Player_num].callsign, Current_mission->filename, Current_level_num * -1);
			sprintf(temp_filename, "ranks/%s/%s/temp.hi", Players[Player_num].callsign, Current_mission->filename);
			time_t timeOfScore = time(NULL);
			temp = PHYSFS_openWrite(temp_filename);
			PHYSFSX_printf(temp, "%i\n", Players[Player_num].hostages_level);
			PHYSFSX_printf(temp, "0");
			PHYSFSX_printf(temp, "0");
			PHYSFSX_printf(temp, "0");
			PHYSFSX_printf(temp, "0");
			PHYSFSX_printf(temp, "0");
			PHYSFSX_printf(temp, "%i\n", Difficulty_level);
			PHYSFSX_printf(temp, "0");
			PHYSFSX_printf(temp, "0");
			PHYSFSX_printf(temp, "%s\n", Current_level_name);
			PHYSFSX_printf(temp, "%s", ctime(&timeOfScore));
			PHYSFSX_printf(temp, "0");
			PHYSFS_close(temp);
			PHYSFS_delete(filename);
			PHYSFSX_rename(temp_filename, filename);
		}
	}
	Ranking.alreadyBeaten = 0;
	if (calculateRank(level_num, 0) > 0)
		Ranking.alreadyBeaten = 1;
}

int previewed_spawn_point = 0;

//initialize the player object position & orientation (at start of game, or new ship)
void InitPlayerPosition(int random_flag)
{
	int NewPlayer=0;

	if (is_observer())
		NewPlayer = 0;
	else if (! ((Game_mode & GM_MULTI) && !(Game_mode&GM_MULTI_COOP)) ) // If not deathmatch
		NewPlayer = Player_num;
#ifdef NETWORK	
	else if ((Game_mode & GM_MULTI) && (Netgame.SpawnStyle == SPAWN_STYLE_PREVIEW) && Dead_player_camera != NULL)
		NewPlayer = previewed_spawn_point; 
#endif	
	else if (random_flag == 1)
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
	else {
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

	//	Boost shield for Thief and Buddy based on level.
	objp->shields = robptr->strength;

	if ((robptr->thief) || (robptr->companion)) {
		objp->shields = (objp->shields * (abs(Current_level_num)+7))/8;

		if (robptr->companion) {
			//	Now, scale guide-bot hits by skill level
			switch (Difficulty_level) {
				case 0:	objp->shields = i2f(20000);	break;		//	Trainee, basically unkillable
				case 1:	objp->shields *= 3;				break;		//	Rookie, pretty dang hard
				case 2:	objp->shields *= 2;				break;		//	Hotshot, a bit tough
				default:	break;
			}
		}
	} else if (robptr->boss_flag)	//	MK, 01/16/95, make boss shields lower on lower diff levels.
		objp->shields = objp->shields/(NDL+3) * (Difficulty_level+4);

	//	Additional wimpification of bosses at Trainee
	if ((robptr->boss_flag) && (Difficulty_level == 0))
		objp->shields /= 2;
}

//	-----------------------------------------------------------------------------------------------------
//	Copy all values from the robot info structure to all instances of robots.
//	This allows us to change bitmaps.tbl and have these changes manifested in existing robots.
//	This function should be called at level load time.
void copy_defaults_to_robot_all()
{
	int	i;

	for (i=0; i<=Highest_object_index; i++)
		if (Objects[i].type == OBJ_ROBOT)
			copy_defaults_to_robot(&Objects[i]);

}

extern void clear_stuck_objects(void);

//	-----------------------------------------------------------------------------------------------------
//called when the player is starting a level (new game or new ship)
void StartLevel(int random_flag)
{
	Assert(!Player_is_dead);

	InitPlayerPosition(random_flag);

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
		clear_stuck_objects(); // and stuck ones.
	}

	ai_reset_all_paths();
	ai_init_boss_for_ship();

	reset_rear_view();
	Auto_fire_fusion_cannon_time = 0;
	Fusion_charge = 0;
}