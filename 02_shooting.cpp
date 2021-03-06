#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>

#include <cstdlib> // rand
#include <ctime>

//---------------------------------------
// Init function
//---------------------------------------
const int WINDOW_WIDTH = 1024;
const int WINDOW_HEIGHT = 768;
static SDL_Window *AA_window = NULL;
static SDL_Renderer *AA_renderer = NULL;
SDL_Renderer *AA_game_init(const char *title) {
    // SDL Init
    if(SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        SDL_Log("SDL Init Err %s", SDL_GetError());
        return NULL;
    }
    // Image Init
    if(!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        SDL_Log("IMG Init Err %s", IMG_GetError());
        return NULL;
    }
    // Font Init
    if(TTF_Init() !=0) {
        SDL_Log("TTF Init Err %s", TTF_GetError());
        return NULL;
    }
    // Mixer Init
    if(Mix_Init(MIX_INIT_OGG | MIX_INIT_MP3) != (MIX_INIT_OGG | MIX_INIT_MP3)) {
        SDL_Log("Mixer Init Err %s", Mix_GetError());
        return NULL;
    }
    if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) == -1) {
        SDL_Log("Mixer OpenAudio Err %s", Mix_GetError());
        return NULL;
    }

    AA_window = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        0
    );
    if(!AA_window) {
        SDL_Log("Create window Err %s", SDL_GetError());
        return NULL;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(
        AA_window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE
        | SDL_RENDERER_PRESENTVSYNC
    );
    if(!renderer) {
        SDL_Log("Create renderer Err %s", SDL_GetError());
        return NULL;
    }
    return renderer; // On Success
}

//---------------------------------------
// End(Quit) Function
//---------------------------------------

void AA_game_quit() {
    SDL_DestroyRenderer(AA_renderer);
    SDL_DestroyWindow(AA_window);
    Mix_CloseAudio();
    Mix_Quit();
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}

//---------------------------------------
// Texture ??????
//---------------------------------------
struct AA_Texture {
    SDL_Texture *tex;
    SDL_Renderer *renderer;
    int w, h;
    AA_Texture(SDL_Renderer *_r, SDL_Surface *_s) 
        : tex(SDL_CreateTextureFromSurface(_r, _s))
        , renderer(_r), w(_s->w), h(_s->h) {
            SDL_FreeSurface(_s);
    }
    ~AA_Texture() {
        SDL_DestroyTexture(tex);
        //SDL_Log("%s (%p)", SDL_GetError(), tex);
    }
};

AA_Texture *AA_load_texture(SDL_Renderer *renderer, const char *img_filename, SDL_FRect *r = NULL) {
    SDL_Surface *sur = IMG_Load(img_filename);
    if(!sur) {
        SDL_Log("IMG_Load(%s) Err %s", img_filename, IMG_GetError());
        return NULL;
    }
    if(r) r->w = sur->w, r->h = sur->h;
    AA_Texture *tex = new AA_Texture(renderer, sur);
    return tex;
}

AA_Texture *AA_load_ttf_texture(SDL_Renderer *renderer, TTF_Font *font,
                                const char *text, SDL_Color *color, SDL_FRect *r = NULL) {
    SDL_Surface *sur = TTF_RenderUTF8_Blended(font, text, *color);
    if(!sur) {
        SDL_Log("TTF_Render Err %s", TTF_GetError());
        return NULL;
    }
    if(r) r->w = sur->w, r->h = sur->h;
    AA_Texture *tex = new AA_Texture(renderer, sur);
    return tex;
}

void AA_free_texture(AA_Texture *&t) {
    if(t) delete t;
    t = NULL;
}

//---------------------------------------
// Object ??? Background ??????
//---------------------------------------

struct _AA_Object { // ?????????... ??????, ????????? ?????? ????????? ?????? ????????? ??????
    AA_Texture *tex;
    bool valid;
    SDL_Rect org_r, edge_r;
    int xvalo, yvalo;
    int cx, cy;
};

// 0. ??????
struct AA_Background_t {
    AA_Texture *atex[2]; // magic number?????? ?????????.. ??????..
    SDL_FRect r[2];
    int load(SDL_Renderer *renderer, const char *s1, const char *s2) { // ?????? ?????????
        atex[0] = AA_load_texture(renderer, s1, &r[0]);
        atex[1] = AA_load_texture(renderer, s2, &r[1]);
        if(!atex[0] || !atex[1]) return 1;
        r[0].x = r[0].y = r[1].y = 0, r[1].x = r[0].w;
        return 0;
    }
    void blit() {  // while ?????? ???
        SDL_RenderCopyF(atex[0]->renderer, atex[0]->tex, NULL, &r[0]);
        SDL_RenderCopyF(atex[1]->renderer, atex[1]->tex, NULL, &r[1]);
    }
    void move() { // while ?????? ???
        r[0].x -= 1.0f;
        r[1].x -= 1.0f;
        if(r[0].x == 0.0f) r[1].x = r[0].w; // ?
        if(r[1].x == 0.0f) r[0].x = r[1].w; // ?
    }
    void free() { // ?????????
        AA_free_texture(atex[0]);
        AA_free_texture(atex[1]);
    }
} AA_Background;

// 1. ???????????? ??????
const int AA_PLAYER_NUM_OF_IMG = 4;
const int AA_PLAYER_NUM_OF_ITEM = 5;
enum AA_Item_list {
    AA_ITEM_AUTOSHOT = 0,
    AA_ITEM_3WAY = 1,
    AA_ITEM_BULLET_SPDUP = 2,
    AA_ITEM_POWERUP = 3,
    AA_ITEM_SPDUP = 4
};
const char *AA_Item_name[] = {
    "RAPID FIRE",
    "3-WAY",
    "BULLET-SPEEDUP",
    "BULLET-POWERUP",
    "SPEED UP"
};
struct AA_Player_t {
    AA_Texture *atex[AA_PLAYER_NUM_OF_IMG];
    SDL_FRect r;
    float cx, cy, radius; // ?????? ??????, ????????????
    int fnum;
    int item[AA_PLAYER_NUM_OF_ITEM];
    int load(SDL_Renderer *renderer, float x, float y, const char *s) {
        char buff[101];
        for(int i=0; i<AA_PLAYER_NUM_OF_IMG; i++) {
            SDL_snprintf(buff, 101, "%s%d.png", s, i+1);
            atex[i] = AA_load_texture(renderer, buff, &r);
            if(!atex[i]) return 1;
        }
        fnum = 0;
        for(int i=0; i<AA_PLAYER_NUM_OF_ITEM; i++) {
            item[i] = 0;
        }
        //r.x = 10.0f, r.y = (float)(WINDOW_HEIGHT / 2);
        r.x = x, r.y = y;
        cx = r.x + r.w/2 + 13, cy = r.y + r.h/2; // 13 : ?????? ?????? ??????
        radius = r.h/2;
        return 0;
    }
    void blit() {
        SDL_RenderCopyF(atex[fnum]->renderer, atex[fnum]->tex, NULL, &r);
        if(++fnum == AA_PLAYER_NUM_OF_IMG) {
            fnum = 0;
        }
    }
    void free() {
        for(int i=0; i<AA_PLAYER_NUM_OF_IMG; i++) {
            AA_free_texture(atex[i]);
        }
    }
} AA_Player[2]; // 2????????? ????????????

// 2. ??? ??????, ????????? ???
const int AA_CAP_OF_OBJ = 500;
struct AA_Object_t {
    static int pindex; // ????????? ?????????
    bool activated;
    AA_Texture *atex;
    SDL_FRect r;
    float dx, dy, cx, cy, radius;
    int hp;

    static int curr_index();
    void load(AA_Texture *t, float x, float y, float mag_rate = 1.0f) {
        activated = true;
        atex = t;
        r.h = t->h * mag_rate, r.w = t->w * mag_rate;
        //atex = AA_load_texture(renderer, s, r);
        r.x = x, r.y = y;
        cx = r.x + r.w/2, cy = r.y + r.h/2;
        radius = r.h/2;
    }
    void blit() {
        if(r.y < 0 || r.y+r.h > (float)WINDOW_HEIGHT) dy = -dy; // bound
        SDL_RenderCopyF(atex->renderer, atex->tex, NULL, &r);
        r.x += dx, r.y += dy;
        cx += dx, cy += dy;
    }
    void free() {
        activated = false;
        atex = NULL;
    }
    AA_Object_t() : activated(false), atex(NULL), dx(0.0f), dy(0.0f), hp(0) {}
} AA_Object[AA_CAP_OF_OBJ]; // ?????? 1000???.
int AA_Object_t::pindex = 0;
int AA_Object_t::curr_index() {
    if(AA_Object_t::pindex == AA_CAP_OF_OBJ) AA_Object_t::pindex = 0;
    return AA_Object_t::pindex++;
}

// 3. ???????????? ??????
const int AA_CAP_OF_BULLET = 500;
struct AA_Bullet_t {
    static int pindex; // ????????? ?????????
    bool activated;
    AA_Texture *atex;
    SDL_Rect src_r;
    SDL_FRect r;
    int hp;
    double degree;
    float dx, dy, cx, cy, radius;

    static int curr_index();
    void load(AA_Texture *t, int h, float start_x, float start_y, float mag_rate = 1.0f, double deg = 0.0, SDL_Rect *s = NULL) {
        activated = true;
        atex = t;
        hp = h;
        degree = deg;
        if(!s) {
            src_r = {0, 0, 0, 0};
            r.h = t->h * mag_rate, r.w = t->w * mag_rate;
        }
        else {
            src_r = *s;
            r.h = s->h * mag_rate, r.w = s->w * mag_rate;
        }
        dx = dy = 0.0f;
        r.x = start_x, r.y = start_y - r.h/2;
        cx = r.x + r.w/2, cy = r.y + r.h/2;
        radius = r.h/2;
    }
    void blit() {
        if(src_r.w == 0) {
            SDL_RenderCopyExF(atex->renderer, atex->tex, NULL, &r, degree, &SDL_FPoint({0.0f, r.h/2}), SDL_FLIP_NONE);
        }
        else {
            SDL_RenderCopyExF(atex->renderer, atex->tex, &src_r, &r, degree, &SDL_FPoint({0.0f, r.h/2}), SDL_FLIP_NONE);
        }
        r.x += dx, r.y += dy;
        cx += dx, cy += dy;
    }
    void free() {
        activated = false;
        atex = NULL;
    }
    AA_Bullet_t() : activated(false), atex(NULL), dx(0.0f), dy(0.0f) {}
} AA_Bullet[AA_CAP_OF_BULLET]; 
int AA_Bullet_t::pindex = 0;
int AA_Bullet_t::curr_index() {
    if(AA_Bullet_t::pindex == AA_CAP_OF_BULLET) AA_Bullet_t::pindex = 0;
    return AA_Bullet_t::pindex++;
}

// 4. ?????????
const int AA_CAP_OF_ITEM = 50;
struct AA_Item_t {
    static int pindex;
    bool activated;
    int kind; // ????????? ??????
    AA_Texture *atex;
    SDL_Rect src_r;
    SDL_FRect r;
    float dx, dy, cx, cy, radius;

    static int curr_index();

    void load(AA_Texture *t, int k, float start_x, float start_y, float mag_rate = 1.0f, SDL_Rect *s = NULL) {
        activated = true;
        atex = t;
        kind = k;
        if(!s) {
            src_r = {0, 0, 0, 0};
            r.h = t->h * mag_rate, r.w = t->w * mag_rate;
        }
        else {
            src_r = *s;
            r.h = s->h * mag_rate, r.w = s->w * mag_rate;
        }
        r.x = start_x, r.y = start_y;
        cx = r.x + r.w/2, cy = r.y + r.h/2;
        radius = r.h/2;
    }
    void blit() {
        if(src_r.w == 0) {
            SDL_RenderCopyF(atex->renderer, atex->tex, NULL, &r);
        }
        else {
            SDL_RenderCopyF(atex->renderer, atex->tex, &src_r, &r);
        }
        r.x += dx, r.y += dy;
        cx += dx, cy += dy;
    }
    void free() {
        activated = false;
        atex = NULL;
    }
    AA_Item_t() : activated(false), atex(NULL), dx(0.0f), dy(0.0f) {}
} AA_Item[AA_CAP_OF_ITEM];
int AA_Item_t::pindex = 0;
int AA_Item_t::curr_index() {
    if(AA_Item_t::pindex == AA_CAP_OF_ITEM) AA_Item_t::pindex = 0;
    return AA_Item_t::pindex++;
}

// 5. ?????????????????????
const int AA_CAP_OF_EXPLOSION = 200;
struct AA_Explosion_t {
    static int pindex; // ????????? ?????????
    bool activated;
    AA_Texture *atex;
    SDL_Rect src_r; // 64x64 ???????????? ?????? 10, ?????? 11 ??? ??????, raster ???.
    SDL_FRect r;
    int findex; // 0~109
    int fskip;
    float cx, cy;

    static int curr_index();

    void load(AA_Texture *t, float center_x, float center_y, float mag_rate = 1.0f, int fs = 1) {
        activated = true;
        atex = t;
        src_r = {0, 0, 64, 64};
        findex = 0, fskip = fs;
        r.x = center_x - 32.0f * mag_rate, r.y = center_y - 32.0f * mag_rate;
        r.h = 64.0f * mag_rate, r.w = 64.0f * mag_rate;
    }
    void blit() {
        src_r.x = findex * 64 % 640, src_r.y = findex / 10 * 64;
        SDL_RenderCopyF(atex->renderer, atex->tex, &src_r, &r);
        if((findex += fskip) >= 110) free();
    }
    void free() {
        activated = false;
        atex = NULL;
    }
    AA_Explosion_t() : activated(false), atex(NULL) {}
} AA_Explosion[AA_CAP_OF_EXPLOSION];
int AA_Explosion_t::pindex = 0;
int AA_Explosion_t::curr_index() {
    if(AA_Explosion_t::pindex == AA_CAP_OF_EXPLOSION) AA_Explosion_t::pindex = 0;
    return AA_Explosion_t::pindex++;
}

// 6. In-game message
const int AA_CAP_OF_INGAME_MESSAGE = 200;
struct AA_Ingame_Message_t {
    static int pindex; // ????????? ?????????
    bool activated;
    AA_Texture *atex;
    SDL_FRect r;
    int fduration;
    float cx, cy;

    static int curr_index();

    void load(SDL_Renderer *renderer, TTF_Font *font, float center_x, float center_y, const char *message, SDL_Color &c, int fd = 30) {
        activated = true;
        atex = AA_load_ttf_texture(renderer, font, message, &c, &r);
        r.x = center_x - r.w/2, r.y = center_y - r.h/2;
        fduration = fd;
        cx = center_x, cy = center_y;
    }
    void blit() {
        SDL_RenderCopyF(atex->renderer, atex->tex, NULL, &r);
        if(--fduration == 0) free();
    }
    void free() {
        activated = false;
        AA_free_texture(atex);
    }
    AA_Ingame_Message_t() : activated(false), atex(NULL) {}
} AA_Ingame_Message[AA_CAP_OF_INGAME_MESSAGE];
int AA_Ingame_Message_t::pindex = 0;
int AA_Ingame_Message_t::curr_index() {
    if(AA_Ingame_Message_t::pindex == AA_CAP_OF_INGAME_MESSAGE) AA_Ingame_Message_t::pindex = 0;
    return AA_Ingame_Message_t::pindex++;
}

// ????????? ?????????
inline bool is_collided1(const SDL_FRect &r1, const SDL_FRect &r2) {
    if(r1.x + r1.w < r2.x || r2.x + r2.w < r1.x
        || r1.y + r1.h < r2.y || r2.y + r2.h < r1.y) return false;
    return true;
}

// ?????? ?????? (???) ?????????
inline bool is_collided2(float x1, float y1, float r1, float x2, float y2, float r2) {
    return (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1) <= (r1+r2)*(r1+r2);
}

int score = 0, hiscore = 30000;
int obj_create_period = 30; // 60????????? ?????? 0.5???

//---------------------------------------
// main
//---------------------------------------
// 1. SDL ?????? ????????? Init ??????.
// 2. window ???????????? renderer ????????????.
// 3. while ????????? ???????????? ????????? ????????? renderer flush
//
// Note : ????????? ??????????????? ????????? sync ?????? fps ??? ?????????
//        ??? frame rate ????????? ???????????? ??????????????? ???????????????
//        (????????? fps??? ????????? ????????? ??????, ????????? ????????? ???????????? ???????????? ?????? ??????;;)
//        ???????????? ????????? frame rate ????????? ?????? System Time Tick ??? ???????????? ????????????
//        ?????????, ?????? ????????? ?????? ???????????? ??????????????? ?????? ???.
enum AA_Game_State {
    AA_STATE_UNDEFINED = 1,
    AA_STATE_READY = 2,
    AA_STATE_INGAME = 4,
    AA_STATE_GAMEOVER = 8
};
enum AA_Game_Autogun_Fire_period {
    AA_AUTOGUN_AUTO = 5,
    AA_AUTOGUN_NORMAL = 15
};

int main(int argc, char** argv) {
    /////////////////
    // Game Init
    /////////////////
    if(!(AA_renderer = AA_game_init("AndyAder Test Window"))) return 1;
    srand(time(NULL));

    bool running = true;

    /////////////////
    // Loading Texture, Sound & Ready to run
    /////////////////
    if(AA_Background.load(AA_renderer, "img/jeanes/Farback01.png", "img/jeanes/Farback02.png"))
//        || AA_Player[0].load(AA_renderer, 10.0f, (float)(WINDOW_HEIGHT / 2), "img/jeanes/Ship0"))
        running = false;
    // Asteroid.png : iCCP ?????? ?????? - libpng ?????? ??????
    AA_Texture *asteroid = AA_load_texture(AA_renderer, "img/jeanes/Asteroid.png");
    AA_Texture *weapon_normal = AA_load_texture(AA_renderer, "img/wenrexa/02.png");
    AA_Texture *weapon_power = AA_load_texture(AA_renderer, "img/wenrexa/51.png");
    AA_Texture *explosion_anim = AA_load_texture(AA_renderer, "img/explosion_set.png");
    if(!asteroid || !weapon_normal || !explosion_anim) running = false;

    AA_Texture *item[AA_PLAYER_NUM_OF_ITEM] = {
        AA_load_texture(AA_renderer, "img/item_a.png"),
        AA_load_texture(AA_renderer, "img/item_3.png"),
        AA_load_texture(AA_renderer, "img/item_f.png"),
        AA_load_texture(AA_renderer, "img/item_p.png"),
        AA_load_texture(AA_renderer, "img/item_s.png")
    };

    AA_Texture *item_rtime[AA_PLAYER_NUM_OF_ITEM] = {
        NULL, NULL, NULL, NULL, NULL
    };

    // Text Texture
    TTF_Font *font = TTF_OpenFont("C:\\Windows\\Fonts\\MALGUN.TTF", 20);
    if(!font) {
        SDL_Log("Open font Err %s", TTF_GetError());
        return -1;
    }
    TTF_Font *bigfont = TTF_OpenFont("C:\\Windows\\Fonts\\MALGUN.TTF", 48);
    if(!bigfont) {
        SDL_Log("Open bigfont Err %s", TTF_GetError());
        return -1;
    }
    TTF_Font *smallfont = TTF_OpenFont("C:\\Windows\\Fonts\\MALGUN.TTF", 15);
    if(!smallfont) {
        SDL_Log("Open bigfont Err %s", TTF_GetError());
        return -1;
    }
    SDL_Color color_yellow = {255, 255, 0}, color_pink = {255, 150, 255}, color_black = {0, 0, 0}, color_white = {255, 255, 255};
    SDL_Color color_mint = {89, 255, 197}, color_orange = {255, 154, 38}, color_skyblue = {107, 206, 255};

    char str_tmp[30];
    // init score
    SDL_snprintf(str_tmp, 30, "SCORE : %d", score);
    AA_Texture *score_f = AA_load_ttf_texture(AA_renderer, font, str_tmp, &color_yellow);
    SDL_FRect score_rectf = {10.0f, 10.0f, (float)score_f->w, (float)score_f->h};
    AA_Texture *score_b = AA_load_ttf_texture(AA_renderer, font, str_tmp, &color_black);
    SDL_FRect score_rectb = {12.0f, 12.0f, (float)score_b->w, (float)score_b->h};

    // init hiscore
    SDL_snprintf(str_tmp, 30, "HISCORE : %d", hiscore);
    AA_Texture *hiscore_f = AA_load_ttf_texture(AA_renderer, font, str_tmp, &color_orange);
    SDL_FRect hiscore_rectf = {200.0f, 10.0f, (float)hiscore_f->w, (float)hiscore_f->h};
    AA_Texture *hiscore_b = AA_load_ttf_texture(AA_renderer, font, str_tmp, &color_black);
    SDL_FRect hiscore_rectb = {202.0f, 12.0f, (float)hiscore_f->w, (float)hiscore_f->h};

    AA_Texture *text_gameover = AA_load_ttf_texture(AA_renderer, bigfont, "GAME OVER", &color_pink);
    SDL_FRect text_gameover_rect = {
        (WINDOW_WIDTH - text_gameover->w) / 2.0f,
        (WINDOW_HEIGHT - text_gameover->h) / 2.0f,
        (float)text_gameover->w,
        (float)text_gameover->h
    };
    AA_Texture *text_game_title = AA_load_ttf_texture(AA_renderer, bigfont, "AA's First Shooter", &color_mint);
    SDL_FRect text_game_title_rect = {
        (WINDOW_WIDTH - text_game_title->w) / 2.0f,
        (WINDOW_HEIGHT - text_game_title->h) / 2.0f - 60.f,
        (float)text_game_title->w,
        (float)text_game_title->h
    };
    AA_Texture *text_start_intro = AA_load_ttf_texture(AA_renderer, bigfont, "Press [Enter] to start...", &color_white);
    SDL_FRect text_start_intro_rect = {
        (WINDOW_WIDTH - text_start_intro->w) / 2.0f,
        (WINDOW_HEIGHT - text_start_intro->h) / 2.0f,
        (float)text_start_intro->w,
        (float)text_start_intro->h
    };

    // Mixer Test : ?????? AA ????????? ?????? ??????
    Mix_Chunk *snd_fire = Mix_LoadWAV("sound/215423__taira-komori__pyo-1.mp3");
    if(!snd_fire) {
        SDL_Log("Mixer LoadWAV Err %s", Mix_GetError());
        return 1;
    }
    Mix_VolumeChunk(snd_fire, 12);
    Mix_Chunk *snd_big_fire = Mix_LoadWAV("sound/215421__taira-komori__beam.mp3");
    if(!snd_big_fire) {
        SDL_Log("Mixer LoadWAV Err %s", Mix_GetError());
        return 1;
    }
    Mix_VolumeChunk(snd_big_fire, 12);
    Mix_Chunk *snd_boom = Mix_LoadWAV("sound/215595__taira-komori__bomb.mp3");
    if(!snd_boom) {
        SDL_Log("Mixer LoadWAV Err %s", Mix_GetError());
        return 1;
    }
    Mix_VolumeChunk(snd_boom, 12);
    Mix_Chunk *player_boom = Mix_LoadWAV("sound/215599__taira-komori__explosion02.mp3");
    if(!player_boom) {
        SDL_Log("Mixer LoadWAV Err %s", Mix_GetError());
        return 1;
    }
    Mix_VolumeChunk(player_boom, 12);
    Mix_Chunk *player_pick_item = Mix_LoadWAV("sound/213428__taira-komori__poka01.mp3");
    if(!player_pick_item) {
        SDL_Log("Mixer LoadWAV Err %s", Mix_GetError());
        return 1;
    }
    Mix_VolumeChunk(player_pick_item, 12);

    /////////////////
    // Game state & configuration
    /////////////////
    SDL_Event event;
    const Uint8 *key_state;
    int dx, dy;
    int frame = 0, b_frame = -1, o_frame = -1;
    int game_state = AA_STATE_READY;

    // Player[0] ??? ?????? ??????
    int shot_frame = AA_AUTOGUN_NORMAL; // normal:15, auto:5
    int player_speed = 5;
    // Uint32 start_tick[1200], end_tick[1200]; // DEBUG

    /////////////////
    // Game progressing
    /////////////////
    while(running) {
        // start_tick[frame] = SDL_GetTicks();
        SDL_PollEvent(&event);
        if(event.type == SDL_QUIT) {  // ?????? X ?????? ????????? ??????
            running = false;
            break;
        }
        // Key Down ??????
        key_state = SDL_GetKeyboardState(NULL);

        // ESC ?????? ??? ??????
        if(key_state[SDL_SCANCODE_ESCAPE]) {
            running = false;
            break;
        }
        if(game_state == AA_STATE_READY) {
            if(key_state[SDL_SCANCODE_RETURN]) {
                game_state = AA_STATE_INGAME;
                if(AA_Player[0].load(AA_renderer, 10.0f, (float)(WINDOW_HEIGHT / 2), "img/jeanes/Ship0")) {
                    running = false;
                    goto QUIT;
                }
                frame = 0;
                score = 0;
                shot_frame = AA_AUTOGUN_NORMAL;
                SDL_snprintf(str_tmp, 30, "SCORE : %d", score);
                AA_free_texture(score_f);
                AA_free_texture(score_b);
                score_f = AA_load_ttf_texture(AA_renderer, font, str_tmp, &color_yellow);
                score_b = AA_load_ttf_texture(AA_renderer, font, str_tmp, &color_black);
                score_rectf.h = (float)score_f->h, score_rectf.w = (float)score_f->w;
                score_rectb.h = (float)score_b->h, score_rectb.w = (float)score_b->w;

            }
        }

        if(game_state == AA_STATE_INGAME) {
            dx = dy = 0;
            if(AA_Player[0].item[AA_ITEM_SPDUP]) player_speed = 10;
            else player_speed = 5;
            if(key_state[SDL_SCANCODE_UP]) dy-=player_speed;
            if(key_state[SDL_SCANCODE_DOWN]) dy+=player_speed;
            if(key_state[SDL_SCANCODE_LEFT]) dx-=player_speed;
            if(key_state[SDL_SCANCODE_RIGHT]) dx+=player_speed;
            if(dx && dy) {
                AA_Player[0].r.x += (float)(dx * 0.707f);
                AA_Player[0].r.y += (float)(dy * 0.707f);
            }
            else {
                AA_Player[0].r.x += (float)dx;
                AA_Player[0].r.y += (float)dy;
            }
            if(AA_Player[0].r.x < 0.0f)
                AA_Player[0].r.x = 0.0f;
            if(AA_Player[0].r.x > (float)WINDOW_WIDTH - AA_Player[0].r.w)
                AA_Player[0].r.x = (float)WINDOW_WIDTH - AA_Player[0].r.w;
            if(AA_Player[0].r.y < 0.0f) 
                AA_Player[0].r.y = 0.0f;
            if(AA_Player[0].r.y > (float)WINDOW_HEIGHT - AA_Player[0].r.h)
                AA_Player[0].r.y = (float)WINDOW_HEIGHT - AA_Player[0].r.h;
            AA_Player[0].cx = AA_Player[0].r.x + AA_Player[0].r.w/2 + 13; // ??????
            AA_Player[0].cy = AA_Player[0].r.y + AA_Player[0].r.h/2;

            if(key_state[SDL_SCANCODE_A] || key_state[SDL_SCANCODE_SPACE]) { // ?????? ??????
                if(b_frame % shot_frame == 0) { // Autofire Term ??????
                    int idx = AA_Bullet_t::curr_index();
                    if(AA_Player[0].item[AA_ITEM_POWERUP]) {
                        AA_Bullet[idx].load(weapon_power, 3,
                            AA_Player[0].r.x + AA_Player[0].r.w,
                            AA_Player[0].r.y + AA_Player[0].r.h/2,
                            0.5f, 0.0, &SDL_Rect({17, 43, 176, 46}));
                        Mix_PlayChannel(0, snd_big_fire, 0);
                    }
                    else {
                        AA_Bullet[idx].load(weapon_normal, 1,
                            AA_Player[0].r.x + AA_Player[0].r.w,
                            AA_Player[0].r.y + AA_Player[0].r.h/2,
                            0.5f, 0.0, &SDL_Rect({39, 45, 45, 33}));
                        Mix_PlayChannel(0, snd_fire, 0);
                    }
                    AA_Bullet[idx].dx = 10.0f;
                    AA_Bullet[idx].dy = 0.0f;
                    if(AA_Player[0].item[AA_ITEM_BULLET_SPDUP]) AA_Bullet[idx].dx = 20.0f;

                    // 3way ??????
                    if(AA_Player[0].item[AA_ITEM_3WAY]) {
                        idx = AA_Bullet_t::curr_index();
                        if(AA_Player[0].item[AA_ITEM_POWERUP]) {
                            AA_Bullet[idx].load(weapon_power, 3,
                                AA_Player[0].r.x + AA_Player[0].r.w,
                                AA_Player[0].r.y + AA_Player[0].r.h/2,
                                0.5f, -20.0, &SDL_Rect({17, 43, 176, 46}));
                        }
                        else {
                            AA_Bullet[idx].load(weapon_normal, 1,
                                AA_Player[0].r.x + AA_Player[0].r.w,
                                AA_Player[0].r.y + AA_Player[0].r.h/2,
                                0.5f, -20.0, &SDL_Rect({39, 45, 45, 33}));
                        }
                        AA_Bullet[idx].dx = 10.0f * SDL_cosf(-20.0f/180*3.14159f);
                        AA_Bullet[idx].dy = 10.0f * SDL_sinf(-20.0f/180*3.14159f);
                        AA_Bullet[idx].cy -= 15.0f;
                        if(AA_Player[0].item[AA_ITEM_BULLET_SPDUP]) AA_Bullet[idx].dx *= 2.0f, AA_Bullet[idx].dy *= 2.0f;

                        idx = AA_Bullet_t::curr_index();
                        if(AA_Player[0].item[AA_ITEM_POWERUP]) {
                            AA_Bullet[idx].load(weapon_power, 3,
                                AA_Player[0].r.x + AA_Player[0].r.w,
                                AA_Player[0].r.y + AA_Player[0].r.h/2,
                                0.5f, 20.0, &SDL_Rect({17, 43, 176, 46}));
                        }
                        else {
                            AA_Bullet[idx].load(weapon_normal, 1,
                                AA_Player[0].r.x + AA_Player[0].r.w,
                                AA_Player[0].r.y + AA_Player[0].r.h/2,
                                0.5f, 20.0, &SDL_Rect({39, 45, 45, 33}));
                        }
                        AA_Bullet[idx].dx = 10.0f * SDL_cosf(20.0f/180*3.14159f);
                        AA_Bullet[idx].dy = 10.0f * SDL_sinf(20.0f/180*3.14159f);
                        AA_Bullet[idx].cy += 15.0f;
                        if(AA_Player[0].item[AA_ITEM_BULLET_SPDUP]) AA_Bullet[idx].dx *= 2.0f, AA_Bullet[idx].dy *= 2.0f;

                    }
                }
            }
            else {
                b_frame = -1;
            }

            // ?????? ??????
            if(score < 10000) obj_create_period = 30;
            else if(score < 76672) obj_create_period = -(score - 10000) / 2778 + 30;
            else obj_create_period = 6;
            if(frame % obj_create_period == 0) { // ?????? 0.5????????? ?????? ??????, ???????????? ????????? ????????? ?????? 0.1????????? ??????
                int idx = AA_Object_t::curr_index();
                AA_Object[idx].load(asteroid,
                    (float)WINDOW_WIDTH,
                    (float)rand() * (WINDOW_HEIGHT - asteroid->h*0.5f) / RAND_MAX,
                    0.5f);
                AA_Object[idx].dx = -(float)(rand() % 5 + 3 + score/10000.0f);
                if(score < 10000) AA_Object[idx].dy = 0.0f;
                else AA_Object[idx].dy = (float)(rand() - RAND_MAX/2) * score/5000.0f / RAND_MAX;
            }

            // ?????? ?????? (???????????? ????????? ??? ????????????)
            // 1. ?????? ?????? ??????????????? ????????? ?????? ??? ?????? ???????????? ????????? ??? ???????????? ?????? ??????
            // 2. ?????????????????? ?????????????????? ?????? (????????? ?????? ????????? ??????????????? ??????, ???????????? ??????)
            // 3. ?????? ?????? ??? ?????? ?????? ????????? ??????
            for(int j=0; j<AA_CAP_OF_OBJ; j++) { // ??? ????????????
                if(AA_Object[j].activated
                    && is_collided2(AA_Player[0].cx, AA_Player[0].cy, AA_Player[0].radius,
                    AA_Object[j].cx, AA_Object[j].cy, AA_Object[j].radius))
                {
                    game_state = AA_STATE_GAMEOVER;
                    o_frame = 0;
                }
            }

            // ?????? ?????? (???????????? ????????? ??? ????????????)
            for(int i=0; i<AA_CAP_OF_BULLET; i++) { // ??????
                if(AA_Bullet[i].activated) {
                    for(int j=0; j<AA_CAP_OF_OBJ; j++) { // ??? ????????????
                        if(AA_Object[j].activated) {
                            if(is_collided2(AA_Bullet[i].cx, AA_Bullet[i].cy, AA_Bullet[i].radius,
                                AA_Object[j].cx, AA_Object[j].cy, AA_Object[j].radius))
                            {
                                // ?????? ?????? ????????? ????????? ??????????????? ???????????? ??????.
                                AA_Bullet[i].blit();
                                AA_Object[j].blit();
                                AA_Explosion[AA_Explosion_t::curr_index()].load(
                                    explosion_anim, AA_Object[j].cx, AA_Object[j].cy, 0.7f, 5
                                );
                                Mix_PlayChannel(-1, snd_boom, 0);

                                // ?????? ????????? ????????? ??????
                                int rnd = rand() % 50;
                                if(rnd < AA_PLAYER_NUM_OF_ITEM) {
                                    int idx = AA_Item_t::curr_index();
                                    AA_Item[idx].load(
                                        item[rnd],
                                        rnd,
                                        AA_Object[j].cx - item[rnd]->w * 0.4f,
                                        AA_Object[j].cy - item[rnd]->h * 0.4f,
                                        0.4f
                                    );
                                    AA_Item[idx].dx = -3.0f;
                                }
                                int hit_score = 100 + (int)AA_Object[j].r.x/10;
                                SDL_snprintf(str_tmp, 30, "%d", hit_score);
                                AA_Ingame_Message[AA_Ingame_Message_t::curr_index()].load(
                                    AA_renderer, smallfont, AA_Object[j].cx, AA_Object[j].cy,
                                    str_tmp, color_skyblue
                                );
                                score += hit_score;
                                --AA_Bullet[i].hp;
                                AA_Object[j].free();


                                SDL_snprintf(str_tmp, 30, "SCORE : %d", score);
                                AA_free_texture(score_f);
                                AA_free_texture(score_b);
                                score_f = AA_load_ttf_texture(AA_renderer, font, str_tmp, &color_yellow);
                                score_b = AA_load_ttf_texture(AA_renderer, font, str_tmp, &color_black);
                                score_rectf.h = (float)score_f->h, score_rectf.w = (float)score_f->w;
                                score_rectb.h = (float)score_b->h, score_rectb.w = (float)score_b->w;

                                // ????????? ????????? ??? ??????????????? ????????? ????????? ?????? ????????? 
                                // ??? ????????? ????????? ???????????? ????????? ????????? break ??? ?????? ??????.
                                if(!AA_Bullet[i].hp) {
                                    AA_Bullet[i].free();
                                    break;
                                }
                            }
                        }
                    }
                }
            }

            // ???????????? (???????????? ????????? ?????????)
            for(int i=0; i<AA_CAP_OF_ITEM; i++) { // ????????? ??????
                if(AA_Item[i].activated
                    && is_collided2(AA_Player[0].cx, AA_Player[0].cy, AA_Player[0].radius,
                    AA_Item[i].cx, AA_Item[i].cy, AA_Item[i].radius))
                {
                    AA_Player[0].item[AA_Item[i].kind] += 900; // 15???
                    AA_Ingame_Message[AA_Ingame_Message_t::curr_index()].load(
                        AA_renderer, smallfont, AA_Item[i].cx, AA_Item[i].cy,
                        AA_Item_name[AA_Item[i].kind], color_skyblue
                    );
                    Mix_PlayChannel(-1, player_pick_item, 0);
                    AA_Item[i].free();
                }
            }

            // ???????????? ????????? ????????? ?????? ?????? 
            if(AA_Player[0].item[AA_ITEM_AUTOSHOT]) shot_frame = AA_AUTOGUN_AUTO;
            else shot_frame = AA_AUTOGUN_NORMAL;
            // if(AA_Player[0].item[AA_ITEM_3WAY]) {
            // }
            // if(AA_Player[0].item[AA_ITEM_BULLET_SPDUP]) {
            // }
            // if(AA_Player[0].item[AA_ITEM_POWERUP]) {
            // }
            // if(AA_Player[0].item[AA_ITEM_SPDUP]) {
            // }

        } // End of if(game_state == AA_STATE_INGAME)

        if(game_state == AA_STATE_GAMEOVER) {
            if(o_frame == 0) {
                AA_Explosion[AA_Explosion_t::curr_index()].load(
                    explosion_anim, AA_Player[0].cx, AA_Player[0].cy
                );
                for(int i=0; i<AA_CAP_OF_OBJ; i++) { // ??? ???????????? ??????
                    if(AA_Object[i].activated) {
                        AA_Object[i].free();
                    }
                }
                for(int i=0; i<AA_CAP_OF_ITEM; i++) { // ????????? ??????
                    if(AA_Item[i].activated) {
                        AA_Item[i].free();
                    }
                }
                for(int i=0; i<AA_PLAYER_NUM_OF_ITEM; i++) { // ?????? ?????? ????????? ?????? ?????? ????????? ??????
                    AA_free_texture(item_rtime[i]);
                }
                AA_Player[0].free();
                Mix_PlayChannel(-1, player_boom, 0);
            }
            if(o_frame == 150) {
                game_state = AA_STATE_READY;
            }
            ++o_frame;
        }

        /////////////////
        // ?????????!!!!
        /////////////////
        // ?????? ?????????
        AA_Background.blit();

        // ??? ???????????? ?????????
        for(int i=0; i<AA_CAP_OF_OBJ; i++) {
            if(AA_Object[i].activated) {
                AA_Object[i].blit();
                if(AA_Object[i].r.x < -AA_Object[i].r.w) AA_Object[i].free();
            }
        }
        // ???????????? ?????? ?????????
        // ?????? ???????????? ????????? ???????????? ????????????.
        for(int i=0; i<AA_CAP_OF_BULLET; i++) {
            if(AA_Bullet[i].activated) {
                AA_Bullet[i].blit();
                if(AA_Bullet[i].r.x > (float)WINDOW_WIDTH) AA_Bullet[i].free();
            }
        }

        // ????????? ?????????
        for(int i=0; i<AA_CAP_OF_ITEM; i++) {
            if(AA_Item[i].activated) {
                AA_Item[i].blit();
                if(AA_Item[i].r.x < -AA_Item[i].r.w) AA_Item[i].free();
            }
        }

        // ?????? ??????????????? ?????????
        for(int i=0; i<AA_CAP_OF_EXPLOSION; i++) {
            if(AA_Explosion[i].activated) {
                AA_Explosion[i].blit();
            }
        }

        // In-game Message ?????????
        for(int i=0; i<AA_CAP_OF_INGAME_MESSAGE; i++) {
            if(AA_Ingame_Message[i].activated) {
                AA_Ingame_Message[i].blit();
            }
        }

        // ???????????? ?????? ?????????
        if(game_state == AA_STATE_INGAME) {
            AA_Player[0].blit();
        }

        // GAME OVER ??????
        if(game_state == AA_STATE_GAMEOVER) {
            SDL_RenderCopyF(AA_renderer, text_gameover->tex, NULL, &text_gameover_rect);
        }

        // Title ??????
        if(game_state == AA_STATE_READY) {
            SDL_RenderCopyF(AA_renderer, text_game_title->tex, NULL, &text_game_title_rect);
            if(frame % 60 < 30) {
                SDL_RenderCopyF(AA_renderer, text_start_intro->tex, NULL, &text_start_intro_rect);
            }
        }

        // Score ??????
        SDL_RenderCopyF(AA_renderer, score_b->tex, NULL, &score_rectb);
        SDL_RenderCopyF(AA_renderer, score_f->tex, NULL, &score_rectf);
        if(score > hiscore) {
            hiscore = score;
            SDL_snprintf(str_tmp, 30, "HISCORE : %d", hiscore);
            AA_free_texture(hiscore_f);
            AA_free_texture(hiscore_b);
            hiscore_f = AA_load_ttf_texture(AA_renderer, font, str_tmp, &color_orange);
            hiscore_rectf.w = (float)hiscore_f->w, hiscore_rectf.h = (float)hiscore_f->h;
            hiscore_b = AA_load_ttf_texture(AA_renderer, font, str_tmp, &color_black);
            hiscore_rectb.w = (float)hiscore_b->w, hiscore_rectb.h = (float)hiscore_b->h;
        }
        SDL_RenderCopyF(AA_renderer, hiscore_b->tex, NULL, &hiscore_rectb);
        SDL_RenderCopyF(AA_renderer, hiscore_f->tex, NULL, &hiscore_rectf);

        // Item ?????? ?????? ??????
        if(game_state == AA_STATE_INGAME) {
            for(int i=0, j=0; i<AA_PLAYER_NUM_OF_ITEM; i++) {
                AA_free_texture(item_rtime[i]);
                if(AA_Player[0].item[i]) {
                    SDL_snprintf(str_tmp, 30, ": %d", AA_Player[0].item[i] / 60);
                    item_rtime[i] = AA_load_ttf_texture(AA_renderer, font, str_tmp, &color_mint);
                    SDL_RenderCopyF(AA_renderer, item[i]->tex, NULL, &SDL_FRect({900.0f, 10.0f + j*35, (float)item[i]->w*0.5f, (float)item[i]->h*0.5f}));
                    SDL_RenderCopyF(AA_renderer, item_rtime[i]->tex, NULL, &SDL_FRect({940.0f, 10.0f + j*35, (float)item_rtime[i]->w, (float)item_rtime[i]->h}));
                    ++j, --AA_Player[0].item[i];
                }
            }
        }

        //Test
        //SDL_RenderCopyF(AA_renderer, item_a->tex, NULL, &SDL_FRect({200.0f, 200.0f, (float)item_a->w/2, (float)item_a->h/2}));

        // end_tick[frame] = SDL_GetTicks();
        SDL_RenderPresent(AA_renderer);
        AA_Background.move();
        ++frame, ++b_frame;
    }

QUIT:
    // Game Quit
    for(int i=0; i<AA_PLAYER_NUM_OF_ITEM; i++) {
        AA_free_texture(item[i]);
        AA_free_texture(item_rtime[i]);
    }
    AA_free_texture(asteroid);
    AA_free_texture(weapon_normal);
    AA_free_texture(weapon_power);
    AA_free_texture(explosion_anim);
    AA_free_texture(score_f);
    AA_free_texture(score_b);
    AA_free_texture(hiscore_f);
    AA_free_texture(hiscore_b);
    AA_free_texture(text_gameover);
    AA_free_texture(text_game_title);
    AA_free_texture(text_start_intro);
    Mix_FreeChunk(snd_fire);
    Mix_FreeChunk(snd_big_fire);
    Mix_FreeChunk(snd_boom);
    Mix_FreeChunk(player_boom);
    Mix_FreeChunk(player_pick_item);
    AA_Player[0].free();
    AA_Background.free();
    AA_game_quit();

    // for(int i=0; i<frame; i++) {
    //     printf("%d : [%u] ~ [%u]\n", i, start_tick[i], end_tick[i]);
    // }
    return 0;
}