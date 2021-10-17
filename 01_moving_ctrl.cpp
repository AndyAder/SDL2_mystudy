#include <SDL.h>
#include <SDL_ttf.h>
#include <cstdio>

// SDL_Delay 를 사용하지 않고 그냥 while 루프를 수행했으나
// fps 가 60으로 고정됨. 그 이유를 찾아야 함.

static TTF_Font *font;
int fcount = 0, fcount_per_sec = 0;
float fps = 0.0f;
SDL_Texture *fps_tex = NULL;
SDL_Rect fps_rect;
static SDL_Color color_cnt = {255, 255, 0, SDL_ALPHA_OPAQUE};

void render_cnt(SDL_Renderer *ren) {
    static char fcount_s[10];
    static SDL_Surface *sur;
    static SDL_Texture *tex;
    static SDL_Rect rect;
    sprintf(fcount_s, "%09d", fcount);
    sur = TTF_RenderUTF8_Blended(font, fcount_s, color_cnt);
    rect.h = sur->h, rect.w = sur->w, rect.x = 100, rect.y = 0;
    tex = SDL_CreateTextureFromSurface(ren, sur);
    SDL_FreeSurface(sur);
    SDL_RenderCopy(ren, tex, NULL, &rect);
    SDL_DestroyTexture(tex);
}

void update_fps_texture(SDL_Renderer *ren) {
    static char fps_s[11];
    static SDL_Surface *sur;
    sprintf(fps_s, "%6.2f fps", fps);
    sur = TTF_RenderUTF8_Blended(font, fps_s, color_cnt);
    fps_rect.h = sur->h, fps_rect.w = sur->w, fps_rect.x = 400, fps_rect.y = 0;
    if(fps_tex) SDL_DestroyTexture(fps_tex);
    fps_tex = SDL_CreateTextureFromSurface(ren, sur);
    SDL_FreeSurface(sur);
}

int main(int argc, char *argv[]) {
    // Init
    if(SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        SDL_Log("Init Err %s", SDL_GetError());
        return 1;
    }
    if(TTF_Init() !=0) {
        SDL_Log("TTF Init Err %s", TTF_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "아인다인",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        640,
        480,
        0
    );
    if(!window) {
        SDL_Log("Create window Err %s", SDL_GetError());
        return -1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE
    );
    if(!renderer) {
        SDL_Log("Create renderer Err %s", SDL_GetError());
        return -1;
    }
    

    // Font
    font = TTF_OpenFont("C:\\Windows\\Fonts\\MALGUN.TTF", 30);
    if(!font) {
        SDL_Log("Open font Err %s", TTF_GetError());
        return -1;
    }
    SDL_Color color = {0, 255, 150, SDL_ALPHA_OPAQUE}, bgcolor = {0, 0, 0, SDL_ALPHA_TRANSPARENT};
    SDL_Surface *surface = TTF_RenderUTF8_Blended(font, "주인공", color);
    int w = surface->w, h = surface->h;
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    bool running = true;
    SDL_Event event;
    const Uint8 *key_state;
    SDL_Rect r = {320 - w/2, 240 - h/2, w, h};
    Uint32 tick = 0;

    while(running) {
        SDL_PollEvent(&event);
        if(event.type == SDL_QUIT) {
            running = false;
        }
        key_state = SDL_GetKeyboardState(NULL);
        if(key_state[SDL_SCANCODE_ESCAPE]) {
            running = false;
        }
        if(key_state[SDL_SCANCODE_RIGHT] && r.x < 640-w) {
            ++r.x;
        }
        if(key_state[SDL_SCANCODE_LEFT] && r.x > 0) {
            --r.x;
        }
        if(key_state[SDL_SCANCODE_DOWN] && r.y < 480-h) {
            ++r.y;
        }
        if(key_state[SDL_SCANCODE_UP] && r.y > 0) {
            --r.y;
        }

    	if(SDL_TICKS_PASSED(SDL_GetTicks(), tick + 1000)) {
            int delta = SDL_GetTicks() - tick;
            fps = (float)fcount_per_sec * 1000.0f / delta;
            update_fps_texture(renderer);
            tick += delta;
            fcount_per_sec = 0;
        }

        SDL_SetRenderDrawColor(renderer, 64, 64, 64, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(renderer);
        if(fps_tex) SDL_RenderCopy(renderer, fps_tex, NULL, &fps_rect);
        SDL_RenderCopy(renderer, texture, NULL, &r);
        render_cnt(renderer);

        SDL_RenderPresent(renderer);
        fcount++;
        fcount_per_sec++;
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyTexture(fps_tex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
