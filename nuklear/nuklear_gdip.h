/*
 * Nuklear - 1.40.8 - public domain
 * no warrenty implied; use at your own risk.
 * authored from 2015-2017 by Micha Mettke
 */
/*
 * ==============================================================
 *
 *                              API
 *
 * ===============================================================
 */
#ifndef NK_GDIP_H_
#define NK_GDIP_H_

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlwapi.h>

/* font */
typedef struct GdipFont GdipFont;
NK_API void nk_gdipfont_del(GdipFont *font);
NK_API GdipFont* nk_gdip_load_font(LPCWSTR name, int size);

NK_API struct nk_context* nk_gdip_init(HWND hwnd, unsigned int width, unsigned int height);
NK_API void nk_gdip_set_font(GdipFont *font);
NK_API int nk_gdip_handle_event(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
NK_API void nk_gdip_render(enum nk_anti_aliasing AA, struct nk_color clear);
NK_API void nk_gdip_shutdown(void);

/* image */
NK_API struct nk_image nk_gdip_load_image_from_memory(const void* membuf, nk_uint membufSize);
NK_API void nk_gdip_image_free(struct nk_image image);

#endif
