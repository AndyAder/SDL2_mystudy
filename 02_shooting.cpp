#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>

#include <cstdio> // for sprintf

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
        AA_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE
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
    SDL_FRect r;
    AA_Texture(SDL_Renderer *_r, SDL_Surface *_s) 
        : tex(SDL_CreateTextureFromSurface(_r, _s))
        , renderer(_r)
        , r{0.0f, 0.0f, (float)_s->w, (float)_s->h} {
            SDL_FreeSurface(_s);
    }
    ~AA_Texture() {
        SDL_DestroyTexture(tex);
    }
};

AA_Texture *AA_load_texture(SDL_Renderer *renderer, const char *img_filename) {
    SDL_Surface *sur = IMG_Load(img_filename);
    if(!sur) {
        SDL_Log("IMG_Load(%s) Err %s", img_filename, Mix_GetError());
        return NULL;
    }
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

struct AA_Object { // 미완성... 수평, 수직이 아닌 경우에 대한 고려가 필요
    AA_Texture *tex;
    bool valid;
    SDL_Rect org_r, edge_r;
    int xvalo, yvalo;
    int cx, cy;
};

struct { // class 명을 일부러 넣지 않음. 전역에 단 하나의 개체만 할당.
    AA_Texture *atex[2]; // magic number이긴 하지만.. 일단..
    void load(SDL_Renderer *renderer, const char *s1, const char *s2) { // 초기 설정시
        atex[0] = AA_load_texture(renderer, s1);
        atex[1] = AA_load_texture(renderer, s2);
        atex[1]->r.x = atex[0]->r.w;
    }
    void blit() {  // while 구문 내
        SDL_RenderCopyF(atex[0]->renderer, atex[0]->tex, NULL, &atex[0]->r);
        SDL_RenderCopyF(atex[1]->renderer, atex[1]->tex, NULL, &atex[1]->r);
    }
    void move() { // while 구문 내
        atex[0]->r.x -= 1.0f;
        atex[1]->r.x -= 1.0f;
        if(atex[0]->r.x == 0.0f) atex[1]->r.x = atex[0]->r.w; // ?
        if(atex[1]->r.x == 0.0f) atex[0]->r.x = atex[1]->r.w; // ?
    }
    void free() { // 종료시
        AA_free_texture(atex[0]);
        AA_free_texture(atex[1]);
    }
} AA_Background;

const int AA_PLAYER_NUM_OF_IMG = 4;
struct {
    AA_Texture *atex[AA_PLAYER_NUM_OF_IMG];
    int fnum;
    void load(SDL_Renderer *renderer, const char *s) {
        char buff[101];
        for(int i=0; i<AA_PLAYER_NUM_OF_IMG; i++) {
            sprintf(buff, "%s%d.png", s, i+1);
            atex[i] = AA_load_texture(renderer, buff);
        }
        fnum = 0;
    }
    void blit(float x, float y) {
        atex[fnum]->r.x = x, atex[fnum]->r.y = y;
        SDL_RenderCopyF(atex[fnum]->renderer, atex[fnum]->tex, NULL, &atex[fnum]->r);
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

    // Loading Texture & Ready to run
    AA_Background.load(AA_renderer, "img/jeanes/Farback01.png", "img/jeanes/Farback02.png");
    AA_Player[0].load(AA_renderer, "img/jeanes/Ship0");

    bool running = true;
    SDL_Event event;
    const Uint8 *key_state;

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



        AA_Background.blit();
        AA_Player[0].blit(500.0f, 500.0f);

        SDL_RenderPresent(AA_renderer);
        AA_Background.move();
    }

    // Game Quit

    AA_Player[0].free();
    AA_Background.free();
    AA_game_quit();
    return 0;
}