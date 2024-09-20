#include <3ds.h>
#include <citro2d.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <assert.h>

#include <cwav.h>
#include <ncsnd.h>

// Defaults
#define DICE_MAX_SPRITES 100
#define FISHING_MAX_SPRITES 50
#define TOP_SCREEN_WIDTH  400
#define TOP_SCREEN_HEIGHT 240
#define BOTTOM_SCREEN_WIDTH 320
#define BOTTOM_SCREEN_HEIGHT 240
#define SEC_NANOSECOND 1000000000

#define DICE_ROLL_AMOUNT 10

#define GAME_DICE 0
#define GAME_FISHING 1

// dice related text
#define DICE_AMOUNT_TEXT_SIZE 1.0f
#define DICE_TYPE_TEXT_SIZE 1.0f
#define DICE_TRAY_TEXT_SIZE 0.6f
#define TEXT_SIZE_REDUCE 0.1f
#define DICE_ROLL_TEXT_SIZE 0.8f

#define FISHING_FLAVOR_SIZE 0.6f
#define TOTAL_TEXT_SIZE 1.2f
#define COPYRIGHT_TEXT_SIZE 0.4f

#define PAGE_DISPLAY_MAX 7

// dice modes
#define DICE_NORMAL 0
#define DICE_ADVANTAGE 1
#define DICE_DISADVANTAGE -1

// rolling modes
#define READY_TO_ROLL 0
#define CURRENTLY_ROLLING 1

// frame related
#define FRAMES_HELD_MIN 0.30f // 1 second = 60 frames = 0.60f
#define FRAMES_HELD_MAX 2.00f

#define FRAMES_IN_SEC 0.60f

#define FRAMES_LOOT_WAIT 0.90f
#define FRAMES_NO_INPUT_MAX 1.20f

// floaty dice controls
#define WAVE_MOVE_MULTIPLIER 4
#define WAVE_SPEED_MULTIPLIER 2

// info modes
#define INFO_CLOSED 0
#define INFO_OPEN 1

// sound related
#define CWAV_LIST_MAX 10
#define CWAV_LIST_MIN 0
#define SOUND_PLAY_MAX 10

#define BTN_CLICK 0
#define BTN_RELEASE 1
#define BTN_RELEASE_HELD 2
#define DICE_THUD 3 //skip 3 more
#define COIN_TINK 7
#define CRIT_SUCCESS 8
#define CRIT_FAIL 9

// Variable cycling Boundries
#define AMOUNT_MAX 10
#define AMOUNT_MIN 1
#define TYPE_MAX 6
#define TYPE_MIN 0
#define DICE_TRAY_MAX 12
#define DICE_TRAY_MIN 0
// ------------------------------------------------------------

// -- Structs: --------------------------------------------
// Simple sprite struct
typedef struct
{
	C2D_Sprite spr;
} Sprite;

// Where the dice info is stored
typedef struct
{
	int amount, type, type_visual, mode;
	char notation[6];
} Dice;

// Sound tuples
typedef struct {
	char path[32];
	CWAV* raw_cwav;
} SoundTuple;

// Dice Roll Result tuples
typedef struct {
	int result1[10], result2[10]; 
	int total;
} DiceRollResult;


// -- VARIABLES: --------------------------------------------
// Fishing
	int game_mode = 0; // 0 = Dice, 1 = Fishing
	int rod_not_cast = 1, rest_engaged = 1; 
	int animate_cast = 0, animate_reel = 0, animate_wobble = 0; 
	float water_animation = 0.0f, rod_cast_animation = 0.0f, rod_wobble_animation = 0.0f, rod_reel_animation = 0.0f;
	float frames_passed_no_input = 0.0f, frame_clock = 0.0f, loot_wait_clock = 0.0f;

	int rarity = 0, rarity_display = 0, variant = 0, variant_display = 0;
	int rand_attrib1 = 0, rand_attrib2 = 0, gold_worth = 0, gold_total = 0;
	int rand_attrib1_disp = 0, rand_attrib2_disp = 0, gold_worth_disp = 0;
	
	int loot_got = 0, activate_loot = 0;

// Dice
	// Left UI;__________________________________________________
	// Data
	int amount = AMOUNT_MIN, dice_col = 0, dice_col_old = 0;
	float time_btn_held = 0.0f; // frames held initialize
	int info_mode = INFO_CLOSED;
	int held_sfx_toggle = 0;

	// Hitboxes
	int a_up_hit = 0, t_up_hit = 0; // UP Arrows
	int a_down_hit = 0, t_down_hit = 0; // Down Arrows
	int left_hit = 0, right_hit = 0; // Col Arrows

	// spam check
	int a_up_hit_old = 0, t_up_hit_old = 0; // UP Arrows
	int a_down_hit_old = 0, t_down_hit_old = 0; // Down Arrows
	int left_hit_old = 0, right_hit_old = 0; // Col Arrows

	// Right UI;_________________________________________________
	// Hitboxes
	int btn_b_hit = 0, btn_a_hit = 0, btn_x_hit = 0, btn_y_hit = 0;

	// spam check
	int btn_b_hit_old = 0, btn_a_hit_old = 0, btn_x_hit_old = 0, btn_y_hit_old = 0;
	// ----------------------------------------------------------

	// Shoulder Buttons;__________________________________________
	// Hitboxes
	int r_hit = 0, l_hit = 0;

	// rolling
	int is_rolling = READY_TO_ROLL, is_rolling_old = READY_TO_ROLL;
	int current_die_index = 0, die_queue_len = 0, was_rolled = 0, rand_rolled = 0, release_r_sound_flag = 0;
	int roll_die_index = 0;
	float roll_frames = 0.0f;

	DiceRollResult roll_results[13] = {{{0}, {0}, 0}};
	int display_page = 0, result_list_len = 0;

	// spam check
	int r_hit_old = 0, l_hit_old = 0;
	// ----------------------------------------------------------

	// Dice;_____________________________________________________
	int type_selected = TYPE_MAX;
	int types[] = {2, 4, 6, 8, 10, 12, 20};
	int dice_mode = DICE_NORMAL, dice_mode_old = DICE_NORMAL;
	float sin_move_rads = 0.0f;

	Dice tray[DICE_TRAY_MAX+1] = {{0, 0, 0, 0, "\0"}}; 
	Dice roll_queue[DICE_TRAY_MAX+1] = {{0, 0, 0, 0, "\0"}};
	int dice_select = 0, select_mod = 0;
	float select_clock = 0.0f;
	// ----------------------------------------------------------

// -- Hitboxes: ---------------------------------------------
	int arwbtn_nvert = 8;
	int ltrbtn_nvert = 12;
	int sldrbtn_nvert = 6;

	// ___UP;________________
	// Amount
	int amount_up_hitx[] = {  4,   4, 18, 19,  33,  33,  32,   5};
	int amount_up_hity[] = {115, 108, 94, 94, 108, 115, 116, 116};
	// Type
	int type_up_hitx[] = { 58,  58, 72, 73,  87,  87,  86,  59};
	int type_up_hity[] = {115, 108, 94, 94, 108, 115, 116, 116};

	// ___DOWN;______________
	// Amount
	int amount_down_hitx[] = {  4,   4,   5,  32,  33,  33,  19,  18};
	int amount_down_hity[] = {161, 154, 153, 153, 154, 161, 175, 175};
	// Type
	int type_down_hitx[] = { 58,  58,  72,  73,  87,  87,  86,  59};
	int type_down_hity[] = {161, 154, 153, 153, 154, 161, 175, 175};

	// ___Mode;______________
	// Left
	int left_hitx[] = {19, 19, 33, 40, 41, 41, 40, 33};
	int left_hity[] = {74, 73, 59, 59, 60, 87, 88, 88};

	// Right
	int right_hitx[] = {50, 50, 51, 58, 72, 72, 58, 51};
	int right_hity[] = {87, 60, 59, 59, 73, 74, 88, 88};

	// ___Letters;___________
	// Letter B
	int b_hitx[] = {259, 259, 262, 269, 278, 285, 288, 288, 285, 278, 269, 262};
	int b_hity[] = {141, 136, 133, 131, 131, 133, 136, 141, 144, 147, 147, 144};
	// Letter A
	int a_hitx[] = {287, 287, 290, 297, 306, 313, 316, 316, 313, 306, 297, 290};
	int a_hity[] = {124, 119, 116, 114, 114, 116, 119, 125, 127, 129, 129, 127};
	// Letter X
	int x_hitx[] = {259, 259, 262, 269, 278, 285, 288, 288, 285, 278, 269, 262};
	int x_hity[] = {108, 103, 100,  98,  98, 100, 103, 108, 111, 113, 113, 111};
	// Letter Y
	int y_hitx[] = {231, 231, 234, 241, 250, 257, 260, 260, 257, 250, 241, 234};
	int y_hity[] = {124, 119, 116, 114, 114, 116, 119, 125, 127, 129, 129, 127};
	// ----------------------------------------------------------
	
	// ___Shoulders;_________
	// Right
	int r_hitx[] = {280, 280, 282, 319, 319, 282};
	int r_hity[] = { 39,   6,   4,   4,  41,  41};
	// Left
	int l_hitx[] = { 0,  0, 37, 39, 39, 37};
	int l_hity[] = {41,  4,  4,  6, 39, 41};
	// ----------------------------------------------------------

// -- Text: -------------------------------------------------
	C2D_TextBuf g_dynamic_roll_buf, g_dynamic_dice_buf, g_dynamic_fishing_buf,g_static_buf;
	C2D_Font font[1];
	C2D_Text g_static_copyright_text[4], g_static_total_text;

	static const char copyright_libs_string[] = "Citro2d * libcwav * libctru\n";
					  
	static const char copyright_sounds_string[] = 
		"Freesound.org; CCBY 4.0\n"
		"Buttons - magedu; ------\n"
		"* old_crt_monitor_turn_on_01\n"
		"* old_pc_turn_off\n"
		"Freesound.org; CC0\n"
		"Dice - sgrowe;\n"
		"Coin - FirediesProductions;\n"
		"Success - HenryRichard;\n"
		"Fail - stumpbutt;\n";

	PrintConsole debug_console;

// -- Graphics/Sprites: -------------------------------------
	static C2D_SpriteSheet bg_sprite_sheet, btn_sprite_sheet, hl_sprite_sheet, dice_sprite_sheet, fishing_sprite_sheet;
	static Sprite sprites[DICE_MAX_SPRITES];
	static Sprite fishing_sprites[FISHING_MAX_SPRITES];

// -- Audio/Sounds: -----------------------------------------
const char* file_list[] =
{
	"romfs:/sounds/btn_click.bcwav",
	"romfs:/sounds/btn_release.bcwav",
	"romfs:/sounds/btn_release_held.bcwav",
	"romfs:/sounds/dice_thud1.bcwav",
	"romfs:/sounds/dice_thud2.bcwav",
	"romfs:/sounds/dice_thud3.bcwav",
	"romfs:/sounds/dice_thud4.bcwav",
	"romfs:/sounds/coin_flip.bcwav",
	"romfs:/sounds/crit_success.bcwav",
	"romfs:/sounds/crit_fail.bcwav"
};

SoundTuple cwav_list[CWAV_LIST_MAX] = {{"\0", NULL}};

// -- Functions: --------------------------------------------
/*
Sprite Alloc;

dice;
0, 1, 80, 81; - bg and windows
2 - 30; - buttons
32 - 54; - highlights
60 - 66; - big dice
70 - 76; - tray dice

fishing;
0 - 2; - docks
3; - water
4 - 8; - fishing rod throwing frames
9; - fishing rod still
10 - 13; - fishing rod wobble
14; - fishing rod pull - depricated
15 - 22; - fishing rod reel
23 - 33; - water
34 - 36; - junk
37 - 39; - fish
40 - 42; - treasure
43, 44; - a button
45, 46; - exits
*/

//---------------------------------------------------------------------------------
static void initBGSprites() {
//---------------------------------------------------------------------------------
	// array locations
	// backgrounds
	Sprite* bg_top_sprite = &sprites[0];
	Sprite* bg_bottom_sprite = &sprites[1];

	// faded overlay
	Sprite* bg_fade_sprite = &sprites[80];
	// info window
	Sprite* bg_info_sprite = &sprites[81];

	
	// Top BG;_____________________________________________________________
	C2D_SpriteFromSheet(&bg_top_sprite->spr, bg_sprite_sheet, 0);
	// make the sprites center be the top-left corner and that be its position
	C2D_SpriteSetCenter(&bg_top_sprite->spr, 0.0f, 0.0f);
	// -------------------------------------------------------------------

	// Bottom BG;__________________________________________________________
	C2D_SpriteFromSheet(&bg_bottom_sprite->spr, bg_sprite_sheet, 1);
	// make the sprites center be the top-left corner and that be its position
	C2D_SpriteSetCenter(&bg_bottom_sprite->spr, 0.0f, 0.0f);
	// -------------------------------------------------------------------

	// Bottom Info Fade;_________________________________________________
	C2D_SpriteFromSheet(&bg_fade_sprite->spr, bg_sprite_sheet, 2);
	C2D_SpriteSetCenter(&bg_fade_sprite->spr, 0.0f, 0.0f);
	// -------------------------------------------------------------------

	// Bottom Info Window;_________________________________________________
	C2D_SpriteFromSheet(&bg_info_sprite->spr, bg_sprite_sheet, 3);
	C2D_SpriteSetCenter(&bg_info_sprite->spr, 0.0f, 0.0f);
	// -------------------------------------------------------------------
}

//---------------------------------------------------------------------------------
static void initButtonSprites() {
//---------------------------------------------------------------------------------
	// array locations
	// amount column
	Sprite* amount_btn_up_r_sprite = &sprites[2];
	Sprite* amount_btn_up_h_sprite = &sprites[3];
	Sprite* amount_btn_down_r_sprite = &sprites[4];
	Sprite* amount_btn_down_h_sprite = &sprites[5];

	// type column
	Sprite* type_btn_up_r_sprite = &sprites[6];
	Sprite* type_btn_up_h_sprite = &sprites[7];
	Sprite* type_btn_down_r_sprite = &sprites[8];
	Sprite* type_btn_down_h_sprite = &sprites[9];

	// column select - dice mode
	Sprite* btn_left_r_sprite = &sprites[10];
	Sprite* btn_left_h_sprite = &sprites[11];
	Sprite* btn_right_r_sprite = &sprites[12];
	Sprite* btn_right_h_sprite = &sprites[13];

	// B button
	Sprite* btn_b_r_sprite = &sprites[14];
	Sprite* btn_b_h_sprite = &sprites[15];

	// A Button
	Sprite* btn_a_r_sprite = &sprites[16];
	Sprite* btn_a_h_sprite = &sprites[17];

	// X Button
	Sprite* btn_x_r_sprite = &sprites[18];
	Sprite* btn_x_h_sprite = &sprites[19];

	// Y Button
	Sprite* btn_y_r_sprite = &sprites[20];
	Sprite* btn_y_h_sprite = &sprites[21];

	// R Button
	Sprite* btn_r_r_sprite = &sprites[22];
	Sprite* btn_r_h_sprite = &sprites[23];

	// L Button
	// open
	Sprite* btn_l_or_sprite = &sprites[24];
	Sprite* btn_l_oh_sprite = &sprites[25];
	// close
	Sprite* btn_l_cr_sprite = &sprites[26];
	Sprite* btn_l_ch_sprite = &sprites[27];

	// C stick
	// released
	Sprite* c_stick_r_sprite = &sprites[28];
	// held right
	Sprite* c_stick_hr_sprite = &sprites[29];
	// held left
	Sprite* c_stick_hl_sprite = &sprites[30];

	// UP;______________________________________________________________
	// ----- AMOUNT -----
	// Up - Released
	C2D_SpriteFromSheet(&amount_btn_up_r_sprite->spr, btn_sprite_sheet, 0);
	C2D_SpriteSetCenter(&amount_btn_up_r_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&amount_btn_up_r_sprite->spr, 4, 94);
	// Up - Held
	C2D_SpriteFromSheet(&amount_btn_up_h_sprite->spr, btn_sprite_sheet, 1);
	C2D_SpriteSetCenter(&amount_btn_up_h_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&amount_btn_up_h_sprite->spr, 4, 94);

	// ----- TYPE -----
	// Up - Released
	C2D_SpriteFromSheet(&type_btn_up_r_sprite->spr, btn_sprite_sheet, 0);
	C2D_SpriteSetCenter(&type_btn_up_r_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&type_btn_up_r_sprite->spr, 58, 94);
	// Up - Held
	C2D_SpriteFromSheet(&type_btn_up_h_sprite->spr, btn_sprite_sheet, 1);
	C2D_SpriteSetCenter(&type_btn_up_h_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&type_btn_up_h_sprite->spr, 58, 94);
	// -------------------------------------------------------------------

	// DOWN;______________________________________________________________
	// ----- AMOUNT -----
	// Down - Released
	C2D_SpriteFromSheet(&amount_btn_down_r_sprite->spr, btn_sprite_sheet, 2);
	C2D_SpriteSetCenter(&amount_btn_down_r_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&amount_btn_down_r_sprite->spr, 4, 153);
	// Down - Held
	C2D_SpriteFromSheet(&amount_btn_down_h_sprite->spr, btn_sprite_sheet, 3);
	C2D_SpriteSetCenter(&amount_btn_down_h_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&amount_btn_down_h_sprite->spr, 4, 153);

	// ----- TYPE -----
	// Down - Released
	C2D_SpriteFromSheet(&type_btn_down_r_sprite->spr, btn_sprite_sheet, 2);
	C2D_SpriteSetCenter(&type_btn_down_r_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&type_btn_down_r_sprite->spr, 58, 153);
	// Down - Held
	C2D_SpriteFromSheet(&type_btn_down_h_sprite->spr, btn_sprite_sheet, 3);
	C2D_SpriteSetCenter(&type_btn_down_h_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&type_btn_down_h_sprite->spr, 58, 153);
	// -------------------------------------------------------------------

	//// Column Select
	// LEFT;______________________________________________________________
	// Left - Released
	C2D_SpriteFromSheet(&btn_left_r_sprite->spr, btn_sprite_sheet, 4);
	C2D_SpriteSetCenter(&btn_left_r_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&btn_left_r_sprite->spr, 19, 59);
	// Left - Held
	C2D_SpriteFromSheet(&btn_left_h_sprite->spr, btn_sprite_sheet, 5);
	C2D_SpriteSetCenter(&btn_left_h_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&btn_left_h_sprite->spr, 19, 59);
	// -------------------------------------------------------------------

	// RIGHT;______________________________________________________________
	// Right - Released
	C2D_SpriteFromSheet(&btn_right_r_sprite->spr, btn_sprite_sheet, 6);
	C2D_SpriteSetCenter(&btn_right_r_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&btn_right_r_sprite->spr, 50, 59);
	// Right - Held
	C2D_SpriteFromSheet(&btn_right_h_sprite->spr, btn_sprite_sheet, 7);
	C2D_SpriteSetCenter(&btn_right_h_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&btn_right_h_sprite->spr, 50, 59);
	// -------------------------------------------------------------------

	//// Letter Buttons
	// B Button;___________________________________________________________
	// B - Released
	C2D_SpriteFromSheet(&btn_b_r_sprite->spr, btn_sprite_sheet, 8);
	C2D_SpriteSetCenter(&btn_b_r_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&btn_b_r_sprite->spr, 259, 130);
	// B - Held
	C2D_SpriteFromSheet(&btn_b_h_sprite->spr, btn_sprite_sheet, 9);
	C2D_SpriteSetCenter(&btn_b_h_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&btn_b_h_sprite->spr, 259, 130);
	// -------------------------------------------------------------------

	// A Button;___________________________________________________________
	// A - Released
	C2D_SpriteFromSheet(&btn_a_r_sprite->spr, btn_sprite_sheet, 10);
	C2D_SpriteSetCenter(&btn_a_r_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&btn_a_r_sprite->spr, 287, 113);
	// A - Held
	C2D_SpriteFromSheet(&btn_a_h_sprite->spr, btn_sprite_sheet, 11);
	C2D_SpriteSetCenter(&btn_a_h_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&btn_a_h_sprite->spr, 287, 113);
	// -------------------------------------------------------------------

	// X Button;___________________________________________________________
	// X - Released
	C2D_SpriteFromSheet(&btn_x_r_sprite->spr, btn_sprite_sheet, 12);
	C2D_SpriteSetCenter(&btn_x_r_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&btn_x_r_sprite->spr, 259, 97);
	// X - Held
	C2D_SpriteFromSheet(&btn_x_h_sprite->spr, btn_sprite_sheet, 13);
	C2D_SpriteSetCenter(&btn_x_h_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&btn_x_h_sprite->spr, 259, 97);
	// -------------------------------------------------------------------

	// Y Button;__________________________________________________________
	// Y - Released
	C2D_SpriteFromSheet(&btn_y_r_sprite->spr, btn_sprite_sheet, 14);
	C2D_SpriteSetCenter(&btn_y_r_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&btn_y_r_sprite->spr, 231, 113);
	// Y - Held
	C2D_SpriteFromSheet(&btn_y_h_sprite->spr, btn_sprite_sheet, 15);
	C2D_SpriteSetCenter(&btn_y_h_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&btn_y_h_sprite->spr, 231, 113);
	// -------------------------------------------------------------------
	
	// R Button;__________________________________________________________
	// R - Released
	C2D_SpriteFromSheet(&btn_r_r_sprite->spr, btn_sprite_sheet, 16);
	C2D_SpriteSetCenter(&btn_r_r_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&btn_r_r_sprite->spr, 280, 4);
	// R - Held
	C2D_SpriteFromSheet(&btn_r_h_sprite->spr, btn_sprite_sheet, 17);
	C2D_SpriteSetCenter(&btn_r_h_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&btn_r_h_sprite->spr, 280, 4);
	// -------------------------------------------------------------------

	// L Button;__________________________________________________________
	// Open State;
	// L - Released
	C2D_SpriteFromSheet(&btn_l_or_sprite->spr, btn_sprite_sheet, 18);
	C2D_SpriteSetCenter(&btn_l_or_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&btn_l_or_sprite->spr, 0, 4);
	// L - Held
	C2D_SpriteFromSheet(&btn_l_oh_sprite->spr, btn_sprite_sheet, 19);
	C2D_SpriteSetCenter(&btn_l_oh_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&btn_l_oh_sprite->spr, 0, 4);

	// Close State;
	// L - Released
	C2D_SpriteFromSheet(&btn_l_cr_sprite->spr, btn_sprite_sheet, 20);
	C2D_SpriteSetCenter(&btn_l_cr_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&btn_l_cr_sprite->spr, 0, 4);
	// L - Held
	C2D_SpriteFromSheet(&btn_l_ch_sprite->spr, btn_sprite_sheet, 21);
	C2D_SpriteSetCenter(&btn_l_ch_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&btn_l_ch_sprite->spr, 0, 4);
	// -------------------------------------------------------------------

	
	// C Stick;____________________________________________________________
	// Released State;
	// L - Released
	C2D_SpriteFromSheet(&c_stick_r_sprite->spr, btn_sprite_sheet, 22);
	C2D_SpriteSetCenter(&c_stick_r_sprite->spr, 0.5f, 0.0f);
	C2D_SpriteSetPos(&c_stick_r_sprite->spr, TOP_SCREEN_WIDTH / 2, TOP_SCREEN_HEIGHT - 24);

	// Held State;
	// Right
	C2D_SpriteFromSheet(&c_stick_hr_sprite->spr, btn_sprite_sheet, 23);
	C2D_SpriteSetCenter(&c_stick_hr_sprite->spr, 0.5f, 0.0f);
	C2D_SpriteSetPos(&c_stick_hr_sprite->spr, TOP_SCREEN_WIDTH / 2 + 8, TOP_SCREEN_HEIGHT - 24);
	// Left
	C2D_SpriteFromSheet(&c_stick_hl_sprite->spr, btn_sprite_sheet, 24);
	C2D_SpriteSetCenter(&c_stick_hl_sprite->spr, 0.5f, 0.0f);
	C2D_SpriteSetPos(&c_stick_hl_sprite->spr, TOP_SCREEN_WIDTH / 2 - 8, TOP_SCREEN_HEIGHT - 24);
	// -------------------------------------------------------------------

}

//---------------------------------------------------------------------------------
static void initHLSprites() {
//---------------------------------------------------------------------------------
	// array locations
	// mode column
	Sprite* arrow_hl_sprite = &sprites[32];

	// Letter Buttons General
	Sprite* ltrbtn_base = &sprites[33];
	// B Button
	Sprite* ltrbtn_b_on = &sprites[34];
	// A Button
	Sprite* ltrbtn_a_on = &sprites[35];
	// X Button
	Sprite* ltrbtn_x_on = &sprites[36];
	// Y Button
	Sprite* ltrbtn_y_on = &sprites[37];

	// Dice box container
	Sprite* dice_box_border = &sprites[38];

	// Dice;
	Sprite* d2_hl = &sprites[39];
	Sprite* d4_hl = &sprites[40];
	Sprite* d6_hl = &sprites[41];
	Sprite* d8_hl = &sprites[42];
	Sprite* d10_hl = &sprites[43];
	Sprite* d12_hl = &sprites[44];
	Sprite* d20_hl = &sprites[45];

	// Tray;
	// Dice Select
	Sprite* hl_select_f1 = &sprites[46];
	Sprite* hl_select_f2 = &sprites[47];

	// Highlight - Mode
	Sprite* hl_tray_d2 = &sprites[48];
	Sprite* hl_tray_d4 = &sprites[49];
	Sprite* hl_tray_d6 = &sprites[50];
	Sprite* hl_tray_d8 = &sprites[51];
	Sprite* hl_tray_d10 = &sprites[52];
	Sprite* hl_tray_d12 = &sprites[53];
	Sprite* hl_tray_d20 = &sprites[54];

	// ARROW COLUMN - amount & type;_______________________________________
	C2D_SpriteFromSheet(&arrow_hl_sprite->spr, hl_sprite_sheet, 0);
	C2D_SpriteSetCenter(&arrow_hl_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&arrow_hl_sprite->spr, 2, 92);
	// -------------------------------------------------------------------

	// LETTER BUTTON - general;_____________________________________________
	C2D_SpriteFromSheet(&ltrbtn_base->spr, hl_sprite_sheet, 1);
	C2D_SpriteSetCenter(&ltrbtn_base->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&ltrbtn_base->spr, 231, 102);
	// -------------------------------------------------------------------

	// B BUTTON ON__________________________________________________________
	C2D_SpriteFromSheet(&ltrbtn_b_on->spr, hl_sprite_sheet, 2);
	C2D_SpriteSetCenter(&ltrbtn_b_on->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&ltrbtn_b_on->spr, 262, 116);
	// -------------------------------------------------------------------

	// A BUTTON ON_________________________________________________________
	C2D_SpriteFromSheet(&ltrbtn_a_on->spr, hl_sprite_sheet, 3);
	C2D_SpriteSetCenter(&ltrbtn_a_on->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&ltrbtn_a_on->spr, 262, 116);
	// -------------------------------------------------------------------

	// X BUTTON ON_________________________________________________________
	C2D_SpriteFromSheet(&ltrbtn_x_on->spr, hl_sprite_sheet, 4);
	C2D_SpriteSetCenter(&ltrbtn_x_on->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&ltrbtn_x_on->spr, 262, 116);
	// -------------------------------------------------------------------

	// Y BUTTON ON_________________________________________________________
	C2D_SpriteFromSheet(&ltrbtn_y_on->spr, hl_sprite_sheet, 5);
	C2D_SpriteSetCenter(&ltrbtn_y_on->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&ltrbtn_y_on->spr, 262, 116);
	// -------------------------------------------------------------------

	// Dice Border_________________________________________________________
	C2D_SpriteFromSheet(&dice_box_border->spr, hl_sprite_sheet, 6);
	C2D_SpriteSetCenter(&dice_box_border->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&dice_box_border->spr, 96, 62);
	// -------------------------------------------------------------------

	// d2;_________________________________________________________________
	C2D_SpriteFromSheet(&d2_hl->spr, hl_sprite_sheet, 7);
	C2D_SpriteSetCenter(&d2_hl->spr, 0.5f, 0.5f);
	C2D_SpriteSetPos(&d2_hl->spr, BOTTOM_SCREEN_WIDTH / 2, BOTTOM_SCREEN_HEIGHT / 2 + 8);
	// -------------------------------------------------------------------

	// d4;_________________________________________________________________
	C2D_SpriteFromSheet(&d4_hl->spr, hl_sprite_sheet, 8);
	C2D_SpriteSetCenter(&d4_hl->spr, 0.5f, 0.5f);
	C2D_SpriteSetPos(&d4_hl->spr, BOTTOM_SCREEN_WIDTH / 2, BOTTOM_SCREEN_HEIGHT / 2 + 8);
	// -------------------------------------------------------------------

	// d6;_________________________________________________________________
	C2D_SpriteFromSheet(&d6_hl->spr, hl_sprite_sheet, 9);
	C2D_SpriteSetCenter(&d6_hl->spr, 0.5f, 0.5f);
	C2D_SpriteSetPos(&d6_hl->spr, BOTTOM_SCREEN_WIDTH / 2, BOTTOM_SCREEN_HEIGHT / 2 + 8);
	// -------------------------------------------------------------------
	
	// d8;_________________________________________________________________
	C2D_SpriteFromSheet(&d8_hl->spr, hl_sprite_sheet, 10);
	C2D_SpriteSetCenter(&d8_hl->spr, 0.5f, 0.5f);
	C2D_SpriteSetPos(&d8_hl->spr, BOTTOM_SCREEN_WIDTH / 2, BOTTOM_SCREEN_HEIGHT / 2 + 8);
	// -------------------------------------------------------------------
	
	// d10;________________________________________________________________
	C2D_SpriteFromSheet(&d10_hl->spr, hl_sprite_sheet, 11);
	C2D_SpriteSetCenter(&d10_hl->spr, 0.5f, 0.5f);
	C2D_SpriteSetPos(&d10_hl->spr, BOTTOM_SCREEN_WIDTH / 2, BOTTOM_SCREEN_HEIGHT / 2 + 8);
	// -------------------------------------------------------------------
	
	// d12;________________________________________________________________
	C2D_SpriteFromSheet(&d12_hl->spr, hl_sprite_sheet, 12);
	C2D_SpriteSetCenter(&d12_hl->spr, 0.5f, 0.5f);
	C2D_SpriteSetPos(&d12_hl->spr, BOTTOM_SCREEN_WIDTH / 2, BOTTOM_SCREEN_HEIGHT / 2 + 8);
	// -------------------------------------------------------------------
	
	// d20;________________________________________________________________
	C2D_SpriteFromSheet(&d20_hl->spr, hl_sprite_sheet, 13);
	C2D_SpriteSetCenter(&d20_hl->spr, 0.5f, 0.5f);
	C2D_SpriteSetPos(&d20_hl->spr, BOTTOM_SCREEN_WIDTH / 2, BOTTOM_SCREEN_HEIGHT / 2 + 8);
	// -------------------------------------------------------------------

	// Dice Tray - Select;_________________________________________________
	// Frame 1
	C2D_SpriteFromSheet(&hl_select_f1->spr, hl_sprite_sheet, 14);
	C2D_SpriteSetCenter(&hl_select_f1->spr, 0.0f, 0.0f);

	// Frame 2
	C2D_SpriteFromSheet(&hl_select_f2->spr, hl_sprite_sheet, 15);
	C2D_SpriteSetCenter(&hl_select_f2->spr, 0.0f, 0.0f);
	// -------------------------------------------------------------------


	// Dice Tray - Highlights;_________________________________________________
	// d2 aura
	C2D_SpriteFromSheet(&hl_tray_d2->spr, hl_sprite_sheet, 16);
	C2D_SpriteSetCenter(&hl_tray_d2->spr, 0.0f, 0.0f);

	// d4 aura
	C2D_SpriteFromSheet(&hl_tray_d4->spr, hl_sprite_sheet, 17);
	C2D_SpriteSetCenter(&hl_tray_d4->spr, 0.0f, 0.0f);
	
	// d6 aura
	C2D_SpriteFromSheet(&hl_tray_d6->spr, hl_sprite_sheet, 18);
	C2D_SpriteSetCenter(&hl_tray_d6->spr, 0.0f, 0.0f);
	
	// d8 aura
	C2D_SpriteFromSheet(&hl_tray_d8->spr, hl_sprite_sheet, 19);
	C2D_SpriteSetCenter(&hl_tray_d8->spr, 0.0f, 0.0f);
	
	// d10 aura
	C2D_SpriteFromSheet(&hl_tray_d10->spr, hl_sprite_sheet, 20);
	C2D_SpriteSetCenter(&hl_tray_d10->spr, 0.0f, 0.0f);
	
	// d12 aura
	C2D_SpriteFromSheet(&hl_tray_d12->spr, hl_sprite_sheet, 21);
	C2D_SpriteSetCenter(&hl_tray_d12->spr, 0.0f, 0.0f);
	
	// d20 aura
	C2D_SpriteFromSheet(&hl_tray_d20->spr, hl_sprite_sheet, 21);
	C2D_SpriteSetCenter(&hl_tray_d20->spr, 0.0f, 0.0f);
	// -------------------------------------------------------------------
}

//---------------------------------------------------------------------------------
static void initDiceSprites() {
//---------------------------------------------------------------------------------
	// array locations
	// big dice
	Sprite* d2_sprite = &sprites[60];
	Sprite* d4_sprite = &sprites[61];
	Sprite* d6_sprite = &sprites[62];
	Sprite* d8_sprite = &sprites[63];
	Sprite* d10_sprite = &sprites[64];
	Sprite* d12_sprite = &sprites[65];
	Sprite* d20_sprite = &sprites[66];

	// tray dice
	Sprite* tray_d2_sprite = &sprites[70];
	Sprite* tray_d4_sprite = &sprites[71];
	Sprite* tray_d6_sprite = &sprites[72];
	Sprite* tray_d8_sprite = &sprites[73];
	Sprite* tray_d10_sprite = &sprites[74];
	Sprite* tray_d12_sprite = &sprites[75];
	Sprite* tray_d20_sprite = &sprites[76];

	// Big Dice
	// d2
	C2D_SpriteFromSheet(&d2_sprite->spr, dice_sprite_sheet, 0);
	C2D_SpriteSetCenter(&d2_sprite->spr, 0.5f, 0.5f);
	C2D_SpriteSetPos(&d2_sprite->spr, BOTTOM_SCREEN_WIDTH / 2, BOTTOM_SCREEN_HEIGHT / 2 + 8);

	// d4
	C2D_SpriteFromSheet(&d4_sprite->spr, dice_sprite_sheet, 1);
	C2D_SpriteSetCenter(&d4_sprite->spr, 0.5f, 0.5f);
	C2D_SpriteSetPos(&d4_sprite->spr, BOTTOM_SCREEN_WIDTH / 2, BOTTOM_SCREEN_HEIGHT / 2 + 8);

	// d6
	C2D_SpriteFromSheet(&d6_sprite->spr, dice_sprite_sheet, 2);
	C2D_SpriteSetCenter(&d6_sprite->spr, 0.5f, 0.5f);
	C2D_SpriteSetPos(&d6_sprite->spr, BOTTOM_SCREEN_WIDTH / 2, BOTTOM_SCREEN_HEIGHT / 2 + 8);
	
	// d8
	C2D_SpriteFromSheet(&d8_sprite->spr, dice_sprite_sheet, 3);
	C2D_SpriteSetCenter(&d8_sprite->spr, 0.5f, 0.5f);
	C2D_SpriteSetPos(&d8_sprite->spr, BOTTOM_SCREEN_WIDTH / 2, BOTTOM_SCREEN_HEIGHT / 2 + 8);
	
	// d10
	C2D_SpriteFromSheet(&d10_sprite->spr, dice_sprite_sheet, 4);
	C2D_SpriteSetCenter(&d10_sprite->spr, 0.5f, 0.5f);
	C2D_SpriteSetPos(&d10_sprite->spr, BOTTOM_SCREEN_WIDTH / 2, BOTTOM_SCREEN_HEIGHT / 2 + 8);

	// d12
	C2D_SpriteFromSheet(&d12_sprite->spr, dice_sprite_sheet, 5);
	C2D_SpriteSetCenter(&d12_sprite->spr, 0.5f, 0.5f);
	C2D_SpriteSetPos(&d12_sprite->spr, BOTTOM_SCREEN_WIDTH / 2, BOTTOM_SCREEN_HEIGHT / 2 + 8);
	
	// d20
	C2D_SpriteFromSheet(&d20_sprite->spr, dice_sprite_sheet, 6);
	C2D_SpriteSetCenter(&d20_sprite->spr, 0.5f, 0.5f);
	C2D_SpriteSetPos(&d20_sprite->spr, BOTTOM_SCREEN_WIDTH / 2, BOTTOM_SCREEN_HEIGHT / 2 + 8);
	// -------------------------------------------------------------------

	// Tray;_________________________________________________
	// d2
	C2D_SpriteFromSheet(&tray_d2_sprite->spr, dice_sprite_sheet, 7);
	C2D_SpriteSetCenter(&tray_d2_sprite->spr, 0.0f, 0.0f);

	// d4
	C2D_SpriteFromSheet(&tray_d4_sprite->spr, dice_sprite_sheet, 8);
	C2D_SpriteSetCenter(&tray_d4_sprite->spr, 0.0f, 0.0f);

	// d6
	C2D_SpriteFromSheet(&tray_d6_sprite->spr, dice_sprite_sheet, 9);
	C2D_SpriteSetCenter(&tray_d6_sprite->spr, 0.0f, 0.0f);
	
	// d8
	C2D_SpriteFromSheet(&tray_d8_sprite->spr, dice_sprite_sheet, 10);
	C2D_SpriteSetCenter(&tray_d8_sprite->spr, 0.0f, 0.0f);
	
	// d10
	C2D_SpriteFromSheet(&tray_d10_sprite->spr, dice_sprite_sheet, 11);
	C2D_SpriteSetCenter(&tray_d10_sprite->spr, 0.0f, 0.0f);
	
	// d12
	C2D_SpriteFromSheet(&tray_d12_sprite->spr, dice_sprite_sheet, 12);
	C2D_SpriteSetCenter(&tray_d12_sprite->spr, 0.0f, 0.0f);
	
	// d20
	C2D_SpriteFromSheet(&tray_d20_sprite->spr, dice_sprite_sheet, 13);
	C2D_SpriteSetCenter(&tray_d20_sprite->spr, 0.0f, 0.0f);
	// -------------------------------------------------------------------
}

//---------------------------------------------------------------------------------
static void initFishingSprites() {
//---------------------------------------------------------------------------------
	// array locations
	// docks
	Sprite* fishing_dockdef_sprite = &fishing_sprites[0];
	Sprite* fishing_dockprethrow_sprite = &fishing_sprites[1];
	Sprite* fishing_dockrest_sprite = &fishing_sprites[2];

	//water actual
	Sprite* fishing_water_sprite = &fishing_sprites[3];

	// Docks --------------------------------
	// cast / default
	C2D_SpriteFromSheet(&fishing_dockdef_sprite->spr, fishing_sprite_sheet, 0);
	C2D_SpriteSetCenter(&fishing_dockdef_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&fishing_dockdef_sprite->spr, 47, 26);

	// pre cast
	C2D_SpriteFromSheet(&fishing_dockprethrow_sprite->spr, fishing_sprite_sheet, 1);
	C2D_SpriteSetCenter(&fishing_dockprethrow_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&fishing_dockprethrow_sprite->spr, 47, 26);
	
	// rest
	C2D_SpriteFromSheet(&fishing_dockrest_sprite->spr, fishing_sprite_sheet, 2);
	C2D_SpriteSetCenter(&fishing_dockrest_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&fishing_dockrest_sprite->spr, 47, 26);
	
	// water actual
	C2D_SpriteFromSheet(&fishing_water_sprite->spr, fishing_sprite_sheet, 3);
	C2D_SpriteSetCenter(&fishing_water_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&fishing_water_sprite->spr, 105, 74);
	// -------------------------------------

	// fishing rod
	Sprite* fishing_rod_throw1_sprite = &fishing_sprites[4];
	Sprite* fishing_rod_throw2_sprite = &fishing_sprites[5];
	Sprite* fishing_rod_throw3_sprite = &fishing_sprites[6];
	Sprite* fishing_rod_throw4_sprite = &fishing_sprites[7];
	Sprite* fishing_rod_throw5_sprite = &fishing_sprites[8];

	Sprite* fishing_rod_still_sprite = &fishing_sprites[9];

	Sprite* fishing_rod_wobbl1_sprite = &fishing_sprites[10];
	Sprite* fishing_rod_wobbl2_sprite = &fishing_sprites[11];
	Sprite* fishing_rod_wobbl3_sprite = &fishing_sprites[12];
	Sprite* fishing_rod_wobbl4_sprite = &fishing_sprites[13];

	Sprite* fishing_rod_reel1_sprite = &fishing_sprites[15];
	Sprite* fishing_rod_reel2_sprite = &fishing_sprites[16];
	Sprite* fishing_rod_reel3_sprite = &fishing_sprites[17];
	Sprite* fishing_rod_reel4_sprite = &fishing_sprites[18];
	Sprite* fishing_rod_reel5_sprite = &fishing_sprites[19];
	Sprite* fishing_rod_reel6_sprite = &fishing_sprites[20];
	Sprite* fishing_rod_reel7_sprite = &fishing_sprites[21];
	Sprite* fishing_rod_reel8_sprite = &fishing_sprites[22];

	// Fishing rod --------------------------------
	// throws;
	// thr1
	C2D_SpriteFromSheet(&fishing_rod_throw1_sprite->spr, fishing_sprite_sheet, 4);
	C2D_SpriteSetCenter(&fishing_rod_throw1_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&fishing_rod_throw1_sprite->spr, 145, 36);
	// thr2
	C2D_SpriteFromSheet(&fishing_rod_throw2_sprite->spr, fishing_sprite_sheet, 5);
	C2D_SpriteSetCenter(&fishing_rod_throw2_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&fishing_rod_throw2_sprite->spr, 145, 36);
	// thr3
	C2D_SpriteFromSheet(&fishing_rod_throw3_sprite->spr, fishing_sprite_sheet, 6);
	C2D_SpriteSetCenter(&fishing_rod_throw3_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&fishing_rod_throw3_sprite->spr, 145, 36);
	// thr4
	C2D_SpriteFromSheet(&fishing_rod_throw4_sprite->spr, fishing_sprite_sheet, 7);
	C2D_SpriteSetCenter(&fishing_rod_throw4_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&fishing_rod_throw4_sprite->spr, 145, 36);
	// thr5
	C2D_SpriteFromSheet(&fishing_rod_throw5_sprite->spr, fishing_sprite_sheet, 8);
	C2D_SpriteSetCenter(&fishing_rod_throw5_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&fishing_rod_throw5_sprite->spr, 145, 36);
	// ------

	// still
	C2D_SpriteFromSheet(&fishing_rod_still_sprite->spr, fishing_sprite_sheet, 9);
	C2D_SpriteSetCenter(&fishing_rod_still_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&fishing_rod_still_sprite->spr, 145, 36);
	// ------

	// wobble;
	// wobbl1
	C2D_SpriteFromSheet(&fishing_rod_wobbl1_sprite->spr, fishing_sprite_sheet, 10);
	C2D_SpriteSetCenter(&fishing_rod_wobbl1_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&fishing_rod_wobbl1_sprite->spr, 145, 36);
	// wobbl2
	C2D_SpriteFromSheet(&fishing_rod_wobbl2_sprite->spr, fishing_sprite_sheet, 11);
	C2D_SpriteSetCenter(&fishing_rod_wobbl2_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&fishing_rod_wobbl2_sprite->spr, 145, 36);
	// wobbl3
	C2D_SpriteFromSheet(&fishing_rod_wobbl3_sprite->spr, fishing_sprite_sheet, 12);
	C2D_SpriteSetCenter(&fishing_rod_wobbl3_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&fishing_rod_wobbl3_sprite->spr, 145, 36);
	// wobbl4
	C2D_SpriteFromSheet(&fishing_rod_wobbl4_sprite->spr, fishing_sprite_sheet, 13);
	C2D_SpriteSetCenter(&fishing_rod_wobbl4_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&fishing_rod_wobbl4_sprite->spr, 145, 36);
	// ------
	
	// reel;
	// reel1
	C2D_SpriteFromSheet(&fishing_rod_reel1_sprite->spr, fishing_sprite_sheet, 14);
	C2D_SpriteSetCenter(&fishing_rod_reel1_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&fishing_rod_reel1_sprite->spr, 145, 36);
	// reel2
	C2D_SpriteFromSheet(&fishing_rod_reel2_sprite->spr, fishing_sprite_sheet, 15);
	C2D_SpriteSetCenter(&fishing_rod_reel2_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&fishing_rod_reel2_sprite->spr, 145, 36);
	// reel3
	C2D_SpriteFromSheet(&fishing_rod_reel3_sprite->spr, fishing_sprite_sheet, 16);
	C2D_SpriteSetCenter(&fishing_rod_reel3_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&fishing_rod_reel3_sprite->spr, 145, 36);
	// reel4
	C2D_SpriteFromSheet(&fishing_rod_reel4_sprite->spr, fishing_sprite_sheet, 17);
	C2D_SpriteSetCenter(&fishing_rod_reel4_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&fishing_rod_reel4_sprite->spr, 145, 36);
	// reel5
	C2D_SpriteFromSheet(&fishing_rod_reel5_sprite->spr, fishing_sprite_sheet, 18);
	C2D_SpriteSetCenter(&fishing_rod_reel5_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&fishing_rod_reel5_sprite->spr, 145, 36);
	// reel6
	C2D_SpriteFromSheet(&fishing_rod_reel6_sprite->spr, fishing_sprite_sheet, 19);
	C2D_SpriteSetCenter(&fishing_rod_reel6_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&fishing_rod_reel6_sprite->spr, 145, 36);
	// reel7
	C2D_SpriteFromSheet(&fishing_rod_reel7_sprite->spr, fishing_sprite_sheet, 20);
	C2D_SpriteSetCenter(&fishing_rod_reel7_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&fishing_rod_reel7_sprite->spr, 145, 36);
	// reel8
	C2D_SpriteFromSheet(&fishing_rod_reel8_sprite->spr, fishing_sprite_sheet, 21);
	C2D_SpriteSetCenter(&fishing_rod_reel8_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&fishing_rod_reel8_sprite->spr, 145, 36);
	// -------------------------------------

	// water
	Sprite* fishing_w1_sprite = &fishing_sprites[23];
	Sprite* fishing_w2_sprite = &fishing_sprites[24];
	Sprite* fishing_w3_sprite = &fishing_sprites[25];
	Sprite* fishing_w4_sprite = &fishing_sprites[26];
	Sprite* fishing_w5_sprite = &fishing_sprites[27];
	Sprite* fishing_w6_sprite = &fishing_sprites[28];
	Sprite* fishing_w7_sprite = &fishing_sprites[29];
	Sprite* fishing_w8_sprite = &fishing_sprites[30];
	Sprite* fishing_w9_sprite = &fishing_sprites[31];
	Sprite* fishing_w10_sprite = &fishing_sprites[32];
	Sprite* fishing_w11_sprite = &fishing_sprites[33];

	
	// water; --------------------------------
	// water1
	C2D_SpriteFromSheet(&fishing_w1_sprite->spr, fishing_sprite_sheet, 22);
	C2D_SpriteSetCenter(&fishing_w1_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&fishing_w1_sprite->spr, 109, 92);
	// water2
	C2D_SpriteFromSheet(&fishing_w2_sprite->spr, fishing_sprite_sheet, 23);
	C2D_SpriteSetCenter(&fishing_w2_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&fishing_w2_sprite->spr, 109, 92);
	// water3
	C2D_SpriteFromSheet(&fishing_w3_sprite->spr, fishing_sprite_sheet, 24);
	C2D_SpriteSetCenter(&fishing_w3_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&fishing_w3_sprite->spr, 109, 92);
	// water4
	C2D_SpriteFromSheet(&fishing_w4_sprite->spr, fishing_sprite_sheet, 25);
	C2D_SpriteSetCenter(&fishing_w4_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&fishing_w4_sprite->spr, 109, 92);
	// water5
	C2D_SpriteFromSheet(&fishing_w5_sprite->spr, fishing_sprite_sheet, 26);
	C2D_SpriteSetCenter(&fishing_w5_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&fishing_w5_sprite->spr, 109, 92);
	// water6
	C2D_SpriteFromSheet(&fishing_w6_sprite->spr, fishing_sprite_sheet, 27);
	C2D_SpriteSetCenter(&fishing_w6_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&fishing_w6_sprite->spr, 109, 92);
	// water7
	C2D_SpriteFromSheet(&fishing_w7_sprite->spr, fishing_sprite_sheet, 28);
	C2D_SpriteSetCenter(&fishing_w7_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&fishing_w7_sprite->spr, 109, 92);
	// water8
	C2D_SpriteFromSheet(&fishing_w8_sprite->spr, fishing_sprite_sheet, 29);
	C2D_SpriteSetCenter(&fishing_w8_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&fishing_w8_sprite->spr, 109, 92);
	// water9
	C2D_SpriteFromSheet(&fishing_w9_sprite->spr, fishing_sprite_sheet, 30);
	C2D_SpriteSetCenter(&fishing_w9_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&fishing_w9_sprite->spr, 109, 92);
	// water10
	C2D_SpriteFromSheet(&fishing_w10_sprite->spr, fishing_sprite_sheet, 31);
	C2D_SpriteSetCenter(&fishing_w10_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&fishing_w10_sprite->spr, 109, 92);
	// water11
	C2D_SpriteFromSheet(&fishing_w11_sprite->spr, fishing_sprite_sheet, 32);
	C2D_SpriteSetCenter(&fishing_w11_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&fishing_w11_sprite->spr, 109, 92);
	// -------------------------------------

	// loot
	// junk
	Sprite* fishing_junk1_sprite = &fishing_sprites[34];
	Sprite* fishing_junk2_sprite = &fishing_sprites[35];
	Sprite* fishing_junk3_sprite = &fishing_sprites[36];

	
	// junk; --------------------------------
	// junk1; can
	C2D_SpriteFromSheet(&fishing_junk1_sprite->spr, fishing_sprite_sheet, 33);
	C2D_SpriteSetCenter(&fishing_junk1_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&fishing_junk1_sprite->spr, 144, 64);
	// junk2; boot
	C2D_SpriteFromSheet(&fishing_junk2_sprite->spr, fishing_sprite_sheet, 34);
	C2D_SpriteSetCenter(&fishing_junk2_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&fishing_junk2_sprite->spr, 144, 64);
	// junk3; bottle
	C2D_SpriteFromSheet(&fishing_junk3_sprite->spr, fishing_sprite_sheet, 35);
	C2D_SpriteSetCenter(&fishing_junk3_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&fishing_junk3_sprite->spr, 144, 64);

	// fish
	Sprite* fishing_fish1_sprite = &fishing_sprites[37];
	Sprite* fishing_fish2_sprite = &fishing_sprites[38];
	Sprite* fishing_fish3_sprite = &fishing_sprites[39];

	// fish; --------------------------------
	// fish1
	C2D_SpriteFromSheet(&fishing_fish1_sprite->spr, fishing_sprite_sheet, 36);
	C2D_SpriteSetCenter(&fishing_fish1_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&fishing_fish1_sprite->spr, 144, 64);
	// fish2
	C2D_SpriteFromSheet(&fishing_fish2_sprite->spr, fishing_sprite_sheet, 37);
	C2D_SpriteSetCenter(&fishing_fish2_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&fishing_fish2_sprite->spr, 144, 64);
	// fish3; shimp
	C2D_SpriteFromSheet(&fishing_fish3_sprite->spr, fishing_sprite_sheet, 38);
	C2D_SpriteSetCenter(&fishing_fish3_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&fishing_fish3_sprite->spr, 144, 64);

	// treasure
	Sprite* fishing_treasure1_sprite = &fishing_sprites[40];
	Sprite* fishing_treasure2_sprite = &fishing_sprites[41];
	Sprite* fishing_treasure3_sprite = &fishing_sprites[42];

	// treasure; --------------------------------
	// treasure1; sword
	C2D_SpriteFromSheet(&fishing_treasure1_sprite->spr, fishing_sprite_sheet, 39);
	C2D_SpriteSetCenter(&fishing_treasure1_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&fishing_treasure1_sprite->spr, 144, 64);
	// treasure2; chest
	C2D_SpriteFromSheet(&fishing_treasure2_sprite->spr, fishing_sprite_sheet, 40);
	C2D_SpriteSetCenter(&fishing_treasure2_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&fishing_treasure2_sprite->spr, 144, 64);
	// treasure3; tome
	C2D_SpriteFromSheet(&fishing_treasure3_sprite->spr, fishing_sprite_sheet, 41);
	C2D_SpriteSetCenter(&fishing_treasure3_sprite->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&fishing_treasure3_sprite->spr, 144, 64);
	// -------------------------------------

	Sprite* fishing_btn_a_r = &fishing_sprites[43];
	Sprite* fishing_btn_a_h = &fishing_sprites[44];

	// A Button;___________________________________________________________
	// A - Released
	C2D_SpriteFromSheet(&fishing_btn_a_r->spr, fishing_sprite_sheet, 42);
	C2D_SpriteSetCenter(&fishing_btn_a_r->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&fishing_btn_a_r->spr, 287, 113);
	// A - Held
	C2D_SpriteFromSheet(&fishing_btn_a_h->spr, fishing_sprite_sheet, 43);
	C2D_SpriteSetCenter(&fishing_btn_a_h->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&fishing_btn_a_h->spr, 287, 113);
	// -------------------------------------------------------------------

	Sprite* fishing_btn_start = &fishing_sprites[45];
	Sprite* fishing_btn_select = &fishing_sprites[46];

	// btn - start
	C2D_SpriteFromSheet(&fishing_btn_start->spr, fishing_sprite_sheet, 44);
	C2D_SpriteSetCenter(&fishing_btn_start->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&fishing_btn_start->spr, 214, 225);
	// btn - select
	C2D_SpriteFromSheet(&fishing_btn_select->spr, fishing_sprite_sheet, 45);
	C2D_SpriteSetCenter(&fishing_btn_select->spr, 0.0f, 0.0f);
	C2D_SpriteSetPos(&fishing_btn_select->spr, 14, 225);
	// -------------------------------------------------------------------
}

//---------------------------------------------------------------------------------
static void initSceneText() {
//---------------------------------------------------------------------------------
	// set up dynamic buffer, support up to 4096 glyphs
	g_dynamic_roll_buf = C2D_TextBufNew(4096);
	g_dynamic_dice_buf = C2D_TextBufNew(4096);
	g_dynamic_fishing_buf = C2D_TextBufNew(4096);
	g_static_buf = C2D_TextBufNew(4096);

	//load fonts
	font[0] = C2D_FontLoad("romfs:/gfx/Saga8.bcfnt");
	C2D_FontSetFilter(font[0], GPU_NEAREST, GPU_NEAREST);
	//C2D_FontSetFilter(font[0], GPU_LINEAR, GPU_LINEAR);

	// set up copyright text
	C2D_TextParse(&g_static_copyright_text[0], g_static_buf, copyright_libs_string);
	C2D_TextParse(&g_static_copyright_text[1], g_static_buf, copyright_sounds_string);
	C2D_TextFontParse(&g_static_total_text, font[0], g_static_buf, "TOTAL");

	// optimize
	C2D_TextOptimize(&g_static_copyright_text[0]);
	C2D_TextOptimize(&g_static_copyright_text[1]);
	C2D_TextOptimize(&g_static_total_text);
}

//---------------------------------------------------------------------------------
static void renderRollText(int current_index, int display_page_n) {
//---------------------------------------------------------------------------------
	int page_mod = 0, results_final_total = 0, page_end_mod = 0;

	// Clear the dynamic text buffer
	C2D_TextBufClear(g_dynamic_roll_buf);

	// --- NOTATIONS TEXT ---
	// Generate roll text
	char buf_notation[6] = "\0", buf_total_amount[5] = "\0", buf_local_total[4] = "\0";
	C2D_Text dyn_notation_text, dyn_total_amount_text;

	// general total - text
	C2D_DrawText(&g_static_total_text, C2D_AlignCenter | C2D_AtBaseline | C2D_WithColor, 261.5f, 209.0f, 0.0f, TOTAL_TEXT_SIZE, TOTAL_TEXT_SIZE, C2D_Color32f(1.0f,1.0f,1.0f,1.0f));

	// display positions
	if(current_index >= PAGE_DISPLAY_MAX) {
		switch (display_page_n)
		{
			case 0:
				page_end_mod = current_index - PAGE_DISPLAY_MAX + 1;
				break;

			case 1:
				page_mod = 7;
				break;
		}
	}

	for (int i = 0 + page_mod; i <= current_index - page_end_mod; i++) {
		if (roll_queue[i].amount == 0) break;
		
		// ---------
		// notations
		// ---------
		// update to current values
		snprintf(buf_notation, sizeof(buf_notation), "%s", roll_queue[i].notation);

		// Size calculation
		float notation_size = DICE_AMOUNT_TEXT_SIZE - TEXT_SIZE_REDUCE * (strlen(buf_notation) - 1);

		// parse notation & optimize
		C2D_TextFontParse(&dyn_notation_text, font[0], g_dynamic_roll_buf, buf_notation);
		C2D_TextOptimize(&dyn_notation_text);

		float notation_x_pos_add = 198.0 * ((i % 7) % 2);
		float notation_y_pos_add = 54 * (((i % 7) / 2) % 4);
		float mini_x_pos_add = notation_x_pos_add > 0 ? notation_x_pos_add - 1 : 0;
				
		C2D_DrawText(&dyn_notation_text, C2D_AlignCenter | C2D_AtBaseline | C2D_WithColor, 54.0f + notation_x_pos_add, 30.0f - (strlen(buf_notation) - 1) + notation_y_pos_add, 0.0f, notation_size, notation_size, C2D_Color32f(1.0f,1.0f,1.0f,1.0f));

		// ---------
		// dice icon
		// --------
		C2D_ImageTint advantage_tint, disadvantage_tint;
		C2D_PlainImageTint(&advantage_tint, C2D_Color32(251, 200, 54, 255), 1.0f);
		C2D_PlainImageTint(&disadvantage_tint, C2D_Color32(182, 58, 202, 255), 1.0f);

		// mini dice
		C2D_SpriteSetPos(&sprites[70 + roll_queue[i].type_visual].spr, 11 + mini_x_pos_add, 13 + notation_y_pos_add);
		C2D_DrawSprite(&sprites[70 + roll_queue[i].type_visual].spr);

		// draw mini highlight
		switch (roll_queue[i].mode)
		{
		// advantage
			case DICE_ADVANTAGE:
			C2D_SpriteSetPos(&sprites[48 + roll_queue[i].type_visual].spr, 11 + mini_x_pos_add, 13 + notation_y_pos_add);
			C2D_DrawSpriteTinted(&sprites[48 + roll_queue[i].type_visual].spr, &advantage_tint);
			break;

		// disadvantage
			case DICE_DISADVANTAGE:
			C2D_SpriteSetPos(&sprites[48 + roll_queue[i].type_visual].spr, 11 + mini_x_pos_add, 13 + notation_y_pos_add);
			C2D_DrawSpriteTinted(&sprites[48 + roll_queue[i].type_visual].spr, &disadvantage_tint);
			break;
		}

		// ---------
		// roll text
		// ---------
		char buf_result1[5] = "\0", buf_result2[5] = "\0";
		C2D_Text dyn_res1_text, dyn_res2_text, dyn_local_total_text;

		int local_total = 0;

		for (int j = 0; j < roll_queue[i].amount; j++) {
			int res1 = roll_results[i].result1[j], res2 = roll_results[i].result2[j];
			local_total += res1;

			snprintf(buf_result1, sizeof(buf_result1), "%d", res1);
			snprintf(buf_result2, sizeof(buf_result2), "%d|", res2);

			// Size calculation
			float res1_len = floor(log10(abs(res1)));
			float res2_len = strlen(buf_result2);

			float result1_size = DICE_ROLL_TEXT_SIZE - TEXT_SIZE_REDUCE * 1.5 * res1_len;
			float result2_size = DICE_ROLL_TEXT_SIZE - (TEXT_SIZE_REDUCE / 1.25) * res2_len;

			// parse notation & optimize
			C2D_TextFontParse(&dyn_res1_text, font[0], g_dynamic_roll_buf, buf_result1);
			C2D_TextFontParse(&dyn_res2_text, font[0], g_dynamic_roll_buf, buf_result2);

			C2D_TextOptimize(&dyn_res1_text);
			C2D_TextOptimize(&dyn_res2_text);

			float notation_x_pos_add = 198.0 * ((i % 7) % 2), notation_y_pos_add = 54 * (((i % 7) / 2) % 4);
			float roll_x_pos_add = 36.0 * (j % 5) + notation_x_pos_add, 
				  roll_y_pos_add = 17.0 * (j / 5) + notation_y_pos_add;

			if (abs(roll_queue[i].mode)) {
				// Dis/Advantage
				C2D_DrawText(&dyn_res1_text, C2D_AlignCenter | C2D_AtBaseline | C2D_WithColor, 27.5f - (8 * (1 + result1_size)) + roll_x_pos_add, 47.0f + roll_y_pos_add, 0.0f, result1_size, result1_size, C2D_Color32f(1.0f,1.0f,1.0f,1.0f));
				C2D_DrawText(&dyn_res2_text, C2D_AlignCenter | C2D_AtBaseline | C2D_WithColor, 27.5f + (8 * (1 - result2_size)) + roll_x_pos_add, 47.0f + roll_y_pos_add, 0.0f, result2_size, result2_size, C2D_Color32f(0.7f,0.7f,0.7f,1.0f));
			} else {
				C2D_DrawText(&dyn_res1_text, C2D_AlignCenter | C2D_AtBaseline | C2D_WithColor, 27.5f + roll_x_pos_add, 47.0f + roll_y_pos_add, 0.0f, result1_size, result1_size, C2D_Color32f(1.0f,1.0f,1.0f,1.0f));
			}
		}

		// ---------
		// local total
		// ---------
		snprintf(buf_local_total, sizeof(buf_local_total), "%d", local_total);

		// Size calculation
		float local_len = floor(log10(abs(local_total)));
		float local_size = DICE_AMOUNT_TEXT_SIZE - TEXT_SIZE_REDUCE * local_len;

		// parse notation & optimize
		C2D_TextFontParse(&dyn_local_total_text, font[0], g_dynamic_roll_buf, buf_local_total);
		C2D_TextOptimize(&dyn_local_total_text);

		C2D_DrawText(&dyn_local_total_text, C2D_AlignCenter | C2D_AtBaseline | C2D_WithColor, 108.5f + notation_x_pos_add, 28.0f - local_len + notation_y_pos_add, 0.0f, local_size, local_size, C2D_Color32f(1.0f,1.0f,1.0f,1.0f));
	}

	// ---------
	// final total
	// ---------
	for (int i = 0; i <= current_index; i++) {
		results_final_total += roll_results[i].total;
	}

	snprintf(buf_total_amount, sizeof(buf_total_amount), "%d", results_final_total);

	// Size calculation
	float total_len = floor(log10(abs(results_final_total)));
	float total_size = TOTAL_TEXT_SIZE - TEXT_SIZE_REDUCE * total_len;

	// parse notation & optimize
	C2D_TextFontParse(&dyn_total_amount_text, font[0], g_dynamic_roll_buf, buf_total_amount);
	C2D_TextOptimize(&dyn_total_amount_text);

	C2D_DrawText(&dyn_total_amount_text, C2D_AlignCenter | C2D_AtBaseline | C2D_WithColor, 351.5f, 210.0f - total_len, 0.0f, total_size, total_size, C2D_Color32f(1.0f,1.0f,1.0f,1.0f));
}

//---------------------------------------------------------------------------------
static void renderDiceText(float amount_len, float type_len) {
//---------------------------------------------------------------------------------
	// clear buffer
	C2D_TextBufClear(g_dynamic_dice_buf);

	// --- BIG TEXT ---
	// Generate big dice text
	char buf_amount[4] = "\0", buf_type[4] = "\0", buf_tray[3] = "\0";
	C2D_Text dyn_dice_amount_text, dyn_dice_type_text, dyn_dice_tray_text;

	// Size calculation
	float amount_size = DICE_AMOUNT_TEXT_SIZE - TEXT_SIZE_REDUCE * amount_len;
	float type_size = DICE_TYPE_TEXT_SIZE - TEXT_SIZE_REDUCE * type_len;

	// update to current values
	snprintf(buf_amount, sizeof(buf_amount), "%d", amount);
	snprintf(buf_type, sizeof(buf_type), "%d", types[type_selected]);

	// Parse text
	C2D_TextFontParse(&dyn_dice_amount_text, font[0], g_dynamic_dice_buf, buf_amount);
	C2D_TextFontParse(&dyn_dice_type_text, font[0], g_dynamic_dice_buf, buf_type);

	// Optimize
	C2D_TextOptimize(&dyn_dice_amount_text);
	C2D_TextOptimize(&dyn_dice_type_text);

	// Display big text
	C2D_DrawText(&dyn_dice_amount_text, C2D_AlignCenter | C2D_AtBaseline, 18.5f, 143.0f - amount_len, 0.0f, amount_size, amount_size);
	C2D_DrawText(&dyn_dice_type_text, C2D_AlignCenter | C2D_AtBaseline, 72.5f, 143.0f - type_len, 0.0f, type_size, type_size);

	// --- TRAY TEXT ---
	for (int i = 0; i <= DICE_TRAY_MAX; i++) {
		if (tray[i].amount == 0) break;

		snprintf(buf_tray, sizeof(buf_tray), "%d", tray[i].amount);

		float tray_amount_len = floor(log10(abs(tray[i].amount)));
		float tray_size = DICE_TRAY_TEXT_SIZE - (TEXT_SIZE_REDUCE / 1.25) * tray_amount_len;

		// load amount
		C2D_TextFontParse(&dyn_dice_tray_text, font[0], g_dynamic_dice_buf, buf_tray);
		
		// optimize
		C2D_TextOptimize(&dyn_dice_tray_text);

		// draw
		C2D_DrawText(&dyn_dice_tray_text, C2D_AlignCenter | C2D_AtBaseline | C2D_WithColor, 68.5f + 15.0 * i, 14.0f, 0.0f, tray_size, tray_size, C2D_Color32f(1.0f,1.0f,1.0f,1.0f));
	}
}

//---------------------------------------------------------------------------------
static void renderFishingText(int rarity_d, int variant_d, int gold, int gold_tot, int rand_att1, int rand_att2) {
//---------------------------------------------------------------------------------
	// clear buffer
	C2D_TextBufClear(g_dynamic_fishing_buf);

	// --- attrib text ---
	// Generate big dice text
	char buf_flavor[200] = "\0", buf_total_gold[6] = "\0";
	C2D_Text dyn_flavor_text, dyn_gold_total_text;

	snprintf(buf_total_gold, sizeof(buf_total_gold), "%dG", gold_tot);

	// Size calculation
	float totalg_len = floor(log10(abs(gold_tot))) - 1;
	float totalg_size = 1.2f - TEXT_SIZE_REDUCE * totalg_len;

	// update to current values
	switch(rarity_d)
	{
		// junk
		case 1:
			switch (variant_d)
			{
				// can
				case 0:
					snprintf(buf_flavor, sizeof(buf_flavor), "You caught a %d year old [Can] worth %dG!", rand_att1, gold);
					break;

				// boot
				case 1:
					snprintf(buf_flavor, sizeof(buf_flavor), "You caught a [Boot] with %d\" laces worth %dG!", (rand_att1 * rand_att2) / 2, gold);
					break;

					
				// bottle
				case 2:
					snprintf(buf_flavor, sizeof(buf_flavor), "You caught a [Bottle] filled with %d [Broken Dreams] worth %dG!", rand_att1, gold);
					break;
			}
			break;
			
		// fish
		case 2:
			switch (variant_d)
			{
				// trout?
				case 0:
					snprintf(buf_flavor, sizeof(buf_flavor), "You caught a %d\"x%d\" [Trout] worth %dG!", rand_att1, rand_att2, gold);
					break;

				// cod?
				case 1:
					snprintf(buf_flavor, sizeof(buf_flavor), "You caught a %d\" [Fish] worth %dG!", rand_att1, gold);
					break;

					
				// shimp?
				case 2:
					snprintf(buf_flavor, sizeof(buf_flavor), "You caught %d [Shrimp] worth %dG!", rand_att1 * rand_att2, gold);
					break;
			}
			break;
			
		// treasure
		case 3:
			switch (variant_d)
			{
				// sword!!
				case 0:
					snprintf(buf_flavor, sizeof(buf_flavor), "You caught a %d\" lvl. %d [Sword] worth %dG!", rand_att1, rand_att2, gold);
					break;

				// treasure chest
				case 1:
					snprintf(buf_flavor, sizeof(buf_flavor), "You caught a [Treasure Chest] from %d years ago worth %dG!", rand_att1 * rand_att2, gold);
					break;

					
				// spell tome
				case 2:
					snprintf(buf_flavor, sizeof(buf_flavor), "You caught a lvl. %d [Tome] containing %d useful spells worth %dG!", rand_att1, rand_att2, gold);
					break;
			}
			break;
	}

	// Parse text
	C2D_TextFontParse(&dyn_flavor_text, font[0], g_dynamic_fishing_buf, buf_flavor);
	C2D_TextFontParse(&dyn_gold_total_text, font[0], g_dynamic_fishing_buf, buf_total_gold);

	// Optimize
	C2D_TextOptimize(&dyn_flavor_text);
	C2D_TextOptimize(&dyn_gold_total_text);

	// Display 
	C2D_DrawText(&dyn_flavor_text, C2D_AlignCenter | C2D_AtBaseline | C2D_WithColor | C2D_WordWrap, 199.5f, 191.0f, 1.0f, FISHING_FLAVOR_SIZE, FISHING_FLAVOR_SIZE, C2D_Color32f(1.0f,1.0f,1.0f,1.0f), 224.0f);
	C2D_DrawText(&dyn_gold_total_text, C2D_AlignCenter | C2D_AtBaseline | C2D_WithColor, 351.5f, 207.0f, 1.0f, totalg_size, totalg_size, C2D_Color32f(1.0f,1.0f,1.0f,1.0f));

}

//---------------------------------------------------------------------------------
static void renderCopyrightText() {
//---------------------------------------------------------------------------------

	// draw copyright info
	C2D_DrawText(&g_static_copyright_text[0], C2D_AlignLeft, 165.0f, 88.0f, 0.0f, COPYRIGHT_TEXT_SIZE, COPYRIGHT_TEXT_SIZE);
	C2D_DrawText(&g_static_copyright_text[1], C2D_AlignLeft, 165.0f, 112.0f, 0.0f, COPYRIGHT_TEXT_SIZE, COPYRIGHT_TEXT_SIZE);
}

//---------------------------------------------------------------------------------
static void exitDiceText() {
//---------------------------------------------------------------------------------
	// Delete the text buffers
	C2D_TextBufDelete(g_dynamic_roll_buf);
	C2D_TextBufDelete(g_dynamic_dice_buf);
	C2D_TextBufDelete(g_dynamic_fishing_buf);
	C2D_TextBufDelete(g_static_buf);
	C2D_FontFree(font[0]);
}

//---------------------------------------------------------------------------------
static int pnpoly(int nvert, int *vertx, int *verty, int testx, int testy) {
//---------------------------------------------------------------------------------
  int i = 0, j = 0, inside_flag = 0;
  for (i = 0, j = nvert-1; i < nvert; j = i++) {
    if ( ((verty[i]>testy) != (verty[j]>testy)) &&
     (testx < (vertx[j]-vertx[i]) * (testy-verty[i]) / (verty[j]-verty[i]) + vertx[i]) )
       inside_flag = !inside_flag;
  }
  return inside_flag;
}

//---------------------------------------------------------------------------------
static void resetDiceTray() {
//---------------------------------------------------------------------------------
	for (int i = 0; i <= DICE_TRAY_MAX; i++) {
		tray[i].amount = 0;
		tray[i].mode = DICE_NORMAL;
		tray[i].type = 0;
		tray[i].type_visual = 0;
		snprintf(tray[i].notation, sizeof(tray[i].notation),"%s", "\0");
	}
}

//---------------------------------------------------------------------------------
static void resetDiceQueue() {
//---------------------------------------------------------------------------------
	for (int i = 0; i <= DICE_TRAY_MAX; i++) {
		roll_queue[i].amount = 0;
		roll_queue[i].mode = DICE_NORMAL;
		roll_queue[i].type = 0;
		roll_queue[i].type_visual = 0;
		snprintf(roll_queue[i].notation, sizeof(roll_queue[i].notation),"%s", "\0");
	}
}

//---------------------------------------------------------------------------------
static void resetDiceResults() {
//---------------------------------------------------------------------------------
	for (int i = 0; i <= DICE_TRAY_MAX; i++) {
		for (int j = 0; j < 10; j++) {
			roll_results[i].result1[j] = 0;
			roll_results[i].result2[j] = 0;
		}
		roll_results[i].total = 0;
	}
}

//---------------------------------------------------------------------------------
static void resetCwavList() {
//---------------------------------------------------------------------------------
	for (int i = 0; i < CWAV_LIST_MAX; i++) {
		snprintf(cwav_list[i].path, sizeof(cwav_list[i].path),"%s", "\0");
		cwav_list[i].raw_cwav = NULL;
	}
}

//---------------------------------------------------------------------------------
static void audioInit() {
//---------------------------------------------------------------------------------
    ndspChnReset(0);
    ndspSetOutputMode(NDSP_OUTPUT_STEREO);
    ndspChnSetInterp(0, NDSP_INTERP_POLYPHASE);
    ndspChnSetRate(0, 96000);
    ndspChnSetFormat(0, NDSP_FORMAT_STEREO_PCM16);
}

//---------------------------------------------------------------------------------
static void populateCwavList() {
//---------------------------------------------------------------------------------
	
	for (int i = 0; i < sizeof(file_list) / sizeof(char*); i++) {
		CWAV* cwav = (CWAV*)malloc(sizeof(CWAV));

		FILE* file = fopen(file_list[i], "rb");
		if (!file)
		{
			cwavFree(cwav);
			free(cwav);
			continue;
		}

		fseek(file, 0, SEEK_END);
		u32 file_size = ftell(file);
		void* buffer = linearAlloc(file_size);
		if (!buffer) {
			// This should never happen (unless we load a file too big to fit)
			printf("Unable to populate cwav list, check spellings");
			svcBreak(USERBREAK_PANIC);
		}

		fseek(file, 0, SEEK_SET); 
		fread(buffer, 1, file_size, file);
		fclose(file);

		// Since we manually allocated the buffer, we use cwavLoad directly...
		cwavLoad(cwav, buffer, SOUND_PLAY_MAX);
		//cwav->dataBuffer = buffer; // (We store the buffer pointer here for convinience, but it's not required.)
		// or, we could have let the library handle it...
		// cwavFileLoad(cwav, fileList[i], maxSPlayList[i]);

		if (cwav->loadStatus == CWAV_SUCCESS)
		{
			snprintf(cwav_list[i].path, sizeof(cwav_list[i].path),"%s", file_list[i]);
			cwav_list[i].raw_cwav = cwav;
		}
		else
		{
			// Manually de-allocating the buffer
			cwavFree(cwav);
			//linearFree(cwav->dataBuffer);
			// or, if we used cwavFileLoad, let the library handle it.
			// cwavFileFree(cwav);

			free(cwav);
		}
	}
}

//---------------------------------------------------------------------------------
static void play_sound(int snd_index) {
//---------------------------------------------------------------------------------
	CWAV* cwav = cwav_list[snd_index].raw_cwav;

	if (cwav->numChannels == 2)
	{
		cwavPlay(cwav, 0, 1);
	}
	else
	{
		cwavPlay(cwav, 0, -1);
	}
}

//---------------------------------------------------------------------------------
static void freeCwavList() {
//---------------------------------------------------------------------------------
	for (int i = 0; i < CWAV_LIST_MAX; i++) {
		// Manually de-allocating the buffer
		cwavFree(cwav_list[i].raw_cwav);
		free(cwav_list[i].raw_cwav);
	}

    ndspChnReset(0);
}

//---------------------------------------------------------------------------------
static void copyDiceTray() {
//---------------------------------------------------------------------------------
	for (int i = 0; i <= DICE_TRAY_MAX; i++) {
		if (tray[i].amount == 0) continue;

		roll_queue[i].amount = tray[i].amount;
		roll_queue[i].type = tray[i].type;
		roll_queue[i].type_visual = tray[i].type_visual;
		roll_queue[i].mode = tray[i].mode;
		snprintf(roll_queue[i].notation, sizeof(roll_queue[i].notation), "%s", tray[i].notation);
	}
}

//---------------------------------------------------------------------------------
int randint(int n) {
//---------------------------------------------------------------------------------
    int end = 0, r = 0;

    if((n - 1) == RAND_MAX) {
        return rand();
    } else {
        assert (n <= RAND_MAX);

        end = RAND_MAX / n;
        assert (end > 0);
        end *= n;

        while((r = rand()) >= end) { }

        return r % n;
    }
}

//---------------------------------------------------------------------------------
int adv_dice_roll(int d_cnt, int d_type, int mode, int index) {
//---------------------------------------------------------------------------------
    int i = 0, dsr = 0, res1 = 0, res2 = 0;
	DiceRollResult* results = &roll_results[index];

	if (mode == DICE_NORMAL) {
		for(i = 0; i < d_cnt; i++) {
			results->result1[i] = randint(d_type) + 1;
			results->total += results->result1[i];
		}
	} else {
		for(i = 0; i < d_cnt / 2; i++) {
			res1 = randint(d_type) + 1;

			res2 = randint(d_type) + 1;

			switch(mode) {
			case DICE_ADVANTAGE:
				if (res1 > res2) {
					results->result1[i] = res1; // bigger result
					results->result2[i] = res2;
				} else {
					results->result1[i] = res2; // bigger result
					results->result2[i] = res1;
				}
				dsr = results->result1[i];
				results->total += dsr;
				break;

			case DICE_DISADVANTAGE:
				if (res1 > res2) {
					results->result1[i] = res2; // smaller result
					results->result2[i] = res1; 
				} else {
					results->result1[i] = res1; // smaller result
					results->result2[i] = res2; 
				}
				dsr = results->result1[i];
				results->total += dsr;
				break;
			}
		}
	}

    return results->total;
}

//---------------------------------------------------------------------------------
static void rollDiceQueue(int index) {
//---------------------------------------------------------------------------------
	Dice* current_dice = &roll_queue[index];

	if (current_dice->amount != 0) {
		if (roll_queue[index].type == 2) {
			play_sound(COIN_TINK);
		} else {
			play_sound(DICE_THUD + randint(4));
		}

		switch (current_dice->mode) 
		{
			case DICE_NORMAL:
				adv_dice_roll(current_dice->amount, current_dice->type, DICE_NORMAL, index);
				break;

			case DICE_ADVANTAGE:
				adv_dice_roll(current_dice->amount * 2, current_dice->type, DICE_ADVANTAGE, index);
				break;

			case DICE_DISADVANTAGE:
				adv_dice_roll(current_dice->amount * 2, current_dice->type, DICE_DISADVANTAGE, index);
				break;
		}

		if (current_dice->type == 20 && current_dice->amount == 1) {
			switch (roll_results[index].total) 
			{
				case 1:
					play_sound(CRIT_FAIL);
					break;

				case 20:
					play_sound(CRIT_SUCCESS);
					break;
			}
		}
	}
}

//---------------------------------------------------------------------------------
int main(int argc, char* argv[]) {
//---------------------------------------------------------------------------------
	// Init libs
	romfsInit();
	cfguInit(); // allow font system to work
	gfxInitDefault();
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
	C2D_Prepare();

	// initalize NDSP
	ndspInit();
	audioInit();

	// set seed for random
    srand(time(NULL));

	// Create screens
	C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
	C3D_RenderTarget* bot = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

	// Load graphics
	bg_sprite_sheet = C2D_SpriteSheetLoad("romfs:/gfx/bg_sprites.t3x");
	if (!bg_sprite_sheet) svcBreak(USERBREAK_PANIC);

	btn_sprite_sheet = C2D_SpriteSheetLoad("romfs:/gfx/button_sprites.t3x");
	if (!btn_sprite_sheet) svcBreak(USERBREAK_PANIC);

	hl_sprite_sheet = C2D_SpriteSheetLoad("romfs:/gfx/highlight_sprites.t3x");
	if (!hl_sprite_sheet) svcBreak(USERBREAK_PANIC);

	dice_sprite_sheet = C2D_SpriteSheetLoad("romfs:/gfx/dice_sprites.t3x");
	if (!dice_sprite_sheet) svcBreak(USERBREAK_PANIC);
	
	fishing_sprite_sheet = C2D_SpriteSheetLoad("romfs:/gfx/fishing_sprites.t3x");
	if (!fishing_sprite_sheet) svcBreak(USERBREAK_PANIC);

	// Initialize sprites
	initBGSprites();
	initButtonSprites();
	initHLSprites();
	initDiceSprites();
	initFishingSprites();

	// Initialize dice tray
	resetDiceTray();
	resetDiceQueue();
	resetDiceResults();

	// Initialize sounds
	resetCwavList();
	populateCwavList();
	
	// Setting up image tints for dice highlights
	C2D_ImageTint advantage_tint, disadvantage_tint, disabled_btn_tint;
	C2D_PlainImageTint(&advantage_tint, C2D_Color32(251, 200, 54, 255), 1.0f);
	C2D_PlainImageTint(&disadvantage_tint, C2D_Color32(182, 58, 202, 255), 1.0f);
	C2D_PlainImageTint(&disabled_btn_tint, C2D_Color32f(0.0f, 0.0f, 0.0f, 1.0f), 0.5f);

	// Initialize text
	initSceneText();

	// Main loop
	while (aptMainLoop())
	{
		hidScanInput();

		// Respond to user input - this frame
		// keys pressed
		u32 k_down = hidKeysDown();
		// keys held
		u32 k_held = hidKeysHeld();
		// keys released
		u32 k_up = hidKeysUp();

		if (k_down & KEY_START) {
			break; // break in order to return to hbmenu
		}

		if (k_down & KEY_SELECT) {
			switch (game_mode)
			{
				// Dice -> Fishing
				case GAME_DICE:
					game_mode = GAME_FISHING;
					break;

				// Fishing -> Dice
				case GAME_FISHING:
					game_mode = GAME_DICE;
					break;

			}
		}
		
		// --------------------------------------------------------------
		// Touch Hit Detect
		// --------------------------------------------------------------
		touchPosition touch;

		//Read the touch screen coordinates
		hidTouchRead(&touch);

		// calculate whats hit
		// Left UI hit detect - arrow buttons
		a_up_hit = pnpoly(arwbtn_nvert, amount_up_hitx, amount_up_hity, touch.px, touch.py);
		t_up_hit = pnpoly(arwbtn_nvert, type_up_hitx, type_up_hity, touch.px, touch.py);
		a_down_hit = pnpoly(arwbtn_nvert, amount_down_hitx, amount_down_hity, touch.px, touch.py);
		t_down_hit = pnpoly(arwbtn_nvert, type_down_hitx, type_down_hity, touch.px, touch.py);
		left_hit = pnpoly(arwbtn_nvert, left_hitx, left_hity, touch.px, touch.py);
		right_hit = pnpoly(arwbtn_nvert, right_hitx, right_hity, touch.px, touch.py);

		// Right UI hit detect - letter buttons
		btn_b_hit = pnpoly(ltrbtn_nvert, b_hitx, b_hity, touch.px, touch.py);
		btn_a_hit = pnpoly(ltrbtn_nvert, a_hitx, a_hity, touch.px, touch.py);
		btn_x_hit = pnpoly(ltrbtn_nvert, x_hitx, x_hity, touch.px, touch.py);
		btn_y_hit = pnpoly(ltrbtn_nvert, y_hitx, y_hity, touch.px, touch.py);

		// Shoulder Buttons
		r_hit = pnpoly(sldrbtn_nvert, r_hitx, r_hity, touch.px, touch.py);
		l_hit = pnpoly(sldrbtn_nvert, l_hitx, l_hity, touch.px, touch.py);

		// Render the scene
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

		if (game_mode == GAME_FISHING) {
		// --------------------------------------------------------------
		// top screen
		// --------------------------------------------------------------
			C2D_TargetClear(top, C2D_Color32f(0.0f, 0.0f, 0.0f, 1.0f));
			C2D_SceneBegin(top);

			// display loot
			if (loot_got) {
				renderFishingText(rarity_display, variant_display, gold_worth_disp, gold_total, rand_attrib1_disp, rand_attrib2_disp);

				switch(rarity_display)
				{
					// junk
					case 1:
						C2D_DrawSprite(&fishing_sprites[34 + variant_display].spr);
						break;
						
					// fish
					case 2:
						C2D_DrawSprite(&fishing_sprites[37 + variant_display].spr);
						break;
						
					// treasure
					case 3:
						C2D_DrawSprite(&fishing_sprites[40 + variant_display].spr);
						break;
				}
			}
					
		// --------------------------------------------------------------
		// bottom screen
		// --------------------------------------------------------------
			C2D_TargetClear(bot, C2D_Color32f(0.0f, 0.0f, 0.0f, 1.0f));
			C2D_SceneBegin(bot);

			// --------------------------------------------------------------
			// background
			// --------------------------------------------------------------
			// dock; if not cast be ready to
			C2D_DrawSprite(&fishing_sprites[rest_engaged + rod_not_cast].spr);
			// exit and back buttons
			C2D_DrawSprite(&fishing_sprites[45].spr);
			C2D_DrawSprite(&fishing_sprites[46].spr);

			// increment variable for cast animation; 5 frames
			if (animate_cast) {
				if (rod_cast_animation >= 5.0f) {
					animate_cast = 0;
				} else {
					rod_cast_animation += 0.1f;
				}
			}

			// random events
			frame_clock += 0.01f;
			if (frame_clock >= FRAMES_IN_SEC) {
				// randomlly wobble, 20%
				if (randint(5) == 0 && rod_cast_animation >= 5 && !animate_wobble) {
					animate_wobble = 1;
				}

				// randomly loot, 5%
				if (randint(20) == 0 && rod_cast_animation >= 5 && !activate_loot) {
					rarity = randint(3) + 1;
					variant = randint(3);
					rand_attrib1 = randint(99) + 1;
					rand_attrib2 = randint(10) + 1;
					gold_worth = randint(51);
					activate_loot = 1;
				}

				// reset clock
				frame_clock = 0.0f;
			}
			
			// loot animations
			if (activate_loot) {
				for (int i = 0; i < rarity; i++) {
					C2D_DrawRectSolid(167 + (i * 6), 72, 0, 4, 14, C2D_Color32f(1.0f, 1.0f, 1.0f, 1.0f));
					C2D_DrawRectSolid(167 + (i * 6), 88, 0, 4, 4, C2D_Color32f(1.0f, 1.0f, 1.0f, 1.0f));
				}

				if (loot_wait_clock >= FRAMES_LOOT_WAIT) {
					if (!animate_reel) {
						activate_loot = 0;
						loot_wait_clock = 0.0f;
					}
				} else {
					loot_wait_clock += 0.01f;
				}
			}

			// increment animation for wobble; 4 frames
			if (animate_wobble) {
				if (rod_wobble_animation > 4.9f) {
					animate_wobble = 0;
					rod_wobble_animation = 4.0f;
				} else {
					rod_wobble_animation += 0.05f;
				}
			} else if (rod_wobble_animation > 0) {
				rod_wobble_animation = 0;
			}

			// draw rod if not in rest
			int rod_mod = rod_cast_animation + rod_wobble_animation;
			if (!rod_not_cast && !animate_reel) {
				C2D_DrawSprite(&fishing_sprites[4 + rod_mod].spr);
			}

			// reel it in, and otherwise draw in rest
			if (animate_reel) {
				if (rod_reel_animation > 7.9f) {
					animate_reel = 0;
					rod_not_cast = 1;

					if (activate_loot) {
						loot_got = 1;
						activate_loot = 0;
						loot_wait_clock = 0.0f;

						// save values
						rarity_display = rarity;
						variant_display = variant;
						rand_attrib1_disp = rand_attrib1;
						rand_attrib2_disp = rand_attrib2;
						gold_worth_disp = gold_worth;
						gold_total += gold_worth;
						if (gold_total > 9999) gold_total = 9999;
					}

					rod_reel_animation = 0.0f;
				} else {
					rod_reel_animation += 0.05f;

					C2D_DrawSprite(&fishing_sprites[15 + (int)rod_reel_animation].spr);
				}
			}

			// ---------
			// water
			// ---------
			C2D_DrawSprite(&fishing_sprites[3].spr);

			if (frames_passed_no_input >= FRAMES_NO_INPUT_MAX) {
				frames_passed_no_input = FRAMES_NO_INPUT_MAX;
				if (rod_not_cast && !animate_reel) rest_engaged = 1;
			} else {
				frames_passed_no_input += 0.01f;
			}

			// water animation
			water_animation += 0.05; // 11 frames in animation
			if (water_animation > 11) water_animation = 0.0f;

			C2D_SpriteSetDepth(&fishing_sprites[23 + (int)water_animation].spr, 0.2f);
			C2D_DrawSprite(&fishing_sprites[23 + (int)water_animation].spr);

			// -------------------------------------------------------------------
			// input
			// -------------------------------------------------------------------
			if (k_down & KEY_A || k_held & KEY_A || btn_a_hit) {
				// pressed
				C2D_DrawSprite(&fishing_sprites[44].spr);

				rest_engaged = 0;
				frames_passed_no_input = 0;
			} else if (k_up & KEY_A || (btn_a_hit_old && btn_a_hit_old ^ btn_a_hit)) {
				switch (rod_not_cast)
				{
					// cast -> withdrawn
					case 0:
						// reset animation
						rod_cast_animation = 0.0f;
						rod_wobble_animation = 0.0f;

						// reel it in
						animate_reel = 1;
						break;

					// withdrawn -> cast
					case 1:
						rod_not_cast = 0;
						animate_cast = 1;	
						break;
				}
			} else {
				// released
				C2D_DrawSprite(&fishing_sprites[43].spr);
			}
			
		// ------------------------------------------------------
		// Save previous key values --- Spam Prevention
		// ------------------------------------------------------
			btn_a_hit_old = btn_a_hit; 	// A button

			C3D_FrameEnd(0);
		} else if (game_mode == GAME_DICE) {
		// --------------------------------------------------------------
		// top screen
		// --------------------------------------------------------------
			C2D_TargetClear(top, C2D_Color32f(0.0f, 0.0f, 0.0f, 1.0f));
			C2D_SceneBegin(top);
			
			// --------------------------------------------------------------
			// background
			// --------------------------------------------------------------
			C2D_DrawSprite(&sprites[0].spr);

			// --------------------------------------------------------------
			// Dice Roll Text
			// --------------------------------------------------------------
			renderRollText(roll_die_index, display_page);

			// --------------------------------------------------------------
			// C pad
			// --------------------------------------------------------------
			if (is_rolling == CURRENTLY_ROLLING) {
				C2D_DrawSpriteTinted(&sprites[28].spr, &disabled_btn_tint);
			} else if (k_down & KEY_CPAD_LEFT || k_held & KEY_CPAD_LEFT) {
				// Released -> Held Left
				C2D_DrawSprite(&sprites[30].spr);

				// move to 2nd page if there are results
				if (display_page == 1 && result_list_len > 0) display_page = 0; 

				if (k_down & KEY_CPAD_LEFT) play_sound(BTN_CLICK);

			} else if (k_down & KEY_CPAD_RIGHT || k_held & KEY_CPAD_RIGHT) {
				// Released -> Held Right
				C2D_DrawSprite(&sprites[29].spr);

				// move to 1st page
				if (display_page == 0 && result_list_len > PAGE_DISPLAY_MAX) display_page = 1; 
					
				if (k_down & KEY_CPAD_RIGHT) play_sound(BTN_CLICK);

			} else {
				// Held -> Released
				C2D_DrawSprite(&sprites[28].spr);

				if (k_up & KEY_CPAD_LEFT || k_up & KEY_CPAD_RIGHT || (release_r_sound_flag && info_mode == INFO_CLOSED)) {
					play_sound(BTN_RELEASE);
				}
			}

			// --------------------------------------------------------------
			// Dice Rolling
			// --------------------------------------------------------------
			if (is_rolling == CURRENTLY_ROLLING && is_rolling_old) {

				// --------------------------------------------------------------
				// Dice Rolling Mechanism
				// --------------------------------------------------------------
				// check if the current position in queue is the end, if so, end this rolling thing
				if (current_die_index >= die_queue_len) {
					// finished rolling all the dice
					is_rolling = READY_TO_ROLL;
		
					// reset variables
					roll_frames = 0.0f;
					current_die_index = 0;
					release_r_sound_flag = 1;

					// if not at the end, check if the current die was rolled, if so, roll it
					// rand rolled makes it cycle through some numbers before committing
				} else if (!was_rolled) {
					// roll dice
					rollDiceQueue(current_die_index);

					// mark rolled
					was_rolled = 1;
				}

				// once 100 frames pass, and it rolled, move on to next dice in queue
				if (roll_frames >= FRAMES_HELD_MAX / 2 && is_rolling == CURRENTLY_ROLLING && was_rolled) {
					current_die_index++;
					if (current_die_index >= PAGE_DISPLAY_MAX && result_list_len > PAGE_DISPLAY_MAX) {
						display_page = 1;
					}

					if (roll_die_index < result_list_len) {
						roll_die_index++;
					}
					
					was_rolled = 0;
					roll_frames = 0.0f;
				} else if (roll_frames < FRAMES_HELD_MAX) roll_frames += 0.01;
			}

		// --------------------------------------------------------------
		// bottom screen
		// --------------------------------------------------------------
			C2D_TargetClear(bot, C2D_Color32f(0.0f, 0.0f, 0.0f, 1.0f));
			C2D_SceneBegin(bot);

			// --------------------------------------------------------------
			// background
			// --------------------------------------------------------------
			C2D_DrawSprite(&sprites[1].spr);

			// --------------------------------------------------------------
			// Dice
			// --------------------------------------------------------------
			// Middle Bit
			// Float the dice
			sin_move_rads += 0.01f * WAVE_SPEED_MULTIPLIER;
			if (sin_move_rads > M_PI * 2) sin_move_rads = 0;

			float dice_sin_pos_add = sinf(sin_move_rads) * WAVE_MOVE_MULTIPLIER;

			// big dice - info toggle
			if (info_mode == INFO_CLOSED) {
				C2D_SpriteSetPos(&sprites[39+type_selected].spr, BOTTOM_SCREEN_WIDTH / 2, BOTTOM_SCREEN_HEIGHT / 2 + 8 + dice_sin_pos_add);
				C2D_SpriteSetPos(&sprites[60+type_selected].spr, BOTTOM_SCREEN_WIDTH / 2, BOTTOM_SCREEN_HEIGHT / 2 + 8 + dice_sin_pos_add);

				// Type Visualization - dice actual
				C2D_DrawSprite(&sprites[60+type_selected].spr);

				// Mode Visualization - dice aura
				switch (dice_mode) {
					// Normal
					case 0:
						C2D_DrawSprite(&sprites[39+type_selected].spr);
						break;
						
					// Advantage
					case 1:
						C2D_DrawSpriteTinted(&sprites[39+type_selected].spr, &advantage_tint);
						break;
						
					// Disadvantage
					case -1:
						C2D_DrawSpriteTinted(&sprites[39+type_selected].spr, &disadvantage_tint);
						break;
				}
			}

			// --------------------------------------------------------------
			// UI Text
			// --------------------------------------------------------------
			// Dice Text
			float amount_digits = floor(log10(abs(amount)));
			float type_digits = floor(log10(abs(types[type_selected])));
			renderDiceText(amount_digits, type_digits);

			// ------------------------------------------------------
			// buttons;
			// ------------------------------------------------------
			// different sfx toggle
			if (time_btn_held >= FRAMES_HELD_MIN) {
				held_sfx_toggle = 1;
			} else {
				held_sfx_toggle = 0;
			}

			// reset time held if these were released or not pressed
			if (!(k_held) && \
				(!a_up_hit && !a_down_hit && !t_up_hit && !t_down_hit  && \
				!r_hit && !l_hit && \
				!btn_b_hit && !btn_a_hit && !btn_x_hit && !btn_y_hit)) {
				time_btn_held = 0.0f;
			}

			// info toggle
			if (info_mode == INFO_CLOSED) {
				// button UP visual
				if ( k_down & KEY_DUP || k_held & KEY_DUP) {
					if (k_down & KEY_DUP) play_sound(BTN_CLICK);

					switch (dice_col)
					{
						// AMOUNT column selected
						case 0:
						// Amount: Released -> Held
							C2D_DrawSprite(&sprites[3].spr);
							C2D_DrawSprite(&sprites[6].spr);

							if (time_btn_held < FRAMES_HELD_MAX) time_btn_held += 0.01f;
							if ((k_down & KEY_DUP || time_btn_held >= FRAMES_HELD_MIN) && amount < AMOUNT_MAX) {
								amount++;
							}
						break;
						// TYPE column selected
						case 1:
							C2D_DrawSprite(&sprites[2].spr);
							C2D_DrawSprite(&sprites[7].spr);

							if (time_btn_held < FRAMES_HELD_MAX) time_btn_held += 0.01f;
							if ((k_down & KEY_DUP || time_btn_held >= FRAMES_HELD_MIN) && type_selected < TYPE_MAX) {
								type_selected++;
							}
						break;
					}
				} else if ( a_up_hit || t_up_hit) {
					if (a_up_hit) {
						C2D_DrawSprite(&sprites[3].spr);
						if (dice_col) dice_col = 0;
						
						if (time_btn_held < FRAMES_HELD_MAX) time_btn_held += 0.01f;
						if ((a_up_hit ^ a_up_hit_old || time_btn_held >= FRAMES_HELD_MIN) && amount < AMOUNT_MAX) {
								amount++;

								if (a_up_hit ^ a_up_hit_old) play_sound(BTN_CLICK);
							}
					} else {
						C2D_DrawSprite(&sprites[2].spr);
					}

					if (t_up_hit) {
						C2D_DrawSprite(&sprites[7].spr);
						if (!dice_col) dice_col = 1;

						if (time_btn_held < FRAMES_HELD_MAX) time_btn_held += 0.01f;
						if ((t_up_hit ^ t_up_hit_old || time_btn_held >= FRAMES_HELD_MIN) && type_selected < TYPE_MAX) {
								type_selected++;

								if (t_up_hit ^ t_up_hit_old) play_sound(BTN_CLICK);
							}
					} else {
						C2D_DrawSprite(&sprites[6].spr);
					}
				} else {
					// Held -> Released
					C2D_DrawSprite(&sprites[2].spr);
					C2D_DrawSprite(&sprites[6].spr);

					if (k_up & KEY_DUP || (a_up_hit ^ a_up_hit_old && a_up_hit_old) || (t_up_hit ^ t_up_hit_old && t_up_hit_old)) {
						if (held_sfx_toggle) {
							play_sound(BTN_RELEASE_HELD);
						} else play_sound(BTN_RELEASE);
					}
				}

				// button DOWN visual
				if (k_down & KEY_DDOWN || k_held & KEY_DDOWN) {
					if (k_down & KEY_DDOWN) play_sound(BTN_CLICK);

					switch (dice_col)
					{
						// AMOUNT column selected
						case 0:
							// Amount: Released -> Held
							C2D_DrawSprite(&sprites[5].spr);
							C2D_DrawSprite(&sprites[8].spr);

							if (time_btn_held < FRAMES_HELD_MAX) time_btn_held += 0.01f;
							if ((k_down & KEY_DDOWN || time_btn_held >= FRAMES_HELD_MIN) && amount > AMOUNT_MIN) {
								amount--;
							}
						break;
						// TYPE column selected
						case 1:
							// Type: Released -> Held
							C2D_DrawSprite(&sprites[4].spr);
							C2D_DrawSprite(&sprites[9].spr);

							if (time_btn_held < FRAMES_HELD_MAX) time_btn_held += 0.01f;
							if ((k_down & KEY_DDOWN || time_btn_held >= FRAMES_HELD_MIN) && type_selected > TYPE_MIN) {
								type_selected--;
							}
						break;
					}
				} else if ( a_down_hit || t_down_hit) {
					if (a_down_hit) {
						C2D_DrawSprite(&sprites[5].spr);
						if (dice_col) dice_col = 0;
						
						if (time_btn_held < FRAMES_HELD_MAX) time_btn_held += 0.01f;
						if ((a_down_hit ^ a_down_hit_old || time_btn_held >= FRAMES_HELD_MIN) && amount > AMOUNT_MIN) {
								amount--;

								if (a_down_hit ^ a_down_hit_old) play_sound(BTN_CLICK);
							}
					} else {
						C2D_DrawSprite(&sprites[4].spr);
					}

					if (t_down_hit) {
						C2D_DrawSprite(&sprites[9].spr);
						if (!dice_col) dice_col = 1;

						if (time_btn_held < FRAMES_HELD_MAX) time_btn_held += 0.01f;
						if ((t_down_hit ^ t_down_hit_old || time_btn_held >= FRAMES_HELD_MIN) && type_selected > TYPE_MIN) {
								type_selected--;

								if (t_down_hit ^ t_down_hit_old) play_sound(BTN_CLICK);
							}
					} else {
						C2D_DrawSprite(&sprites[8].spr);
					}
				} else {
					// Held -> Released
					C2D_DrawSprite(&sprites[4].spr);
					C2D_DrawSprite(&sprites[8].spr);

					if (k_up & KEY_DDOWN || (a_down_hit ^ a_down_hit_old && a_down_hit_old) || (t_down_hit ^ t_down_hit_old && t_down_hit_old)) {
						if (held_sfx_toggle) {
							play_sound(BTN_RELEASE_HELD);
						} else play_sound(BTN_RELEASE);
					}
				}

				// Column Select
				// button LEFT visual
				if (k_down & KEY_DLEFT || k_held & KEY_DLEFT || left_hit) {
					// Released -> Held
					C2D_DrawSprite(&sprites[11].spr);

					if (k_down & KEY_DLEFT || left_hit ^ left_hit_old) play_sound(BTN_CLICK);

					if (dice_col) dice_col = 0;
					if (time_btn_held < FRAMES_HELD_MAX) time_btn_held += 0.01f;
				} else {
					// Held -> Released
					C2D_DrawSprite(&sprites[10].spr);

					// click release sound
					if (k_up & KEY_DLEFT || (left_hit ^ left_hit_old && left_hit_old)) {
						if (held_sfx_toggle) {
							play_sound(BTN_RELEASE_HELD);
						} else play_sound(BTN_RELEASE);
					}
				}

				// button RIGHT visual
				if (k_down & KEY_DRIGHT || k_held & KEY_DRIGHT || right_hit) {
					// Released -> Held
					C2D_DrawSprite(&sprites[13].spr);

					if (k_down & KEY_DRIGHT || right_hit ^ right_hit_old) play_sound(BTN_CLICK);

					if (!dice_col) dice_col = 1;
					if (time_btn_held < FRAMES_HELD_MAX) time_btn_held += 0.01f;
				} else {
					// Held -> Released
					C2D_DrawSprite(&sprites[12].spr);

					// click release sound
					if (k_up & KEY_DRIGHT || (right_hit ^ right_hit_old && right_hit_old)) {
						if (held_sfx_toggle) {
							play_sound(BTN_RELEASE_HELD);
						} else play_sound(BTN_RELEASE);
					}
				}
			}

			// -----------------------------------------------------
			// Letters
			// -----------------------------------------------------
			if (info_mode == INFO_CLOSED) {
				// button B visual
				if ( k_down & KEY_B || k_held & KEY_B || btn_b_hit) {
					// Released -> Held
					C2D_DrawSprite(&sprites[15].spr);
					// Disco: Off -> On
					C2D_DrawSprite(&sprites[34].spr);
						

					if (time_btn_held < FRAMES_HELD_MAX) time_btn_held += 0.01f;
					if (k_down & KEY_B || btn_b_hit ^ btn_b_hit_old || time_btn_held >= FRAMES_HELD_MIN) 
					{
						if (k_down & KEY_B || btn_b_hit ^ btn_b_hit_old) play_sound(BTN_CLICK);

						// Delete recent dice added
						// get handle on info
						Dice* die = &tray[dice_select];
						if (die->amount == 0 && dice_select > DICE_TRAY_MIN) {
						// move selection backward
							if (dice_select > DICE_TRAY_MIN) dice_select--;
							die = &tray[dice_select];
						}

						// reset info
						die->amount = 0;
						die->mode = DICE_NORMAL;
						die->type = 0; 	
						die->type_visual = 0; 	
						// update notation to be empty
						snprintf(die->notation, sizeof(die->notation), "%s", "\0");  
					}
				} else {
					// Held -> Released
					C2D_DrawSprite(&sprites[14].spr);

					// click release sound
					if (k_up & KEY_B || (btn_b_hit ^ btn_b_hit_old && btn_b_hit_old)) {
						if (held_sfx_toggle) {
							play_sound(BTN_RELEASE_HELD);
						} else play_sound(BTN_RELEASE);
					}
				}

				// button A visual
				if (k_down & KEY_A || k_held & KEY_A || btn_a_hit) {
					// Released -> Held
					C2D_DrawSprite(&sprites[17].spr);
					// Disco: Off -> On
					C2D_DrawSprite(&sprites[35].spr);

					if (time_btn_held < FRAMES_HELD_MAX) time_btn_held += 0.01f;
					if ((k_down & KEY_A || btn_a_hit ^ btn_a_hit_old)) 
					{
						play_sound(BTN_CLICK);

						// Add Properties to tray cell selected
						Dice* die = &tray[dice_select];

						die->amount = amount; 				// amount of die
						die->mode = dice_mode; 				// normal / advantage / disadv.
						die->type = types[type_selected]; 	// literal type
						die->type_visual = type_selected; 	// index of type in type array
						// save notation for later text usage
						snprintf(die->notation, sizeof(die->notation), "%dd%d", amount, types[type_selected]); 

						// move selection forward
						if (dice_select < DICE_TRAY_MAX) {
							dice_select++;
						}
					}
				} else {
					// Held -> Released
					C2D_DrawSprite(&sprites[16].spr);

					// click release sound
					if (k_up & KEY_A || (btn_a_hit ^ btn_a_hit_old && btn_a_hit_old)) {
						if (held_sfx_toggle) {
							play_sound(BTN_RELEASE_HELD);
						} else play_sound(BTN_RELEASE);
					}
				}

				// button X visual
				if (k_down & KEY_X || k_held & KEY_X || btn_x_hit ) {
					// Released -> Held
					C2D_DrawSprite(&sprites[19].spr);
					// Disco: Off -> On
					C2D_DrawSprite(&sprites[36].spr);

					if (time_btn_held < FRAMES_HELD_MAX) time_btn_held += 0.01f;
					if ((k_down & KEY_X || btn_x_hit ^ btn_x_hit_old)) 
					{
						play_sound(BTN_CLICK);

						// Add or remove disdvantage
						// remove advantage
						switch (dice_mode) {
							// Normal, Advantage -> Disadvantage
							case DICE_NORMAL:
							case DICE_ADVANTAGE:
								dice_mode = DICE_DISADVANTAGE;
								break;
							// Disadvantage -> Normal
							case DICE_DISADVANTAGE:
								dice_mode = DICE_NORMAL;
								break;
						}
					}
				} else {
					// Held -> Released
					C2D_DrawSprite(&sprites[18].spr);

					// click release sound
					if (k_up & KEY_X || (btn_x_hit ^ btn_x_hit_old && btn_x_hit_old)) {
						if (held_sfx_toggle) {
							play_sound(BTN_RELEASE_HELD);
						} else play_sound(BTN_RELEASE);
					}
				}

				// button Y visual
				if (k_down & KEY_Y || k_held & KEY_Y || btn_y_hit) {
					// Released -> Held
					C2D_DrawSprite(&sprites[21].spr);
					// Disco: Off -> On
					C2D_DrawSprite(&sprites[37].spr);

					if (time_btn_held < FRAMES_HELD_MAX) time_btn_held += 0.01f;
					if ((k_down & KEY_Y || btn_y_hit ^ btn_y_hit_old)) 
					{
						play_sound(BTN_CLICK);

						// Add or remove advantage
						// remove disadvantage
						switch (dice_mode) {
							// Normal, Disdvantage -> Advantage
							case DICE_NORMAL:
							case DICE_DISADVANTAGE:
								dice_mode = DICE_ADVANTAGE;
								break;
							// Advantage -> Normal
							case DICE_ADVANTAGE:
								dice_mode = DICE_NORMAL;
								break;
						}
					}
				} else {
					// Held -> Released
					C2D_DrawSprite(&sprites[20].spr);

					// click release sound
					if (k_up & KEY_Y || (btn_y_hit ^ btn_y_hit_old && btn_y_hit_old)) {
						if (held_sfx_toggle) {
							play_sound(BTN_RELEASE_HELD);
						} else play_sound(BTN_RELEASE);
					}
				}

				// ------------------------------------------------------
				// highlights:
				// ------------------------------------------------------
				// arrow column
				C2D_DrawSprite(&sprites[32].spr);
				if (dice_col != dice_col_old)
				{
					switch (dice_col)
					{
						case 0:
						C2D_SpriteSetPos(&sprites[32].spr, 2, 92);
						break;
						case 1:
						C2D_SpriteSetPos(&sprites[32].spr, 56, 92);
						break;
					}
				}

				// Letter Buttons General
				C2D_DrawSprite(&sprites[33].spr);

				// Dice Border
				C2D_DrawSprite(&sprites[38].spr); 
			}

			// ------------------------------------------------------
			// Tray;
			// ------------------------------------------------------
			// loop over the dice in the tray array
			for (int i = 0; i <= DICE_TRAY_MAX; i++) {
				if (tray[i].amount == 0) continue;

				// draw mini sprites
				C2D_SpriteSetPos(&sprites[70 + tray[i].type_visual].spr, 61 + 15 * i, 16);
				C2D_DrawSprite(&sprites[70 + tray[i].type_visual].spr);

				// draw mini highlight
				switch (tray[i].mode)
				{
				// advantage
				case DICE_ADVANTAGE:
					C2D_SpriteSetPos(&sprites[48 + tray[i].type_visual].spr, 61 + 15 * i, 16);
					C2D_DrawSpriteTinted(&sprites[48 + tray[i].type_visual].spr, &advantage_tint);
					break;

				// disadvantage
				case DICE_DISADVANTAGE:
					C2D_SpriteSetPos(&sprites[48 + tray[i].type_visual].spr, 61 + 15 * i, 16);
					C2D_DrawSpriteTinted(&sprites[48 + tray[i].type_visual].spr, &disadvantage_tint);
					break;
				}
			}

			// Dice Select - Tray
			select_clock += 0.01f;
			if (select_clock > 0.60f) select_clock = 0;

			C2D_SpriteSetPos(&sprites[46 + (int)(select_clock / 0.3f)].spr, 61 + 15 * dice_select, 16);
			C2D_DrawSprite(&sprites[46 + (int)(select_clock / 0.3f)].spr);

			// ------------------------------------------------------
			// shoulder buttons; 
			// ------------------------------------------------------
			// R Button
			if (is_rolling == CURRENTLY_ROLLING) {
				C2D_DrawSpriteTinted(&sprites[23].spr, &disabled_btn_tint);
				if (time_btn_held < FRAMES_HELD_MAX) time_btn_held += 0.01f;
			} else if ((k_down & KEY_R || k_held & KEY_R || r_hit) && info_mode == INFO_CLOSED) {
				// Released -> Held
				C2D_DrawSprite(&sprites[23].spr);

				if (time_btn_held < FRAMES_HELD_MAX) time_btn_held += 0.01f;
				if (k_down & KEY_R || r_hit ^ r_hit_old) {
					// click sound
					play_sound(BTN_CLICK);

					// copy the dice and clear tray
					if (is_rolling == READY_TO_ROLL && dice_select > 0) {
						// empty queue, update it with current tray, clear tray
						resetDiceQueue();
						copyDiceTray();
						resetDiceTray();
						// clear results
						resetDiceResults();
						roll_die_index = 0;

						// disable R button temporarily
						is_rolling = CURRENTLY_ROLLING;

						// save length and reset position
						die_queue_len = dice_select + 1;
						result_list_len = dice_select;
						dice_select = 0;
						display_page = 0;
					}
				}
			} else {
				// Held -> Released
				C2D_DrawSprite(&sprites[22].spr);
			
				// click release sound
				if (((release_r_sound_flag && info_mode == INFO_CLOSED) || k_up & KEY_R || (r_hit ^ r_hit_old && r_hit_old)) && info_mode == INFO_CLOSED) {
					if (held_sfx_toggle || release_r_sound_flag) {
						play_sound(BTN_RELEASE_HELD);
						if (release_r_sound_flag) release_r_sound_flag = 0;
					} else play_sound(BTN_RELEASE);
				}
			}

			// ------------------------------------------------------
			// info window:
			// ------------------------------------------------------
			if (info_mode == INFO_OPEN) 
			{
				// display over everything else but the return button
				// fade
				C2D_DrawSprite(&sprites[80].spr);
				// window
				C2D_DrawSprite(&sprites[81].spr);

				// copyright text
				renderCopyrightText();
			}
			
			// L Button
			if (k_down & KEY_L || k_held & KEY_L || l_hit) {
				// Released -> Held
				C2D_DrawSprite(&sprites[25 + info_mode*2].spr);

				if (time_btn_held < FRAMES_HELD_MAX) time_btn_held += 0.01f;
				if (k_down & KEY_L || l_hit ^ l_hit_old) {
					// click sound
					play_sound(BTN_CLICK);
					
					// switch button and window state
					switch (info_mode) {
						case INFO_OPEN:
							info_mode = INFO_CLOSED;
							break;
							
						case INFO_CLOSED:
							info_mode = INFO_OPEN;
							break;
					}
				}
			} else {
				// Held -> Released
				C2D_DrawSprite(&sprites[24 + info_mode*2].spr);

				if (k_up & KEY_L || (l_hit ^ l_hit_old && l_hit_old)) {
					if (held_sfx_toggle) {
						play_sound(BTN_RELEASE_HELD);
					} else play_sound(BTN_RELEASE);
				}
			}

		// ------------------------------------------------------
		// Save previous key values --- Spam Prevention
		// ------------------------------------------------------
			dice_col_old = dice_col; 	// to make sure that the column highlight 
										// doesnt need to move as much
			dice_mode_old = dice_mode; // Dis/Advantage

			a_up_hit_old = a_up_hit; 	// Amount UP
			t_up_hit_old = t_up_hit; 	// Type UP
			a_down_hit_old = a_down_hit; // Amount DOWN
			t_down_hit_old = t_down_hit; // Type DOWN
			right_hit_old = right_hit;	//Column right
			left_hit_old = left_hit;	//Column left

			btn_b_hit_old = btn_b_hit; 	// B button
			btn_a_hit_old = btn_a_hit; 	// A button
			btn_x_hit_old = btn_x_hit; 	// X button
			btn_y_hit_old = btn_y_hit; 	// Y button

			r_hit_old = r_hit; // R shoulder button
			l_hit_old = l_hit; // L shoulder button

			is_rolling_old = is_rolling; // current rolling sitch

			C3D_FrameEnd(0);
		}
	}

	// Deinitialize Scene Text
	exitDiceText();

	// Free sounds
	freeCwavList();

	// Delete graphics
	C2D_SpriteSheetFree(bg_sprite_sheet);
	C2D_SpriteSheetFree(btn_sprite_sheet);  
	C2D_SpriteSheetFree(hl_sprite_sheet);
	C2D_SpriteSheetFree(dice_sprite_sheet);
	C2D_SpriteSheetFree(fishing_sprite_sheet);
	

	// Deinit libs
	ndspExit();
	C2D_Fini();
	C3D_Fini();
	gfxExit();
	cfguExit();
	romfsExit();
	return 0;
}