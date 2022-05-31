#include <stdio.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include <math.h>

#define LOG_ENABLED

const int FPS = 60;
const int SCREEN_W = 800;
const int SCREEN_H = 600;
const int RESERVE_SAMPLES = 40;
enum {
    SCENE_MENU = 1,
    SCENE_START = 2,
    SCENE_SETTINGS = 3,
	SCENE_DEATH = 4,
	SCENE_CHARSELECT = 5,
	SCENE_HOWTOPLAY = 6
};

int active_scene;
bool key_state[ALLEGRO_KEY_MAX];
bool *mouse_state;
int mouse_x, mouse_y;

ALLEGRO_DISPLAY* game_display;
ALLEGRO_EVENT_QUEUE* game_event_queue;
ALLEGRO_TIMER* game_update_timer;

ALLEGRO_FONT* font_pirulen_20;
ALLEGRO_FONT* font_pirulen_24;
ALLEGRO_FONT* font_pirulen_32;
ALLEGRO_FONT* font_pirulen_40;

ALLEGRO_BITMAP* main_img_background;
ALLEGRO_BITMAP* img_settings;
ALLEGRO_BITMAP* img_settings2;
ALLEGRO_BITMAP* img_charselect;
ALLEGRO_BITMAP* img_charselect2;
ALLEGRO_BITMAP* img_exit;
ALLEGRO_BITMAP* img_exit2;
ALLEGRO_SAMPLE* menu_bgm;
ALLEGRO_SAMPLE_ID menu_bgm_id;
ALLEGRO_SAMPLE* submenu_bgm;
ALLEGRO_SAMPLE_ID submenu_bgm_id;

ALLEGRO_BITMAP* img_bg_red;
ALLEGRO_BITMAP* start_img_background;
ALLEGRO_BITMAP* img_white;
ALLEGRO_BITMAP* img_red;
ALLEGRO_BITMAP* img_blue;
ALLEGRO_BITMAP* img_bullet1;
ALLEGRO_BITMAP* img_bullet2;
ALLEGRO_BITMAP* img_bullet3;
ALLEGRO_BITMAP* img_hitmarker;
ALLEGRO_BITMAP* img_ult3;
ALLEGRO_BITMAP* img_enemy;
ALLEGRO_BITMAP* img_explosion;
ALLEGRO_SAMPLE* battle_bgm;
ALLEGRO_SAMPLE_ID battle_bgm_id;
ALLEGRO_SAMPLE* bullet_sound;
ALLEGRO_SAMPLE_ID bullet_sound_id;
ALLEGRO_SAMPLE* hit_sound;
ALLEGRO_SAMPLE_ID hit_sound_id;
ALLEGRO_SAMPLE* explosion_sound;
ALLEGRO_SAMPLE_ID explosion_sound_id;
ALLEGRO_SAMPLE* ult3_sound;
ALLEGRO_SAMPLE_ID ult3_sound_id;

ALLEGRO_BITMAP* img_checkbox;
ALLEGRO_BITMAP* img_checkbox2;
ALLEGRO_BITMAP* img_button;
ALLEGRO_BITMAP* img_button2;
ALLEGRO_BITMAP* img_howtoplay;
ALLEGRO_BITMAP* img_howtoplay2;

ALLEGRO_SAMPLE* death_bgm;
ALLEGRO_SAMPLE_ID death_bgm_id;

ALLEGRO_BITMAP* img_square;

typedef struct {
    float x, y;  // center coordinate
    float w, h;
    float v, vx, vy;
    bool hidden;
    ALLEGRO_BITMAP* img;
	float hp;
	int ultcd;
} MovableObject;

void draw_movable_object(MovableObject obj);

#define MAX_ENEMY 3

MovableObject plane;
MovableObject enemies[MAX_ENEMY];

int MAX_BULLET = 8;
MovableObject bullets[100];
MovableObject ult1_bullets[10];
MovableObject ult2_bullets[6];

const float MAX_COOLDOWN = 0.2;
double now;
double last_shoot_timestamp;
double last_spawn_timestamp;
double last_hit_timestamp[100];
double posY[100];
double posX[100];
double last_death_timestamp[100];
double posY2[100];
double posX2[100];
double last_ult_timestamp;
double last_ultshoot_timestamp;
double last_ulthit_timestamp[10];
double posYult[100];
double posXult[100];
int planechoice = 2;
long long score;
char highscore[20];
long long hscore = 0;
float volume = 1;
int muted = 0;
int control = 1;
int shoot;
int ult;
int reset;

void allegro5_init(void);
void game_init(void);
void game_start_event_loop(void);
void game_update(void);
void game_draw(void);
void game_destroy(void);
void game_change_scene(int next_scene);

ALLEGRO_BITMAP *load_bitmap_resized(const char *filename, int w, int h);

bool pnt_in_rect(int px, int py, int x, int y, int w, int h);
bool collide(MovableObject obj1, MovableObject obj2);
int finddir(float a, float b);
int press(int btn);

void on_key_down(int keycode);
void on_mouse_down(int btn, int x, int y);

void game_abort(const char* format, ...);
void game_log(const char* format, ...);
void game_vlog(const char* format, va_list arg);

int main(int argc, char** argv) {
    srand(time(NULL));
    allegro5_init();
    game_log("Allegro5 initialized");
    game_log("Game begin");
    game_init();
    game_log("Game initialized");
    game_draw();
    game_log("Game start event loop");
    game_start_event_loop();
    game_log("Game end");
    game_destroy();
    return 0;
}

void allegro5_init(void) {
    if (!al_init())
        game_abort("failed to initialize allegro");

    if (!al_init_primitives_addon())
        game_abort("failed to initialize primitives add-on");
    if (!al_init_font_addon())
        game_abort("failed to initialize font add-on");
    if (!al_init_ttf_addon())
        game_abort("failed to initialize ttf add-on");
    if (!al_init_image_addon())
        game_abort("failed to initialize image add-on");
    if (!al_install_audio())
        game_abort("failed to initialize audio add-on");
    if (!al_init_acodec_addon())
        game_abort("failed to initialize audio codec add-on");
    if (!al_reserve_samples(RESERVE_SAMPLES))
        game_abort("failed to reserve samples");
    if (!al_install_keyboard())
        game_abort("failed to install keyboard");
    if (!al_install_mouse())
        game_abort("failed to install mouse");

    game_display = al_create_display(SCREEN_W, SCREEN_H);
    if (!game_display)
        game_abort("failed to create display");
    al_set_window_title(game_display, "I2P(I)_2020 Final Project 109006241");

    game_update_timer = al_create_timer(1.0f / FPS);
    if (!game_update_timer)
        game_abort("failed to create timer");

    game_event_queue = al_create_event_queue();
    if (!game_event_queue)
        game_abort("failed to create event queue");

    const unsigned m_buttons = al_get_mouse_num_buttons();
    game_log("There are total %u supported mouse buttons", m_buttons);
    mouse_state = malloc((m_buttons + 1) * sizeof(bool));
    memset(mouse_state, false, (m_buttons + 1) * sizeof(bool));

    al_register_event_source(game_event_queue, al_get_display_event_source(game_display));
    al_register_event_source(game_event_queue, al_get_timer_event_source(game_update_timer));
    al_register_event_source(game_event_queue, al_get_keyboard_event_source());
    al_register_event_source(game_event_queue, al_get_mouse_event_source());

    al_start_timer(game_update_timer);
}

void game_init(void) {
	font_pirulen_20 = al_load_font("pirulen.ttf", 20, 0);
	if (!font_pirulen_20)
		game_abort("failed to load font: pirulen.ttf with size 20");

	font_pirulen_24 = al_load_font("pirulen.ttf", 24, 0);
	if (!font_pirulen_24)
		game_abort("failed to load font: pirulen.ttf with size 24");

    font_pirulen_32 = al_load_font("pirulen.ttf", 32, 0);
    if (!font_pirulen_32)
        game_abort("failed to load font: pirulen.ttf with size 32");
	
	font_pirulen_40 = al_load_font("pirulen.ttf", 40, 0);
    if (!font_pirulen_40)
        game_abort("failed to load font: pirulen.ttf with size 40");

    main_img_background = load_bitmap_resized("main-bg.jpg", SCREEN_W, SCREEN_H);

    menu_bgm = al_load_sample("menu.ogg");
    if (!menu_bgm)
        game_abort("failed to load audio: menu.ogg");

	submenu_bgm = al_load_sample("submenu.ogg");
	if (!submenu_bgm)
		game_abort("failed to load audio: submenu.ogg");

    img_settings = load_bitmap_resized("settings.png", 45, 45);
    if (!img_settings)
        game_abort("failed to load image: settings.png");
    img_settings2 = load_bitmap_resized("settings2.png", 45, 45);
    if (!img_settings2)
        game_abort("failed to load image: settings2.png");
	
	img_charselect = load_bitmap_resized("char_select.jpg", 45, 45);
    if (!img_charselect)
        game_abort("failed to load image: char_select.jpg");
    img_charselect2 = load_bitmap_resized("char_select2.jpg", 45, 45);
    if (!img_charselect2)
        game_abort("failed to load image: char_select2.jpg");
	
	img_exit = load_bitmap_resized("exit.png", 45, 45);
    if (!img_exit)
        game_abort("failed to load image: exit.png");
    img_exit2 = load_bitmap_resized("exit2.png", 45, 45);
    if (!img_exit2)
        game_abort("failed to load image: exit2.png");

	img_checkbox = load_bitmap_resized("checkbox.png", 50, 50);
	if (!img_checkbox)
		game_abort("failed to load image: checkbox.png");
	img_checkbox2 = load_bitmap_resized("checkbox2.png", 50, 50);
	if (!img_checkbox2)
		game_abort("failed to load image: checkbox2.png");

	img_button = load_bitmap_resized("button.png", 50, 50);
	if (!img_button)
		game_abort("failed to load image: button.png");
	img_button2 = load_bitmap_resized("button2.png", 50, 50);
	if (!img_button2)
		game_abort("failed to load image: button2.png");

	img_howtoplay = load_bitmap_resized("howtoplay.png", 45, 45);
	if (!img_howtoplay)
		game_abort("failed to load image: howtoplay.png");
	img_howtoplay2 = load_bitmap_resized("howtoplay2.png", 45, 45);
	if (!img_howtoplay2)
		game_abort("failed to load image: howtoplay2.png");

    start_img_background = load_bitmap_resized("start-bg.jpg", SCREEN_W, SCREEN_H);

	img_white = load_bitmap_resized("white.png", 100, 100);
	if (!img_white)
		game_abort("failed to load image: white.png");

	img_red = load_bitmap_resized("red.png", 100, 104);
    if (!img_red)
        game_abort("failed to load image: red.png");
	
	img_blue = load_bitmap_resized("blue.png", 100, 113);
    if (!img_blue)
        game_abort("failed to load image: blue.png");

    img_enemy = load_bitmap_resized("alien.png", 60, 75);
    if (!img_enemy)
        game_abort("failed to load image: alien.png");

	img_explosion = load_bitmap_resized("explosion.png", 54, 52);
	if (!img_explosion)
		game_abort("failed to load image: explosion.png");

    battle_bgm = al_load_sample("battle.ogg");
    if (!battle_bgm)
        game_abort("failed to load audio: battle.ogg");
	
	bullet_sound = al_load_sample("bullet.mp3");
    if (!bullet_sound)
        game_abort("failed to load audio: bullet.mp3");

	hit_sound = al_load_sample("hit.wav");
	if (!hit_sound)
		game_abort("failed to load audio: hit.wav");
	
	explosion_sound = al_load_sample("explosion.mp3");
	if (!explosion_sound)
		game_abort("failed to load audio: explosion.mp3");

	ult3_sound = al_load_sample("ult3.mp3");
	if (!ult3_sound)
		game_abort("failed to load audio: ult3.mp3");

    img_bullet1 = al_load_bitmap("bullet1.png");
    if (!img_bullet1)
        game_abort("failed to load image: bullet1.png");

    img_bullet2 = al_load_bitmap("bullet2.png");
    if (!img_bullet2)
        game_abort("failed to load image: bullet2.png");

    img_bullet3 = al_load_bitmap("bullet3.png");
    if (!img_bullet3)
        game_abort("failed to load image: bullet3.png");

	img_hitmarker = load_bitmap_resized("hitmarker.png", 50, 50);
	if (!img_hitmarker)
		game_abort("failed to load image: hitmarker.png");

	img_ult3 = load_bitmap_resized("ult3.png", 90, 77);
	if (!img_ult3)
		game_abort("failed to load image: ult3.png");

	death_bgm = al_load_sample("death.mp3");
    if (!death_bgm)
        game_abort("failed to load image: death.mp3");

	img_square = load_bitmap_resized("square.png", 150, 150);
	if (!img_square)
		game_abort("failed to load image: square.png");

    game_change_scene(SCENE_MENU);
}

void game_start_event_loop(void) {
    bool done = false;
    ALLEGRO_EVENT event;
    int redraws = 0;
    while (!done) {
        al_wait_for_event(game_event_queue, &event);
        if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            game_log("Window close button clicked");
            done = true;
        } else if (event.type == ALLEGRO_EVENT_TIMER) {
            if (event.timer.source == game_update_timer)
                redraws++;
        } else if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
            game_log("Key with keycode %d down", event.keyboard.keycode);
            key_state[event.keyboard.keycode] = true;
            on_key_down(event.keyboard.keycode);
        } else if (event.type == ALLEGRO_EVENT_KEY_UP) {
            game_log("Key with keycode %d up", event.keyboard.keycode);
            key_state[event.keyboard.keycode] = false;
        } else if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
            game_log("Mouse button %d down at (%d, %d)", event.mouse.button, event.mouse.x, event.mouse.y);
            mouse_state[event.mouse.button] = true;
            on_mouse_down(event.mouse.button, event.mouse.x, event.mouse.y);

			if (event.mouse.button == 1 && active_scene == SCENE_START && control == 2 && shoot == 0)
				shoot = 1;
			else if (event.mouse.button == 2 && active_scene == SCENE_START && control == 2 && ult == 0)
				ult = 1;
			else if (event.mouse.button == 1 && active_scene == SCENE_SETTINGS && pnt_in_rect(mouse_x, mouse_y, SCREEN_W - 200, 410, 50, 50) && reset == 0)
				reset = 1;
        } else if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_UP) {
            game_log("Mouse button %d up at (%d, %d)", event.mouse.button, event.mouse.x, event.mouse.y);
            mouse_state[event.mouse.button] = false;

			if (event.mouse.button == 1 && active_scene == SCENE_START && control == 2 && shoot == 1)
				shoot = 0;
			else if (event.mouse.button == 2 && active_scene == SCENE_START && control == 2 && ult == 1)
				ult = 0;
			else if (event.mouse.button == 1 && active_scene == SCENE_SETTINGS && pnt_in_rect(mouse_x, mouse_y, SCREEN_W - 200, 410, 50, 50) && reset == 1)
				reset = 0;
        } else if (event.type == ALLEGRO_EVENT_MOUSE_AXES) {
            if (event.mouse.dx != 0 || event.mouse.dy != 0) {
                //game_log("Mouse move to (%d, %d)", event.mouse.x, event.mouse.y);
                mouse_x = event.mouse.x;
                mouse_y = event.mouse.y;
            } else if (event.mouse.dz != 0) {
                game_log("Mouse scroll at (%d, %d) with delta %d", event.mouse.x, event.mouse.y, event.mouse.dz);
            }
        }

        if (redraws > 0 && al_is_event_queue_empty(game_event_queue)) {
            //if (redraws > 1)
			//	game_log("%d frame(s) dropped", redraws - 1);
            game_update();
            game_draw();
            redraws = 0;
        }
    }
}

void game_update(void) {
	if (active_scene == SCENE_START) {
		plane.vx = plane.vy = 0;
		if (control == 1) {
			if (key_state[ALLEGRO_KEY_UP] || key_state[ALLEGRO_KEY_W])
				plane.vy -= 1;
			if (key_state[ALLEGRO_KEY_DOWN] || key_state[ALLEGRO_KEY_S])
				plane.vy += 1;
			if (key_state[ALLEGRO_KEY_LEFT] || key_state[ALLEGRO_KEY_A])
				plane.vx -= 1;
			if (key_state[ALLEGRO_KEY_RIGHT] || key_state[ALLEGRO_KEY_D])
				plane.vx += 1;
		}
		else if (control == 2) {
			plane.vx = finddir(mouse_x, plane.x);
			plane.vy = finddir(mouse_y, plane.y);
		}
		plane.y += plane.v * (plane.vy * 4 * (plane.vx ? 0.71f : 1));
		plane.x += plane.v * (plane.vx * 4 * (plane.vy ? 0.71f : 1));

		if (plane.x < plane.w / 2)
			plane.x = plane.w / 2;
		else if (plane.x > SCREEN_W - (plane.w / 2))
			plane.x = SCREEN_W - (plane.w / 2);
		if (plane.y < plane.h / 2)
			plane.y = plane.h / 2;
		else if (plane.y > SCREEN_H - (plane.h / 2))
			plane.y = SCREEN_H - (plane.h / 2);

		int i;
		for (i = 0; i < MAX_BULLET; i++) {
			if (bullets[i].hidden)
				continue;
			bullets[i].x += bullets[i].vx;
			bullets[i].y += bullets[i].vy;
			if (bullets[i].y < -(bullets[i].h / 2))
				bullets[i].hidden = true;
		}
		if (planechoice == 1) {
			for (i = 0; i < 10; i++) {
				if (ult1_bullets[i].hidden)
					continue;
				ult1_bullets[i].x += 0;
				ult1_bullets[i].y += -2.5;
				if (ult1_bullets[i].y < -(ult1_bullets[i].h / 2))
					ult1_bullets[i].hidden = true;
			}
		} else if (planechoice == 2) {
			for (i = 0; i < 6; i++) {
				if (ult2_bullets[i].hidden)
					continue;
				if (i < 3) {
					ult2_bullets[i].x += -1 * (i + 1) + 0.5;
					ult2_bullets[i].y += -0.75 * (3 - i);
				}
				else {
					ult2_bullets[i].x += 1 * (i - 2) - 0.5;
					ult2_bullets[i].y += -0.75 * (6 - i);
				}
				if (ult2_bullets[i].y < -(ult2_bullets[i].h / 2))
					ult2_bullets[i].hidden = true;
			}
		}
		for (i = 0; i < MAX_ENEMY; i++) {
			if (enemies[i].hidden)
				continue;
			if (enemies[i].vx == 2)
				enemies[i].x += 8 * sin(al_get_time() * 4);
			else if (enemies[i].vx == 0) {
				if (plane.x - enemies[i].x > 4)
					enemies[i].x += 2;
				else if (plane.x - enemies[i].x < -4)
					enemies[i].x += -2;
			} else
				enemies[i].x += 2 * enemies[i].vx;
			enemies[i].y += enemies[i].vy;

			if (enemies[i].x <= (enemies[i].w / 2) || enemies[i].x >= SCREEN_W - (enemies[i].w / 2)) {
				if (enemies[i].vx == 2)
					enemies[i].vx /= -2;
				else
					enemies[i].vx *= -1;
				if (enemies[i].x <= (enemies[i].w / 2))
					enemies[i].x += 5;
				else
					enemies[i].x += -5;
			}
			if (enemies[i].y > SCREEN_H + (enemies[i].h / 2)) {
				enemies[i].hidden = true;
			}
		}
		now = al_get_time();
		if (control == 1) {
			if (key_state[ALLEGRO_KEY_SPACE] && now - last_shoot_timestamp >= MAX_COOLDOWN) {
				for (i = 0; i < MAX_BULLET; i++) {
					if (bullets[i].hidden) {
						al_play_sample(bullet_sound, volume * 2, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, &bullet_sound_id);
						last_shoot_timestamp = now;
						bullets[i].hidden = false;
						bullets[i].x = plane.x;
						bullets[i].y = plane.y - (plane.h / 2);
						break;
					}
				}
			} else if (ult || (key_state[ALLEGRO_KEY_ALT] && now - last_ult_timestamp >= plane.ultcd)) {
				ult = 1;
				last_ult_timestamp = now;
				if (planechoice == 1) {
					for (int i = 0; i < 10; i++) {
						if (ult1_bullets[i].hidden && now - last_ultshoot_timestamp >= 0.2) {
							al_play_sample(bullet_sound, volume * 2, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, &bullet_sound_id);
							last_ultshoot_timestamp = now;
							ult1_bullets[i].hidden = false;
							ult1_bullets[i].y = plane.y - (plane.h / 2);
							ult1_bullets[i].x = plane.x;
							if (i == 9)
								ult = 0;
						}
					}
				} else if (planechoice == 2) {
					for (int i = 0; i < 6; i++) {
						if (ult2_bullets[i].hidden && now - last_ultshoot_timestamp >= 0) {
							al_play_sample(bullet_sound, volume * 2, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, &bullet_sound_id);
							last_ultshoot_timestamp = now;
							ult2_bullets[i].hidden = false;
							ult2_bullets[i].y = plane.y - (plane.h / 2);
							ult2_bullets[i].x = plane.x;
							if (i == 5)
								ult = 0;
						}
					}
				} else if (planechoice == 3) {
					al_play_sample(ult3_sound, volume, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, &ult3_sound_id);
					plane.hp += 1;
					last_ult_timestamp = now;
					ult = 0;
				}
			}
		} else if (control == 2) {
			if (shoot && now - last_shoot_timestamp >= MAX_COOLDOWN) {
				for (i = 0; i < MAX_BULLET; i++) {
					if (bullets[i].hidden) {
						al_play_sample(bullet_sound, volume * 2, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, &bullet_sound_id);
						last_shoot_timestamp = now;
						bullets[i].hidden = false;
						bullets[i].x = plane.x;
						bullets[i].y = plane.y - (plane.h / 2);
						break;
					}
				}
			} else if (ult && now - last_ult_timestamp >= plane.ultcd) {
				ult = 1;
				last_ult_timestamp = now;
				if (planechoice == 1) {
					for (int i = 0; i < 10; i++) {
						if (ult1_bullets[i].hidden && now - last_ultshoot_timestamp >= 0.2) {
							al_play_sample(bullet_sound, volume * 2, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, &bullet_sound_id);
							last_ultshoot_timestamp = now;
							ult1_bullets[i].hidden = false;
							ult1_bullets[i].y = plane.y - (plane.h / 2);
							ult1_bullets[i].x = plane.x;
							if (i == 9)
								ult = 0;
						}
					}
				}
				else if (planechoice == 2) {
					for (int i = 0; i < 6; i++) {
						if (ult2_bullets[i].hidden && now - last_ultshoot_timestamp >= 0) {
							al_play_sample(bullet_sound, volume * 2, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, &bullet_sound_id);
							last_ultshoot_timestamp = now;
							ult2_bullets[i].hidden = false;
							ult2_bullets[i].y = plane.y - (plane.h / 2);
							ult2_bullets[i].x = plane.x;
							if (i == 5)
								ult = 0;
						}
					}
				} else if (planechoice == 3) {
					al_play_sample(ult3_sound, volume, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, &ult3_sound_id);
					plane.hp += 1;
					last_ult_timestamp = now;
					ult = 0;
				}
			}
		}
		now = al_get_time();
		for (i = 0; i < MAX_ENEMY; i++) {
			if (enemies[i].hidden && now - last_spawn_timestamp >= 1) {
				last_spawn_timestamp = now;
				enemies[i].hidden = false;
				enemies[i].x = enemies[i].w / 2 + (float)rand() / RAND_MAX * (SCREEN_W - enemies[i].w);
				enemies[i].y = -enemies[i].h / 2;
				enemies[i].vy = 2;
				enemies[i].vx = rand() % 4 - 1;
				enemies[i].hp = 2;
			}
		}
		now = al_get_time();
		for (int i = 0; i < MAX_ENEMY; i++) {
			if (enemies[i].hidden)
				continue;
			if (collide(plane, enemies[i])) {
				if (!al_play_sample(explosion_sound, volume * 0.6, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, &explosion_sound_id))
					game_abort("failed to play audio (bgm)");
				last_death_timestamp[i] = now;
				posY2[i] = enemies[i].y;
				posX2[i] = enemies[i].x;
				plane.hp--;
				if(plane.hp > 0)
					score -= 200;
				if (score < 0)
					score = 0;
				enemies[i].hidden = true;
				if (plane.hp == 0) {
					game_change_scene(SCENE_DEATH);
					break;
				}
			}
		}
		now = al_get_time();
		for (int i = 0; i < MAX_BULLET; i++) {
			if (bullets[i].hidden)
				continue;
			for(int j = 0; j < MAX_ENEMY; j++) {
				if (enemies[j].hidden)
					continue;
				if (collide(bullets[i], enemies[j])) {
					if (!al_play_sample(hit_sound, volume, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, &hit_sound_id))
						game_abort("failed to play audio (bgm)");
					last_hit_timestamp[i] = now;
					posY[i] = enemies[j].y;
					posX[i] = enemies[j].x;
					bullets[i].hidden = true;
					enemies[j].hp--;
					if (enemies[j].hp == 0) {
						if (!al_play_sample(explosion_sound, volume * 0.6, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, &explosion_sound_id))
							game_abort("failed to play audio (bgm)");
						last_death_timestamp[j] = now;
						posY2[j] = enemies[j].y;
						posX2[j] = enemies[j].x;
						enemies[j].hidden = true;
						score += 100;
					}
				}
			}
		}
		now = al_get_time();
		if (planechoice == 1) {
			for (int i = 0; i < 10; i++) {
				if (ult1_bullets[i].hidden)
					continue;
				for (int j = 0; j < MAX_ENEMY; j++) {
					if (enemies[j].hidden)
						continue;
					if (collide(ult1_bullets[i], enemies[j])) {
						if (!al_play_sample(hit_sound, volume, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, &hit_sound_id))
							game_abort("failed to play audio (bgm)");
						last_ulthit_timestamp[i] = now;
						posYult[i] = enemies[j].y;
						posXult[i] = enemies[j].x;
						ult1_bullets[i].hidden = true;
						enemies[j].hp--;
						if (enemies[j].hp == 0) {
							if (!al_play_sample(explosion_sound, volume * 0.6, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, &explosion_sound_id))
								game_abort("failed to play audio (bgm)");
							last_death_timestamp[j] = now;
							posY2[j] = enemies[j].y;
							posX2[j] = enemies[j].x;
							enemies[j].hidden = true;
							score += 100;
						}
					}
				}
			}
		} else if (planechoice == 2) {
			for (int i = 0; i < 6; i++) {
				if (ult2_bullets[i].hidden)
					continue;
				for (int j = 0; j < MAX_ENEMY; j++) {
					if (enemies[j].hidden)
						continue;
					if (collide(ult2_bullets[i], enemies[j])) {
						if (!al_play_sample(hit_sound, volume, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, &hit_sound_id))
							game_abort("collide(bullets[i], enemies[j]) 1 failed to play audio (bgm)");
						last_ulthit_timestamp[i] = now;
						posYult[i] = enemies[j].y;
						posXult[i] = enemies[j].x;
						ult2_bullets[i].hidden = true;
						enemies[j].hp--;
						if (enemies[j].hp == 0) {
							if (!al_play_sample(explosion_sound, volume * 0.6, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, &explosion_sound_id))
								game_abort("failed to play audio (bgm)");
							last_death_timestamp[j] = now;
							posY2[j] = enemies[j].y;
							posX2[j] = enemies[j].x;
							enemies[j].hidden = true;
							score += 100;
						}
					}
				}
			}
		}
    }
}

void game_draw(void) {
    if (active_scene == SCENE_MENU) {
        al_draw_bitmap(main_img_background, 0, 0, 0);
        al_draw_text(font_pirulen_40, al_map_rgb(255, 255, 255), SCREEN_W / 2, 25, ALLEGRO_ALIGN_CENTER, "Space Shooter");

		FILE* f = fopen("highscore.txt", "r");
		fscanf(f, "%s", highscore);
		fclose(f);
		int i = 0;
		hscore = 0;
		while (i < strlen(highscore)) {
			hscore = hscore * 10 + (highscore[i] - '0');
			i++;
		}
		char char_hs[40];
		sprintf(char_hs, "Highscore : %lld", hscore);
        al_draw_text(font_pirulen_24, al_map_rgb(255, 255, 255), 20, SCREEN_H - 80, 0, char_hs);
        al_draw_text(font_pirulen_24, al_map_rgb(255, 255, 255), 20, SCREEN_H - 45, 0, "Press enter to start");

        if (pnt_in_rect(mouse_x, mouse_y, SCREEN_W - 55, 10, 45, 45))
			al_draw_bitmap(img_settings2, SCREEN_W - 55, 10, 0);
        else
            al_draw_bitmap(img_settings, SCREEN_W - 55, 10, 0);
        
		if (pnt_in_rect(mouse_x, mouse_y, 11, 11, 45, 45))
			al_draw_bitmap(img_charselect2, 11, 11, 0);
        else
            al_draw_bitmap(img_charselect, 11, 11, 0);
		
		if (pnt_in_rect(mouse_x, mouse_y, SCREEN_W - 55, SCREEN_H - 55, 45, 45))
			al_draw_bitmap(img_exit2, SCREEN_W - 55, SCREEN_H - 55, 0);
        else
            al_draw_bitmap(img_exit, SCREEN_W - 55, SCREEN_H - 55, 0);
    } else if (active_scene == SCENE_START) {
        int i;
        al_draw_bitmap(start_img_background, 0, 0, 0);
		char char_hp[20];
		char char_score[20];
		sprintf(char_hp, "Lives x %.0f", plane.hp);
		sprintf(char_score, "Score : %lld", score);
		al_draw_text(font_pirulen_20, al_map_rgb(255, 255, 255), 10, 10, 0, char_hp);
		al_draw_text(font_pirulen_20, al_map_rgb(255, 255, 255), SCREEN_W - 10, 10, ALLEGRO_ALIGN_RIGHT, char_score);
		now = al_get_time();
		for (i = 0; i < MAX_BULLET; i++) {
			draw_movable_object(bullets[i]);
			if(bullets[i].hidden && now - last_hit_timestamp[i] < 0.2)
				al_draw_bitmap(img_hitmarker, posX[i], posY[i], 0);
		}
		draw_movable_object(plane);
		if (planechoice == 1) {
			for (i = 0; i < 10; i++) {
				draw_movable_object(ult1_bullets[i]);
				if (ult1_bullets[i].hidden && now - last_ulthit_timestamp[i] < 0.2)
					al_draw_bitmap(img_hitmarker, posXult[i], posYult[i], 0);
			}
		} else if (planechoice == 2) {
			for (i = 0; i < 6; i++) {
				draw_movable_object(ult2_bullets[i]);
				if (ult2_bullets[i].hidden && now - last_ulthit_timestamp[i] < 0.2)
					al_draw_bitmap(img_hitmarker, posXult[i], posYult[i], 0);
			}
		} else if (planechoice == 3) {
			if (now - last_ult_timestamp <= 0.8)
				if (now - last_ult_timestamp <= 0.2)
					al_draw_bitmap(img_ult3, plane.x - 60, plane.y - plane.h / 1.5, 0);
				else if(now - last_ult_timestamp <= 0.4)
					al_draw_bitmap(img_ult3, plane.x - 10, plane.y - (plane.h / 2) + 5, 0);
				else if(now - last_ult_timestamp <= 0.6)
					al_draw_bitmap(img_ult3, plane.x - 30, plane.y, 0);
				else if(now - last_ult_timestamp <= 0.8)
					al_draw_bitmap(img_ult3, plane.x - 70, plane.y - 30, 0);
		}
		for (i = 0; i < MAX_ENEMY; i++) {
			draw_movable_object(enemies[i]);
			if (enemies[i].hidden && now - last_death_timestamp[i] < 0.4)
				al_draw_bitmap(img_explosion, posX2[i], posY2[i], 0);
		}
	} else if (active_scene == SCENE_SETTINGS) {
        al_clear_to_color(al_map_rgb(0, 0, 0));
		al_draw_text(font_pirulen_32, al_map_rgb(255, 255, 255), SCREEN_W / 2, 20, ALLEGRO_ALIGN_CENTER, "Settings");
		al_draw_text(font_pirulen_20, al_map_rgb(255, 255, 255), 20, SCREEN_H - 50, 0, "Press ESC to return");

		al_draw_text(font_pirulen_24, al_map_rgb(255, 255, 255), 120, 120, ALLEGRO_ALIGN_LEFT, "Mute Audio");
		if (muted)
			al_draw_bitmap(img_checkbox2, SCREEN_W - 200, 110, 0);
		else
			al_draw_bitmap(img_checkbox, SCREEN_W - 200, 110, 0);
		
		al_draw_text(font_pirulen_24, al_map_rgb(255, 255, 255), 120, 200, ALLEGRO_ALIGN_LEFT, "Controls :");
		if (pnt_in_rect(mouse_x, mouse_y, 350, 192, 45, 45))
			al_draw_bitmap(img_howtoplay2, 350, 192, 0);
		else
			al_draw_bitmap(img_howtoplay, 350, 192, 0);

		al_draw_text(font_pirulen_24, al_map_rgb(255, 255, 255), 140, 260, ALLEGRO_ALIGN_LEFT, "- Keyboard");
		al_draw_text(font_pirulen_24, al_map_rgb(255, 255, 255), 140, 320, ALLEGRO_ALIGN_LEFT, "- Mouse");
		if (control == 1) {
			al_draw_bitmap(img_checkbox2, SCREEN_W - 200, 250, 0);
			al_draw_bitmap(img_checkbox, SCREEN_W - 200, 310, 0);
		}
		else {
			al_draw_bitmap(img_checkbox, SCREEN_W - 200, 250, 0);
			al_draw_bitmap(img_checkbox2, SCREEN_W - 200, 310, 0);
		}

		al_draw_text(font_pirulen_24, al_map_rgb(255, 255, 255), 120, 420, ALLEGRO_ALIGN_LEFT, "Reset High Score");
		if (reset) {
			al_draw_bitmap(img_button2, SCREEN_W - 200, 410, 0);
			FILE* f = fopen("highscore.txt", "w");
			fprintf(f, "0");
			fclose(f);
		} else {
			al_draw_bitmap(img_button, SCREEN_W - 200, 410, 0);
		}
    } else if (active_scene == SCENE_DEATH) {
		al_clear_to_color(al_map_rgb(0, 0, 0));
		char char_score[40];
		if (score > hscore) {
			FILE* f = fopen("highscore.txt", "w");
			fprintf(f, "%lld", score);
			fclose(f);
			sprintf(char_score, "You got a new high score : %lld", score);
		} else {
			sprintf(char_score, "Your score : %lld", score);
		}
		al_draw_text(font_pirulen_40, al_map_rgb(255, 255, 255), SCREEN_W / 2, 200, ALLEGRO_ALIGN_CENTER, "GAME OVER");
		al_draw_text(font_pirulen_20, al_map_rgb(255, 255, 255), SCREEN_W / 2, 280, ALLEGRO_ALIGN_CENTER, char_score);
		al_draw_text(font_pirulen_24, al_map_rgb(255, 255, 255), SCREEN_W / 2, 340, ALLEGRO_ALIGN_CENTER, "Press ESC to return to main menu");
	} else if (active_scene == SCENE_CHARSELECT) {
		al_clear_to_color(al_map_rgb(0, 0, 0));
		al_draw_text(font_pirulen_24, al_map_rgb(255, 255, 255), SCREEN_W / 2, 20, ALLEGRO_ALIGN_CENTER, "Character Select");
		al_draw_text(font_pirulen_20, al_map_rgb(255, 255, 255), 20, SCREEN_H - 50, 0, "Press ESC to return");

		if(planechoice == 1)
			al_draw_bitmap(img_square, 75, 100, 0);
		else if(planechoice == 2)
			al_draw_bitmap(img_square, 325, 100, 0);
		else
			al_draw_bitmap(img_square, 575, 100, 0);

		al_draw_bitmap(img_red, 100, 120, 0);
		al_draw_text(font_pirulen_20, al_map_rgb(255, 255, 255), 150, 260, ALLEGRO_ALIGN_CENTER, "Red");
		al_draw_text(font_pirulen_20, al_map_rgb(255, 255, 255), 110, 320, ALLEGRO_ALIGN_CENTER, "ATK  :");
		al_draw_text(font_pirulen_20, al_map_rgb(255, 255, 255), 220, 320, ALLEGRO_ALIGN_CENTER, "5");
		al_draw_text(font_pirulen_20, al_map_rgb(255, 255, 255), 110, 360, ALLEGRO_ALIGN_CENTER, "DEF  :");
		al_draw_text(font_pirulen_20, al_map_rgb(255, 255, 255), 220, 360, ALLEGRO_ALIGN_CENTER, "2");
		al_draw_text(font_pirulen_20, al_map_rgb(255, 255, 255), 110, 400, ALLEGRO_ALIGN_CENTER, "SPD  :");
		al_draw_text(font_pirulen_20, al_map_rgb(255, 255, 255), 220, 400, ALLEGRO_ALIGN_CENTER, "5");

		al_draw_bitmap(img_white, 350, 130, 0);
		al_draw_text(font_pirulen_20, al_map_rgb(255, 255, 255), 400, 260, ALLEGRO_ALIGN_CENTER, "White");
		al_draw_text(font_pirulen_20, al_map_rgb(255, 255, 255), 360, 320, ALLEGRO_ALIGN_CENTER, "ATK  :");
		al_draw_text(font_pirulen_20, al_map_rgb(255, 255, 255), 470, 320, ALLEGRO_ALIGN_CENTER, "4");
		al_draw_text(font_pirulen_20, al_map_rgb(255, 255, 255), 360, 360, ALLEGRO_ALIGN_CENTER, "DEF  :");
		al_draw_text(font_pirulen_20, al_map_rgb(255, 255, 255), 470, 360, ALLEGRO_ALIGN_CENTER, "4");
		al_draw_text(font_pirulen_20, al_map_rgb(255, 255, 255), 360, 400, ALLEGRO_ALIGN_CENTER, "SPD  :");
		al_draw_text(font_pirulen_20, al_map_rgb(255, 255, 255), 470, 400, ALLEGRO_ALIGN_CENTER, "4");

		al_draw_bitmap(img_blue, 600, 120, 0);
		al_draw_text(font_pirulen_20, al_map_rgb(255, 255, 255), 650, 260, ALLEGRO_ALIGN_CENTER, "Blue");
		al_draw_text(font_pirulen_20, al_map_rgb(255, 255, 255), 610, 320, ALLEGRO_ALIGN_CENTER, "ATK  :");
		al_draw_text(font_pirulen_20, al_map_rgb(255, 255, 255), 720, 320, ALLEGRO_ALIGN_CENTER, "3");
		al_draw_text(font_pirulen_20, al_map_rgb(255, 255, 255), 610, 360, ALLEGRO_ALIGN_CENTER, "DEF  :");
		al_draw_text(font_pirulen_20, al_map_rgb(255, 255, 255), 720, 360, ALLEGRO_ALIGN_CENTER, "6");
		al_draw_text(font_pirulen_20, al_map_rgb(255, 255, 255), 610, 400, ALLEGRO_ALIGN_CENTER, "SPD  :");
		al_draw_text(font_pirulen_20, al_map_rgb(255, 255, 255), 720, 400, ALLEGRO_ALIGN_CENTER, "3");
	} else if (active_scene == SCENE_HOWTOPLAY) {
		al_clear_to_color(al_map_rgb(0, 0, 0));
		al_draw_text(font_pirulen_32, al_map_rgb(255, 255, 255), SCREEN_W / 2, 20, ALLEGRO_ALIGN_CENTER, "Controls");
		al_draw_text(font_pirulen_20, al_map_rgb(255, 255, 255), 20, SCREEN_H - 50, 0, "Press ESC to return");

		if (control == 1) {
			al_draw_text(font_pirulen_24, al_map_rgb(255, 255, 255), SCREEN_W / 2, 120, ALLEGRO_ALIGN_CENTER, ">> Keyboard Controls <<");
			al_draw_text(font_pirulen_24, al_map_rgb(255, 255, 255), 120, 200, ALLEGRO_ALIGN_LEFT, "Movement");
			al_draw_text(font_pirulen_24, al_map_rgb(255, 255, 255), SCREEN_W - 120, 200, ALLEGRO_ALIGN_RIGHT, "WASD");
			al_draw_text(font_pirulen_24, al_map_rgb(255, 255, 255), SCREEN_W - 120, 240, ALLEGRO_ALIGN_RIGHT, "or Arrow Keys");
			al_draw_text(font_pirulen_24, al_map_rgb(255, 255, 255), 120, 320, ALLEGRO_ALIGN_LEFT, "Shoot");
			al_draw_text(font_pirulen_24, al_map_rgb(255, 255, 255), SCREEN_W - 120, 320, ALLEGRO_ALIGN_RIGHT, "Space");
			al_draw_text(font_pirulen_24, al_map_rgb(255, 255, 255), 120, 400, ALLEGRO_ALIGN_LEFT, "Ult");
			al_draw_text(font_pirulen_24, al_map_rgb(255, 255, 255), SCREEN_W - 120, 400, ALLEGRO_ALIGN_RIGHT, "Left-Alt");
		} else if (control == 2) {
			al_draw_text(font_pirulen_24, al_map_rgb(255, 255, 255), SCREEN_W / 2, 120, ALLEGRO_ALIGN_CENTER, ">> Mouse Controls <<");
			al_draw_text(font_pirulen_24, al_map_rgb(255, 255, 255), 120, 200, ALLEGRO_ALIGN_LEFT, "Movement");
			al_draw_text(font_pirulen_24, al_map_rgb(255, 255, 255), SCREEN_W - 120, 200, ALLEGRO_ALIGN_RIGHT, "Cursor");
			al_draw_text(font_pirulen_24, al_map_rgb(255, 255, 255), 120, 280, ALLEGRO_ALIGN_LEFT, "Shoot");
			al_draw_text(font_pirulen_24, al_map_rgb(255, 255, 255), SCREEN_W - 120, 280, ALLEGRO_ALIGN_RIGHT, "Left-Click");
			al_draw_text(font_pirulen_24, al_map_rgb(255, 255, 255), 120, 360, ALLEGRO_ALIGN_LEFT, "Ult");
			al_draw_text(font_pirulen_24, al_map_rgb(255, 255, 255), SCREEN_W - 120, 360, ALLEGRO_ALIGN_RIGHT, "Right-Click");
		}
	}
    al_flip_display();
}

void game_destroy(void) {
    al_destroy_font(font_pirulen_20);
    al_destroy_font(font_pirulen_24);
    al_destroy_font(font_pirulen_32);
    al_destroy_font(font_pirulen_40);

    al_destroy_bitmap(main_img_background);
    al_destroy_bitmap(img_settings);
    al_destroy_bitmap(img_settings2);
    al_destroy_bitmap(img_charselect);
    al_destroy_bitmap(img_charselect2);
    al_destroy_bitmap(img_exit);
    al_destroy_bitmap(img_exit2);
    al_destroy_sample(menu_bgm);
    al_destroy_sample(submenu_bgm);

    al_destroy_bitmap(start_img_background);
    al_destroy_bitmap(img_white);
    al_destroy_bitmap(img_red);
    al_destroy_bitmap(img_blue);
    al_destroy_bitmap(img_bullet1);
    al_destroy_bitmap(img_bullet2);
    al_destroy_bitmap(img_bullet3);
    al_destroy_bitmap(img_hitmarker);
	al_destroy_bitmap(img_ult3);
    al_destroy_bitmap(img_enemy);
    al_destroy_bitmap(img_explosion);
    al_destroy_sample(battle_bgm);
    al_destroy_sample(bullet_sound);
    al_destroy_sample(hit_sound);
    al_destroy_sample(explosion_sound);

	al_destroy_bitmap(img_checkbox);
	al_destroy_bitmap(img_checkbox2);
	al_destroy_bitmap(img_button);
	al_destroy_bitmap(img_button2);
	al_destroy_bitmap(img_howtoplay);
	al_destroy_bitmap(img_howtoplay2);

	al_destroy_sample(death_bgm);

	al_destroy_bitmap(img_square);

    al_destroy_timer(game_update_timer);
    al_destroy_event_queue(game_event_queue);
    al_destroy_display(game_display);
    free(mouse_state);
}

void game_change_scene(int next_scene) {
    game_log("Change scene from %d to %d", active_scene, next_scene);
	int prev_scene = active_scene;
    if (active_scene == SCENE_MENU) {
        al_stop_sample(&menu_bgm_id);
        game_log("stop audio (bgm)");
	} else if (active_scene == SCENE_SETTINGS && next_scene == SCENE_MENU) {
		al_stop_sample(&submenu_bgm_id);
		game_log("stop audio (bgm)");
	} else if (active_scene == SCENE_START) {
        al_stop_sample(&battle_bgm_id);
        game_log("stop audio (bgm)");
    } else if (active_scene == SCENE_DEATH) {
		al_stop_sample(&death_bgm_id);
		game_log("stop audio (bgm)");
	} else if (active_scene == SCENE_CHARSELECT) {
		al_stop_sample(&submenu_bgm_id);
		game_log("stop audio (bgm)");
	}
	active_scene = next_scene;
    if (active_scene == SCENE_MENU) {
		score = 0;
		last_ult_timestamp = -30;
        if (!al_play_sample(menu_bgm, volume * 0.4, 0.0, 1.0, ALLEGRO_PLAYMODE_LOOP, &menu_bgm_id))
            game_abort("failed to play audio (bgm)");
    } else if ((active_scene == SCENE_SETTINGS && prev_scene != SCENE_HOWTOPLAY) || active_scene == SCENE_CHARSELECT) {
		if (!al_play_sample(submenu_bgm, volume * 0.15, 0.0, 1.0, ALLEGRO_PLAYMODE_LOOP, &submenu_bgm_id))
			game_abort("failed to play audio (bgm)");
	} else if (active_scene == SCENE_START) {
        int i;
		if (planechoice == 1) {
			plane.img = img_red;
			plane.x = 400;
			plane.y = 500;
			plane.w = al_get_bitmap_width(plane.img);
			plane.h = al_get_bitmap_height(plane.img);
			MAX_BULLET = 10;
			plane.hp = 2;
			plane.v = 1.25;
			plane.ultcd = 10;
		} else if (planechoice == 2) {
			plane.img = img_white;
			plane.x = 400;
			plane.y = 500;
			plane.w = al_get_bitmap_width(plane.img);
			plane.h = al_get_bitmap_height(plane.img);
			MAX_BULLET = 8;
			plane.hp = 4;
			plane.v = 1;
			plane.ultcd = 15;
		} else if (planechoice == 3) {
			plane.img = img_blue;
			plane.x = 400;
			plane.y = 500;
			plane.w = al_get_bitmap_width(plane.img);
			plane.h = al_get_bitmap_height(plane.img);
			MAX_BULLET = 6;
			plane.hp = 6;
			plane.v = 0.75;
			plane.ultcd = 30;
		}
        for (i = 0; i < MAX_ENEMY; i++) {
            enemies[i].img = img_enemy;
            enemies[i].w = al_get_bitmap_width(img_enemy);
            enemies[i].h = al_get_bitmap_height(img_enemy);
            enemies[i].x = enemies[i].w / 2 + (float)rand() / RAND_MAX * (SCREEN_W - enemies[i].w);
            enemies[i].y = -enemies[i].h / 2;
			enemies[i].vy = 2;
			enemies[i].vx = rand()%4 - 1;
			enemies[i].hp = 2;
			if (i == 0) {
				enemies[i].hidden = false;
				last_spawn_timestamp = al_get_time();
			}
			else
				enemies[i].hidden = true;
        }
		for (int i = 0; i < MAX_BULLET; i++) {
			if (planechoice == 1) {
				bullets[i].img = img_bullet1;
				bullets[i].w = al_get_bitmap_width(img_bullet1);
				bullets[i].h = al_get_bitmap_height(img_bullet1);
			} else if (planechoice == 2) {
				bullets[i].img = img_bullet2;
				bullets[i].w = al_get_bitmap_width(img_bullet2);
				bullets[i].h = al_get_bitmap_height(img_bullet2);
			} else if (planechoice == 3) {
				bullets[i].img = img_bullet3;
				bullets[i].w = al_get_bitmap_width(img_bullet3);
				bullets[i].h = al_get_bitmap_height(img_bullet3);
			}
            bullets[i].vx = 0;
            bullets[i].vy = -4 * plane.v;
            bullets[i].hidden = true;
        }
		if (planechoice == 1) {
			for (int i = 0; i < 10; i++) {
				ult1_bullets[i].img = img_bullet1;
				ult1_bullets[i].w = al_get_bitmap_width(img_bullet1);
				ult1_bullets[i].h = al_get_bitmap_height(img_bullet1);
				ult1_bullets[i].hidden = true;
			}
		} else if (planechoice == 2) {
			for (int i = 0; i < 6; i++) {
				ult2_bullets[i].img = img_bullet2;
				ult2_bullets[i].w = al_get_bitmap_width(img_bullet2);
				ult2_bullets[i].h = al_get_bitmap_height(img_bullet2);
				ult2_bullets[i].hidden = true;
			}
		}
        if (!al_play_sample(battle_bgm, volume * 0.3, 0.0, 1.0, ALLEGRO_PLAYMODE_LOOP, &battle_bgm_id))
            game_abort("failed to play audio (bgm)");
	} else if (active_scene == SCENE_DEATH) {
		if (!al_play_sample(death_bgm, volume, 0.0, 1.0, ALLEGRO_PLAYMODE_LOOP, &death_bgm_id))
			game_abort("failed to play audio (bgm)");
	}
}

void on_key_down(int keycode) {
    if (active_scene == SCENE_MENU) {
		if (keycode == ALLEGRO_KEY_ENTER)
			game_change_scene(SCENE_START);
    } else if (active_scene == SCENE_START) {
		if (keycode == ALLEGRO_KEY_ESCAPE)
			game_change_scene(SCENE_MENU);
	} else if (active_scene == SCENE_SETTINGS) {
        if (keycode == ALLEGRO_KEY_ESCAPE)
            game_change_scene(SCENE_MENU);
    } else if (active_scene == SCENE_DEATH) {
		if (keycode == ALLEGRO_KEY_ESCAPE)
			game_change_scene(SCENE_MENU);
	} else if (active_scene == SCENE_CHARSELECT) {
		if (keycode == ALLEGRO_KEY_ESCAPE)
			game_change_scene(SCENE_MENU);
	} else if (active_scene == SCENE_HOWTOPLAY) {
		if (keycode == ALLEGRO_KEY_ESCAPE)
			game_change_scene(SCENE_SETTINGS);
	}
}

void on_mouse_down(int btn, int x, int y) {
    if (active_scene == SCENE_MENU) {
        if (btn == 1) {
			if (pnt_in_rect(x, y, SCREEN_W - 55, 10, 45, 45))
				game_change_scene(SCENE_SETTINGS);
			else if (pnt_in_rect(x, y, 11, 11, 45, 45))
				game_change_scene(SCENE_CHARSELECT);
			else if (pnt_in_rect(x, y, SCREEN_W - 55, SCREEN_H - 55, 45, 45))
				exit(0);
        }
	} else if (active_scene == SCENE_SETTINGS) {
		if (btn == 1) {
			if (pnt_in_rect(x, y, SCREEN_W - 200, 110, 50, 50)) {
				if (muted) {
					muted = 0;
					volume = 1;
					if (!al_play_sample(submenu_bgm, volume * 0.15, 0.0, 1.0, ALLEGRO_PLAYMODE_LOOP, &submenu_bgm_id))
						game_abort("failed to play audio (bgm)");
				} else {
					muted = 1;
					volume = 0;
					al_stop_sample(&submenu_bgm_id);
				}
			} else if (pnt_in_rect(x, y, SCREEN_W - 200, 250, 50, 50)) {
				control = 1;
			} else if (pnt_in_rect(x, y, SCREEN_W - 200, 310, 50, 50)) {
				control = 2;
			} else if (pnt_in_rect(x, y, 350, 192, 45, 45)) {
				game_change_scene(SCENE_HOWTOPLAY);
			}
		}
	} else if (active_scene == SCENE_CHARSELECT) {
		if (btn == 1) {
			if (pnt_in_rect(x, y, 100, 120, 100, 100)) {
				planechoice = 1;
			} else if (pnt_in_rect(x, y, 350, 130, 100, 104)) {
				planechoice = 2;
			} else if (pnt_in_rect(x, y, 600, 120, 100, 113)) {
				planechoice = 3;
			}
		}
	}
}

void draw_movable_object(MovableObject obj) {
    if (obj.hidden)
        return;
    al_draw_bitmap(obj.img, round(obj.x - obj.w / 2), round(obj.y - obj.h / 2), 0);
}

ALLEGRO_BITMAP *load_bitmap_resized(const char *filename, int w, int h) {
    ALLEGRO_BITMAP* loaded_bmp = al_load_bitmap(filename);
    if (!loaded_bmp)
        game_abort("failed to load image: %s", filename);
    ALLEGRO_BITMAP *resized_bmp = al_create_bitmap(w, h);
    ALLEGRO_BITMAP *prev_target = al_get_target_bitmap();

    if (!resized_bmp)
        game_abort("failed to create bitmap when creating resized image: %s", filename);
    al_set_target_bitmap(resized_bmp);
    al_draw_scaled_bitmap(loaded_bmp, 0, 0,
        al_get_bitmap_width(loaded_bmp),
        al_get_bitmap_height(loaded_bmp),
        0, 0, w, h, 0);
    al_set_target_bitmap(prev_target);
    al_destroy_bitmap(loaded_bmp);

    game_log("resized image: %s", filename);

    return resized_bmp;
}

bool pnt_in_rect(int px, int py, int x, int y, int w, int h) {
    return (px <= x + w && py <= y + h && px >= x && py >= y) ? true : false;
}

bool collide(MovableObject obj1, MovableObject obj2) {
	if (obj1.y + obj1.h / 2 >= obj2.y - obj2.h / 2 && obj1.y - obj1.h / 2 <= obj2.y + obj2.h / 2 && obj1.x - obj1.w / 2 <= obj2.x + obj2.w / 2 && obj1.x + obj1.w / 2 >= obj2.x - obj2.w / 2)
		//if (obj1.x - obj1.w / 2 <= obj2.x + obj2.w / 2 && obj1.x + obj1.w / 2 >= obj2.x - obj2.w / 2)
		return true;
	else
		return false;
}

int finddir(float a, float b) {
	if (a - b > 8)
		return 1;
	else if (a - b < -8)
		return -1;
	else
		return 0;
}

int press(int btn) {
	if (btn == 1)
		return 1;
	else
		return 0;
}

// +=================================================================+
// | Code below is for debugging purpose, it's fine to remove it.    |
// | Deleting the code below and removing all calls to the functions |
// | doesn't affect the game.                                        |
// +=================================================================+

void game_abort(const char* format, ...) {
    va_list arg;
    va_start(arg, format);
    game_vlog(format, arg);
    va_end(arg);
    fprintf(stderr, "error occured, exiting after 2 secs");
    // Wait 2 secs before exiting.
    al_rest(2);
    // Force exit program.
    exit(1);
}

void game_log(const char* format, ...) {
#ifdef LOG_ENABLED
    va_list arg;
    va_start(arg, format);
    game_vlog(format, arg);
    va_end(arg);
#endif
}

void game_vlog(const char* format, va_list arg) {
#ifdef LOG_ENABLED
    static bool clear_file = true;
    // Write log to file for later debugging.
    FILE* pFile = fopen("log.txt", clear_file ? "w" : "a");
    if (pFile) {
        va_list arg2;
        va_copy(arg2, arg);
        vfprintf(pFile, format, arg2);
        va_end(arg2);
        fprintf(pFile, "\n");
        fclose(pFile);
    }
    vprintf(format, arg);
    printf("\n");
    clear_file = false;
#endif
}
