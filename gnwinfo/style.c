// SPDX-License-Identifier: Unlicense

#include "gnwinfo.h"

struct nk_color g_color_warning = NK_COLOR_YELLOW;
struct nk_color g_color_error = NK_COLOR_RED;
struct nk_color g_color_good = NK_COLOR_GREEN;
struct nk_color g_color_unknown = NK_COLOR_BLUE;
struct nk_color g_color_text_l = NK_COLOR_WHITE;
struct nk_color g_color_text_d = NK_COLOR_LIGHT;
struct nk_color g_color_back = NK_COLOR_GRAY;

static struct nk_color
convert_color(struct nk_color color, float offset)
{
	float h, s, v;
	nk_color_hsv_f(&h, &s, &v, color);
	v += offset;
	return nk_hsv_f(h, s, v);
}

VOID
gnwinfo_set_style(struct nk_context* ctx)
{
	struct nk_color text_p10 = convert_color(g_color_text_d, -0.10f);
	struct nk_color text_p20 = convert_color(g_color_text_d, -0.20f);
	struct nk_color text_p30 = convert_color(g_color_text_d, -0.30f);
	struct nk_color back_p2 = convert_color(g_color_back, -0.02f);
	struct nk_color back_p3 = convert_color(g_color_back, -0.03f);
	struct nk_color back_p4 = convert_color(g_color_back, -0.04f);
	struct nk_color back_p8 = convert_color(g_color_back, -0.08f);
	struct nk_color back_n2 = convert_color(g_color_back, 0.02f);

	ctx->style.text.color = g_color_text_d;

	ctx->style.window.background = g_color_back;
	ctx->style.window.scaler = nk_style_item_color(g_color_text_d);
	ctx->style.window.fixed_background = nk_style_item_color(g_color_back);
	ctx->style.window.min_row_height_padding = 2;
	ctx->style.window.header.normal = nk_style_item_color(back_p2);
	ctx->style.window.header.hover = nk_style_item_color(back_p2);
	ctx->style.window.header.active = nk_style_item_color(back_p2);
	ctx->style.window.header.label_normal = g_color_text_d;
	ctx->style.window.header.label_hover = g_color_text_d;
	ctx->style.window.header.label_active = g_color_text_d;
	ctx->style.window.header.close_button.normal = nk_style_item_color(back_p2);
	ctx->style.window.header.close_button.hover = nk_style_item_color(back_p2);
	ctx->style.window.header.close_button.active = nk_style_item_color(back_p2);
	ctx->style.window.header.close_button.text_normal = g_color_text_d;
	ctx->style.window.header.close_button.text_hover = g_color_text_d;
	ctx->style.window.header.close_button.text_active = g_color_text_d;

	ctx->style.window.border_color = text_p10;
	ctx->style.window.popup_border_color = back_p8;
	ctx->style.window.combo_border_color = back_p8;
	ctx->style.window.contextual_border_color = back_p8;
	ctx->style.window.menu_border_color = back_p8;
	ctx->style.window.group_border_color = back_p8;
	ctx->style.window.tooltip_border_color = back_p8;

	ctx->style.button.text_normal = g_color_text_d;
	ctx->style.button.text_hover = g_color_text_d;
	ctx->style.button.text_active = g_color_text_d;
	//ctx->style.button.normal = nk_style_item_color(back_n2);
	ctx->style.button.normal = nk_style_item_color(g_color_back);
	ctx->style.button.hover = nk_style_item_color(back_p2);
	ctx->style.button.active = nk_style_item_color(back_p4);
	ctx->style.button.rounding = 0;
	ctx->style.button.border = 1.0f;
	ctx->style.button.padding = nk_vec2(0.0f, 0.0f);
	ctx->style.button.border_color = back_p8;

	ctx->style.checkbox.text_normal = g_color_text_d;
	ctx->style.checkbox.text_hover = g_color_text_d;
	ctx->style.checkbox.text_active = g_color_text_d;
	ctx->style.checkbox.cursor_normal = nk_style_item_color(g_color_back);
	ctx->style.checkbox.cursor_hover = nk_style_item_color(g_color_back);
	ctx->style.checkbox.normal = nk_style_item_color(text_p30);
	ctx->style.checkbox.hover = nk_style_item_color(text_p20);
	ctx->style.checkbox.active = nk_style_item_color(text_p20);

	ctx->style.slider.bar_normal = back_p3;
	ctx->style.slider.bar_hover = back_p3;
	ctx->style.slider.bar_active = back_p3;
	ctx->style.slider.bar_filled = text_p30;
	ctx->style.slider.cursor_normal = nk_style_item_color(text_p30);
	ctx->style.slider.cursor_hover = nk_style_item_color(text_p20);
	ctx->style.slider.cursor_active = nk_style_item_color(text_p10);
	ctx->style.slider.border_color = back_p8;

	ctx->style.progress.normal = nk_style_item_color(g_color_back);
	ctx->style.progress.hover = nk_style_item_color(g_color_back);
	ctx->style.progress.active = nk_style_item_color(g_color_back);
	ctx->style.progress.border_color = back_p8;
	ctx->style.progress.padding = nk_vec2(4.0f, 4.0f);
	ctx->style.progress.border = 1.0f;

	ctx->style.combo.normal = nk_style_item_color(g_color_back);
	ctx->style.combo.hover = nk_style_item_color(g_color_back);
	ctx->style.combo.active = nk_style_item_color(g_color_back);
	ctx->style.combo.border_color = back_p8;
	ctx->style.combo.label_normal = g_color_text_d;
	ctx->style.combo.label_hover = g_color_text_d;
	ctx->style.combo.label_active = g_color_text_d;
	ctx->style.combo.button.normal = nk_style_item_color(g_color_back);
	ctx->style.combo.button.hover = nk_style_item_color(g_color_back);
	ctx->style.combo.button.active = nk_style_item_color(g_color_back);
	ctx->style.combo.button.text_normal = g_color_text_d;
	ctx->style.combo.button.text_hover = g_color_text_d;
	ctx->style.combo.button.text_active = g_color_text_d;

	ctx->style.property.normal = nk_style_item_color(back_p3);
	ctx->style.property.hover = nk_style_item_color(back_p3);
	ctx->style.property.active = nk_style_item_color(back_p3);
	ctx->style.property.label_normal = g_color_text_d;
	ctx->style.property.label_hover = g_color_text_d;
	ctx->style.property.label_active = g_color_text_d;
	ctx->style.property.dec_button.normal = nk_style_item_color(back_p3);
	ctx->style.property.dec_button.hover = nk_style_item_color(back_p3);
	ctx->style.property.dec_button.active = nk_style_item_color(back_p3);
	ctx->style.property.dec_button.text_normal = g_color_text_d;
	ctx->style.property.dec_button.text_hover = g_color_text_d;
	ctx->style.property.dec_button.text_active = g_color_text_d;
	ctx->style.property.inc_button = ctx->style.property.dec_button;
	ctx->style.property.edit.normal = nk_style_item_color(back_p3);
	ctx->style.property.edit.hover = nk_style_item_color(back_p3);
	ctx->style.property.edit.active = nk_style_item_color(back_p3);
	ctx->style.property.edit.cursor_normal = g_color_text_d;
	ctx->style.property.edit.cursor_hover = g_color_text_d;
	ctx->style.property.edit.cursor_text_normal = back_p3;
	ctx->style.property.edit.cursor_text_hover = back_p3;
	ctx->style.property.edit.text_normal = g_color_text_d;
	ctx->style.property.edit.text_hover = g_color_text_d;
	ctx->style.property.edit.text_active = g_color_text_d;
	ctx->style.property.edit.selected_normal = g_color_text_d;
	ctx->style.property.edit.selected_hover = g_color_text_d;
	ctx->style.property.edit.selected_text_normal = back_p3;
	ctx->style.property.edit.selected_text_hover = back_p3;

	ctx->style.scrollh.normal = nk_style_item_color(back_p2);
	ctx->style.scrollh.hover = nk_style_item_color(back_p2);
	ctx->style.scrollh.active = nk_style_item_color(back_p2);
	ctx->style.scrollh.cursor_normal = nk_style_item_color(text_p30);
	ctx->style.scrollh.cursor_hover = nk_style_item_color(text_p20);
	ctx->style.scrollh.cursor_active = nk_style_item_color(text_p10);
	ctx->style.scrollh.border_color = back_p2;
	ctx->style.scrollh.cursor_border_color = back_p2;
	ctx->style.scrollv = ctx->style.scrollh;

	ctx->style.edit.normal = nk_style_item_color(back_p3);
	ctx->style.edit.hover = nk_style_item_color(back_p3);
	ctx->style.edit.active = nk_style_item_color(back_p3);
	ctx->style.edit.cursor_normal = g_color_text_d;
	ctx->style.edit.cursor_hover = g_color_text_d;
	ctx->style.edit.cursor_text_normal = back_p3;
	ctx->style.edit.cursor_text_hover = back_p3;
	ctx->style.edit.text_normal = g_color_text_d;
	ctx->style.edit.text_hover = g_color_text_d;
	ctx->style.edit.text_active = g_color_text_d;
	ctx->style.edit.selected_normal = g_color_text_d;
	ctx->style.edit.selected_hover = g_color_text_d;
	ctx->style.edit.selected_text_normal = back_p3;
	ctx->style.edit.selected_text_hover = back_p3;
	ctx->style.edit.border_color = back_p8;
	//ctx->style.edit.padding = nk_vec2(0.0f, 0.0f);

	ctx->style.tab.background = nk_style_item_color(back_p2);
	ctx->style.tab.text = g_color_text_d;
	ctx->style.tab.border_color = back_p8;
	ctx->style.tab.tab_minimize_button.normal = nk_style_item_color(back_p2);
	ctx->style.tab.tab_minimize_button.hover = nk_style_item_color(back_p2);
	ctx->style.tab.tab_minimize_button.active = nk_style_item_color(back_p2);
	ctx->style.tab.tab_minimize_button.text_background = back_p2;
	ctx->style.tab.tab_minimize_button.text_normal = g_color_text_d;
	ctx->style.tab.tab_minimize_button.text_hover = g_color_text_d;
	ctx->style.tab.tab_minimize_button.text_active = g_color_text_d;

	ctx->style.contextual_button.normal = nk_style_item_color(back_n2);
	ctx->style.contextual_button.hover = nk_style_item_color(back_p2);
	ctx->style.contextual_button.active = nk_style_item_color(back_p4);
	ctx->style.contextual_button.text_background = g_color_back;
	ctx->style.contextual_button.text_normal = g_color_text_d;
	ctx->style.contextual_button.text_hover = g_color_text_d;
	ctx->style.contextual_button.text_active = g_color_text_d;
	ctx->style.contextual_button.border_color = back_p8;
}
