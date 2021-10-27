#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <cstdio>

// SDL_Delay 를 사용하지 않고 그냥 while 루프를 수행했으나
// fps 가 60으로 고정됨.
// Renderer 에서 SDL_RENDERER_PRESENTVSYNC 가 세팅되어 있으면
// fps 가 HW(모니터) 수직주파수(60Hz)와 동기화되므로...

const int WINDOW_WIDTH = 1024;
const int WINDOW_HEIGHT = 768;

static TTF_Font *font, *font_frame;
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
    sur = TTF_RenderUTF8_Blended(font_frame, fcount_s, color_cnt);
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
    sur = TTF_RenderUTF8_Blended(font_frame, fps_s, color_cnt);
    fps_rect.h = sur->h, fps_rect.w = sur->w, fps_rect.x = 400, fps_rect.y = 0;
    if(fps_tex) SDL_DestroyTexture(fps_tex);
    fps_tex = SDL_CreateTextureFromSurface(ren, sur);
    SDL_FreeSurface(sur);
}

int main(int argc, char *argv[]) {
    // Init
    if(SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        SDL_Log("SDL Init Err %s", SDL_GetError());
        return 1;
    }

    if(!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        SDL_Log("IMG Init Err %s", IMG_GetError());
        return 1;
    }
    if(TTF_Init() !=0) {
        SDL_Log("TTF Init Err %s", TTF_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "테스트컨트롤",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
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
    font_frame = TTF_OpenFont("C:\\Windows\\Fonts\\MALGUN.TTF", 30);
    if(!font) {
        SDL_Log("Open font Err %s", TTF_GetError());
        return -1;
    }
    SDL_Color color1 = {0, 255, 150, SDL_ALPHA_OPAQUE}, color2 = {255, 150, 255, SDL_ALPHA_OPAQUE};
    SDL_Surface *surface1 = TTF_RenderUTF8_Blended(font, "★", color1);
    int w1 = surface1->w, h1 = surface1->h;
    SDL_Texture *texture1 = SDL_CreateTextureFromSurface(renderer, surface1);
    SDL_FreeSurface(surface1);

    SDL_Surface *surface2 = TTF_RenderUTF8_Blended(font, "♥", color2);
    int w2 = surface2->w, h2 = surface2->h;
    SDL_Texture *texture2 = SDL_CreateTextureFromSurface(renderer, surface2);
    SDL_FreeSurface(surface2);


    bool running = true;
    SDL_Event event;
    const Uint8 *key_state;
    SDL_Rect r1 = {WINDOW_WIDTH - w1, WINDOW_HEIGHT/2 - h1/2, w1, h1};
    SDL_Rect r2 = {0, WINDOW_HEIGHT/2 - h2/2, w2, h2};
    int angle = 0;
    Uint32 tick = 0;

    while(running) {
        SDL_PollEvent(&event);
        if(event.type == SDL_QUIT) {
            running = false;
            break;
        }
        key_state = SDL_GetKeyboardState(NULL);
        if(key_state[SDL_SCANCODE_ESCAPE]) {
            running = false;
            break;
        }
        if(key_state[SDL_SCANCODE_RIGHT]) {
            r1.x += 5;
            if(r1.x > WINDOW_WIDTH - w1) r1.x = WINDOW_WIDTH - w1;
        }
        if(key_state[SDL_SCANCODE_LEFT]) {
            r1.x -= 5;
            if(r1.x < 0) r1.x = 0;
        }
        if(key_state[SDL_SCANCODE_DOWN]) {
            r1.y += 5;
            if(r1.y > WINDOW_HEIGHT - h1) r1.y = WINDOW_HEIGHT - h1;
        }
        if(key_state[SDL_SCANCODE_UP]) {
            r1.y -= 5;
            if(r1.y < 0) r1.y = 0;
        }

        if(key_state[SDL_SCANCODE_D]) {
            r2.x += 5;
            if(r2.x > WINDOW_WIDTH - w2) r2.x = WINDOW_WIDTH - w2;
        }
        if(key_state[SDL_SCANCODE_A]) {
            r2.x -= 5;
            if(r2.x < 0) r2.x = 0;
        }
        if(key_state[SDL_SCANCODE_S]) {
            r2.y += 5;
            if(r2.y > WINDOW_HEIGHT - h2) r2.y = WINDOW_HEIGHT - h2;
        }
        if(key_state[SDL_SCANCODE_W]) {
            r2.y -= 5;
            if(r2.y < 0) r2.y = 0;
        }
        angle += 6;
        if(angle >= 360) angle -= 360;

    	if(SDL_TICKS_PASSED(SDL_GetTicks(), tick + 1000)) {
            int delta = SDL_GetTicks() - tick;
            fps = (float)fcount_per_sec * 1000.0f / delta;
            update_fps_texture(renderer);
            tick += delta;
            fcount_per_sec = 0;
        }

        SDL_SetRenderDrawColor(renderer, 64, 64, 64, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(renderer);
        SDL_RenderCopyEx(renderer, texture1, NULL, &r1, (double)angle, NULL, SDL_FLIP_NONE);
        SDL_RenderCopyEx(renderer, texture2, NULL, &r2, (double)angle, NULL, SDL_FLIP_NONE);

        render_cnt(renderer);
        if(fps_tex) SDL_RenderCopy(renderer, fps_tex, NULL, &fps_rect);

        SDL_RenderPresent(renderer);
        fcount++;
        fcount_per_sec++;
    }

    SDL_DestroyTexture(texture1);
    SDL_DestroyTexture(texture2);
    SDL_DestroyTexture(fps_tex);
    TTF_CloseFont(font_frame);
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    return 0;
}
