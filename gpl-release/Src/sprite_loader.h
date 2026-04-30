/*
 * sprite_loader.h - Sprite Loading System for Conquer V5
 * Phase 4d/5b: Sprite Loader with PNG Support
 *
 * Provides modder-friendly sprite loading with hash table lookup,
 * animation support, and graceful fallbacks for missing assets.
 */

#ifndef SPRITE_LOADER_H
#define SPRITE_LOADER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

/* Sprite categories */
#define SPRITE_CAT_TERRAIN      0
#define SPRITE_CAT_VEGETATION   1
#define SPRITE_CAT_UNITS        2
#define SPRITE_CAT_NAVY         3
#define SPRITE_CAT_BUILDINGS    4
#define SPRITE_CAT_UI           5
#define SPRITE_CAT_COUNT        6

/* Maximum sprite name length */
#define SPRITE_NAME_MAX         64
#define SPRITE_PATH_MAX         256

/* Animation data */
typedef struct {
    int frames;         /* Number of frames in sprite strip */
    int speed;          /* Milliseconds per frame */
    int frame_width;    /* Width of single frame */
    int frame_height;   /* Height of single frame */
    int current_frame;  /* Current frame for animation */
    Uint32 last_update; /* Last frame update time */
} sprite_anim_t;

/* Sprite entry */
typedef struct sprite_entry {
    char name[SPRITE_NAME_MAX];
    int category;
    SDL_Texture *texture;
    int width;          /* Full texture width */
    int height;         /* Full texture height */
    sprite_anim_t anim;
    struct sprite_entry *next;  /* Hash table collision chain */
} sprite_entry_t;

/* Sprite manager */
typedef struct {
    SDL_Renderer *renderer;
    sprite_entry_t **hash_table;
    int hash_size;
    int sprite_count;
    char base_path[SPRITE_PATH_MAX];
} sprite_manager_t;

/* Function declarations */

/* Initialization */
sprite_manager_t *sprite_manager_create(SDL_Renderer *renderer, const char *base_path);
void sprite_manager_destroy(sprite_manager_t *mgr);

/* Loading */
int sprite_load(sprite_manager_t *mgr, int category, const char *name);
int sprite_load_all(sprite_manager_t *mgr);
void sprite_reload_all(sprite_manager_t *mgr);

/* Retrieval */
SDL_Texture *sprite_get_texture(sprite_manager_t *mgr, int category, const char *name);
sprite_entry_t *sprite_get_entry(sprite_manager_t *mgr, int category, const char *name);
int sprite_get_frame_rect(sprite_manager_t *mgr, int category, const char *name, 
                          int frame, SDL_Rect *rect);

/* Animation */
void sprite_update_animation(sprite_entry_t *entry);
int sprite_get_current_frame(sprite_manager_t *mgr, int category, const char *name);

/* Rendering */
void sprite_render(sprite_manager_t *mgr, int category, const char *name,
                   int x, int y, int w, int h);
void sprite_render_ex(sprite_manager_t *mgr, int category, const char *name,
                      int x, int y, int w, int h, double angle, SDL_RendererFlip flip);
void sprite_render_frame(sprite_manager_t *mgr, int category, const char *name,
                         int frame, int x, int y, int w, int h);

/* Fallback rendering (when sprite is missing) */
void sprite_render_fallback(SDL_Renderer *renderer, int x, int y, int w, int h,
                            int category, const char *name);

/* Utility */
const char *sprite_category_name(int category);
unsigned int sprite_hash(int category, const char *name);
SDL_Color sprite_fallback_color(int category, const char *name);

/* Category path helpers */
const char *sprite_category_path(int category);

#endif /* SPRITE_LOADER_H */
