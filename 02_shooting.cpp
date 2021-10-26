#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>

#include <cstdio>

//---------------------------------------
// Init function
//---------------------------------------
const int WINDOW_WIDTH = 1024;
const int WINDOW_HEIGHT = 768;
static SDL_Window *AAwindow = NULL;
static SDL_Renderer *AArenderer = NULL;
SDL_Renderer *AAgame_init(const char *title) {
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

    AAwindow = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        0
    );
    if(!AAwindow) {
        SDL_Log("Create window Err %s", SDL_GetError());
        return NULL;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(
        AAwindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE
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

void AAgame_quit() {
    SDL_DestroyRenderer(AArenderer);
    SDL_DestroyWindow(AAwindow);
    Mix_Quit();
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}


//---------------------------------------
// main
//---------------------------------------
// 1. SDL 관련 모듈을 Init 한다.
// 2. window 생성하고 renderer 생성한다.
// 3. while 루프를 이용하여 그림을 그리고 renderer flush
int main(int argc, char** argv) {
    // Game Init
    if(!(AArenderer = AAgame_init("AndyAder Test Window"))) return 1;

    // Continue coding .......

    // Game Quit
    AAgame_quit();
    return 0;
}