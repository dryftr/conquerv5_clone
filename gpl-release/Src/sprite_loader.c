/*
 * sprite_loader.c - Sprite Loading System Implementation
 * Phase 4d/5b: Sprite Loader with PNG Support
 */

#include "sprite_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* json-c is optional - only needed for sprites.json manifest */
#ifdef HAS_JSON_C
#include <json-c/json.h>
#endif

/* Hash table size (prime number for better distribution) */
#define HASH_TABLE_SIZE 256

/* Category paths */
static const char *category_paths[SPRITE_CAT_COUNT] = {
    "terrain/elevation",
    "terrain/vegetation",
    "units",
    "navy",
    "buildings",
    "ui"
};

static const char *category_names[SPRITE_CAT_COUNT] = {
    "terrain",
    "vegetation",
    "units",
    "navy",
    "buildings",
    "ui"
};

/* Simple hash function */
unsigned int sprite_hash(int category, const char *name) {
    unsigned int hash = category * 31;
    while (*name) {
        hash = (hash * 31) + *name++;
    }
    return hash % HASH_TABLE_SIZE;
}

const char *sprite_category_name(int category) {
    if (category >= 0 && category < SPRITE_CAT_COUNT) {
        return category_names[category];
    }
    return "unknown";
}

const char *sprite_category_path(int category) {
    if (category >= 0 && category < SPRITE_CAT_COUNT) {
        return category_paths[category];
    }
    return "";
}

/* Generate a deterministic color from name */
SDL_Color sprite_fallback_color(int category, const char *name) {
    SDL_Color color;
    unsigned int hash = 0;
    
    /* Hash the name */
    const char *p = name;
    while (*p) {
        hash = (hash * 31) + *p++;
    }
    hash += category * 7919;  /* Mix in category */
    
    /* Generate color - keep it somewhat muted for visibility */
    color.r = ((hash >> 16) & 0xFF) | 0x40;
    color.g = ((hash >> 8) & 0xFF) | 0x40;
    color.b = (hash & 0xFF) | 0x40;
    color.a = 255;
    
    return color;
}

sprite_manager_t *sprite_manager_create(SDL_Renderer *renderer, const char *base_path) {
    sprite_manager_t *mgr = calloc(1, sizeof(sprite_manager_t));
    if (!mgr) {
        fprintf(stderr, "sprite_manager_create: Failed to allocate manager\n");
        return NULL;
    }
    
    mgr->renderer = renderer;
    mgr->hash_size = HASH_TABLE_SIZE;
    mgr->sprite_count = 0;
    
    if (base_path) {
        strncpy(mgr->base_path, base_path, SPRITE_PATH_MAX - 1);
        mgr->base_path[SPRITE_PATH_MAX - 1] = '\0';
    } else {
        strcpy(mgr->base_path, "./sprites");
    }
    
    mgr->hash_table = calloc(HASH_TABLE_SIZE, sizeof(sprite_entry_t *));
    if (!mgr->hash_table) {
        fprintf(stderr, "sprite_manager_create: Failed to allocate hash table\n");
        free(mgr);
        return NULL;
    }
    
    return mgr;
}

void sprite_manager_destroy(sprite_manager_t *mgr) {
    if (!mgr) return;
    
    /* Free all sprite entries and textures */
    for (int i = 0; i < mgr->hash_size; i++) {
        sprite_entry_t *entry = mgr->hash_table[i];
        while (entry) {
            sprite_entry_t *next = entry->next;
            if (entry->texture) {
                SDL_DestroyTexture(entry->texture);
            }
            free(entry);
            entry = next;
        }
    }
    
    free(mgr->hash_table);
    free(mgr);
}

/* Parse sprites.json for animation configs - json-c optional */
static int parse_sprite_config(sprite_manager_t *mgr, int category, const char *name, sprite_anim_t *anim) {
    /* Default values */
    anim->frames = 1;
    anim->speed = 0;
    anim->frame_width = 0;
    anim->frame_height = 0;
    anim->current_frame = 0;
    anim->last_update = 0;

#ifdef HAS_JSON_C
    char path[SPRITE_PATH_MAX];
    snprintf(path, sizeof(path), "%s/%s.json", mgr->base_path, "sprites");
    
    FILE *fp = fopen(path, "r");
    if (!fp) {
        /* No config file - sprite is static */
        return 0;
    }
    
    /* Read file into buffer */
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    char *buffer = malloc(size + 1);
    if (!buffer) {
        fclose(fp);
        return -1;
    }
    
    fread(buffer, 1, size, fp);
    buffer[size] = '\0';
    fclose(fp);
    
    /* Parse JSON */
    struct json_object *root = json_tokener_parse(buffer);
    free(buffer);
    
    if (!root) {
        return -1;
    }
    
    /* Navigate to category */
    struct json_object *cat_obj = NULL;
    json_object_object_get_ex(root, category_names[category], &cat_obj);
    
    if (cat_obj) {
        /* Get sprite config */
        struct json_object *sprite_obj = NULL;
        json_object_object_get_ex(cat_obj, name, &sprite_obj);
        
        if (sprite_obj) {
            struct json_object *val;
            if (json_object_object_get_ex(sprite_obj, "frames", &val)) {
                anim->frames = json_object_get_int(val);
            }
            if (json_object_object_get_ex(sprite_obj, "speed", &val)) {
                anim->speed = json_object_get_int(val);
            }
            if (json_object_object_get_ex(sprite_obj, "size", &val)) {
                if (json_object_is_type(val, json_type_array) &&
                    json_object_array_length(val) >= 2) {
                    anim->frame_width = json_object_get_int(json_object_array_get_idx(val, 0));
                    anim->frame_height = json_object_get_int(json_object_array_get_idx(val, 1));
                }
            }
        }
    }
    
    json_object_put(root);
#else
    (void)mgr; (void)category; (void)name;  /* Unused without json-c */
#endif
    return 0;
}

int sprite_load(sprite_manager_t *mgr, int category, const char *name) {
    if (!mgr || category < 0 || category >= SPRITE_CAT_COUNT) return -1;
    
    /* Check if already loaded */
    if (sprite_get_entry(mgr, category, name)) {
        return 0;  /* Already loaded */
    }
    
    /* Build path */
    char path[SPRITE_PATH_MAX];
    snprintf(path, sizeof(path), "%s/%s/%s.png",
             mgr->base_path, category_paths[category], name);
    
    /* Load image */
    SDL_Surface *surface = IMG_Load(path);
    if (!surface) {
        /* Sprite not found - will use fallback later */
        return -1;
    }
    
    /* Create texture */
    SDL_Texture *texture = SDL_CreateTextureFromSurface(mgr->renderer, surface);
    SDL_FreeSurface(surface);
    
    if (!texture) {
        fprintf(stderr, "sprite_load: Failed to create texture for %s/%s\n",
                category_paths[category], name);
        return -1;
    }
    
    /* Create entry */
    sprite_entry_t *entry = calloc(1, sizeof(sprite_entry_t));
    if (!entry) {
        SDL_DestroyTexture(texture);
        return -1;
    }
    
    strncpy(entry->name, name, SPRITE_NAME_MAX - 1);
    entry->name[SPRITE_NAME_MAX - 1] = '\0';
    entry->category = category;
    entry->texture = texture;
    
    /* Get dimensions */
    SDL_QueryTexture(texture, NULL, NULL, &entry->width, &entry->height);
    
    /* Parse animation config */
    parse_sprite_config(mgr, category, name, &entry->anim);
    
    /* If config didn't specify frame size, assume single frame */
    if (entry->anim.frame_width == 0) {
        entry->anim.frame_width = entry->width;
        entry->anim.frame_height = entry->height;
    }
    
    /* Insert into hash table */
    unsigned int hash = sprite_hash(category, name);
    entry->next = mgr->hash_table[hash];
    mgr->hash_table[hash] = entry;
    mgr->sprite_count++;
    
    return 0;
}

/* Placeholder sprites to auto-load */
static const char *placeholder_terrain[] = {
    "water", "valley", "clear", "hill", "mountain", "peak"
};
static const char *placeholder_vegetation[] = {
    "volcano", "desert", "tundra", "barren", "lt_veg", "good",
    "wood", "forest", "jungle", "swamp", "ice", "none"
};
static const char *placeholder_buildings[] = {
    "farm", "city", "capital", "mine", "town"
};
static const char *placeholder_ui[] = {
    "cursor", "selection"
};

int sprite_load_all(sprite_manager_t *mgr) {
    if (!mgr) return -1;
    
    /* Load terrain elevation sprites */
    for (size_t i = 0; i < sizeof(placeholder_terrain) / sizeof(char *); i++) {
        sprite_load(mgr, SPRITE_CAT_TERRAIN, placeholder_terrain[i]);
    }
    
    /* Load vegetation sprites */
    for (size_t i = 0; i < sizeof(placeholder_vegetation) / sizeof(char *); i++) {
        sprite_load(mgr, SPRITE_CAT_VEGETATION, placeholder_vegetation[i]);
    }
    
    /* Load building sprites */
    for (size_t i = 0; i < sizeof(placeholder_buildings) / sizeof(char *); i++) {
        sprite_load(mgr, SPRITE_CAT_BUILDINGS, placeholder_buildings[i]);
    }
    
    /* Load UI sprites */
    for (size_t i = 0; i < sizeof(placeholder_ui) / sizeof(char *); i++) {
        sprite_load(mgr, SPRITE_CAT_UI, placeholder_ui[i]);
    }
    
    return mgr->sprite_count;
}

void sprite_reload_all(sprite_manager_t *mgr) {
    if (!mgr) return;
    
    /* Destroy all existing sprites */
    for (int i = 0; i < mgr->hash_size; i++) {
        sprite_entry_t *entry = mgr->hash_table[i];
        while (entry) {
            sprite_entry_t *next = entry->next;
            if (entry->texture) {
                SDL_DestroyTexture(entry->texture);
            }
            free(entry);
            entry = next;
        }
        mgr->hash_table[i] = NULL;
    }
    
    mgr->sprite_count = 0;
    
    /* Reload all */
    sprite_load_all(mgr);
}

sprite_entry_t *sprite_get_entry(sprite_manager_t *mgr, int category, const char *name) {
    if (!mgr || !name) return NULL;
    
    unsigned int hash = sprite_hash(category, name);
    sprite_entry_t *entry = mgr->hash_table[hash];
    
    while (entry) {
        if (entry->category == category && strcmp(entry->name, name) == 0) {
            return entry;
        }
        entry = entry->next;
    }
    
    return NULL;
}

SDL_Texture *sprite_get_texture(sprite_manager_t *mgr, int category, const char *name) {
    sprite_entry_t *entry = sprite_get_entry(mgr, category, name);
    return entry ? entry->texture : NULL;
}

void sprite_update_animation(sprite_entry_t *entry) {
    if (!entry || entry->anim.frames <= 1) return;
    
    Uint32 now = SDL_GetTicks();
    if (now - entry->anim.last_update >= (Uint32)entry->anim.speed) {
        entry->anim.current_frame = (entry->anim.current_frame + 1) % entry->anim.frames;
        entry->anim.last_update = now;
    }
}

int sprite_get_current_frame(sprite_manager_t *mgr, int category, const char *name) {
    sprite_entry_t *entry = sprite_get_entry(mgr, category, name);
    if (!entry) return 0;
    
    sprite_update_animation(entry);
    return entry->anim.current_frame;
}

int sprite_get_frame_rect(sprite_manager_t *mgr, int category, const char *name,
                          int frame, SDL_Rect *rect) {
    sprite_entry_t *entry = sprite_get_entry(mgr, category, name);
    if (!entry || !rect) return -1;
    
    rect->x = frame * entry->anim.frame_width;
    rect->y = 0;
    rect->w = entry->anim.frame_width;
    rect->h = entry->anim.frame_height;
    
    return 0;
}

void sprite_render(sprite_manager_t *mgr, int category, const char *name,
                   int x, int y, int w, int h) {
    sprite_render_ex(mgr, category, name, x, y, w, h, 0.0, SDL_FLIP_NONE);
}

void sprite_render_ex(sprite_manager_t *mgr, int category, const char *name,
                      int x, int y, int w, int h, double angle, SDL_RendererFlip flip) {
    if (!mgr) return;
    
    sprite_entry_t *entry = sprite_get_entry(mgr, category, name);
    
    if (!entry) {
        /* Render fallback */
        sprite_render_fallback(mgr->renderer, x, y, w, h, category, name);
        return;
    }
    
    /* Update animation */
    sprite_update_animation(entry);
    
    /* Calculate source rect for current frame */
    SDL_Rect src_rect;
    src_rect.x = entry->anim.current_frame * entry->anim.frame_width;
    src_rect.y = 0;
    src_rect.w = entry->anim.frame_width;
    src_rect.h = entry->anim.frame_height;
    
    /* Destination rect */
    SDL_Rect dst_rect = {x - w/2, y - h/2, w, h};
    
    /* Render */
    SDL_RenderCopyEx(mgr->renderer, entry->texture, &src_rect, &dst_rect,
                     angle, NULL, flip);
}

void sprite_render_frame(sprite_manager_t *mgr, int category, const char *name,
                         int frame, int x, int y, int w, int h) {
    if (!mgr) return;
    
    sprite_entry_t *entry = sprite_get_entry(mgr, category, name);
    
    if (!entry) {
        /* Render fallback */
        sprite_render_fallback(mgr->renderer, x, y, w, h, category, name);
        return;
    }
    
    /* Clamp frame */
    if (frame < 0) frame = 0;
    if (frame >= entry->anim.frames) frame = entry->anim.frames - 1;
    
    /* Calculate source rect for specific frame */
    SDL_Rect src_rect;
    src_rect.x = frame * entry->anim.frame_width;
    src_rect.y = 0;
    src_rect.w = entry->anim.frame_width;
    src_rect.h = entry->anim.frame_height;
    
    /* Destination rect */
    SDL_Rect dst_rect = {x - w/2, y - h/2, w, h};
    
    /* Render */
    SDL_RenderCopy(mgr->renderer, entry->texture, &src_rect, &dst_rect);
}

void sprite_render_fallback(SDL_Renderer *renderer, int x, int y, int w, int h,
                            int category, const char *name) {
    if (!renderer) return;
    
    /* Get fallback color */
    SDL_Color color = sprite_fallback_color(category, name);
    
    /* Fill rect */
    SDL_Rect rect = {x - w/2, y - h/2, w, h};
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &rect);
    
    /* White border for visibility */
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &rect);
    
    /* Optional: draw first letter of sprite name */
    /* (Would need SDL_ttf for text, skipping for now) */
}
