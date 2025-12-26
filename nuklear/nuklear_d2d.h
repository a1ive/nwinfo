// SPDX-License-Identifier: Unlicense

#ifndef NK_D2D_H_
#define NK_D2D_H_

#include <windows.h>
#include <nuklear.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct nk_d2d_font
{
	struct nk_user_font nk;
	void* format;
} nk_d2d_font;

NK_API nk_d2d_font*
	nk_d2d_load_font(LPCWSTR name, int size);

NK_API void
	nk_d2d_font_del(nk_d2d_font* font);

NK_API struct nk_context*
	nk_d2d_init(HWND hwnd, unsigned int width, unsigned int height);

NK_API void
	nk_d2d_set_font(nk_d2d_font* font);

NK_API int
	nk_d2d_handle_event(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

NK_API void
	nk_d2d_render(enum nk_anti_aliasing AA, struct nk_color clear);

NK_API void
	nk_d2d_shutdown(void);

NK_API struct nk_image
	nk_d2d_load_image_from_file(const WCHAR* filename);

NK_API struct nk_image
	nk_d2d_load_image_from_memory(const void* membuf, nk_uint membufSize);

NK_API void
	nk_d2d_image_free(struct nk_image image);

#ifdef __cplusplus
}
#endif

#endif /* NK_D2D_H_ */
