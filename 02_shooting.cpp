#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>

#include <cstdio> // for sprintf
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
// Texture 구조
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

void AA_free_texture(AA_Texture *t) {
    delete t;
    t = NULL;
}

//---------------------------------------
// Object 및 Background 구조
//---------------------------------------

struct _AA_Object { // 미완성... 수평, 수직이 아닌 경우에 대한 고려가 필요
    AA_Texture *tex;
    bool valid;
    SDL_Rect org_r, edge_r;
    int xvalo, yvalo;
    int cx, cy;
};

// 0. 배경
struct AA_Background_t {
    AA_Texture *atex[2]; // magic number이긴 하지만.. 일단..
    SDL_FRect r[2];
    int load(SDL_Renderer *renderer, const char *s1, const char *s2) { // 초기 설정시
        atex[0] = AA_load_texture(renderer, s1, &r[0]);
        atex[1] = AA_load_texture(renderer, s2, &r[1]);
        if(!atex[0] || !atex[1]) return 1;
        r[0].x = r[0].y = r[1].y = 0, r[1].x = r[0].w;
        return 0;
    }
    void blit() {  // while 구문 내
        SDL_RenderCopyF(atex[0]->renderer, atex[0]->tex, NULL, &r[0]);
        SDL_RenderCopyF(atex[1]->renderer, atex[1]->tex, NULL, &r[1]);
    }
    void move() { // while 구문 내
        r[0].x -= 1.0f;
        r[1].x -= 1.0f;
        if(r[0].x == 0.0f) r[1].x = r[0].w; // ?
        if(r[1].x == 0.0f) r[0].x = r[1].w; // ?
    }
    void free() { // 종료시
        AA_free_texture(atex[0]);
        AA_free_texture(atex[1]);
    }
} AA_Background;

// 1. 플레이어 기체
const int AA_PLAYER_NUM_OF_IMG = 4;
struct AA_Player_t {
    AA_Texture *atex[AA_PLAYER_NUM_OF_IMG];
    SDL_FRect r;
    float cx, cy, radius; // 중심 좌표, 충돌반경
    int fnum;
    int load(SDL_Renderer *renderer, float x, float y, const char *s) {
        char buff[101];
        for(int i=0; i<AA_PLAYER_NUM_OF_IMG; i++) {
            sprintf(buff, "%s%d.png", s, i+1);
            atex[i] = AA_load_texture(renderer, buff, &r);
            if(!atex[i]) return 1;
        }
        fnum = 0;
        //r.x = 10.0f, r.y = (float)(WINDOW_HEIGHT / 2);
        r.x = x, r.y = y;
        cx = r.x + r.w/2 + 13, cy = r.y + r.h/2; // 13 : 반경 조금 조정
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
} AA_Player[2]; // 2인용을 고려하여

// 2. 적 기체, 장애물 등
const int AA_CAP_OF_OBJ = 500;
struct AA_Object_t {
    static int pindex; // 들어갈 번지수
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
        SDL_RenderCopyF(atex->renderer, atex->tex, NULL, &r);
        r.x += dx, r.y += dy;
        cx += dx, cy += dy;
    }
    void free() {
        activated = false;
        atex = NULL;
    }
    AA_Object_t() : activated(false), atex(NULL), dx(0.0f), dy(0.0f), hp(0) {}
} AA_Object[AA_CAP_OF_OBJ]; // 최대 1000개.
int AA_Object_t::pindex = 0;
int AA_Object_t::curr_index() {
    if(AA_Object_t::pindex == AA_CAP_OF_OBJ) AA_Object_t::pindex = 0;
    return AA_Object_t::pindex++;
}

// 3. 플레이어 탄환
const int AA_CAP_OF_BULLET = 500;
struct AA_Bullet_t {
    static int pindex; // 들어갈 번지수
    bool activated;
    AA_Texture *atex;
    SDL_Rect src_r;
    SDL_FRect r;
    float dx, dy, cx, cy, radius;

    static int curr_index();
    void load(AA_Texture *t, float start_x, float start_y, float mag_rate = 1.0f, SDL_Rect *s = NULL) {
        activated = true;
        atex = t;
        if(!s) {
            src_r = {0, 0, 0, 0};
            r.h = t->h * mag_rate, r.w = t->w * mag_rate;
        }
        else {
            src_r = *s;
            r.h = s->h * mag_rate, r.w = s->w * mag_rate;
        }
        r.x = start_x, r.y = start_y - r.h/2;
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
    AA_Bullet_t() : activated(false), atex(NULL), dx(0.0f), dy(0.0f) {}
} AA_Bullet[AA_CAP_OF_BULLET]; 
int AA_Bullet_t::pindex = 0;
int AA_Bullet_t::curr_index() {
    if(AA_Bullet_t::pindex == AA_CAP_OF_BULLET) AA_Bullet_t::pindex = 0;
    return AA_Bullet_t::pindex++;
}

// 4. 폭발애니메이션
const int AA_CAP_OF_EXPLOSION = 200;
struct AA_Explosion_t {
    static int pindex; // 들어갈 번지수
    bool activated;
    AA_Texture *atex;
    SDL_Rect src_r; // 64x64 이미지가 가로 10, 세로 11 개 존재, raster 순.
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

// 사각형 판정법
inline bool is_collided1(const SDL_FRect &r1, const SDL_FRect &r2) {
    if(r1.x + r1.w < r2.x || r2.x + r2.w < r1.x
        || r1.y + r1.h < r2.y || r2.y + r2.h < r1.y) return false;
    return true;
}

// 거리 반경 (원) 판정법
inline bool is_collided2(float x1, float y1, float r1, float x2, float y2, float r2) {
    return (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1) <= (r1+r2)*(r1+r2);
}

int score = 0;

//---------------------------------------
// main
//---------------------------------------
// 1. SDL 관련 모듈을 Init 한다.
// 2. window 생성하고 renderer 생성한다.
// 3. while 루프를 이용하여 그림을 그리고 renderer flush
//
// Note : 현재는 수직동기화 속도에 sync 되어 fps 가 잡히고
//        그 frame rate 속도를 기준으로 오브젝트가 이동하지만
//        (따라서 fps가 느리면 느리게 이동, 빠르면 빠르게 이동하는 불합리한 점이 ㅡㅡ;;)
//        추후에는 속도를 frame rate 기준이 아닌 System Time Tick 을 기준으로 맞추어야
//        하므로, 이를 반영할 것을 고려하여 프로그래밍 해야 함.
enum AA_Game_State {
    AA_STATE_UNDEFINED = 0,
    AA_STATE_READY = 1,
    AA_STATE_INGAME = 2,
    AA_STATE_GAMEOVER = 3
};

int main(int argc, char** argv) {
    // Game Init
    if(!(AA_renderer = AA_game_init("AndyAder Test Window"))) return 1;
    srand(time(NULL));

    bool running = true;

    // Loading Texture, Sound & Ready to run
    if(AA_Background.load(AA_renderer, "img/jeanes/Farback01.png", "img/jeanes/Farback02.png")
        || AA_Player[0].load(AA_renderer, 10.0f, (float)(WINDOW_HEIGHT / 2), "img/jeanes/Ship0"))
        running = false;
    // Asteroid.png : iCCP 정보 깨짐 - libpng 경고 발생
    AA_Texture *asteroid = AA_load_texture(AA_renderer, "img/jeanes/Asteroid.png");
    AA_Texture *weapon_normal = AA_load_texture(AA_renderer, "img/wenrexa/02.png");
    AA_Texture *explosion_anim = AA_load_texture(AA_renderer, "img/explosion_set.png");
    if(!asteroid || !weapon_normal || !explosion_anim) running = false;

    // Text Texture
    TTF_Font *font = TTF_OpenFont("C:\\Windows\\Fonts\\MALGUN.TTF", 20);
    if(!font) {
        SDL_Log("Open font Err %s", TTF_GetError());
        return -1;
    }
    SDL_Color fgcolor = {255, 255, 0}, bgcolor = {0, 0, 0};
    char str_score[20];
    sprintf(str_score, "HITS : %d", score);
    AA_Texture *score_f = AA_load_ttf_texture(AA_renderer, font, str_score, &fgcolor);
    SDL_FRect score_rectf = {10.0f, 10.0f, (float)score_f->w, (float)score_f->h};
    AA_Texture *score_b = AA_load_ttf_texture(AA_renderer, font, str_score, &bgcolor);
    SDL_FRect score_rectb = {12.0f, 12.0f, (float)score_b->w, (float)score_b->h};

    // Mixer Test : 추후 AA 함수로 만들 예정
    Mix_Chunk *snd_fire = Mix_LoadWAV("sound/215423__taira-komori__pyo-1.mp3");
    if(!snd_fire) {
        SDL_Log("Mixer LoadWAV Err %s", Mix_GetError());
        return 1;
    }
    Mix_VolumeChunk(snd_fire, 12);
    Mix_Chunk *snd_boom = Mix_LoadWAV("sound/215595__taira-komori__bomb.mp3");
    if(!snd_boom) {
        SDL_Log("Mixer LoadWAV Err %s", Mix_GetError());
        return 1;
    }
    Mix_VolumeChunk(snd_boom, 12);

    SDL_Event event;
    const Uint8 *key_state;
    int dx, dy;
    int frame = 0, b_frame = 0;
    int game_state = AA_STATE_READY;
    // Uint32 start_tick[1200], end_tick[1200]; // DEBUG

    while(running) {
        // start_tick[frame] = SDL_GetTicks();
        SDL_PollEvent(&event);
        if(event.type == SDL_QUIT) {  // 윈도 X 버튼 클릭시 종료
            running = false;
            break;
        }
        // Key Down 상태
        key_state = SDL_GetKeyboardState(NULL);

        // ESC 누를 시 종료
        if(key_state[SDL_SCANCODE_ESCAPE]) {
            running = false;
            break;
        }

        dx = dy = 0;
        if(key_state[SDL_SCANCODE_UP]) dy-=5;
        if(key_state[SDL_SCANCODE_DOWN]) dy+=5;
        if(key_state[SDL_SCANCODE_LEFT]) dx-=5;
        if(key_state[SDL_SCANCODE_RIGHT]) dx+=5;
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
        AA_Player[0].cx = AA_Player[0].r.x + AA_Player[0].r.w/2 + 13; // 보정
        AA_Player[0].cy = AA_Player[0].r.y + AA_Player[0].r.h/2;

        if(key_state[SDL_SCANCODE_A]) { // 탄환 발사
            if(b_frame % 40 == 0) { // Autofire Term 설정
                int idx = AA_Bullet_t::curr_index();
                AA_Bullet[idx].load(weapon_normal, 
                    AA_Player[0].r.x + AA_Player[0].r.w,
                    AA_Player[0].r.y + AA_Player[0].r.h/2,
                    0.5f, &SDL_Rect({39, 45, 45, 33}));
                //AA_Bullet[idx].r.x = AA_Player[0].r.x + AA_Player[0].r.w;
                //AA_Bullet[idx].r.y = AA_Player[0].r.y + (AA_Player[0].r.h - AA_Bullet[idx].r.h)/2.0f;
                AA_Bullet[idx].dx = 10.0f;
                Mix_PlayChannel(-1, snd_fire, 0);
            }
        }
        else {
            b_frame = -1;
        }

        // 배경 그리기
        AA_Background.blit();

        // 운석 생성
        if(frame % 30 == 29) { // 0.5초마다 운석 생성
            int idx = AA_Object_t::curr_index();
            AA_Object[idx].load(asteroid,
                (float)WINDOW_WIDTH,
                (float)rand() * (WINDOW_HEIGHT - asteroid->h*0.5f) / RAND_MAX,
                0.5f);
            AA_Object[idx].dx = -(float)(rand() % 5 + 3);
        }

        // 충돌 판정 (플레이어 기체와 적 오브젝트)
        // 1. 먼저 폭발 애니메이션 텍스쳐 생성 및 폭발 오브젝트 클래스 및 인스턴스 공간 생성
        // 2. 거리판정할지 사각판정할지 선택 (텍스쳐 굴곡 판정은 어려우므로 패스, 거리판정 결정)
        // 3. 충돌 판정 및 폭발 로직 이곳에 코딩
        for(int j=0; j<AA_CAP_OF_OBJ; j++) { // 적 오브젝트
            if(AA_Object[j].activated
                && is_collided2(AA_Player[0].cx, AA_Player[0].cy, AA_Player[0].radius,
                AA_Object[j].cx, AA_Object[j].cy, AA_Object[j].radius))
            {
                running = false; // 일단 걍 바로 끝내자.
                goto QUIT;
            }
        }

        // 충돌 판정 (플레이어 탄환과 적 오브젝트)
        for(int i=0; i<AA_CAP_OF_BULLET; i++) { // 탄환
            if(AA_Bullet[i].activated) {
                for(int j=0; j<AA_CAP_OF_OBJ; j++) { // 적 오브젝트
                    if(AA_Object[j].activated) {
                        if(is_collided2(AA_Bullet[i].cx, AA_Bullet[i].cy, AA_Bullet[i].radius,
                            AA_Object[j].cx, AA_Object[j].cy, AA_Object[j].radius))
                        {
                            // 충돌 폭발 직전의 상황을 그려주어서 어색함을 없앰.
                            AA_Bullet[i].blit();
                            AA_Object[j].blit();
                            AA_Explosion[AA_Explosion_t::curr_index()].load(
                                explosion_anim, AA_Object[j].cx, AA_Object[j].cy, 0.7f, 5
                            );
                            Mix_PlayChannel(-1, snd_boom, 0);
                            AA_Bullet[i].free();
                            AA_Object[j].free();

                            sprintf(str_score, "HITS : %d", ++score);
                            AA_free_texture(score_f);
                            AA_free_texture(score_b);
                            score_f = AA_load_ttf_texture(AA_renderer, font, str_score, &fgcolor);
                            score_b = AA_load_ttf_texture(AA_renderer, font, str_score, &bgcolor);

                            // 탄환이 두개의 적 오브젝트에 동시에 맞았을 경우 생기는 
                            // 널 포인터 오류를 방지하기 위하여 반드시 break 를 해아 한다.
                            break;
                        }
                    }
                }
            }
        }

        // 폭발 애니메이션 그리기
        for(int i=0; i<AA_CAP_OF_EXPLOSION; i++) {
            if(AA_Explosion[i].activated) {
                AA_Explosion[i].blit();
            }
        }

        // 적 오브젝트 그리기
        for(int i=0; i<AA_CAP_OF_OBJ; i++) {
            if(AA_Object[i].activated) {
                AA_Object[i].blit();
                if(AA_Object[i].r.x < -AA_Object[i].r.w) AA_Object[i].free();
            }
        }
        // 플레이어 탄환 그리기
        // 운석 그리기와 비슷한 방법으로 구현한다.
        for(int i=0; i<AA_CAP_OF_BULLET; i++) {
            if(AA_Bullet[i].activated) {
                AA_Bullet[i].blit();
                if(AA_Bullet[i].r.x > (float)WINDOW_WIDTH) AA_Bullet[i].free();
            }
        }

        // 플레이어 기체 그리기
        AA_Player[0].blit();

        // Score 표시
        SDL_RenderCopyF(AA_renderer, score_b->tex, NULL, &score_rectb);
        SDL_RenderCopyF(AA_renderer, score_f->tex, NULL, &score_rectf);

        // end_tick[frame] = SDL_GetTicks();
        SDL_RenderPresent(AA_renderer);
        AA_Background.move();
        ++frame, ++b_frame;
    }

QUIT:
    // Game Quit
    AA_free_texture(asteroid);
    AA_free_texture(weapon_normal);
    AA_free_texture(explosion_anim);
    AA_free_texture(score_f);
    AA_free_texture(score_b);
    Mix_FreeChunk(snd_fire);
    Mix_FreeChunk(snd_boom);
    AA_Player[0].free();
    AA_Background.free();
    AA_game_quit();

    // for(int i=0; i<frame; i++) {
    //     printf("%d : [%u] ~ [%u]\n", i, start_tick[i], end_tick[i]);
    // }
    return 0;
}