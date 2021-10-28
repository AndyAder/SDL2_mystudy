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
        SDL_Log("IMG_Load(%s) Err %s", img_filename, Mix_GetError());
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

struct { // class 명을 일부러 넣지 않음. 전역에 단 하나의 개체만 할당.
    AA_Texture *atex[2]; // magic number이긴 하지만.. 일단..
    SDL_FRect r[2];
    void load(SDL_Renderer *renderer, const char *s1, const char *s2) { // 초기 설정시
        atex[0] = AA_load_texture(renderer, s1, &r[0]);
        atex[1] = AA_load_texture(renderer, s2, &r[1]);
        r[0].x = r[0].y = r[1].y = 0, r[1].x = r[0].w;
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

const int AA_PLAYER_NUM_OF_IMG = 4;
struct AA_Player_t {
    AA_Texture *atex[AA_PLAYER_NUM_OF_IMG];
    SDL_FRect r;
    int fnum;
    void load(SDL_Renderer *renderer, const char *s) {
        char buff[101];
        for(int i=0; i<AA_PLAYER_NUM_OF_IMG; i++) {
            sprintf(buff, "%s%d.png", s, i+1);
            atex[i] = AA_load_texture(renderer, buff, &r);
        }
        fnum = 0;
        r.x = 10.0f, r.y = (float)(WINDOW_HEIGHT / 2);
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

const int AA_CAP_OF_OBJ = 1000;
struct AA_Object_t {
    static int pindex;
    bool activated;
    AA_Texture *atex;
    SDL_FRect r;
    float dx, dy;

    static int curr_index();
    void load(AA_Texture *t, float mag_rate = 1.0f) {
        activated = true;
        atex = t;
        r.h = t->h * mag_rate, r.w = t->w * mag_rate;
        //atex = AA_load_texture(renderer, s, r);
    }
    void blit() {
        SDL_RenderCopyF(atex->renderer, atex->tex, NULL, &r);
        r.x += dx, r.y += dy;
    }
    void free() {
        activated = false;
        atex = NULL;
    }
    AA_Object_t() : activated(false), atex(NULL), dx(0.0f), dy(0.0f) {}
} AA_Object[AA_CAP_OF_OBJ]; // 최대 1000개.
int AA_Object_t::pindex = 0;
int AA_Object_t::curr_index() {
        if(AA_Object_t::pindex == AA_CAP_OF_OBJ) AA_Object_t::pindex = 0;
        return AA_Object_t::pindex++;
}

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
int main(int argc, char** argv) {
    // Game Init
    if(!(AA_renderer = AA_game_init("AndyAder Test Window"))) return 1;
    srand(time(NULL));

    // Loading Texture & Ready to run
    AA_Background.load(AA_renderer, "img/jeanes/Farback01.png", "img/jeanes/Farback02.png");
    AA_Player[0].load(AA_renderer, "img/jeanes/Ship0");
    AA_Texture *asteroid = AA_load_texture(AA_renderer, "img/jeanes/Asteroid.png");
    AA_Texture *weapon_normal = AA_load_texture(AA_renderer, "img/wenrexa/02.png");

    bool running = true;
    SDL_Event event;
    const Uint8 *key_state;
    int dx, dy;
    int frame = 0;

    while(running) {
        SDL_PollEvent(&event);
        if(event.type == SDL_QUIT) {  // 종료버튼 클릭
            running = false;
            break;
        }
        // Key Down 상태
        key_state = SDL_GetKeyboardState(NULL);
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
        if(AA_Player[0].r.x < 0.0f) AA_Player[0].r.x = 0.0f;
        if(AA_Player[0].r.x > (float)WINDOW_WIDTH - AA_Player[0].r.w)
            AA_Player[0].r.x = (float)WINDOW_WIDTH - AA_Player[0].r.w;
        if(AA_Player[0].r.y < 0.0f) AA_Player[0].r.y = 0.0f;
        if(AA_Player[0].r.y > (float)WINDOW_HEIGHT - AA_Player[0].r.h)
            AA_Player[0].r.y = (float)WINDOW_HEIGHT - AA_Player[0].r.h;

        if(key_state[SDL_SCANCODE_A]) {
            // 총알 생성로직 (frame으로 오토샷 주기 설정)
            // 총일 인스턴스 공간 확보한 후, 생성한 총알 텍스쳐를 가공하여 인스턴스 완성
            // 오토샷 관련 변수 및 로직 제작. (else 구문 포함)
        }

        // 배경 그리기
        AA_Background.blit();

        // 운석 생성 및 그리기
        if(frame % 30 == 29) { // 0.5초마다 운석 생성
            int idx = AA_Object_t::curr_index();
            AA_Object[idx].load(asteroid, 0.5f);
            AA_Object[idx].r.x = (float)WINDOW_WIDTH;
            AA_Object[idx].r.y = (float)(rand() % (WINDOW_HEIGHT - (int)AA_Object[idx].r.h));
            AA_Object[idx].dx = -(float)(rand() % 5 + 3);
        }
        for(int i=0; i<AA_CAP_OF_OBJ; i++) {
            if(AA_Object[i].activated) {
                AA_Object[i].blit();
                if(AA_Object[i].r.x < -AA_Object[i].r.w) AA_Object[i].free();
            }
        }

        // 플레이어 탄환 그리기
        // 운석 그리기와 비슷한 방법으로 구현한다.


        // 플레이어 기체 그리기
        AA_Player[0].blit();

        // 충돌 판정 (플레이어 기체와 운석)
        // 1. 먼저 폭발 애니메이션 텍스쳐 생성 및 폭발 오브젝트 클래스 및 인스턴스 공간 생성
        // 2. 거리판정할지 사각판정할지 선택 (텍스쳐 굴곡 판정은 어려우므로 패스)
        // 3. 충돌 판정 및 폭발 로직 이곳에 코딩


        SDL_RenderPresent(AA_renderer);
        AA_Background.move();
        ++frame;
    }

    // Game Quit
    AA_free_texture(asteroid);
    AA_free_texture(weapon_normal);
    AA_Player[0].free();
    AA_Background.free();
    AA_game_quit();
    return 0;
}