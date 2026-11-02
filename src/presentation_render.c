#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *const music_labels[16] = {
    "FEAR FACTORY", "GRAVITY KILLS", "KMFDM", "KMFDM", "PITCHSHIFTER", "PITCHSHIFTER", "PITCHSHIFTER", "JUNKIE XL", "FEAR FACTORY", "FEAR FACTORY", "GRAVITY KILLS", "KMFDM", "KMFDM", "PITCHSHIFTER", "PITCHSHIFTER", "JUNKIE XL",
};

#include "presentation_render.h"
#include "game.h"
#include "psx_gpu.h"

typedef char PRESENTATION_PACKET_SIZE_CHECK[sizeof(POLY_GT4) == 52 ? 1 : -1];
typedef char PRESENTATION_COLOR_SIZE_CHECK[sizeof(RGBC) == 4 && sizeof(PRESENTATION_COLOR) == 4 ? 1 : -1];

PRESENTATION_PACKET_STORAGE presentation_packets;
typedef char PRESENTATION_UI_PACKET_SIZE_CHECK[sizeof(POLY_F4) == 24 && sizeof(POLY_G4) == 36 && sizeof(PRESENTATION_PACKET_STORAGE) == 52528 ? 1 : -1];

PRESENTATION_MODEL_IMAGE presentation_model_image;

PRESENTATION_ITEM presentation_items[70];
static const VEC3_16 menu_light_dir = {.v = {-2508, 2048, -2508}};
static const PSX_RECT menu_vram_rect = {512, 0, 512, 256};

PRESENTATION_MENU_STATE presentation_menu;
PRESENTATION_BACKGROUND_STATE presentation_background;
static uint32 loading_image_cycle = 12;
static const char loading_format[20] = "PRES\\LOADNG%01X.TIM";

const char presentation_background_format[16] = "PRES\\BACK%d.TIM";

char presentation_player_name[10] = {0, 'H', 'S', 'C', 'O', 'R', 'E', '1', 0, 0};
PRESENTATION_NAME_EDITOR presentation_name_editor;

static const MODEL_MATRIX base_matrix = {.words = {0x1000, 0, 0x1000, 0, 0x1000, 0, 0, 0}};

static const SVECTOR presentation_n[4] = {{-2048, -2048, 4096, 0}, {2048, -2048, 4096, 0}, {-2048, 2048, 4096, 0}, {2048, 2048, 4096, 0}};

static const VEC3_16 lights[3] = {{.v = {-2508, 2048, -2508}}, {.v = {-2508, 2048, 2508}}, {.v = {2508, 2048, -2508}}};

const PRESENTATION_RENDER_STATE presentation_render_defaults = {.pals = {{{.word = 0x00808080}, {.word = 0x00808080}, {.word = 0x00808080}, {.word = 0x00808080}}, {{.word = 0x00808080}, {.word = 0x00808080}, {.word = 0x00808080}, {.word = 0x00808080}}, {{.word = 0x00008080}, {.word = 0x00008080}, {.word = 0x0040A0A0}, {.word = 0x0040A0A0}}, {{.word = 0x00000080}, {.word = 0x00000080}, {.word = 0x00404080}, {.word = 0x00404080}}, {{.word = 0x00000080}, {.word = 0x00008000}, {.word = 0x00800080}, {.word = 0x00808080}}, {{.word = 0x00208020}, {.word = 0x00208020}, {.word = 0x00004000}, {.word = 0x00004000}}, {{.word = 0x00008080}, {.word = 0x00008080}, {.word = 0x0040A0A0}, {.word = 0x0040A0A0}}, {{.word = 0x00202020}, {.word = 0x00202020}, {.word = 0x00202020}, {.word = 0x00202020}}},
                                                                .angles = {.v = {2048, 0, 0}},
                                                                .fonts = {{.tex_name = "Eye12", .cell_width = 12, .cell_height = 12, .columns = 21, .spacing = 0}, {.tex_name = "SmFont", .cell_width = 8, .cell_height = 8, .columns = 21, .spacing = 0}, {.tex_name = "Impact", .cell_width = 20, .cell_height = 20, .columns = 12, .spacing = 0}, {.tex_name = "Title", .cell_width = 20, .cell_height = 20, .columns = 12, .spacing = 0}, {.tex_name = "Italic", .cell_width = 20, .cell_height = 20, .columns = 12, .spacing = 0}}};

PRESENTATION_RENDER_STATE presentation_render;

static void run_menu(void);
static void load_saved_settings(void);
static void edit_scr_pos(void);

static uint32 text_half_width(const PRESENTATION_FONT *font, const char *text)
{
    uint32 len;
    uint32 idx;
    uint32 character;
    uint32 glyph;
    uint32 line;
    uint32 maximum;

    len = (uint32)strlen(text);
    line = 0;
    maximum = 0;
    if ((sint32)len > 0)
    {
        for (idx = 0; idx < len; ++idx)
        {
            character = (uint8)text[idx];
            if (character == 10)
            {
                if ((sint32)maximum < (sint32)line)
                    maximum = line;
                line = 0;
            }
            else if (character == 32)
                line += font->widths[0];
            else
            {
                glyph = (character - 32) & 255;
                line += 1 + (glyph < 128 ? font->widths[glyph] : font[1].widths[glyph - 140]);
            }
        }
    }
    if ((sint32)maximum < (sint32)line)
        maximum = line;
    return ((sint32)maximum / 2);
}

uint32 presentation_draw_text(uint32 mode, uint32 x, sint32 y, const char *text)
{
    DISPLAY_ORDERING head;
    const PRESENTATION_FONT *font;
    uint32 initial_x;
    uint32 cursor_x;
    uint32 cursor_y;
    uint32 len;
    uint32 idx;
    uint32 character;
    uint32 glyph;
    uint32 glyph_width;
    uint32 counter;
    POLY_GT4 *poly;
    uint32 vertex;
    uint32 component;
    uint32 columns;
    uint32 quotient;
    uint32 cell;
    uint32 product;
    const uint8 *tex;
    uint32 val;
    uint32 adjust;

    uint32 tag;
    uint32 head_tag;
    uint32 count;

    DISPLAY_FRAME *cur_frame;

    FUNCTION_MARKER(0x800B2790, "SLUS_006.10");
    font = presentation_render.fonts + (mode & 15);
    if ((mode & 0x80) != 0)
        x -= text_half_width(font, text);
    initial_x = x;
    cursor_x = x;
    cursor_y = y;
    len = (uint32)strlen(text);
    if ((sint32)len <= 0)
        return len;
    for (idx = 0; idx < len; ++idx)
    {
        character = (uint8)text[idx];
        if (character == 10)
        {
            val = font->cell_height;
            cursor_x = initial_x;
            cursor_y += val;
            continue;
        }
        if (character == 13)
        {
            val = text_half_width(font, text + idx + 1);
            cursor_x = presentation_render.tab_stop - (val << 1);
            continue;
        }
        if (character == 125)
        {
            val = text_half_width(font, text + idx + 1);
            cursor_x = initial_x - (val << 1);
            continue;
        }
        if (character == 9)
        {
            cursor_x = presentation_render.tab_stop;
            continue;
        }
        glyph = (character - 32) & 255;
        if (character == 32)
        {
            cursor_x += (glyph < 128 ? font->widths[glyph] : font[1].widths[glyph - 140]);
            continue;
        }
        counter = presentation_render.packet_idx;
        glyph_width = (glyph < 128 ? font->widths[glyph] : font[1].widths[glyph - 140]);
        XASSERT(!(counter >= sizeof(presentation_packets.text) / sizeof(presentation_packets.text[0])));
        poly = presentation_packets.text + counter;
        poly->p[0].xy.x = (sint16)(cursor_x);
        poly->p[0].xy.y = (sint16)(cursor_y);
        adjust = font->spacing;
        poly->p[1].xy.y = (sint16)(cursor_y);
        poly->p[2].xy.x = (sint16)(cursor_x);
        poly->p[1].xy.x = (sint16)(cursor_x + adjust + glyph_width);
        val = font->cell_height;
        poly->p[2].xy.y = (sint16)(cursor_y + val);
        adjust = font->spacing;
        poly->p[3].xy.x = (sint16)(cursor_x + adjust + glyph_width);
        val = font->cell_height;
        poly->p[3].xy.y = (sint16)(cursor_y + val);
        for (vertex = 0; vertex < 4; ++vertex)
        {
            columns = font->columns;
            quotient = (uint32)fixed_divide_checked((sint32)glyph, (sint32)columns);
            cell = glyph - quotient * columns;
            val = font->cell_width;
            product = val * cell;
            tex = font->tex;
            val = tex[14];
            if ((vertex & 1) != 0)
            {
                adjust = font->spacing;
                val += product + adjust + glyph_width;
            }
            else
                val += product;
            poly->p[vertex].uv.u = (uint8)(val);
            columns = font->columns;
            quotient = (uint32)fixed_divide_checked((sint32)glyph, (sint32)columns);
            cell = font->cell_height;
            product = cell * quotient;
            tex = font->tex;
            val = tex[15];
            val += product;
            if ((vertex & 2) != 0)
                val += cell;
            poly->p[vertex].uv.v = (uint8)(val);
        }
        for (vertex = 0; vertex < 4; ++vertex)
        {
            for (component = 0; component < 3; ++component)
            {
                val = presentation_render.colors[vertex].channels[component];
                ((uint8 *)&poly->p[vertex].rgbc)[component] = (uint8)(val);
            }
        }
        tex = font->tex;
        val = ((const TEXTURE_DESCRIPTOR *)tex)->page;
        poly->p[1].attr = (uint16)(val);
        tex = font->tex;
        cur_frame = disp_state.cur;
        val = ((const TEXTURE_DESCRIPTOR *)tex)->clut;
        poly->p[0].attr = (uint16)(val);
        hd_ui_gouraud(poly->p, 0);
        head.tags = cur_frame->ordering;
        head.byte_offset = 0;
        tag = poly->tag;
        head_tag = head.tags[(sint32)head.byte_offset / 4];
        poly->tag = (tag & 0xFF000000) | (head_tag & 0xFFFFFF);
        head.tags = cur_frame->ordering;
        head.byte_offset = 0;
        counter = presentation_render.packet_idx;
        count = presentation_render.packet_count;
        cursor_x += 1 + glyph_width;
        head_tag = head.tags[(sint32)head.byte_offset / 4];
        presentation_render.packet_idx = counter + 1;
        presentation_render.packet_count = count + 1;
        head.tags[(sint32)head.byte_offset / 4] = (head_tag & 0xFF000000) | gpu_packet_link((uintptr_t)poly);
    }
    return 0;
}

uint32 presentation_draw_image(uint32 mode, uint32 x, sint32 y, const char *name)
{
    DISPLAY_ORDERING head;
    POLY_GT4 *poly;
    uint32 counter, vertex, component, val, other;
    const uint8 *tex;
    uint32 header, link, count, result;

    DISPLAY_FRAME *cur_frame;

    FUNCTION_MARKER(0x800B2F8C, "SLUS_006.10");
    tex = tex_find(name);
    counter = presentation_render.packet_idx;
    XASSERT(!(counter >= sizeof(presentation_packets.text) / sizeof(presentation_packets.text[0])));
    poly = presentation_packets.text + counter;
    for (vertex = 0; vertex < 4; ++vertex)
    {
        for (component = 0; component < 3; ++component)
        {
            val = presentation_render.colors[vertex].channels[component];
            if (vertex == 3 && component == 2)
            {
                setlen(poly, 12);
                setcode(poly, 0x3E);
            }
            ((uint8 *)&poly->p[vertex].rgbc)[component] = (uint8)(val);
        }
    }
    val = ((const TEXTURE_DESCRIPTOR *)tex)->page;
    poly->p[1].attr = (uint16)(val);
    val = ((const TEXTURE_DESCRIPTOR *)tex)->clut;
    poly->p[0].attr = (uint16)(val);
    if ((mode & 0x80) == 0)
    {
        poly->p[0].xy.x = (sint16)(x);
        poly->p[0].xy.y = (sint16)((uint32)y);
        val = tex[12];
        poly->p[1].xy.y = (sint16)((uint32)y);
        poly->p[2].xy.x = (sint16)(x);
        poly->p[1].xy.x = (sint16)(x + val + 0xFFFF);
        val = tex[13];
        poly->p[2].xy.y = (sint16)((uint32)y + val + 0xFFFF);
        val = tex[12];
        poly->p[3].xy.x = (sint16)(x + val + 0xFFFF);
        val = tex[13];
        poly->p[3].xy.y = (sint16)((uint32)y + val + 0xFFFF);
    }
    else
    {
        val = tex[12];
        poly->p[0].xy.x = (sint16)(x - (val >> 1));
        val = tex[13];
        poly->p[0].xy.y = (sint16)((uint32)y - (val >> 1));
        val = tex[12];
        poly->p[1].xy.x = (sint16)(x + (val >> 1) - 1);
        val = tex[13];
        poly->p[1].xy.y = (sint16)((uint32)y - (val >> 1));
        val = tex[12];
        poly->p[2].xy.x = (sint16)(x - (val >> 1));
        val = tex[13];
        poly->p[2].xy.y = (sint16)((uint32)y + (val >> 1) - 1);
        val = tex[12];
        poly->p[3].xy.x = (sint16)(x + (val >> 1) - 1);
        val = tex[13];
        poly->p[3].xy.y = (sint16)((uint32)y + (val >> 1) - 1);
    }
    val = tex[14];
    poly->p[0].uv.u = (uint8)(val);
    val = tex[15];
    poly->p[0].uv.v = (uint8)(val);
    val = tex[14];
    other = tex[12];
    poly->p[1].uv.u = (uint8)(other + val + 255);
    val = tex[15];
    poly->p[1].uv.v = (uint8)(val);
    val = tex[14];
    poly->p[2].uv.u = (uint8)(val);
    val = tex[15];
    other = tex[13];
    poly->p[2].uv.v = (uint8)(other + val + 255);
    val = tex[14];
    other = tex[12];
    poly->p[3].uv.u = (uint8)(other + val + 255);
    val = tex[15];
    other = tex[13];
    poly->p[3].uv.v = (uint8)(other + val + 255);
    cur_frame = disp_state.cur;
    hd_ui_gouraud(poly->p, 0);
    head.tags = cur_frame->ordering;
    head.byte_offset = 0;
    header = poly->tag;
    link = head.tags[(sint32)head.byte_offset / 4];
    poly->tag = (header & 0xFF000000) | (link & 0xFFFFFF);
    head.tags = cur_frame->ordering;
    head.byte_offset = 0;
    counter = presentation_render.packet_idx;
    count = presentation_render.packet_count;
    link = head.tags[(sint32)head.byte_offset / 4];
    result = counter + 1;
    presentation_render.packet_idx = result;
    presentation_render.packet_count = count + 1;
    head.tags[(sint32)head.byte_offset / 4] = (link & 0xFF000000) | gpu_packet_link((uintptr_t)poly);
    return result;
}

uint32 presentation_select_pal(uint32 pal)
{
    const PRESENTATION_COLOR *src;
    uint32 idx;
    uint32 words[3];
    uint32 val;

    FUNCTION_MARKER(0x800B32E4, "SLUS_006.10");
    src = presentation_render.pals[pal];
    /* Aligned LWL/LWR and SWL/SWR pairs each access the whole word twice */
    for (idx = 0; idx < 3; ++idx)
    {
        (void)src[idx].word;
        words[idx] = src[idx].word;
    }
    for (idx = 0; idx < 3; ++idx)
    {
        presentation_render.colors[idx].word = words[idx];
        presentation_render.colors[idx].word = words[idx];
    }
    (void)src[3].word;
    val = src[3].word;
    presentation_render.colors[3].word = val;
    presentation_render.colors[3].word = val;
    return val;
}

static uint32 measure_glyph(uint32 family, uint32 glyph, const uint8 *pixels, uint32 byte_count)
{
    const PRESENTATION_FONT *font = presentation_render.fonts + family;
    uint32 divisor, column, row, width, row_stride, rows, height;
    const uint8 *tex;
    uint32 glyph_offset, pixel_offset, column_stride, scan, ink, idx, count, val;
    sint32 remaining, scan_height;

    FUNCTION_MARKER(0x800B22A4, "SLUS_006.10");
    divisor = font->columns;
    column = (uint32)fixed_divide_checked((sint32)glyph, (sint32)divisor);
    row = glyph % divisor;
    tex = font->tex;
    width = tex[12];
    rows = font->cell_height;
    row_stride = ((width + 3) & 0x1FC) >> 1;
    height = font->cell_width;
    glyph_offset = row * (height >> 1) + column * (rows * row_stride);
    remaining = (sint32)height - 1;
    scan_height = remaining;
    if (remaining <= 0)
        return remaining;
    column_stride = row_stride;
    do
    {
        ink = 0;
        idx = 0;
        scan = 0;
        if (rows != 0)
        {
            count = font->cell_height;
            do
            {
                pixel_offset = glyph_offset + (uint32)(scan_height / 2) + scan;
                val = pixel_offset < byte_count ? pixels[pixel_offset] : 0;
                val = (scan_height & 1) != 0 ? val & 15 : val >> 4;
                if (val != 0)
                    ++ink;
                ++idx;
                scan += column_stride;
            } while ((sint32)idx < (sint32)count);
        }
        if (ink == 0)
            --remaining;
        --scan_height;
    } while (scan_height > 0 && ink == 0);
    return remaining;
}

static void init_font_metrics(void)
{
    uint32 family, len, width, height, byte_count;
    uint8 *pixels;
    const uint8 *tex_rec;
    const char *filename;
    const uint8 *tex;
    PRESENTATION_FONT *font;
    uint32 page, val, glyph;
    PSX_RECT rect;

    FUNCTION_MARKER(0x800B2438, "SLUS_006.10");
    for (family = 0; family < 5; ++family)
    {
        font = presentation_render.fonts + family;
        filename = font->tex_name;
        len = 0;
        while (filename[len] != 0)
            ++len;
        tex_rec = tex_find(filename);
        tex = tex_rec;
        font->tex = tex;
        width = (tex[12] + 3) & 0xFFFC;
        height = tex[13];
        byte_count = (width >> 1) * height;
        pixels = (uint8 *)malloc(byte_count ? byte_count : 1);
        XASSERT(!(pixels == NULL));
        tex = font->tex;
        val = tex[14];
        page = ((const TEXTURE_DESCRIPTOR *)tex)->page;
        rect.x = (sint16)((val >> 2) + ((page << 6) & 0x3FF));
        tex = font->tex;
        page = ((const TEXTURE_DESCRIPTOR *)tex)->page;
        val = tex[15];
        rect.y = (sint16)(val + ((page << 4) & 0x100) + ((page >> 2) & 0x200));
        rect.w = (sint16)(width >> 2);
        rect.h = (sint16)height;
        StoreImage(&rect, (uint32 *)pixels);
        DrawSync(0);
        for (glyph = 0; glyph < 128; ++glyph)
        {
            val = measure_glyph(family, glyph, pixels, byte_count);
            val -= font->spacing;
            font->widths[glyph] = (uint8)val;
        }
        val = font->cell_width;
        font->widths[0] = (uint8)(val >> 1);
        free(pixels);
    }
}

uint32 presentation_init_packets(void)
{
    uint32 idx;
    POLY_GT4 *poly;

    FUNCTION_MARKER(0x800B25E0, "SLUS_006.10");
    init_font_metrics();
    {
        sint32 operation_ok = !(!gpu_register_packet_range(&presentation_packets, sizeof(presentation_packets)));
        XASSERT(operation_ok);
    }
    {
        sint32 operation_ok = !(!gpu_register_packet_range(presentation_items, sizeof(presentation_items)));
        XASSERT(operation_ok);
    }
    for (idx = 0; idx < 1000; ++idx)
    {
        poly = presentation_packets.text + idx;
        setPolyGT4(poly);
        setSemiTrans(poly, 1);
    }
    return 0;
}

static sint16 *update_pals(void)
{
    MODEL_MATRIX matrix;
    uint32 words[8];
    uint32 idx, angle;
    sint16 *result;

    FUNCTION_MARKER(0x800B3394, "SLUS_006.10");
    for (idx = 0; idx < 8; ++idx)
        words[idx] = base_matrix.words[idx];
    for (idx = 0; idx < 8; ++idx)
        matrix.words[idx] = words[idx];
    model_set_light(0, &lights[0], 110, 0, 0);
    model_set_light(1, &lights[1], 110, 0, 110);
    model_set_light(2, &lights[2], 0, 0, 110);
    model_set_back_color(90, 50, 50);
    angle = (uint16)presentation_render.angles.x + 16;
    presentation_render.angles.x = (sint16)angle;
    RotMatrixX((sint16)angle, &matrix.sdk);
    RotMatrixY(presentation_render.angles.y, &matrix.sdk);
    angle = (uint16)presentation_render.angles.z + 32;
    presentation_render.angles.z = (sint16)angle;
    RotMatrixZ((sint16)angle, &matrix.sdk);
    SetRotMatrix(&matrix.sdk);
    for (idx = 0; idx < 3; ++idx)
        words[idx] = matrix.words[idx];
    xport_gte_write_control(5, words[0]);
    xport_gte_write_control(6, words[1]);
    xport_gte_write_control(7, words[2]);
    model_apply_light_matrix(&matrix.sdk);
    presentation_render.pals[1][0].word = gte_normal_color_col(&presentation_n[0], presentation_render.pals[0][0].word);
    presentation_render.pals[1][1].word = gte_normal_color_col(&presentation_n[1], presentation_render.pals[0][1].word);
    presentation_render.pals[1][2].word = gte_normal_color_col(&presentation_n[2], presentation_render.pals[0][2].word);
    presentation_render.pals[1][3].word = gte_normal_color_col(&presentation_n[3], presentation_render.pals[0][3].word);
    model_set_back_color(90, 90, 50);
    model_apply_light_matrix(&matrix.sdk);
    presentation_render.pals[6][0].word = gte_normal_color_col(&presentation_n[0], presentation_render.pals[0][0].word);
    presentation_render.pals[6][1].word = gte_normal_color_col(&presentation_n[1], presentation_render.pals[0][1].word);
    presentation_render.pals[6][2].word = gte_normal_color_col(&presentation_n[2], presentation_render.pals[0][2].word);
    presentation_render.pals[6][3].word = gte_normal_color_col(&presentation_n[3], presentation_render.pals[0][3].word);
    model_set_back_color(50, 50, 100);
    model_apply_light_matrix(&matrix.sdk);
    model_set_light(0, &lights[0], 0, 50, 50);
    model_set_light(1, &lights[1], 0, 0, 110);
    presentation_render.pals[4][0].word = gte_normal_color_col(&presentation_n[0], presentation_render.pals[0][0].word);
    presentation_render.pals[4][1].word = gte_normal_color_col(&presentation_n[1], presentation_render.pals[0][1].word);
    presentation_render.pals[4][2].word = gte_normal_color_col(&presentation_n[2], presentation_render.pals[0][2].word);
    presentation_render.pals[4][3].word = gte_normal_color_col(&presentation_n[3], presentation_render.pals[0][3].word);
    model_set_light(0, &lights[0], 110, 110, 110);
    model_set_light(1, &lights[0], 0, 0, 0);
    result = model_set_light(2, &lights[0], 0, 0, 0);
    model_set_back_color(50, 50, 50);
    return result;
}

sint16 *presentation_begin_frame(void)
{
    uint32 bank;

    FUNCTION_MARKER(0x800B2638, "SLUS_006.10");
    bank = disp_state.bank;
    presentation_render.packet_count = 0;
    presentation_render.packet_idx = bank * 500;
    return update_pals();
}

static void clear_items(void)
{
    sint32 offset = 69;

    FUNCTION_MARKER(0x80057310, "SLUS_006.10");
    do
    {
        presentation_items[offset].active = (0);
        --offset;
    } while (offset >= 0);
}

static uint32 find_free_item(void)
{
    uint32 idx;

    FUNCTION_MARKER(0x80057334, "SLUS_006.10");
    for (idx = 0; idx < 70; ++idx)
        if (presentation_items[idx].active == 0)
            return idx;
    return 0;
}

uint32 presentation_load_items(const PRESENTATION_RECORD_LIST *records)
{
    uint32 idx, slot, kind;
    PRESENTATION_ITEM *entry;
    const PRESENTATION_RECORD *rec;

    FUNCTION_MARKER(0x800B7294, "SLUS_006.10");
    clear_items();
    presentation_menu.records = records;
    if (records == NULL)
        return 0xFFFF;
    XASSERT(!(records->count > 70));
    for (idx = 0; idx < records->count; ++idx)
    {
        slot = find_free_item();
        entry = presentation_items + slot;
        presentation_menu.indices[idx] = slot;
        rec = records->entries + idx;
        entry->active = 1;
        entry->kind = rec->kind;
        entry->content.text = rec->content;
        entry->x_flags = rec->x_flags;
        entry->y = rec->y;
        entry->param = rec->param;
        entry->pal = rec->pal;
        kind = rec->kind & 0xFF00;
        if (kind == 0)
        {
            entry->content.text = rec->content;
            entry->kind = (uint8)entry->kind;
        }
        else if (kind == 0x8000)
            entry->content.tex = tex_find(rec->content);
    }
    return 0xFFFF;
}

uint32 presentation_draw_items(void)
{
    DISPLAY_ORDERING head;
    uint32 idx;
    PRESENTATION_ITEM *object;
    uint32 kind;
    POLY_GT4 *poly;
    uint32 vertex;
    uint32 component;
    uint32 val;
    const uint8 *tex;
    uint32 centered;
    uint32 size;
    uint32 coord;
    uint32 font;
    uint32 x;
    sint32 y;
    const char *text;

    uint32 tag;
    uint32 head_tag;

    DISPLAY_FRAME *cur_frame;

    FUNCTION_MARKER(0x800B73EC, "SLUS_006.10");
    for (idx = 0; idx < 70; ++idx)
    {
        object = presentation_items + idx;
        if (object->active == 0)
            continue;
        if ((object->x_flags & 0x4000) != 0)
            continue;
        kind = object->kind & 0xFF00;
        if (kind != 0 && kind != 0x8000)
            continue;
        if (presentation_menu.sel == idx)
            presentation_select_pal(1);
        else
        {
            presentation_select_pal(object->pal);
            if ((object->x_flags & 0x8000) != 0)
                presentation_select_pal(7);
        }
        if (kind == 0)
        {
            font = (uint8)object->kind;
            x = object->x_flags;
            y = object->y;
            text = object->content.text;
            {
                uint32 first = hd_ui_mark();
                presentation_draw_text(font, x & 0xFFF, y, text);
                if (session_flow.menu_state == 81 && (idx == 7 || idx == 8))
                {
                    float px = presentation_menu.banner_y_angle * RAD12;
                    float py = presentation_menu.banner_x_angle * RAD12;
                    float base_y = idx == 7 ? 100.0f : 108.0f;
                    hd_ui_motion(first, 400.0f + sinf(px - 48.0f * RAD12) * 32.0f - (x & 0xFFF), base_y + sinf(py - 32.0f * RAD12) * 32.0f - y, 400.0f + sinf(px) * 32.0f - (x & 0xFFF), base_y + sinf(py) * 32.0f - y);
                }
            }
            continue;
        }
        val = disp_state.bank;
        poly = &object->packets[val];
        for (vertex = 0; vertex < 3; ++vertex)
        {
            for (component = 0; component < 3; ++component)
            {
                val = presentation_render.colors[vertex].channels[component];
                ((uint8 *)&poly->p[vertex].rgbc)[component] = (uint8)(val);
            }
        }
        val = presentation_render.colors[3].channels[0];
        poly->p[3].rgbc.r = (uint8)(val);
        val = presentation_render.colors[3].channels[1];
        poly->p[3].rgbc.g = (uint8)(val);
        val = presentation_render.colors[3].channels[2];
        setlen(poly, 12);
        setcode(poly, 0x3E);
        poly->p[3].rgbc.b = (uint8)(val);
        tex = object->content.tex;
        val = ((const TEXTURE_DESCRIPTOR *)tex)->page;
        poly->p[1].attr = (uint16)(val);
        tex = object->content.tex;
        val = ((const TEXTURE_DESCRIPTOR *)tex)->clut;
        poly->p[0].attr = (uint16)(val);
        centered = object->kind & 0x80;
        tex = object->content.tex;
        for (vertex = 0; vertex < 4; ++vertex)
        {
            if (centered != 0)
            {
                size = tex[12] >> 1;
                coord = object->x_flags & 0xFFF;
                val = (vertex & 1) != 0 ? coord + size - 1 : coord - size;
            }
            else
            {
                val = object->x_flags & 0xFFF;
                if ((vertex & 1) != 0)
                    val += tex[12] + 0xFFFF;
            }
            poly->p[vertex].xy.x = (sint16)(val);
            if (centered != 0)
            {
                size = tex[13] >> 1;
                coord = (uint16)object->y;
                val = (vertex & 2) != 0 ? coord + size - 1 : coord - size;
            }
            else if ((vertex & 2) != 0)
            {
                size = tex[13];
                coord = (uint16)object->y;
                val = coord + size + 0xFFFF;
            }
            else
                val = (uint16)object->y;
            poly->p[vertex].xy.y = (sint16)(val);
        }
        for (vertex = 0; vertex < 4; ++vertex)
        {
            tex = object->content.tex;
            val = tex[14];
            if ((vertex & 1) != 0)
                val += tex[12] + 255;
            poly->p[vertex].uv.u = (uint8)(val);
            tex = object->content.tex;
            val = tex[15];
            if ((vertex & 2) != 0)
                val += tex[13] + 255;
            poly->p[vertex].uv.v = (uint8)(val);
        }
        cur_frame = disp_state.cur;
        hd_ui_gouraud(poly->p, 16);
        head.tags = cur_frame->ordering;
        head.byte_offset = 0;
        tag = poly->tag;
        head_tag = head.tags[(sint32)(head.byte_offset + (uint32)(0x40)) / 4];
        poly->tag = (tag & 0xFF000000) | (head_tag & 0xFFFFFF);
        head.tags = cur_frame->ordering;
        head.byte_offset = 0;
        head_tag = head.tags[(sint32)(head.byte_offset + (uint32)(0x40)) / 4];
        head.tags[(sint32)(head.byte_offset + (uint32)(0x40)) / 4] = (head_tag & 0xFF000000) | gpu_packet_link((uintptr_t)poly);
    }
    return 0;
}

uint32 presentation_show_image(uint8 *buffer, uint32 capacity, const char *filename, sint32 seconds, uint32 skippable)
{
    uint32 elapsed;
    uint32 buttons;
    uint32 prev_buttons;
    uint32 pressed;
    PSX_RECT rect;

    DISPLAY_FRAME *cur_frame;

    FUNCTION_MARKER(0x80019EA0, "SLUS_006.10");
    hd_suspend();
    XASSERT(!(capacity < 20 + 320 * 256 * 2));
    VSync(0);
    DrawSync(0);
    SetDispMask(0);
    disp_prepare(0);
    disp_swap();
    disp_state.frames[0].disp.screen.x = (sint16)((uint16)disp_options.scr_x);
    disp_state.frames[1].disp.screen.x = (sint16)((uint16)disp_options.scr_x);
    disp_state.frames[0].disp.screen.y = (sint16)((uint16)disp_options.scr_y);
    disp_state.frames[1].disp.screen.y = (sint16)((uint16)disp_options.scr_y);
    cur_frame = disp_state.cur;
    cur_frame->draw.clip.y = (sint16)((uint16)disp_state.frames[(disp_state.bank ^ 1)].draw.clip.y);
    cur_frame = disp_state.cur;
    cur_frame->draw.ofs[1] = (sint16)((uint16)disp_state.frames[(disp_state.bank ^ 1)].draw.ofs[1]);
    disp_apply();
    archive_read_file(filename, buffer, capacity);
    rect.w = 320;
    rect.h = 256;
    rect.x = 0;
    rect.y = 0;
    LoadImagePSX(&rect, (uint32 *)(buffer + 20));
    rect.w = 320;
    rect.h = 256;
    rect.x = 0;
    rect.y = 256;
    LoadImagePSX(&rect, (uint32 *)(buffer + 20));
    DrawSync(0);
    VSync(0);
    SetDispMask(1);
    elapsed = 0;
    /* Require a fresh Cross/Start press for each startup splash */
    prev_buttons = skippable ? xport_input_read(0) | xport_input_read(1) : 0;
    while ((sint32)elapsed < (sint32)((uint32)seconds * disp_state.field_rate))
    {
        SetDispMask(1);
        VSync(0);
        ++elapsed;
        if (skippable)
        {
            buttons = xport_input_read(0) | xport_input_read(1);
            pressed = buttons & ~prev_buttons;
            prev_buttons = buttons;
            if ((pressed & (PADstart | PADRdown)) != 0)
                break;
        }
    }
    return elapsed;
}

static void submit_player_name(void)
{
    uint32 val;

    FUNCTION_MARKER(0x800A6F00, "SLUS_006.10");
    if (strcmp(presentation_player_name, "MTHREE") == 0)
        session_unlock_bonus_tracks();
    if (strcmp(presentation_player_name, "RONE") == 0)
        session_unlock_bonus_models();
    if (strcmp(presentation_player_name, "VRSIX") == 0)
        session_unlock_features();
    if (strcmp(presentation_player_name, "FIESTA") == 0)
        session_options.spawn_node_override = 1;
    if (strcmp(presentation_player_name, "SPURT") == 0)
        car_physics.enhanced_mode = 1;
    if (strcmp(presentation_player_name, "NOLIFE") == 0)
        session_unlock_special_models();
    if (strcmp(presentation_player_name, "HAHA") == 0)
        session_options.fast_attract = (session_options.fast_attract ^ 1);
    if (strcmp(presentation_player_name, "AUXYRAY") == 0)
        progression_profile.special_model_30 = (uint8)(1);
    if (session_flow.event_started == 0)
    {
        val = menu_choices.single_event;
        session_flow.player_name_entered = 1;
        if (val == 2)
            session_push_menu(20);
        else
            session_push_menu(10);
    }
    else
        session_flow.menu_state = (120);
    save_autosave();
}

static void update_player_name(void)
{
    uint32 buttons, sel, cursor, character, column, row;
    char *name, *pointer;

    FUNCTION_MARKER(0x800A70B4, "SLUS_006.10");
    buttons = input_ports[0].pressed;
    name = presentation_player_name;
    if ((buttons & 0x1000) != 0 && session_flow.event_started == 0)
    {
        session_pop_menu();
        audio_play_menu_accept();
    }
    if ((input_ports[0].pressed & 0x10) != 0 && (sint32)presentation_name_editor.column > 0)
    {
        audio_play_menu_high();
        presentation_name_editor.column = (uint32)(presentation_name_editor.column - 1);
    }
    if ((input_ports[0].pressed & 0x40) != 0 && (sint32)presentation_name_editor.column < 3)
    {
        audio_play_menu_high();
        presentation_name_editor.column = (uint32)(presentation_name_editor.column + 1);
    }
    if ((input_ports[0].pressed & 0x80) != 0 && (sint32)presentation_name_editor.row > 0)
    {
        audio_play_menu_high();
        presentation_name_editor.row = (uint32)(presentation_name_editor.row - 1);
    }
    if ((input_ports[0].pressed & 0x20) != 0 && (sint32)presentation_name_editor.row < 7)
    {
        audio_play_menu_high();
        presentation_name_editor.row = (uint32)(presentation_name_editor.row + 1);
    }
    column = presentation_name_editor.column;
    row = presentation_name_editor.row;
    buttons = input_ports[0].pressed;
    presentation_menu.sel = (row + column * 8);
    if ((buttons & 0x4008) != 0)
    {
        audio_play_menu_move();
        sel = presentation_menu.sel;
        if (sel == 30)
        {
            cursor = presentation_name_editor.cursor;
            if ((sint32)cursor > 0)
                presentation_name_editor.cursor = (uint32)(cursor - 1);
        }
        else if (sel == 31)
        {
            cursor = presentation_name_editor.cursor;
            name[cursor] = 0;
            submit_player_name();
            return;
        }
        else
        {
            if ((sint32)sel < 31 && sel == 29)
            {
                cursor = presentation_name_editor.cursor;
                name[cursor] = 32;
            }
            cursor = presentation_name_editor.cursor;
            if ((sint32)cursor < 8)
                presentation_name_editor.cursor = (uint32)(cursor + 1);
            else
            {
                if (cursor == 7)
                    presentation_name_editor.cursor = (uint32)(8);
                presentation_name_editor.row = (uint32)(7);
                presentation_name_editor.column = (uint32)(3);
            }
        }
    }
    sel = presentation_menu.sel;
    pointer = presentation_items[sel].content.text;
    cursor = presentation_name_editor.cursor;
    character = (uint8)*pointer;
    name[cursor] = (char)(uint8)character;
    cursor = presentation_name_editor.cursor;
    name[cursor + 1] = 0;
    pointer = presentation_player_name;
    presentation_items[32].content.text = pointer;
}

sint32 presentation_upload_background(void)
{
    const uint8 *image;
    uint32 idx;
    PSX_RECT rect;

    FUNCTION_MARKER(0x80057AE0, "SLUS_006.10");
    image = (const uint8 *)presentation_background.image;
    idx = disp_state.bank;
    rect.h = 256;
    rect.w = 512;
    rect.x = 0;
    rect.y = (sint16)(idx << 8);
    return LoadImagePSX(&rect, (uint32 *)(image + 20));
}

void presentation_show_loading(void)
{
#if AP_WEB
    if (session_flow.phase != 1 && session_flow.phase != 6 && session_flow.phase != 7 && session_flow.phase != 9)
        return;
#endif

    uint8 *image = startup_buffer.bytes;
    uint32 image_bytes;
    uint32 next;

    PSX_RECT rect;
    char filename[64];

    DISPLAY_FRAME *cur_frame;

    FUNCTION_MARKER(0x80019BB8, "SLUS_006.10");
    if (session_flow.loading_image_shown == 1)
        return;
    hd_suspend();
    session_flow.loading_image_shown = (1);
    next = loading_image_cycle + 1;
    loading_image_cycle = (uint32)((sint32)next % 20);
#if !AP_WEB
    VSync(0);
#endif
    DrawSync(0);
    SetDispMask(0);
    disp_prepare(0);
    disp_swap();
    disp_state.frames[0].disp.screen.x = (sint16)((uint16)disp_options.scr_x);
    disp_state.frames[1].disp.screen.x = (sint16)((uint16)disp_options.scr_x);
    disp_state.frames[0].disp.screen.y = (sint16)((uint16)disp_options.scr_y);
    disp_state.frames[1].disp.screen.y = (sint16)((uint16)disp_options.scr_y);
    cur_frame = disp_state.cur;
    cur_frame->draw.clip.y = (sint16)((uint16)disp_state.frames[(disp_state.bank ^ 1)].draw.clip.y);
    cur_frame = disp_state.cur;
    cur_frame->draw.ofs[1] = (sint16)((uint16)disp_state.frames[(disp_state.bank ^ 1)].draw.ofs[1]);
    disp_apply();
    sprintf(filename, loading_format, loading_image_cycle + 1);
    image_bytes = archive_read_file(filename, image, 0x28800);
    XASSERT(!(image_bytes < 20 + 320 * 256 * 2 || image_bytes > 0x28800));
    rect.w = 320;
    rect.h = 256;
    rect.x = 0;
    rect.y = 0;
    LoadImagePSX(&rect, (uint32 *)(image + 20));
    rect.w = 320;
    rect.h = 256;
    rect.x = 0;
    rect.y = 256;
    LoadImagePSX(&rect, (uint32 *)(image + 20));
    DrawSync(0);
#if !AP_WEB
    VSync(0);
#endif
    SetDispMask(1);
    if (!hd_loading_screen())
    {
#if AP_WEB
        // submit one frame so the browser paints before race loading continues
        gpu_present();
#endif
    }
}

void presentation_show_credits_legal_images(uint8 *buffer, uint32 capacity)
{
    FUNCTION_MARKER(0x8001A1BC, "SLUS_006.10");
    XASSERT(!(buffer == NULL || capacity < 0x28800));
    presentation_show_image(buffer, 0x28800, "PRES\\MLEGAL1.TIM", 4, 0);
    presentation_show_image(buffer, 0x28800, "PRES\\MLEGAL2.TIM", 4, 0);
    presentation_show_image(buffer, 0x28800, "PRES\\MLEGAL3.TIM", 4, 0);
    presentation_show_image(buffer, 0x28800, "PRES\\MLEGAL4.TIM", 4, 0);
    presentation_show_image(buffer, 0x28800, "PRES\\MLEGAL5.TIM", 4, 0);
    SetDispMask(0);
}

void presentation_load_menu(void)
{
    uint8 *model_file;
    uint8 *vram;
    uint32 vram_size;
    uint8 *cursor;
    uint32 val, other, bytes, count, len;
    PSX_RECT rect;
    char filename[sizeof(presentation_background_format)];
    sint16 width, height;
    sint32 random;

    FUNCTION_MARKER(0x800A7B84, "SLUS_006.10");
    ;
    asset_register_gpu_storage();
    disp_init_ordering_tables(0x1101);
    model_set_ot_shift(2);
    model_set_proj_distance(180);
    model_set_depth_range(0, 0x10000);
    model_set_back_color(50, 50, 50);
    model_set_light(0, &menu_light_dir, 110, 110, 110);
    model_set_light(1, &menu_light_dir, 0, 0, 0);
    model_set_light(2, &menu_light_dir, 0, 0, 0);
    model_set_fog(3000, 16000, 0, 0, 0);
    rect = menu_vram_rect;
    vram = (uint8 *)malloc(0x80000);
    XASSERT(!(vram == NULL));
    vram_size = archive_read_file("PRESDAT\\VRAM.OUT", vram, 0x80000);
    XASSERT(!(vram_size > 0x80000));
    rect.y = 0;
    width = rect.w;
    height = rect.h;
    bytes = width > 0 && height > 0 ? (uint32)width * (uint32)height * 2 : 0;
    XASSERT(!(bytes > vram_size));
    LoadImagePSX(&rect, (uint32 *)vram);
    rect.y = 256;
    width = rect.w;
    height = rect.h;
    bytes = width > 0 && height > 0 ? (uint32)width * (uint32)height * 2 : 0;
    XASSERT(!(vram_size < 0x40000 || bytes > vram_size - 0x40000));
    LoadImagePSX(&rect, (uint32 *)(vram + 0x40000));
    DrawSync(0);
    free(vram);
    bytes = archive_aligned_size("PRESDAT\\PRES.DAT");
    XASSERT(!(bytes > sizeof(game_resource_storage.file_words)));
    presentation_model_image.bytes = (uint8 *)game_resource_storage.file_words;
    presentation_model_image.size = bytes;
    presentation_model_image.size = archive_read_file("PRESDAT\\PRES.DAT", presentation_model_image.bytes, bytes);
    XASSERT(!(presentation_model_image.size > bytes || presentation_model_image.size < 8));
    model_file = presentation_model_image.bytes;
    memcpy(&val, model_file, sizeof(val));
    XASSERT(!(val > (presentation_model_image.size - 4) / 4));
    count = 1;
    cursor = model_file + 4;
    if (val != 0)
    {
        do
        {
            memcpy(&val, cursor, sizeof(val));
            XASSERT(!(val > presentation_model_image.size));
            memcpy(&val, model_file, sizeof(val));
            ++count;
            cursor += 4;
        } while (val >= count);
    }
    model_file = presentation_model_image.bytes;
    memcpy(&val, model_file + 4, sizeof(val));
    XASSERT(!(val > presentation_model_image.size || presentation_model_image.size - val < 4));
    tex_set_table(model_file + val, presentation_model_image.size - val);
    val = presentation_background.cycle + 1;
    val = ((sint32)val % 6);
    presentation_background.cycle = (uint32)(val);
    len = (uint32)sprintf(filename, presentation_background_format, (sint32)(val + 1));
    {
        sint32 operation_ok = !(archive_read_file(filename, presentation_background.image, (uint32)sizeof(presentation_background.image)) < 20 + 512 * 256 * 2);
        XASSERT(operation_ok);
    }
    if (session_flow.menu_initialized == 0)
    {
        audio_stop_xa();
        audio_stop_voice(20);
    }
    else
    {
        presentation_wait_loading();
    }
    random = randPSX();
    audio_play_xa(random % 9);
    presentation_init_packets();
    rect.w = 512;
    rect.h = 512;
    rect.x = 0;
    rect.y = 0;
    ClearImage(&rect, 0, 0, 0);
    SetDispMask(1);
    disp_prepare(2);
    val = disp_options.scr_x;
    other = disp_options.scr_y;
    presentation_menu.save_load_attempted = 0;
    disp_state.frames[0].disp.screen.x = (sint16)(val);
    disp_state.frames[1].disp.screen.x = (sint16)(val);
    disp_state.frames[0].disp.screen.y = (sint16)(other);
    disp_state.frames[1].disp.screen.y = (sint16)(other);
    disp_swap();
    disp_apply();
    ;
    run_menu();
    ;
}

static void init_menu_frame_packet(POLY_F4 *packet)
{
    packet->tag = (packet->tag & 0x00FFFFFF) | ((uint32)(5) << 24);
    packet->rgbc.code = (uint8)(40);
    packet->rgbc.r = (uint8)(0);
    packet->rgbc.g = (uint8)(0);
    packet->rgbc.b = (uint8)(16);
}

static void draw_menu_frame(void)
{
    DISPLAY_ORDERING head;
    POLY_F4 *packet, *next_packet;
    uint32 tag, prev, buffer, val;

    DISPLAY_FRAME *cur_frame;

    FUNCTION_MARKER(0x800A9568, "SLUS_006.10");
    buffer = disp_state.bank;
    packet = &presentation_packets.menu_frame[0][buffer];
    init_menu_frame_packet(packet);
    cur_frame = disp_state.cur;
    packet->p[0].x = (sint16)(uint16)(32);
    packet->p[0].y = (sint16)(uint16)(0);
    packet->p[1].x = (sint16)(uint16)(91);
    packet->p[1].y = (sint16)(uint16)(0);
    packet->p[2].x = (sint16)(uint16)(32);
    packet->p[2].y = (sint16)(uint16)(200);
    packet->p[3].x = (sint16)(uint16)(91);
    packet->p[3].y = (sint16)(uint16)(200);
    head.tags = cur_frame->ordering;
    head.byte_offset = 0;
    tag = packet->tag;
    prev = head.tags[(sint32)(head.byte_offset + (uint32)(0x800)) / 4];
    packet->tag = (tag & 0xFF000000) | (prev & 0xFFFFFF);
    head.tags = cur_frame->ordering;
    head.byte_offset = 0;
    buffer = disp_state.bank;
    prev = head.tags[(sint32)(head.byte_offset + (uint32)(0x800)) / 4];
    next_packet = &presentation_packets.menu_frame[1][buffer];
    hd_ui_color_quad(packet->p, &packet->rgbc, 512);
    head.tags[(sint32)(head.byte_offset + (uint32)(0x800)) / 4] = (prev & 0xFF000000) | gpu_packet_link((uintptr_t)packet);
    packet = next_packet;
    init_menu_frame_packet(packet);
    packet->p[0].x = (sint16)(uint16)(0);
    packet->p[0].y = (sint16)(uint16)(27);
    packet->p[1].x = (sint16)(uint16)(450);
    packet->p[1].y = (sint16)(uint16)(27);
    packet->p[2].x = (sint16)(uint16)(0);
    packet->p[2].y = (sint16)(uint16)(32);
    val = (uint16)packet->p[1].x;
    cur_frame = disp_state.cur;
    packet->p[3].x = (sint16)(uint16)(450);
    packet->p[3].y = (sint16)(uint16)(32);
    packet->p[1].x = (sint16)(uint16)(val + 5);
    head.tags = cur_frame->ordering;
    head.byte_offset = 0;
    tag = packet->tag;
    prev = head.tags[(sint32)(head.byte_offset + (uint32)(0x800)) / 4];
    packet->tag = (tag & 0xFF000000) | (prev & 0xFFFFFF);
    head.tags = cur_frame->ordering;
    head.byte_offset = 0;
    buffer = disp_state.bank;
    prev = head.tags[(sint32)(head.byte_offset + (uint32)(0x800)) / 4];
    next_packet = &presentation_packets.menu_frame[2][buffer];
    hd_ui_color_quad(packet->p, &packet->rgbc, 512);
    head.tags[(sint32)(head.byte_offset + (uint32)(0x800)) / 4] = (prev & 0xFF000000) | gpu_packet_link((uintptr_t)packet);
    packet = next_packet;
    packet->p[1].x = (sint16)(uint16)(442);
    val = (uint16)packet->p[1].x;
    init_menu_frame_packet(packet);
    cur_frame = disp_state.cur;
    packet->p[0].y = (sint16)(uint16)(35);
    packet->p[1].y = (sint16)(uint16)(35);
    packet->p[0].x = (sint16)(uint16)(0);
    packet->p[2].x = (sint16)(uint16)(0);
    packet->p[2].y = (sint16)(uint16)(40);
    packet->p[3].x = (sint16)(uint16)(442);
    packet->p[3].y = (sint16)(uint16)(40);
    packet->p[1].x = (sint16)(uint16)(val + 5);
    head.tags = cur_frame->ordering;
    head.byte_offset = 0;
    tag = packet->tag;
    prev = head.tags[(sint32)(head.byte_offset + (uint32)(0x800)) / 4];
    packet->tag = (tag & 0xFF000000) | (prev & 0xFFFFFF);
    head.tags = cur_frame->ordering;
    head.byte_offset = 0;
    prev = head.tags[(sint32)(head.byte_offset + (uint32)(0x800)) / 4];
    hd_ui_color_quad(packet->p, &packet->rgbc, 512);
    head.tags[(sint32)(head.byte_offset + (uint32)(0x800)) / 4] = (prev & 0xFF000000) | gpu_packet_link((uintptr_t)packet);
    buffer = disp_state.bank;
    packet = &presentation_packets.menu_frame[3][buffer];
    packet->p[1].x = (sint16)(uint16)(434);
    val = (uint16)packet->p[1].x;
    init_menu_frame_packet(packet);
    cur_frame = disp_state.cur;
    packet->p[0].y = (sint16)(uint16)(43);
    packet->p[1].y = (sint16)(uint16)(43);
    packet->p[0].x = (sint16)(uint16)(0);
    packet->p[2].x = (sint16)(uint16)(0);
    packet->p[2].y = (sint16)(uint16)(47);
    packet->p[3].x = (sint16)(uint16)(434);
    packet->p[3].y = (sint16)(uint16)(47);
    packet->p[1].x = (sint16)(uint16)(val + 5);
    head.tags = cur_frame->ordering;
    head.byte_offset = 0;
    tag = packet->tag;
    prev = head.tags[(sint32)(head.byte_offset + (uint32)(0x800)) / 4];
    buffer = disp_state.bank;
    packet->tag = (tag & 0xFF000000) | (prev & 0xFFFFFF);
    head.tags = cur_frame->ordering;
    head.byte_offset = 0;
    prev = head.tags[(sint32)(head.byte_offset + (uint32)(0x800)) / 4];
    next_packet = &presentation_packets.menu_frame[4][buffer];
    hd_ui_color_quad(packet->p, &packet->rgbc, 512);
    head.tags[(sint32)(head.byte_offset + (uint32)(0x800)) / 4] = (prev & 0xFF000000) | gpu_packet_link((uintptr_t)packet);
    packet = next_packet;
    init_menu_frame_packet(packet);
    packet->p[0].x = (sint16)(uint16)(91);
    packet->p[0].y = (sint16)(uint16)(227);
    packet->p[1].x = (sint16)(uint16)(513);
    packet->p[1].y = (sint16)(uint16)(227);
    val = (uint16)packet->p[1].x;
    cur_frame = disp_state.cur;
    packet->p[2].x = (sint16)(uint16)(91);
    packet->p[2].y = (sint16)(uint16)(232);
    packet->p[3].x = (sint16)(uint16)(513);
    packet->p[3].y = (sint16)(uint16)(232);
    packet->p[1].x = (sint16)(uint16)(val + 5);
    head.tags = cur_frame->ordering;
    head.byte_offset = 0;
    tag = packet->tag;
    prev = head.tags[(sint32)(head.byte_offset + (uint32)(0x800)) / 4];
    packet->tag = (tag & 0xFF000000) | (prev & 0xFFFFFF);
    head.tags = cur_frame->ordering;
    head.byte_offset = 0;
    prev = head.tags[(sint32)(head.byte_offset + (uint32)(0x800)) / 4];
    hd_ui_color_quad(packet->p, &packet->rgbc, 512);
    head.tags[(sint32)(head.byte_offset + (uint32)(0x800)) / 4] = (prev & 0xFF000000) | gpu_packet_link((uintptr_t)packet);
}

static void edit_audio_options(void)
{
    DISPLAY_ORDERING head;
    char *dst;
    const char *format;
    uint32 row_offset, y;
    POLY_G4 *packet;
    uint32 tag, prev, amount, left, right;
    sint32 width;

    DISPLAY_FRAME *cur_frame;

    FUNCTION_MARKER(0x800AAF68, "SLUS_006.10");
    if ((input_ports[0].pressed & 0x80) != 0 && menu_choices.audio_row == 1 && (sint32)audio_options.effects_volume > 0)
    {
        audio_play_menu_high();
        audio_options.effects_volume = (audio_options.effects_volume - 512);
    }
    if ((input_ports[0].pressed & 0x20) != 0 && menu_choices.audio_row == 1 && (sint32)audio_options.effects_volume < 4096)
    {
        audio_play_menu_high();
        audio_options.effects_volume = (audio_options.effects_volume + 512);
    }
    if ((input_ports[0].pressed & 0x80) != 0 && menu_choices.audio_row == 0 && (sint32)audio_options.music_volume > 0)
    {
        audio_play_menu_high();
        audio_options.music_volume = (audio_options.music_volume - 512);
        audio_update_cd_volume();
    }
    if ((input_ports[0].pressed & 0x20) != 0 && menu_choices.audio_row == 0 && (sint32)audio_options.music_volume < 4096)
    {
        audio_play_menu_high();
        audio_options.music_volume = (audio_options.music_volume + 512);
        audio_update_cd_volume();
    }
    if (menu_choices.audio_row == 2 && (input_ports[0].pressed & 0xA0) != 0)
    {
        audio_play_menu_move();
        audio_options.paired = (audio_options.paired ^ 1);
    }
    dst = presentation_find_text(presentation_items[2].content.text, " - ");
    format = audio_options.paired != 0 ? "ON " : "OFF";
    presentation_copy_text(dst + 3, format);
    y = 80;
    for (row_offset = 0; row_offset < 144; row_offset += 72)
    {
        packet = &presentation_packets.volume_background[row_offset / 72][disp_state.bank];
        packet->tag = (packet->tag & 0x00FFFFFF) | ((uint32)(8) << 24);
        packet->p[0].rgbc.code = (uint8)(0x38);
        packet->p[0].rgbc.r = (uint8)(0);
        packet->p[0].rgbc.g = (uint8)(255);
        packet->p[0].rgbc.b = (uint8)(0);
        packet->p[1].rgbc.r = (uint8)(255);
        packet->p[1].rgbc.g = (uint8)(0);
        packet->p[1].rgbc.b = (uint8)(0);
        packet->p[2].rgbc.r = (uint8)(0);
        packet->p[2].rgbc.g = (uint8)(255);
        packet->p[2].rgbc.b = (uint8)(0);
        packet->p[3].rgbc.r = (uint8)(255);
        packet->p[3].rgbc.g = (uint8)(0);
        packet->p[3].rgbc.b = (uint8)(0);
        cur_frame = disp_state.cur;
        packet->p[0].xy.x = (sint16)(uint16)(96);
        packet->p[0].xy.y = (sint16)(uint16)(y);
        packet->p[1].xy.x = (sint16)(uint16)(224);
        packet->p[1].xy.y = (sint16)(uint16)(y);
        packet->p[2].xy.x = (sint16)(uint16)(96);
        packet->p[2].xy.y = (sint16)(uint16)(y + 8);
        packet->p[3].xy.x = (sint16)(uint16)(224);
        packet->p[3].xy.y = (sint16)(uint16)(y + 8);
        head.tags = cur_frame->ordering;
        head.byte_offset = 0;
        tag = packet->tag;
        prev = head.tags[(sint32)(head.byte_offset + (uint32)(8)) / 4];
        packet->tag = (tag & 0xFF000000) | (prev & 0xFFFFFF);
        head.tags = cur_frame->ordering;
        head.byte_offset = 0;
        prev = head.tags[(sint32)(head.byte_offset + (uint32)(8)) / 4];
        hd_ui_gradient(packet->p, 2);
        head.tags[(sint32)(head.byte_offset + (uint32)(8)) / 4] = (prev & 0xFF000000) | gpu_packet_link((uintptr_t)packet);
        amount = 4096 - (row_offset != 0 ? audio_options.effects_volume : audio_options.music_volume);
        packet = &presentation_packets.volume_mask[row_offset / 72][disp_state.bank];
        packet->tag = (packet->tag & 0x00FFFFFF) | ((uint32)(8) << 24);
        packet->p[0].rgbc.code = (uint8)(0x38);
        packet->p[0].rgbc.r = (uint8)(15);
        packet->p[0].rgbc.g = (uint8)(0);
        packet->p[0].rgbc.b = (uint8)(0);
        packet->p[1].rgbc.r = (uint8)(15);
        packet->p[1].rgbc.g = (uint8)(0);
        packet->p[1].rgbc.b = (uint8)(0);
        packet->p[2].rgbc.r = (uint8)(15);
        packet->p[2].rgbc.g = (uint8)(0);
        packet->p[2].rgbc.b = (uint8)(0);
        packet->p[3].rgbc.r = (uint8)(15);
        packet->p[3].rgbc.g = (uint8)(0);
        packet->p[3].rgbc.b = (uint8)(0);
        width = (sint32)amount / 32;
        left = 224 - (uint32)width;
        right = left + (uint32)width;
        packet->p[0].xy.x = (sint16)(uint16)(left);
        packet->p[2].xy.x = (sint16)(uint16)(left);
        packet->p[0].xy.y = (sint16)(uint16)(y);
        packet->p[1].xy.x = (sint16)(uint16)(right);
        packet->p[1].xy.y = (sint16)(uint16)(y);
        packet->p[2].xy.y = (sint16)(uint16)(y + 8);
        cur_frame = disp_state.cur;
        packet->p[3].xy.x = (sint16)(uint16)(right);
        packet->p[3].xy.y = (sint16)(uint16)(y + 8);
        head.tags = cur_frame->ordering;
        head.byte_offset = 0;
        tag = packet->tag;
        prev = head.tags[(sint32)head.byte_offset / 4];
        packet->tag = (tag & 0xFF000000) | (prev & 0xFFFFFF);
        head.tags = cur_frame->ordering;
        head.byte_offset = 0;
        prev = head.tags[(sint32)head.byte_offset / 4];
        y = 130;
        hd_ui_gradient(packet->p, 0);
        head.tags[(sint32)head.byte_offset / 4] = (prev & 0xFF000000) | gpu_packet_link((uintptr_t)packet);
    }
}

PRESENTATION_PRES_RECORDS presentation_pres_recs;
PRESENTATION_CARSEL_RECORDS presentation_carsel_recs;
/* Decoded menu record defaults from accepted PRES and CARSEL resources */
const PRESENTATION_PRES_RECORDS presentation_pres_rec_defaults =
    {
        .controls =
            {
                {0x0081, presentation_pres_text.controls_cfg_0, 0x00B0, 64, 0, 0, 0},      {0x0081, presentation_pres_text.controls_deadzone_0, 0x00B0, 74, 0, 0, 0},      {0x0081, presentation_pres_text.controls_senistivity_0, 0x00B0, 84, 0, 0, 0}, {0x0081, presentation_pres_text.controls_calibrate, 0x00B0, 94, 0, 0, 0}, {0x0081, presentation_pres_text.controls_ctazmad, 0x00B0, 104, 0, 0, 0},    {0x0081, presentation_pres_text.controls_no_pad_present, 0x00B0, 128, 2, 0, 0}, {0x0001, presentation_pres_text.controls_accelerate, 0x00AE, 128, 5, 0, 0}, {0x0001, presentation_pres_text.controls_brake, 0x00AE, 136, 5, 0, 0},     {0x0001, presentation_pres_text.controls_steer_left, 0x00AE, 144, 5, 0, 0}, {0x0001, presentation_pres_text.controls_steer_right, 0x00AE, 152, 5, 0, 0},  {0x0001, presentation_pres_text.controls_gear_up, 0x00AE, 160, 5, 0, 0},
                {0x0001, presentation_pres_text.controls_gear_down, 0x00AE, 168, 5, 0, 0}, {0x0001, presentation_pres_text.controls_hand_brake, 0x00AE, 176, 5, 0, 0},     {0x0001, presentation_pres_text.controls_rear_view, 0x00AE, 184, 5, 0, 0},    {0x0001, presentation_pres_text.controls_horn, 0x00AE, 192, 5, 0, 0},     {0x0001, presentation_pres_text.controls_camera, 0x00AE, 200, 5, 0, 0},     {0x0001, presentation_pres_text.controls_empty, 0x00B2, 128, 0, 0, 0},          {0x0001, presentation_pres_text.controls_empty, 0x00B2, 136, 0, 0, 0},      {0x0001, presentation_pres_text.controls_empty, 0x00B2, 144, 0, 0, 0},     {0x0001, presentation_pres_text.controls_empty, 0x00B2, 152, 0, 0, 0},      {0x0001, presentation_pres_text.controls_empty, 0x00B2, 160, 0, 0, 0},        {0x0001, presentation_pres_text.controls_empty, 0x00B2, 168, 0, 0, 0},
                {0x0001, presentation_pres_text.controls_empty, 0x00B2, 176, 0, 0, 0},     {0x0001, presentation_pres_text.controls_empty, 0x00B2, 184, 0, 0, 0},          {0x0001, presentation_pres_text.controls_empty, 0x00B2, 192, 0, 0, 0},        {0x0001, presentation_pres_text.controls_empty, 0x00B2, 200, 0, 0, 0},    {0x0081, presentation_pres_text.controls_empty, 0x00B2, 128, 0, 0, 0},      {0x0081, presentation_pres_text.controls_empty, 0x00B2, 136, 0, 0, 0},          {0x0081, presentation_pres_text.controls_empty, 0x00B2, 144, 0, 0, 0},      {0x0081, presentation_pres_text.controls_cfg_1, 0x018C, 64, 0, 0, 0},      {0x0081, presentation_pres_text.controls_deadzone_1, 0x018C, 74, 0, 0, 0},  {0x0081, presentation_pres_text.controls_senistivity_1, 0x018C, 84, 0, 0, 0}, {0x0081, presentation_pres_text.controls_calibrate, 0x018C, 94, 0, 0, 0},
                {0x0081, presentation_pres_text.controls_catzmad, 0x018C, 104, 0, 0, 0},   {0x0081, presentation_pres_text.controls_no_pad_present, 0x018C, 128, 2, 0, 0}, {0x0001, presentation_pres_text.controls_accelerate, 0x018A, 128, 5, 0, 0},   {0x0001, presentation_pres_text.controls_brake, 0x018A, 136, 5, 0, 0},    {0x0001, presentation_pres_text.controls_steer_left, 0x018A, 144, 5, 0, 0}, {0x0001, presentation_pres_text.controls_steer_right, 0x018A, 152, 5, 0, 0},    {0x0001, presentation_pres_text.controls_gear_up, 0x018A, 160, 5, 0, 0},    {0x0001, presentation_pres_text.controls_gear_down, 0x018A, 168, 5, 0, 0}, {0x0001, presentation_pres_text.controls_hand_brake, 0x018A, 176, 5, 0, 0}, {0x0001, presentation_pres_text.controls_rear_view, 0x018A, 184, 5, 0, 0},    {0x0001, presentation_pres_text.controls_horn, 0x018A, 192, 5, 0, 0},
                {0x0001, presentation_pres_text.controls_camera, 0x018A, 200, 5, 0, 0},    {0x0001, presentation_pres_text.controls_empty, 0x018E, 128, 0, 0, 0},          {0x0001, presentation_pres_text.controls_empty, 0x018E, 136, 0, 0, 0},        {0x0001, presentation_pres_text.controls_empty, 0x018E, 144, 0, 0, 0},    {0x0001, presentation_pres_text.controls_empty, 0x018E, 152, 0, 0, 0},      {0x0001, presentation_pres_text.controls_empty, 0x018E, 160, 0, 0, 0},          {0x0001, presentation_pres_text.controls_empty, 0x018E, 168, 0, 0, 0},      {0x0001, presentation_pres_text.controls_empty, 0x018E, 176, 0, 0, 0},     {0x0001, presentation_pres_text.controls_empty, 0x018E, 184, 0, 0, 0},      {0x0001, presentation_pres_text.controls_empty, 0x018E, 192, 0, 0, 0},        {0x0001, presentation_pres_text.controls_empty, 0x018E, 200, 0, 0, 0},
                {0x0081, presentation_pres_text.controls_empty, 0x018E, 128, 0, 0, 0},     {0x0081, presentation_pres_text.controls_empty, 0x018E, 136, 0, 0, 0},          {0x0081, presentation_pres_text.controls_empty, 0x018E, 144, 0, 0, 0},        {0x0080, presentation_pres_text.controls_pad_1, 0x00B0, 48, 3, 0, 0},     {0x0080, presentation_pres_text.controls_pad_2, 0x018C, 48, 3, 0, 0},       {0x8000, presentation_pres_text.controls_chris1, 0x0020, 200, 0, 0, 0},         {0x8000, presentation_pres_text.controls_chris2, 0x0020, 26, 0, 0, 0},      {0x8000, presentation_pres_text.controls_triangle, 0x0060, 208, 0, 0, 0},  {0x0000, presentation_pres_text.controls_back, 0x0075, 206, 2, 0, 0},       {0x0083, presentation_pres_text.controls_pad_cfg, 0x0120, 22, 2, 0, 0},
            },
        .quick =
            {
                {0x0082, presentation_pres_text.quick_track, 0x0100, 70, 0, 0, 0},
                {0x0082, presentation_pres_text.quick_car, 0x0100, 110, 0, 0, 0},
                {0x0082, presentation_pres_text.quick_race, 0x0100, 150, 0, 0, 0},
                {0x8080, presentation_pres_text.quick_srroleft, 0x00D4, 81, 0, 0, 0},
                {0x8080, presentation_pres_text.quick_srrorght, 0x012C, 81, 0, 0, 0},
                {0x8080, presentation_pres_text.quick_srroleft, 0x00E0, 121, 0, 0, 0},
                {0x8080, presentation_pres_text.quick_srrorght, 0x0120, 121, 0, 0, 0},
                {0x8000, presentation_pres_text.quick_chris1, 0x0020, 200, 0, 0, 0},
                {0x8000, presentation_pres_text.quick_chris2, 0x0020, 26, 0, 0, 0},
                {0x8000, presentation_pres_text.quick_toss, 0x01A4, 180, 0, 0, 0},
                {0x8000, presentation_pres_text.quick_cross, 0x0060, 200, 0, 0, 0},
                {0x0000, presentation_pres_text.quick_select, 0x0075, 198, 2, 0, 0},
                {0x8000, presentation_pres_text.quick_triangle, 0x00C8, 200, 0, 0, 0},
                {0x0000, presentation_pres_text.quick_back, 0x00DD, 198, 2, 0, 0},
                {0x0083, presentation_pres_text.quick_quick_race, 0x0100, 22, 2, 0, 0},
            },
        .main =
            {
                {0x0002, presentation_pres_text.quick_quick_race, 0x0060, 60, 0, 210, 0},
                {0x0002, presentation_pres_text.main_full_race, 0x0060, 80, 0, 70, 0},
                {0x0002, presentation_pres_text.main_two_player_race, 0x0060, 100, 0, 200, 0},
                {0x0002, presentation_pres_text.main_options, 0x0060, 120, 0, 40, 0},
                {0x0002, presentation_pres_text.main_high_scores, 0x0060, 140, 0, 120, 0},
                {0x0002, presentation_pres_text.main_credits, 0x0060, 160, 0, 60, 0},
                {0x0002, presentation_pres_text.main_fear_factory_video, 0x0060, 180, 0, 60, 0},
                {0x0081, presentation_pres_text.main_now_playing, 0x01A4, 100, 1, 0, 0},
                {0x0081, presentation_pres_text.main_empty, 0x01A4, 100, 1, 0, 0},
                {0x0080, presentation_pres_text.main_searching_for_prev, 0x0100, 200, 0, 0, 0},
                {0x0080, presentation_pres_text.main_testdrive_5_savegame_2, 0x0100, 208, 0, 0, 0},
                {0x0080, presentation_pres_text.main_loading_prev, 0x0100, 200, 0, 0, 0},
                {0x0080, presentation_pres_text.main_testdrive_5_savegame, 0x0100, 208, 0, 0, 0},
                {0x0080, presentation_pres_text.main_from_memory_card_slot_1, 0x0100, 216, 0, 0, 0},
                {0x0080, presentation_pres_text.main_from_memory_card_slot_2, 0x0100, 216, 0, 0, 0},
                {0x0080, presentation_pres_text.main_error_loading_prev, 0x0100, 200, 0, 0, 0},
                {0x8000, presentation_pres_text.quick_cross, 0x0186, 52, 0, 0, 0},
                {0x0000, presentation_pres_text.quick_select, 0x019B, 50, 2, 0, 0},
                {0x8000, presentation_pres_text.quick_chris1, 0x0020, 200, 0, 0, 0},
                {0x8000, presentation_pres_text.quick_chris2, 0x0020, 26, 0, 0, 0},
                {0x8000, presentation_pres_text.quick_toss, 0x01A4, 180, 0, 0, 0},
                {0x0083, presentation_pres_text.main_main_menu, 0x0100, 22, 2, 0, 0},
            },
        .multiplayer =
            {
                {0x0002, presentation_pres_text.multiplayer_single_race, 0x0060, 80, 0, 10, 0},
                {0x0002, presentation_pres_text.multiplayer_quad_race, 0x0060, 100, 0, 220, 0},
                {0x0002, presentation_pres_text.multiplayer_cop_chase, 0x0060, 120, 0, 10, 0},
                {0x0002, presentation_pres_text.multiplayer_drag_race, 0x0060, 140, 0, 20, 0},
                {0x8000, presentation_pres_text.quick_cross, 0x0060, 200, 0, 0, 0},
                {0x0000, presentation_pres_text.quick_select, 0x0075, 198, 2, 0, 0},
                {0x8000, presentation_pres_text.quick_triangle, 0x00C8, 200, 0, 0, 0},
                {0x0000, presentation_pres_text.quick_back, 0x00DD, 198, 2, 0, 0},
                {0x8000, presentation_pres_text.quick_chris1, 0x0020, 200, 0, 0, 0},
                {0x8000, presentation_pres_text.quick_chris2, 0x0020, 26, 0, 0, 0},
                {0x8000, presentation_pres_text.quick_toss, 0x01A4, 180, 0, 0, 0},
                {0x0083, presentation_pres_text.multiplayer_two_player_race_type, 0x0100, 22, 2, 0, 0},
            },
        .single_event =
            {
                {0x0002, presentation_pres_text.multiplayer_single_race, 0x0060, 60, 0, 10, 0},
                {0x0002, presentation_pres_text.single_event_time_trials, 0x0060, 80, 0, 190, 0},
                {0x0002, presentation_pres_text.single_event_cup_race, 0x0060, 100, 0, 150, 0},
                {0x0002, presentation_pres_text.single_event_continue_cup, 0x0060, 120, 0, 130, 0},
                {0x0002, presentation_pres_text.multiplayer_drag_race, 0x0060, 140, 0, 20, 0},
                {0x0002, presentation_pres_text.multiplayer_cop_chase, 0x0060, 160, 0, 190, 0},
                {0x8000, presentation_pres_text.quick_cross, 0x0060, 200, 0, 0, 0},
                {0x0000, presentation_pres_text.quick_select, 0x0075, 198, 2, 0, 0},
                {0x8000, presentation_pres_text.quick_triangle, 0x00C8, 200, 0, 0, 0},
                {0x0000, presentation_pres_text.quick_back, 0x00DD, 198, 2, 0, 0},
                {0x8000, presentation_pres_text.quick_chris1, 0x0020, 200, 0, 0, 0},
                {0x8000, presentation_pres_text.quick_chris2, 0x0020, 26, 0, 0, 0},
                {0x8000, presentation_pres_text.quick_toss, 0x01A4, 180, 0, 0, 0},
                {0x0083, presentation_pres_text.single_event_select_race_type, 0x0100, 22, 2, 0, 0},
            },
        .cup =
            {
                {0x0002, presentation_pres_text.cup_championship_cup, 0x0060, 60, 0, 190, 0},
                {0x0002, presentation_pres_text.cup_era_cup_beauty, 0x0060, 80, 0, 190, 0},
                {0x0002, presentation_pres_text.cup_challenge_cup, 0x0060, 110, 0, 190, 0},
                {0x0002, presentation_pres_text.cup_pitbull_cup, 0x0060, 130, 0, 190, 0},
                {0x0002, presentation_pres_text.cup_masters_cup, 0x0060, 160, 0, 190, 0},
                {0x0002, presentation_pres_text.cup_ultimate_cup, 0x0060, 180, 0, 190, 0},
                {0x8000, presentation_pres_text.quick_cross, 0x0060, 200, 0, 0, 0},
                {0x0000, presentation_pres_text.quick_select, 0x0075, 198, 2, 0, 0},
                {0x8000, presentation_pres_text.quick_triangle, 0x00C8, 200, 0, 0, 0},
                {0x0000, presentation_pres_text.quick_back, 0x00DD, 198, 2, 0, 0},
                {0x8000, presentation_pres_text.quick_chris1, 0x0020, 200, 0, 0, 0},
                {0x8000, presentation_pres_text.quick_chris2, 0x0020, 26, 0, 0, 0},
                {0x8000, presentation_pres_text.quick_toss, 0x01A4, 180, 0, 0, 0},
                {0x0083, presentation_pres_text.cup_choose_cup_to_enter, 0x0100, 22, 2, 0, 0},
            },
        .options =
            {
                {0x0002, presentation_pres_text.options_pad_cfg, 0x0060, 70, 0, 160, 0},
                {0x0002, presentation_pres_text.options_audio_cfg, 0x0060, 90, 0, 90, 0},
                {0x0002, presentation_pres_text.options_scr_cfg, 0x0060, 110, 0, 100, 0},
                {0x0002, presentation_pres_text.options_game_options, 0x0060, 130, 0, 110, 0},
                {0x8000, presentation_pres_text.quick_cross, 0x0060, 200, 0, 0, 0},
                {0x0000, presentation_pres_text.quick_select, 0x0075, 198, 2, 0, 0},
                {0x8000, presentation_pres_text.quick_triangle, 0x00C8, 200, 0, 0, 0},
                {0x0000, presentation_pres_text.quick_back, 0x00DD, 198, 2, 0, 0},
                {0x8000, presentation_pres_text.quick_chris1, 0x0020, 200, 0, 0, 0},
                {0x8000, presentation_pres_text.quick_chris2, 0x0020, 26, 0, 0, 0},
                {0x8000, presentation_pres_text.quick_toss, 0x01A4, 180, 0, 0, 0},
                {0x0083, presentation_pres_text.options_main_options, 0x0100, 22, 2, 0, 0},
            },
        .audio =
            {
                {0x0002, presentation_pres_text.audio_music_volume, 0x0060, 60, 0, 0, 0},
                {0x0002, presentation_pres_text.audio_effect_volume, 0x0060, 110, 0, 0, 0},
                {0x0002, presentation_pres_text.audio_surround_on, 0x0060, 150, 0, 0, 0},
                {0x8000, presentation_pres_text.quick_triangle, 0x0060, 200, 0, 0, 0},
                {0x0000, presentation_pres_text.quick_back, 0x0075, 198, 2, 0, 0},
                {0x8000, presentation_pres_text.quick_chris1, 0x0020, 200, 0, 0, 0},
                {0x8000, presentation_pres_text.quick_chris2, 0x0020, 26, 0, 0, 0},
                {0x8000, presentation_pres_text.quick_toss, 0x01A4, 180, 0, 0, 0},
                {0x0083, presentation_pres_text.options_audio_cfg, 0x0100, 22, 2, 0, 0},
            },
        .scr =
            {
                {0x0080, presentation_pres_text.scr_up, 0x0100, 96, 0, 0, 0},
                {0x0080, presentation_pres_text.scr_down, 0x0100, 160, 0, 0, 0},
                {0x0080, presentation_pres_text.scr_left, 0x00C0, 128, 0, 0, 0},
                {0x0080, presentation_pres_text.scr_right, 0x0140, 128, 0, 0, 0},
                {0x8000, presentation_pres_text.quick_cross, 0x0060, 200, 0, 0, 0},
                {0x0000, presentation_pres_text.quick_select, 0x0075, 198, 2, 0, 0},
                {0x8000, presentation_pres_text.quick_triangle, 0x00C8, 200, 0, 0, 0},
                {0x0000, presentation_pres_text.quick_back, 0x00DD, 198, 2, 0, 0},
                {0x8000, presentation_pres_text.quick_chris1, 0x0020, 200, 0, 0, 0},
                {0x8000, presentation_pres_text.quick_chris2, 0x0020, 26, 0, 0, 0},
                {0x8000, presentation_pres_text.quick_toss, 0x01A4, 180, 0, 0, 0},
                {0x0083, presentation_pres_text.options_scr_cfg, 0x0100, 22, 2, 0, 0},
            },
        .settings =
            {
                {0x0000, presentation_pres_text.settings_speed_readout_mph, 0x0060, 50, 0, 0, 0},
                {0x0000, presentation_pres_text.settings_checkpoint_timers_on, 0x0060, 62, 0, 0, 0},
                {0x0000, presentation_pres_text.settings_traffic_on, 0x0060, 74, 0, 0, 0},
                {0x0000, presentation_pres_text.settings_cops_on, 0x0060, 86, 0, 0, 0},
                {0x0000, presentation_pres_text.settings_difficulty_n, 0x0060, 98, 0, 0, 0},
                {0x0000, presentation_pres_text.settings_dynamics_simulation, 0x0060, 110, 0, 0, 0},
                {0x0000, presentation_pres_text.settings_3d_collisions_on, 0x0060, 122, 0, 0, 0},
                {0x0000, presentation_pres_text.settings_two_player_catchup_000, 0x0060, 134, 0, 0, 0},
                {0x0000, presentation_pres_text.settings_vibration_off, 0x0060, 146, 0, 0, 0},
                {0x0000, presentation_pres_text.settings_camera_damping_000, 0x0060, 158, 0, 0, 0},
                {0x0000, presentation_pres_text.settings_circuit_laps_000, 0x0060, 170, 0, 0, 0},
                {0x8000, presentation_pres_text.quick_triangle, 0x0060, 200, 0, 0, 0},
                {0x0000, presentation_pres_text.quick_back, 0x0075, 198, 2, 0, 0},
                {0x8000, presentation_pres_text.quick_chris1, 0x0020, 200, 0, 0, 0},
                {0x8000, presentation_pres_text.quick_chris2, 0x0020, 26, 0, 0, 0},
                {0x8000, presentation_pres_text.quick_toss, 0x01A4, 180, 0, 0, 0},
                {0x0083, presentation_pres_text.options_game_options, 0x0100, 22, 2, 0, 0},
            },
        .track =
            {
                {0x8000, presentation_pres_text.quick_chris1, 0x0020, 200, 0, 0, 0},
                {0x8000, presentation_pres_text.quick_chris2, 0x0020, 26, 0, 0, 0},
                {0x8000, presentation_pres_text.quick_toss, 0x01A4, 180, 0, 0, 0},
                {0x8080, presentation_pres_text.track_arroleft, 0x0080, 128, 0, 0, 0},
                {0x8080, presentation_pres_text.track_arrorght, 0x0180, 128, 0, 0, 0},
                {0x8000, presentation_pres_text.quick_cross, 0x0060, 200, 0, 0, 0},
                {0x0000, presentation_pres_text.quick_select, 0x0075, 198, 2, 0, 0},
                {0x8000, presentation_pres_text.quick_triangle, 0x00C8, 200, 0, 0, 0},
                {0x0000, presentation_pres_text.quick_back, 0x00DD, 198, 2, 0, 0},
                {0x0083, presentation_pres_text.track_select_track, 0x0100, 22, 2, 0, 0},
            },
        .multiplayer_track =
            {
                {0x8000, presentation_pres_text.quick_chris1, 0x0020, 200, 0, 0, 0},
                {0x8000, presentation_pres_text.quick_chris2, 0x0020, 26, 0, 0, 0},
                {0x8000, presentation_pres_text.quick_cross, 0x0060, 208, 0, 0, 0},
                {0x0000, presentation_pres_text.quick_select, 0x0075, 206, 2, 0, 0},
                {0x8000, presentation_pres_text.quick_triangle, 0x00C8, 208, 0, 0, 0},
                {0x0000, presentation_pres_text.quick_back, 0x00DD, 206, 2, 0, 0},
                {0x0083, presentation_pres_text.multiplayer_track_select_tracks, 0x0100, 22, 2, 0, 0},
            },
        .credits =
            {
                {0x8080, presentation_pres_text.credits_hicar, 0x0100, 100, 0, 0, 0},
                {0x8000, presentation_pres_text.quick_toss, 0x0020, 180, 0, 0, 0},
            },
        .records =
            {
                {0x8080, presentation_pres_text.track_arroleft, 0x0064, 70, 0, 0, 0},
                {0x8080, presentation_pres_text.track_arrorght, 0x019C, 70, 0, 0, 0},
                {0x8000, presentation_pres_text.quick_triangle, 0x0060, 206, 0, 0, 0},
                {0x0000, presentation_pres_text.quick_back, 0x0075, 204, 2, 0, 0},
                {0x0083, presentation_pres_text.records_highscores, 0x0100, 22, 2, 0, 0},
            },
        .player_name =
            {
                {0x0082, presentation_pres_text.player_name_a_2, 0x00AC, 96, 2, 0, 0},
                {0x0082, presentation_pres_text.player_name_b_2, 0x00C4, 96, 2, 0, 0},
                {0x0082, presentation_pres_text.player_name_c, 0x00DC, 96, 2, 0, 0},
                {0x0082, presentation_pres_text.player_name_d, 0x00F4, 96, 2, 0, 0},
                {0x0082, presentation_pres_text.player_name_e, 0x010C, 96, 2, 0, 0},
                {0x0082, presentation_pres_text.player_name_f, 0x0124, 96, 2, 0, 0},
                {0x0082, presentation_pres_text.player_name_g, 0x013C, 96, 2, 0, 0},
                {0x0082, presentation_pres_text.player_name_h, 0x0154, 96, 2, 0, 0},
                {0x0082, presentation_pres_text.player_name_i, 0x00AC, 116, 2, 0, 0},
                {0x0082, presentation_pres_text.player_name_j, 0x00C4, 116, 2, 0, 0},
                {0x0082, presentation_pres_text.player_name_k, 0x00DC, 116, 2, 0, 0},
                {0x0082, presentation_pres_text.player_name_l, 0x00F4, 116, 2, 0, 0},
                {0x0082, presentation_pres_text.player_name_m, 0x010C, 116, 2, 0, 0},
                {0x0082, presentation_pres_text.player_name_n, 0x0124, 116, 2, 0, 0},
                {0x0082, presentation_pres_text.player_name_o, 0x013C, 116, 2, 0, 0},
                {0x0082, presentation_pres_text.player_name_p, 0x0154, 116, 2, 0, 0},
                {0x0082, presentation_pres_text.player_name_q, 0x00AC, 136, 2, 0, 0},
                {0x0082, presentation_pres_text.player_name_r, 0x00C4, 136, 2, 0, 0},
                {0x0082, presentation_pres_text.player_name_s, 0x00DC, 136, 2, 0, 0},
                {0x0082, presentation_pres_text.player_name_t, 0x00F4, 136, 2, 0, 0},
                {0x0082, presentation_pres_text.player_name_u, 0x010C, 136, 2, 0, 0},
                {0x0082, presentation_pres_text.player_name_v, 0x0124, 136, 2, 0, 0},
                {0x0082, presentation_pres_text.player_name_w, 0x013C, 136, 2, 0, 0},
                {0x0082, presentation_pres_text.player_name_x, 0x0154, 136, 2, 0, 0},
                {0x0082, presentation_pres_text.player_name_y, 0x00AC, 156, 2, 0, 0},
                {0x0082, presentation_pres_text.player_name_z, 0x00C4, 156, 2, 0, 0},
                {0x0082, presentation_pres_text.player_name_empty_4, 0x00DC, 156, 2, 0, 0},
                {0x0082, presentation_pres_text.player_name_empty_3, 0x00F4, 156, 2, 0, 0},
                {0x0082, presentation_pres_text.player_name_empty_2, 0x010C, 156, 2, 0, 0},
                {0x0082, presentation_pres_text.player_name_empty, 0x0124, 156, 2, 0, 0},
                {0x0082, presentation_pres_text.player_name_b, 0x013C, 156, 2, 0, 0},
                {0x0082, presentation_pres_text.player_name_a, 0x0154, 156, 2, 0, 0},
                {0x0082, presentation_pres_text.main_empty, 0x0100, 64, 0, 0, 0},
                {0x8000, presentation_pres_text.quick_chris1, 0x0020, 200, 0, 0, 0},
                {0x8000, presentation_pres_text.quick_chris2, 0x0020, 26, 0, 0, 0},
                {0x8000, presentation_pres_text.quick_toss, 0x01A4, 180, 0, 0, 0},
                {0x8000, presentation_pres_text.quick_cross, 0x0060, 200, 0, 0, 0},
                {0x0000, presentation_pres_text.quick_select, 0x0075, 198, 2, 0, 0},
                {0x8000, presentation_pres_text.quick_triangle, 0x00C8, 200, 0, 0, 0},
                {0x0000, presentation_pres_text.quick_back, 0x00DD, 198, 2, 0, 0},
                {0x0083, presentation_pres_text.player_name_enter_your_name, 0x0100, 22, 2, 0, 0},
            },
        .event_success =
            {
                {0x0083, presentation_pres_text.event_success_player_1, 0x0100, 40, 2, 0, 0},
                {0x0083, presentation_pres_text.event_success_congratulations, 0x0100, 60, 2, 0, 0},
                {0x0083, presentation_pres_text.event_success_you_have_won_the, 0x0100, 80, 2, 0, 0},
                {0x0083, presentation_pres_text.event_success_test_drive_5, 0x0100, 100, 2, 0, 0},
                {0x0083, presentation_pres_text.event_success_quad_cup, 0x0100, 120, 2, 0, 0},
                {0x8000, presentation_pres_text.quick_cross, 0x0038, 200, 0, 0, 0},
                {0x0000, presentation_pres_text.event_success_advance, 0x004D, 198, 2, 0, 0},
            },
        .event_failure =
            {
                {0x0083, presentation_pres_text.event_failure_sorry, 0x0100, 60, 2, 0, 0},
                {0x0083, presentation_pres_text.event_failure_you_have_failed_to_win, 0x0100, 80, 2, 0, 0},
                {0x0083, presentation_pres_text.event_failure_the_test_drive_5, 0x0100, 100, 2, 0, 0},
                {0x0083, presentation_pres_text.event_success_quad_cup, 0x0100, 120, 2, 0, 0},
                {0x8000, presentation_pres_text.quick_cross, 0x0038, 200, 0, 0, 0},
                {0x0000, presentation_pres_text.event_success_advance, 0x004D, 198, 2, 0, 0},
            },
        .cup_success =
            {
                {0x0083, presentation_pres_text.event_success_congratulations, 0x0100, 40, 2, 0, 0},
                {0x0083, presentation_pres_text.event_success_you_have_won_the, 0x0100, 60, 2, 0, 0},
                {0x0083, presentation_pres_text.event_success_test_drive_5, 0x0100, 80, 2, 0, 0},
                {0x0083, presentation_pres_text.main_empty, 0x0100, 100, 2, 0, 0},
                {0x0081, presentation_pres_text.cup_success_you_have_unlocked, 0x0100, 130, 3, 0, 0},
                {0x0081, presentation_pres_text.main_empty, 0x0100, 140, 0, 0, 0},
                {0x0081, presentation_pres_text.main_empty, 0x0100, 150, 0, 0, 0},
                {0x0081, presentation_pres_text.main_empty, 0x0100, 160, 0, 0, 0},
                {0x0081, presentation_pres_text.main_empty, 0x0100, 170, 0, 0, 0},
                {0x0081, presentation_pres_text.main_empty, 0x0100, 180, 0, 0, 0},
                {0x0081, presentation_pres_text.main_empty, 0x0100, 190, 0, 0, 0},
                {0x0081, presentation_pres_text.main_empty, 0x0100, 200, 0, 0, 0},
                {0x0081, presentation_pres_text.main_empty, 0x0100, 210, 0, 0, 0},
                {0x0081, presentation_pres_text.main_empty, 0x0100, 220, 0, 0, 0},
                {0x8000, presentation_pres_text.quick_cross, 0x0038, 200, 0, 0, 0},
                {0x0000, presentation_pres_text.event_success_advance, 0x004D, 198, 2, 0, 0},
            },
        .cup_failure =
            {
                {0x0083, presentation_pres_text.event_failure_sorry, 0x0100, 60, 2, 0, 0},
                {0x0083, presentation_pres_text.event_failure_you_have_failed_to_win, 0x0100, 80, 2, 0, 0},
                {0x0083, presentation_pres_text.event_failure_the_test_drive_5, 0x0100, 100, 2, 0, 0},
                {0x0083, presentation_pres_text.main_empty, 0x0100, 120, 2, 0, 0},
                {0x8000, presentation_pres_text.quick_cross, 0x0038, 200, 0, 0, 0},
                {0x0000, presentation_pres_text.quick_select, 0x004D, 198, 2, 0, 0},
            },
};

const PRESENTATION_CARSEL_RECORDS presentation_carsel_rec_defaults = {
    .car =
        {
            {0x8080, presentation_carsel_text.car_cd, 0x0100, 96, 0, 0, 0},
            {0x8080, presentation_carsel_text.car_arroleft, 0x0040, 96, 0, 0, 0},
            {0x8080, presentation_carsel_text.car_arrorght, 0x01C0, 96, 0, 0, 0},
            {0x8080, presentation_carsel_text.car_srrorght, 0x012C, 81, 0, 0, 0},
            {0x8080, presentation_carsel_text.car_srroleft, 0x00D4, 81, 0, 0, 0},
            {0x8000, presentation_carsel_text.car_cross, 0x0010, 48, 0, 0, 0},
            {0x0000, presentation_carsel_text.car_select, 0x0025, 46, 2, 0, 0},
            {0x8000, presentation_carsel_text.car_triangle, 0x0010, 64, 0, 0, 0},
            {0x0000, presentation_carsel_text.car_back, 0x0025, 62, 2, 0, 0},
        },
    .card =
        {
            {0x0081, presentation_carsel_text.card_load, 0x0100, 90, 0, 0, 0},
            {0x0081, presentation_carsel_text.card_save, 0x0100, 110, 0, 0, 0},
            {0x0081, presentation_carsel_text.card_delete, 0x0100, 130, 0, 0, 0},
            {0x0081, presentation_carsel_text.card_message_window_1, 0x0100, 168, 0, 0, 0},
            {0x0081, presentation_carsel_text.card_message_window_2, 0x0100, 180, 0, 0, 0},
            {0x0081, presentation_carsel_text.card_memory_card_1, 0x0060, 64, 3, 0, 0},
            {0x0081, presentation_carsel_text.card_memory_card_2, 0x01A0, 64, 3, 0, 0},
            {0x0081, presentation_carsel_text.card_1memory_card_status, 0x0060, 76, 3, 0, 0},
            {0x0081, presentation_carsel_text.card_2memory_card_status, 0x01A0, 76, 3, 0, 0},
            {0x0081, presentation_carsel_text.card_empty_2, 0x00F0, 192, 0, 0, 0},
            {0x0081, presentation_carsel_text.card_empty, 0x0110, 192, 0, 0, 0},
            {0x8000, presentation_carsel_text.card_sq01, 0x0020, 90, 0, 0, 0},
            {0x8000, presentation_carsel_text.card_sq02, 0x0039, 90, 0, 0, 0},
            {0x8000, presentation_carsel_text.card_sq03, 0x0052, 90, 0, 0, 0},
            {0x8000, presentation_carsel_text.card_sq04, 0x006B, 90, 0, 0, 0},
            {0x8000, presentation_carsel_text.card_sq05, 0x0084, 90, 0, 0, 0},
            {0x8000, presentation_carsel_text.card_sq11, 0x0173, 90, 0, 0, 0},
            {0x8000, presentation_carsel_text.card_sq12, 0x018C, 90, 0, 0, 0},
            {0x8000, presentation_carsel_text.card_sq13, 0x01A5, 90, 0, 0, 0},
            {0x8000, presentation_carsel_text.card_sq14, 0x01BE, 90, 0, 0, 0},
            {0x8000, presentation_carsel_text.card_sq15, 0x01D7, 90, 0, 0, 0},
            {0x8000, presentation_carsel_text.card_sq06, 0x0020, 110, 0, 0, 0},
            {0x8000, presentation_carsel_text.card_sq07, 0x0039, 110, 0, 0, 0},
            {0x8000, presentation_carsel_text.card_sq08, 0x0052, 110, 0, 0, 0},
            {0x8000, presentation_carsel_text.card_sq09, 0x006B, 110, 0, 0, 0},
            {0x8000, presentation_carsel_text.card_sq0a, 0x0084, 110, 0, 0, 0},
            {0x8000, presentation_carsel_text.card_sq16, 0x0173, 110, 0, 0, 0},
            {0x8000, presentation_carsel_text.card_sq17, 0x018C, 110, 0, 0, 0},
            {0x8000, presentation_carsel_text.card_sq18, 0x01A5, 110, 0, 0, 0},
            {0x8000, presentation_carsel_text.card_sq19, 0x01BE, 110, 0, 0, 0},
            {0x8000, presentation_carsel_text.card_sq1a, 0x01D7, 110, 0, 0, 0},
            {0x8000, presentation_carsel_text.card_sq0b, 0x0020, 130, 0, 0, 0},
            {0x8000, presentation_carsel_text.card_sq0c, 0x0039, 130, 0, 0, 0},
            {0x8000, presentation_carsel_text.card_sq0d, 0x0052, 130, 0, 0, 0},
            {0x8000, presentation_carsel_text.card_sq0e, 0x006B, 130, 0, 0, 0},
            {0x8000, presentation_carsel_text.card_sq0f, 0x0084, 130, 0, 0, 0},
            {0x8000, presentation_carsel_text.card_sq1b, 0x0173, 130, 0, 0, 0},
            {0x8000, presentation_carsel_text.card_sq1c, 0x018C, 130, 0, 0, 0},
            {0x8000, presentation_carsel_text.card_sq1d, 0x01A5, 130, 0, 0, 0},
            {0x8000, presentation_carsel_text.card_sq1e, 0x01BE, 130, 0, 0, 0},
            {0x8000, presentation_carsel_text.card_sq1f, 0x01D7, 130, 0, 0, 0},
            {0x8000, presentation_carsel_text.card_cross, 0x0060, 200, 0, 0, 0},
            {0x0000, presentation_carsel_text.card_select, 0x0075, 198, 2, 0, 0},
            {0x8000, presentation_carsel_text.card_triangle, 0x00C8, 200, 0, 0, 0},
            {0x0000, presentation_carsel_text.card_back, 0x00DD, 198, 2, 0, 0},
            {0x0082, presentation_carsel_text.card_memory_card_manager, 0x0100, 22, 2, 0, 0},
        },
};

const PRESENTATION_RECORD_LIST presentation_rec_lists[PRESENTATION_LIST_COUNT] = {
    [PRESENTATION_LIST_CONTROLS] = {presentation_pres_recs.controls, 65},
    [PRESENTATION_LIST_DEBUG] = {NULL, 0},
    [PRESENTATION_LIST_QUICK] = {presentation_pres_recs.quick, 15},
    [PRESENTATION_LIST_MAIN] = {presentation_pres_recs.main, 22},
    [PRESENTATION_LIST_MULTIPLAYER] = {presentation_pres_recs.multiplayer, 12},
    [PRESENTATION_LIST_SINGLE_EVENT] = {presentation_pres_recs.single_event, 14},
    [PRESENTATION_LIST_CUP] = {presentation_pres_recs.cup, 14},
    [PRESENTATION_LIST_OPTIONS] = {presentation_pres_recs.options, 12},
    [PRESENTATION_LIST_AUDIO] = {presentation_pres_recs.audio, 9},
    [PRESENTATION_LIST_SCREEN] = {presentation_pres_recs.scr, 12},
    [PRESENTATION_LIST_SETTINGS] = {presentation_pres_recs.settings, 17},
    [PRESENTATION_LIST_TRACK] = {presentation_pres_recs.track, 10},
    [PRESENTATION_LIST_MULTIPLAYER_TRACK] = {presentation_pres_recs.multiplayer_track, 7},
    [PRESENTATION_LIST_CREDITS] = {presentation_pres_recs.credits, 2},
    [PRESENTATION_LIST_RECORDS] = {presentation_pres_recs.records, 5},
    [PRESENTATION_LIST_PLAYER_NAME] = {presentation_pres_recs.player_name, 41},
    [PRESENTATION_LIST_EVENT_SUCCESS] = {presentation_pres_recs.event_success, 7},
    [PRESENTATION_LIST_EVENT_FAILURE] = {presentation_pres_recs.event_failure, 6},
    [PRESENTATION_LIST_CUP_SUCCESS] = {presentation_pres_recs.cup_success, 16},
    [PRESENTATION_LIST_CUP_FAILURE] = {presentation_pres_recs.cup_failure, 6},
    [PRESENTATION_LIST_CAR] = {presentation_carsel_recs.car, 9},
    [PRESENTATION_LIST_CARD] = {presentation_carsel_recs.card, 46},
};

sint32 presentation_move_menu_selection(sint32 original, sint32 count)
{
    const PRESENTATION_RECORD_LIST *list;
    sint32 sel, candidate;

    FUNCTION_MARKER(0x8005736C, "SLUS_006.10");
    list = presentation_menu.records;
    if (list == NULL || count <= 0 || list->count == 0)
    {
        presentation_menu.sel = 0xFFFFFFFF;
        return -1;
    }
    if ((uint32)count > list->count)
        count = (sint32)list->count;
    sel = original;
    if (sel < 0 || sel >= count || (list->entries[sel].x_flags & 0xC000) != 0)
    {
        for (sel = 0; sel < count; ++sel)
            if ((list->entries[sel].x_flags & 0xC000) == 0)
                break;
        if (sel == count)
        {
            presentation_menu.sel = 0xFFFFFFFF;
            return -1;
        }
    }
    if ((input_ports[0].pressed & 0x10) != 0)
    {
        candidate = sel - 1;
        while (candidate >= 0 && (list->entries[candidate].x_flags & 0xC000) != 0)
            --candidate;
        if (candidate >= 0)
            sel = candidate;
    }
    if ((input_ports[0].pressed & 0x40) != 0)
    {
        candidate = sel + 1;
        while (candidate < count && (list->entries[candidate].x_flags & 0xC000) != 0)
            ++candidate;
        if (candidate < count)
            sel = candidate;
    }
    if (original != sel)
        audio_play_menu_move();
    presentation_menu.sel = (uint32)sel;
    return sel;
}

static uint32 activate_menu_selection(sint32 sel, sint32 count)
{
    sint32 idx;
    uint32 val;
    const PRESENTATION_RECORD_LIST *list;
    const PRESENTATION_RECORD *item;
    uint32 argument;

    FUNCTION_MARKER(0x800574F0, "SLUS_006.10");
    if (sel < 0)
        idx = 0;
    else
    {
        idx = ((uint32)count - 1);
        if (idx >= sel)
            idx = sel;
    }
    list = presentation_menu.records;
    val = input_ports[0].pressed;
    if ((val & 0x4008) != 0 && count > 0 && sel >= 0 && list != NULL && (uint32)idx < list->count && (list->entries[idx].x_flags & 0xC000) == 0)
    {
        item = list->entries + idx;
        argument = item->action;
        if (argument != 0)
        {
            session_push_menu(argument);
            audio_play_menu_accept();
        }
    }
    val = input_ports[0].pressed & 0x1000;
    if (val != 0)
    {
        session_pop_menu();
        return (uint32)audio_play_menu_accept();
    }
    return val;
}

static void update_menu_flags(void)
{
    uint32 flags0, flags1, flags2, flags3, flags4, flags5, flags6, enabled;

    FUNCTION_MARKER(0x800ADE68, "SLUS_006.10");
    flags0 = presentation_pres_recs.single_event[5].x_flags;
    flags1 = presentation_pres_recs.multiplayer[2].x_flags;
    flags2 = presentation_pres_recs.cup[2].x_flags;
    flags3 = presentation_pres_recs.cup[3].x_flags;
    flags4 = presentation_pres_recs.cup[4].x_flags;
    flags5 = presentation_pres_recs.cup[5].x_flags;
    flags6 = presentation_pres_recs.main[6].x_flags;
    enabled = progression_profile.special_model_30;
    flags0 |= 0x8000;
    flags1 |= 0x8000;
    flags2 |= 0x8000;
    flags3 |= 0x8000;
    flags4 |= 0x4000;
    flags5 |= 0x4000;
    flags6 |= 0x4000;
    presentation_pres_recs.single_event[5].x_flags = (uint16)(flags0);
    presentation_pres_recs.multiplayer[2].x_flags = (uint16)(flags1);
    presentation_pres_recs.cup[2].x_flags = (uint16)(flags2);
    presentation_pres_recs.cup[3].x_flags = (uint16)(flags3);
    presentation_pres_recs.cup[4].x_flags = (uint16)(flags4);
    presentation_pres_recs.cup[5].x_flags = (uint16)(flags5);
    presentation_pres_recs.main[6].x_flags = (uint16)(flags6);
    if (enabled != 0)
        presentation_pres_recs.main[6].x_flags = (uint16)(flags6 & 0x3FFF);
    if (progression_profile.features[0] != 0)
    {
        presentation_pres_recs.cup[2].x_flags = (uint16)(flags2 & 0x3FFF);
        presentation_pres_recs.cup[4].x_flags = (uint16)((flags4 & 0x3FFF) | 0x8000);
    }
    if (progression_profile.features[1] != 0)
    {
        presentation_pres_recs.cup[3].x_flags = (uint16)(flags3 & 0x3FFF);
        presentation_pres_recs.cup[5].x_flags = (uint16)((flags5 & 0x3FFF) | 0x8000);
    }
    if (progression_profile.features[2] != 0)
        presentation_pres_recs.cup[4].x_flags = (uint16)(presentation_pres_recs.cup[4].x_flags & 0x3FFF);
    if (progression_profile.features[3] != 0)
        presentation_pres_recs.cup[5].x_flags = (uint16)(presentation_pres_recs.cup[5].x_flags & 0x3FFF);
    if (progression_profile.features[4] != 0)
    {
        presentation_pres_recs.single_event[5].x_flags = (uint16)(flags0 & 0x3FFF);
        presentation_pres_recs.multiplayer[2].x_flags = (uint16)(flags1 & 0x3FFF);
    }
}

static uint32 store_text(char *dst, const char *text)
{
    uint32 idx = 0;

    do
    {
        dst[idx] = text[idx];
    } while (text[idx++] != 0);
    return idx - 1;
}

uint32 presentation_format_number(char *dst, const char *format, uint32 val)
{
    char text[64];

    sprintf(text, format, (sint32)val);
    return store_text(dst, text);
}

uint32 presentation_format_time(char *dst, const char *format, uint32 time, sint32 seconds_unit)
{
    sint32 minutes, seconds, fraction;
    uint32 remainder;
    char text[64];

    minutes = (sint32)time / (seconds_unit * 60);
    remainder = time - (uint32)minutes * (uint32)(seconds_unit * 60);
    seconds = (sint32)remainder / seconds_unit;
    fraction = (remainder - (uint32)seconds * (uint32)seconds_unit);
    sprintf(text, format, minutes, seconds, fraction);
    return store_text(dst, text);
}

char *presentation_find_text(char *text, const char *needle)
{
    return strstr(text, needle);
}

char *presentation_copy_text(char *dst, const char *src)
{
    strcpy(dst, src);
    return dst;
}

PRESENTATION_PRES_TEXT presentation_pres_text;
PRESENTATION_CARSEL_TEXT presentation_carsel_text;
/* Native owned menu strings preserve accepted resource defaults */
const PRESENTATION_PRES_TEXT presentation_pres_text_defaults = {
    .controls_empty = "",
    .controls_pad_cfg = "CONTROLLER CONFIGURATION",
    .controls_back = "BACK",
    .controls_triangle = "Triangle",
    .controls_chris2 = "Chris2",
    .controls_chris1 = "Chris1",
    .controls_pad_2 = "CONTROLLER 2",
    .controls_pad_1 = "CONTROLLER 1",
    .controls_catzmad = "CATZMAD",
    .controls_senistivity_1 = "SENISTIVITY 1",
    .controls_deadzone_1 = "DEADZONE 1                 ",
    .controls_cfg_1 = "CONFIGURATION 1                                ",
    .controls_camera = "}CAMERA",
    .controls_horn = "}HORN",
    .controls_rear_view = "}REAR VIEW",
    .controls_hand_brake = "}HAND BRAKE",
    .controls_gear_down = "}GEAR DOWN",
    .controls_gear_up = "}GEAR UP",
    .controls_steer_right = "}STEER RIGHT",
    .controls_steer_left = "}STEER LEFT",
    .controls_brake = "}BRAKE",
    .controls_accelerate = "}ACCELERATE",
    .controls_no_pad_present = "NO CONTROLLER PRESENT",
    .controls_ctazmad = "CTAZMAD",
    .controls_calibrate = "CALIBRATE",
    .controls_senistivity_0 = "SENISTIVITY 0",
    .controls_deadzone_0 = "DEADZONE 0                 ",
    .controls_cfg_0 = "CONFIGURATION 0                               ",
    .quick_quick_race = "QUICK RACE",
    .quick_back = "BACK",
    .quick_triangle = "Triangle",
    .quick_select = "SELECT",
    .quick_cross = "cross",
    .quick_toss = "Toss",
    .quick_chris2 = "Chris2",
    .quick_chris1 = "Chris1",
    .quick_srrorght = "SrroRght",
    .quick_srroleft = "SrroLeft",
    .quick_race = "RACE",
    .quick_car = "CAR",
    .quick_track = "TRACK",
    .main_main_menu = "MAIN MENU",
    .main_error_loading_prev = "ERROR LOADING PREVIOUS",
    .main_from_memory_card_slot_2 = "FROM MEMORY CARD SLOT 2.",
    .main_from_memory_card_slot_1 = "FROM MEMORY CARD SLOT 1.",
    .main_testdrive_5_savegame = "TESTDRIVE 5 SAVEGAME",
    .main_loading_prev = "LOADING PREVIOUS",
    .main_testdrive_5_savegame_2 = "TESTDRIVE 5 SAVEGAME.",
    .main_searching_for_prev = "SEARCHING FOR PREVIOUS",
    .main_empty = "",
    .main_now_playing = "NOW PLAYING",
    .main_fear_factory_video = "FEAR FACTORY VIDEO",
    .main_credits = "CREDITS",
    .main_high_scores = "HIGH SCORES",
    .main_options = "OPTIONS",
    .main_two_player_race = "TWO PLAYER RACE",
    .main_full_race = "FULL RACE",
    .multiplayer_two_player_race_type = "TWO PLAYER RACE TYPE",
    .multiplayer_drag_race = "DRAG RACE",
    .multiplayer_cop_chase = "COP CHASE",
    .multiplayer_quad_race = "QUAD RACE",
    .multiplayer_single_race = "SINGLE RACE",
    .single_event_select_race_type = "SELECT RACE TYPE",
    .single_event_continue_cup = "CONTINUE CUP",
    .single_event_cup_race = "CUP RACE",
    .single_event_time_trials = "TIME TRIALS",
    .cup_choose_cup_to_enter = "CHOOSE CUP TO ENTER",
    .cup_ultimate_cup = "ULTIMATE CUP",
    .cup_masters_cup = "MASTERS CUP",
    .cup_pitbull_cup = "PITBULL CUP",
    .cup_challenge_cup = "CHALLENGE CUP",
    .cup_era_cup_beauty = "ERA CUP - BEAUTY ",
    .cup_championship_cup = "CHAMPIONSHIP CUP",
    .options_main_options = "MAIN OPTIONS",
    .options_game_options = "GAME OPTIONS",
    .options_scr_cfg = "SCREEN CONFIGURATION",
    .options_memory_card_manager = "MEMORY CARD MANAGER",
    .options_audio_cfg = "AUDIO CONFIGURATION",
    .options_pad_cfg = "CONTROLLER CONFIGURATION",
    .audio_surround_on = "SURROUND - ON ",
    .audio_effect_volume = "EFFECT VOLUME",
    .audio_music_volume = "MUSIC VOLUME",
    .scr_right = "RIGHT",
    .scr_left = "LEFT",
    .scr_down = "DOWN",
    .scr_up = "UP",
    .settings_circuit_laps_000 = "CIRCUIT LAPS - 000 ",
    .settings_camera_damping_000 = "CAMERA DAMPING - 000 ",
    .settings_vibration_off = "VIBRATION - OFF ",
    .settings_two_player_catchup_000 = "TWO PLAYER CATCHUP - 000 ",
    .settings_3d_collisions_on = "3D COLLISIONS - ON ",
    .settings_dynamics_simulation = "DYNAMICS - SIMULATION",
    .settings_difficulty_n = "DIFFICULTY - NORMAL ",
    .settings_cops_on = "COPS - ON ",
    .settings_traffic_on = "TRAFFIC - ON ",
    .settings_checkpoint_timers_on = "CHECKPOINT TIMERS - ON ",
    .settings_speed_readout_mph = "SPEED READOUT - MPH",
    .track_select_track = "SELECT TRACK",
    .track_arrorght = "ArroRght",
    .track_arroleft = "ArroLeft",
    .multiplayer_track_select_tracks = "SELECT TRACKS",
    .credits_hicar = "Hicar",
    .records_highscores = "HIGHSCORES",
    .player_name_enter_your_name = "ENTER YOUR NAME",
    .player_name_a = "a",
    .player_name_b = "b",
    .player_name_empty = "`",
    .player_name_empty_2 = "?",
    .player_name_empty_3 = "!",
    .player_name_empty_4 = ".",
    .player_name_z = "Z",
    .player_name_y = "Y",
    .player_name_x = "X",
    .player_name_w = "W",
    .player_name_v = "V",
    .player_name_u = "U",
    .player_name_t = "T",
    .player_name_s = "S",
    .player_name_r = "R",
    .player_name_q = "Q",
    .player_name_p = "P",
    .player_name_o = "O",
    .player_name_n = "N",
    .player_name_m = "M",
    .player_name_l = "L",
    .player_name_k = "K",
    .player_name_j = "J",
    .player_name_i = "I",
    .player_name_h = "H",
    .player_name_g = "G",
    .player_name_f = "F",
    .player_name_e = "E",
    .player_name_d = "D",
    .player_name_c = "C",
    .player_name_b_2 = "B",
    .player_name_a_2 = "A",
    .event_success_advance = "ADVANCE",
    .event_success_quad_cup = "QUAD CUP",
    .event_success_test_drive_5 = "TEST DRIVE 5",
    .event_success_you_have_won_the = "YOU HAVE WON THE",
    .event_success_congratulations = "CONGRATULATIONS",
    .event_success_player_1 = "PLAYER 1",
    .event_failure_the_test_drive_5 = "THE TEST DRIVE 5",
    .event_failure_you_have_failed_to_win = "YOU HAVE FAILED TO WIN",
    .event_failure_sorry = "SORRY",
    .cup_success_you_have_unlocked = "YOU HAVE UNLOCKED",
};

const PRESENTATION_CARSEL_TEXT presentation_carsel_text_defaults = {
    .car_back = "BACK",
    .car_triangle = "Triangle",
    .car_select = "SELECT",
    .car_cross = "cross",
    .car_srroleft = "SrroLeft",
    .car_srrorght = "SrroRght",
    .car_arrorght = "ArroRght",
    .car_arroleft = "ArroLeft",
    .car_cd = "Cd",
    .card_memory_card_manager = "MEMORY CARD MANAGER",
    .card_back = "BACK",
    .card_triangle = "Triangle",
    .card_select = "SELECT",
    .card_cross = "cross",
    .card_sq1f = "sq1f",
    .card_sq1e = "sq1e",
    .card_sq1d = "sq1d",
    .card_sq1c = "sq1c",
    .card_sq1b = "sq1b",
    .card_sq0f = "sq0f",
    .card_sq0e = "sq0e",
    .card_sq0d = "sq0d",
    .card_sq0c = "sq0c",
    .card_sq0b = "sq0b",
    .card_sq1a = "sq1a",
    .card_sq19 = "sq19",
    .card_sq18 = "sq18",
    .card_sq17 = "sq17",
    .card_sq16 = "sq16",
    .card_sq0a = "sq0a",
    .card_sq09 = "sq09",
    .card_sq08 = "sq08",
    .card_sq07 = "sq07",
    .card_sq06 = "sq06",
    .card_sq15 = "sq15",
    .card_sq14 = "sq14",
    .card_sq13 = "sq13",
    .card_sq12 = "sq12",
    .card_sq11 = "sq11",
    .card_sq05 = "sq05",
    .card_sq04 = "sq04",
    .card_sq03 = "sq03",
    .card_sq02 = "sq02",
    .card_sq01 = "sq01",
    .card_empty = "  ",
    .card_empty_2 = "   ",
    .card_2memory_card_status = "2MEMORY CARD STATUS.................",
    .card_1memory_card_status = "1MEMORY CARD STATUS.................",
    .card_memory_card_2 = "MEMORY CARD 2",
    .card_memory_card_1 = "MEMORY CARD 1",
    .card_message_window_2 = "........MESSAGE WINDOW 2.........",
    .card_message_window_1 = "........MESSAGE WINDOW 1.........",
    .card_delete = "DELETE",
    .card_save = "SAVE",
    .card_load = "LOAD",
    .card_src_reading_memory_cards = "READING MEMORY CARDS",
    .card_src_please_wait = "PLEASE WAIT!",
    .card_src_empty = "",
    .card_src_load_test_drive_5_block = "LOAD TEST DRIVE 5 BLOCK",
    .card_src_save_test_drive_5_block = "SAVE TEST DRIVE 5 BLOCK",
    .card_src_delete_block = "DELETE BLOCK",
    .card_src_race = "RACE",
    .card_src_no_cup_race = "NO CUP RACE",
    .card_src_loaded = "LOADED!",
    .card_src_select_memory_card_to = "SELECT MEMORY CARD TO",
    .card_src_load_from = "LOAD FROM.",
    .card_src_save_to = "SAVE TO.",
    .card_src_saving_memory_card = "SAVING MEMORY CARD",
    .card_src_block = "BLOCK.",
    .card_src_error_creating = "ERROR CREATING",
    .card_src_memory_card_block = "MEMORY CARD BLOCK.",
    .card_src_block_saved_ok = "BLOCK SAVED OK",
    .card_src_error_writing_block = "ERROR WRITING BLOCK",
    .card_src_memory_card_is_unformatted = "MEMORY CARD IS UNFORMATTED",
    .card_src_format_it = "FORMAT IT?",
    .card_src_yes = "YES",
    .card_src_no = "NO",
    .card_src_formating_memory_card = "FORMATING MEMORY CARD",
    .card_src_memory_card = "MEMORY CARD",
    .card_src_formatted_ok = "FORMATTED OK!",
    .card_src_error_formating = "ERROR FORMATING",
    .card_src_memory_card_2 = "MEMORY CARD!",
    .card_src_loading_memory_card = "LOADING MEMORY CARD",
    .card_src_block_2 = "BLOCK!",
    .card_src_block_loaded_ok = "BLOCK LOADED OK!",
    .card_src_error_loading_block = "ERROR LOADING BLOCK!",
    .card_src_overwrite_existing = "OVERWRITE EXISTING",
    .card_src_test_drive_block = "TEST DRIVE BLOCK!",
    .card_src_delete_memory_card = "DELETE MEMORY CARD",
    .card_src_block_deleted = "BLOCK DELETED",
    .card_src_successfully = "SUCCESSFULLY!",
    .card_src_error_deleting = "ERROR DELETING",
    .card_src_no_memory_card = "NO MEMORY CARD",
    .card_src_unformatted_memory_card = "UNFORMATTED MEMORY CARD",
    .card_src_memory_card_is_full = "MEMORY CARD IS FULL",
    .card_src_memory_card_is_ok = "MEMORY CARD IS OK",
};

const VECTOR presentation_menu_view_pos = {0, 0, 0, 0};
const SVECTOR presentation_menu_view_angles = {0, 0, 2048, 0};

static void update_menu_selection(uint32 *selection, sint32 count)
{
    uint32 sel;

    sel = (uint32)presentation_move_menu_selection((sint32)*selection, count);
    *selection = sel;
    activate_menu_selection((sint32)sel, count);
}

static void update_variant_label(void)
{
    char *dst;
    const char *format;

    dst = presentation_find_text(presentation_items[1].content.text, " - ");
    format = session_options.beast_cars != 0 ? "BEAST" : "BEAUTY";
    presentation_copy_text(dst + 3, format);
}

static void hide_option_items(void)
{
    uint32 idx;

    for (idx = 0; idx < 7; ++idx)
        presentation_items[9 + idx].active = (0);
}

static void update_menu(void)
{
    char *dst;
    char *rec;
    uint32 state, val, other, sel, idx;
    sint32 s;

    uint32 *const reset_fields[8] = {
        &menu_choices.audio_row, &menu_choices.settings_row, &menu_choices.options_page, &menu_choices.legacy_vehicle, &menu_choices.car_grp, &menu_choices.legacy_general, &menu_choices.single_event, &menu_choices.main,
    };

    FUNCTION_MARKER(0x800ABEBC, "SLUS_006.10");
    state = session_flow.menu_state;
    switch (state)
    {
        case 0:
            session_flow.menu_state = (1);
            return;
        case 10:
            track_selection_build_list();
            presentation_load_items(&presentation_rec_lists[PRESENTATION_LIST_TRACK]);
            session_flow.menu_state = (11);
            draw_menu_frame();
            return;
        case 11:
            draw_menu_frame();
            session_tick_menu_idle();
            track_selection_update();
            return;
        case 220:
            track_selection_prepare_previews();
            presentation_load_items(&presentation_rec_lists[PRESENTATION_LIST_MULTIPLAYER_TRACK]);
            session_flow.menu_state = (221);
            draw_menu_frame();
            return;
        case 221:
            draw_menu_frame();
            session_tick_menu_idle();
            track_selection_update_multiplayer();
            return;
        case 20:
            car_selection_begin(0);
            session_flow.menu_state = (21);
            return;
        case 21:
            session_tick_menu_idle();
            car_selection_update(0);
            return;
        case 80:
            save_autosave();
            presentation_load_items(&presentation_rec_lists[PRESENTATION_LIST_MAIN]);
            session_flow.menu_state = (81);
            draw_menu_frame();
            car_selection.stage = (0);
            session_flow.player_name_entered = 0;
            session_flow.event_started = (0);
            menu_choices.panel_selection = (0);
            hide_option_items();
            return;
        case 81:
            SetDispMask(1);
            idx = audio_xa.track;
            val = presentation_menu.banner_y_angle;
            sel = (uint8)input_ports[1].type;
            rec = ((uint32)(idx) < 16 ? music_labels[idx] : "");
            other = presentation_menu.banner_x_angle;
            presentation_menu.banner_y_angle = (val + 48);
            presentation_menu.banner_x_angle = (other + 32);
            presentation_items[8].content.text = rec;
            val = presentation_items[2].x_flags;
            other = presentation_pres_recs.main[2].x_flags;
            if (sel == 255)
            {
                presentation_items[2].x_flags = (val | 0x8000);
                presentation_pres_recs.main[2].x_flags = (uint16)(other | 0x8000);
            }
            else
            {
                presentation_items[2].x_flags = (val & 0x3FFF);
                presentation_pres_recs.main[2].x_flags = (uint16)(other & 0x3FFF);
            }
            s = rsin((sint32)presentation_menu.banner_x_angle);
            val = presentation_menu.banner_y_angle;
            presentation_items[7].y = (sint16)((uint32)(s / 128) + 100);
            s = rsin((sint32)val);
            val = presentation_menu.banner_x_angle;
            presentation_items[7].x_flags = ((uint32)(s / 128) + 400);
            s = rsin((sint32)val);
            val = presentation_menu.banner_y_angle;
            presentation_items[8].y = (sint16)((uint32)(s / 128) + 108);
            s = rsin((sint32)val);
            presentation_items[8].x_flags = ((uint32)(s / 128) + 400);
            draw_menu_frame();
            session_tick_menu_idle();
            val = session_flow.menu_initialized;
            hide_option_items();
            if (val == 0 && presentation_menu.save_load_attempted == 0)
            {
                load_saved_settings();
                return;
            }
            sel = (uint32)presentation_move_menu_selection((sint32)menu_choices.main, 7);
            val = input_ports[0].pressed;
            menu_choices.main = (sel);
            if ((val & 0x4008) == 0)
                return;
            if (sel == 6)
            {
                audio_play_menu_accept();
                session_flow.phase = (10);
                presentation_menu.finished = (1);
                return;
            }
            if (presentation_menu.records == NULL || sel >= presentation_menu.records->count)
                return;
            val = presentation_menu.records->entries[sel].action;
            if (val != 0)
            {
                session_push_menu(val);
                audio_play_menu_accept();
            }
            return;
        case 70:
            presentation_load_items(&presentation_rec_lists[PRESENTATION_LIST_SINGLE_EVENT]);
            session_flow.menu_state = (71);
            draw_menu_frame();
            return;
        case 71:
            draw_menu_frame();
            session_tick_menu_idle();
            update_menu_selection(&menu_choices.single_event, 6);
            val = input_ports[0].pressed;
            session_flow.load_requested = (uint32)(0);
            if ((val & 0x4008) != 0 && menu_choices.single_event == 3)
                session_flow.load_requested = (uint32)(1);
            return;
        case 150:
            presentation_load_items(&presentation_rec_lists[PRESENTATION_LIST_CUP]);
            update_variant_label();
            session_flow.menu_state = (151);
            draw_menu_frame();
            return;
        case 151:
            update_variant_label();
            if ((input_ports[0].pressed & 0xA0) != 0 && menu_choices.cup == 1)
            {
                session_options.beast_cars = (session_options.beast_cars ^ 1);
                audio_play_menu_high();
            }
            draw_menu_frame();
            session_tick_menu_idle();
            update_menu_selection(&menu_choices.cup, 6);
            return;
        case 40:
            if (menu_choices.options_page >= 4)
                menu_choices.options_page = 3;
            presentation_load_items(&presentation_rec_lists[PRESENTATION_LIST_OPTIONS]);
            session_flow.menu_state = (41);
            draw_menu_frame();
            return;
        case 41:
            draw_menu_frame();
            session_tick_menu_idle();
            update_menu_selection(&menu_choices.options_page, 4);
            return;
        case 90:
            presentation_load_items(&presentation_rec_lists[PRESENTATION_LIST_AUDIO]);
            session_flow.menu_state = (91);
            draw_menu_frame();
            return;
        case 91:
            draw_menu_frame();
            session_tick_menu_idle();
            update_menu_selection(&menu_choices.audio_row, 3);
            edit_audio_options();
            return;
        case 100:
            menu_choices.panel_selection = (0);
            presentation_load_items(&presentation_rec_lists[PRESENTATION_LIST_SCREEN]);
            session_flow.menu_state = (101);
            draw_menu_frame();
            return;
        case 101:
            draw_menu_frame();
            session_tick_menu_idle();
            activate_menu_selection((sint32)menu_choices.panel_selection, 4);
            edit_scr_pos();
            return;
        case 110:
            presentation_load_items(&presentation_rec_lists[PRESENTATION_LIST_SETTINGS]);
            session_flow.menu_state = (111);
            draw_menu_frame();
            return;
        case 111:
            draw_menu_frame();
            session_tick_menu_idle();
            update_menu_selection(&menu_choices.settings_row, 11);
            session_edit_options();
            return;
        case 120:
            presentation_load_items(&presentation_rec_lists[PRESENTATION_LIST_RECORDS]);
            recs_menu_begin();
            session_flow.menu_state = (121);
            return;
        case 121:
            session_tick_menu_idle();
            recs_menu_update();
            if (session_flow.event_started == 0)
                activate_menu_selection(0, 1);
            else if ((input_ports[0].pressed & 0x4008) != 0)
                session_flow.menu_state = (80);
            return;
        case 130:
            presentation_load_items(&presentation_rec_lists[PRESENTATION_LIST_CARD]);
            session_flow.menu_state = (131);
            card_menu_begin(0);
            return;
        case 131:
            session_tick_menu_idle();
            card_menu_update(0);
            return;
        case 140:
            session_flow.phase = (9);
            for (idx = 0; idx < 8; ++idx)
                *reset_fields[idx] = 0;
            presentation_menu.finished = (1);
            session_flow.attract_active = (1);
            audio_stop_xa();
            val = audio_options.music_volume;
            audio_play_request(13, 20, (sint32)val, 0, 5120);
            return;
        case 160:
            presentation_load_items(&presentation_rec_lists[PRESENTATION_LIST_CONTROLS]);
            session_flow.menu_state = (161);
            draw_menu_frame();
            input_reset_menu_devices();
            return;
        case 161:
            input_menu_update(0);
            input_menu_update(1);
            draw_menu_frame();
            session_tick_menu_idle();
            return;
        case 1:
            session_tick_menu_idle();
            return;
        case 170:
            val = session_flow.event_success;
            presentation_menu.sel = (0xFFFFFFFF);
            if (val == 0)
                presentation_load_items(&presentation_rec_lists[PRESENTATION_LIST_CUP_FAILURE]);
            else
            {
                presentation_load_items(&presentation_rec_lists[PRESENTATION_LIST_CUP_SUCCESS]);
                session_award_cup_rewards();
            }
            save_autosave();
            val = race_session.cup;
            session_flow.menu_state = (171);
            presentation_items[3].content.text = ((uint32)(41 + val) < 48 ? session_event_names[41 + val] : "");
            return;
        case 171:
            val = input_ports[0].pressed;
            presentation_menu.sel = (0xFFFFFFFF);
            if ((val & 0x4008) == 0)
                return;
            val = recs_has_candidate();
            if (val != 1 || race_control.exit_reason != 0)
                session_flow.menu_state = (80);
            else if (session_flow.player_name_entered == 1)
                session_flow.menu_state = (120);
            else
                session_flow.menu_state = (190);
            return;
        case 240:
            val = session_flow.event_success;
            presentation_menu.sel = (0xFFFFFFFF);
            if (val == 0)
                presentation_load_items(&presentation_rec_lists[PRESENTATION_LIST_EVENT_FAILURE]);
            else
            {
                presentation_load_items(&presentation_rec_lists[PRESENTATION_LIST_EVENT_SUCCESS]);
                dst = presentation_copy_text(presentation_items[0].content.text, " 1");
                sel = race_order[0];
                val = race_results.entries[sel].player_tag;
                presentation_format_number(dst + 1, "%d", val);
            }
            session_flow.menu_state = (241);
            return;
        case 241:
            val = input_ports[0].pressed;
            presentation_menu.sel = (0xFFFFFFFF);
            if ((val & 0x4008) != 0)
                session_flow.menu_state = (80);
            return;
        case 190:
            presentation_load_items(&presentation_rec_lists[PRESENTATION_LIST_PLAYER_NAME]);
            session_flow.menu_state = (191);
            draw_menu_frame();
            if (session_flow.event_started != 0)
            {
                presentation_items[38].active = (0);
                presentation_items[39].active = (0);
            }
            return;
        case 191:
            draw_menu_frame();
            update_player_name();
            return;
        case 200:
            menu_choices.multiplayer = (0);
            presentation_load_items(&presentation_rec_lists[PRESENTATION_LIST_MULTIPLAYER]);
            session_flow.menu_state = (201);
            draw_menu_frame();
            return;
        case 201:
            draw_menu_frame();
            session_tick_menu_idle();
            update_menu_selection(&menu_choices.multiplayer, 4);
            return;
        case 180:
            presentation_load_items(&presentation_rec_lists[PRESENTATION_LIST_DEBUG]);
            session_flow.menu_state = (181);
            draw_menu_frame();
            return;
        case 181:
            draw_menu_frame();
            session_tick_menu_idle();
            presentation_menu.sel = 0xFFFFFFFF;
            activate_menu_selection(-1, 0);
            return;
        case 210:
            menu_choices.panel_selection = (2);
            menu_choices.car_grp = (0);
            presentation_load_items(&presentation_rec_lists[PRESENTATION_LIST_QUICK]);
            session_flow.menu_state = (211);
            draw_menu_frame();
            track_selection_build_list();
            car_selection_build_list();
            track_selection.quick_model_idx = (uint32)(car_selection.list_idx);
            return;
        case 211:
            draw_menu_frame();
            session_tick_menu_idle();
            track_selection_update_quick();
            update_menu_selection(&menu_choices.panel_selection, 3);
            return;
        case 60:
            presentation_load_items(&presentation_rec_lists[PRESENTATION_LIST_CREDITS]);
            credits_prepare();
            session_flow.menu_state = (61);
            return;
        case 61:
            credits_update();
            return;
        default:
            return;
    }
}

static void run_menu(void)
{
    uint32 val, previous_menu;
    DISPLAY_CLOCK clock;

    FUNCTION_MARKER(0x800A79B0, "SLUS_006.10");
    session_reset_menu_history(0);
    clear_items();
    presentation_menu.finished = (0);
    session_flow.load_requested = (uint32)(0);
    session_flow.menu_idle_fields = (0);
    disp_clock_init(&clock, 1, session_flow.menu_state);
    while (presentation_menu.finished == 0)
    {
        if (!disp_clock_step(&clock, session_flow.menu_state))
            continue;
        hd_begin_frame(1);
        update_menu_flags();
        input_poll();
        disp_swap();
        hd_menu_view();
        disp_clear_ordering_table();
        model_set_view_pos(&presentation_menu_view_pos);
        model_set_view_angles(&presentation_menu_view_angles);
        presentation_begin_frame();
        previous_menu = session_flow.menu_state;
        update_menu();
        if (session_flow.menu_state != previous_menu && (previous_menu == 91 || previous_menu == 101 || previous_menu == 111 || previous_menu == 161))
            save_autosave();
        presentation_draw_items();
        val = (uint32)audio_poll_xa();
        val = (uint32)timing_read_counter(1, (sint32)val);
        disp_state.pre_sync_ticks = val;
        val = (uint32)DrawSync(0);
        val = (uint32)timing_read_counter(1, (sint32)val);
        disp_state.post_sync_ticks = val;
        timing_reset_counter(1);
        presentation_menu.frame_counter = (presentation_menu.frame_counter + 1);
        disp_apply();
        presentation_upload_background();
        hd_menu_background(disp_state.bank, 256);
        disp_draw_ordering_table();
        hd_end_frame();
    }
    hd_present_frame(clock.field, clock.fields);
    hd_suspend();
}

static void load_saved_settings(void)
{
    uint32 x, y;

    FUNCTION_MARKER(0x800ADC90, "SLUS_006.10");
    save_load_game();
    presentation_menu.save_load_attempted = 1;
    x = disp_options.scr_x;
    y = disp_options.scr_y;
    disp_state.frames[0].disp.screen.x = (sint16)(x);
    disp_state.frames[1].disp.screen.x = (sint16)(x);
    disp_state.frames[0].disp.screen.y = (sint16)(y);
    disp_state.frames[1].disp.screen.y = (sint16)(y);
    audio_update_cd_volume();
}

void presentation_wait_loading(void)
{
    uint32 elapsed, buttons, prev;
    uint32 fields = disp_state.field_rate * 4;
    uint32 fade_fields = fields / 2;
    uint32 volume = audio_options.music_volume;

    prev = (xport_input_read(0) | xport_input_read(1)) & (PADRdown | PADstart);
    for (elapsed = 0; elapsed < fields; ++elapsed)
    {
        if (elapsed >= fields - fade_fields)
            audio_fade(volume * (fields - elapsed - 1) / fade_fields);
        VSync(0);
        buttons = (xport_input_read(0) | xport_input_read(1)) & (PADRdown | PADstart);
        if ((buttons & ~prev) != 0)
            break;
        prev = buttons;
    }
    audio_fade(0);
    audio_stop_xa();
    audio_stop_voice(20);
}

uint32 presentation_time_from_ticks(uint32 ticks, uint32 divisor)
{
    if (divisor == 0)
        xport_mips_break(7);
    return ticks / divisor;
}

uint32 presentation_result_speed(uint32 fixed_speed, uint32 multiplier, uint32 shift)
{
    uint32 high = (((sint64)(sint32)fixed_speed * (sint64)(sint32)multiplier) >> 32);
    uint32 quotient = ((sint32)(high + fixed_speed) >> shift);

    return quotient - (uint32)((sint32)fixed_speed >> 31);
}

static void edit_scr_pos(void)
{
    uint32 sel, buttons, old_x, old_y, cur_x;
    sint32 x, y;

    FUNCTION_MARKER(0x800AAD1C, "SLUS_006.10");
    if ((input_ports[0].pressed & 0x10) != 0 && menu_choices.panel_selection != 0)
    {
        menu_choices.panel_selection = (0);
        audio_play_menu_move();
    }
    if ((input_ports[0].pressed & 0x40) != 0 && menu_choices.panel_selection != 1)
    {
        menu_choices.panel_selection = (1);
        audio_play_menu_move();
    }
    if ((input_ports[0].pressed & 0x80) != 0 && menu_choices.panel_selection != 2)
    {
        menu_choices.panel_selection = (2);
        audio_play_menu_move();
    }
    if ((input_ports[0].pressed & 0x20) != 0 && menu_choices.panel_selection != 3)
    {
        menu_choices.panel_selection = (3);
        audio_play_menu_move();
    }
    buttons = input_ports[0].pressed;
    sel = menu_choices.panel_selection;
    presentation_menu.sel = (sel);
    if ((buttons & 0x4008) == 0)
        return;
    old_x = disp_options.scr_x;
    old_y = disp_options.scr_y;
    switch (sel)
    {
        case 0:
            disp_options.scr_y = (old_y - 1);
            break;
        case 1:
            disp_options.scr_y = (old_y + 1);
            break;
        case 2:
            disp_options.scr_x = (old_x - 1);
            break;
        case 3:
            disp_options.scr_x = (old_x + 1);
            break;
    }
    x = (sint32)disp_options.scr_x;
    if (x < 0)
        x = 0;
    else if (x > 32)
        x = 32;
    y = (sint32)disp_options.scr_y;
    disp_options.scr_x = ((uint32)x);
    if (y < 0)
        y = 0;
    else if (y > 32)
        y = 32;
    cur_x = disp_options.scr_x;
    disp_options.scr_y = ((uint32)y);
    if (old_x != cur_x || old_y != (uint32)y)
        audio_play_menu_accept();
    cur_x = disp_options.scr_x;
    sel = disp_options.scr_y;
    disp_state.frames[0].disp.screen.x = (sint16)(cur_x);
    disp_state.frames[1].disp.screen.x = (sint16)(cur_x);
    disp_state.frames[0].disp.screen.y = (sint16)(sel);
    disp_state.frames[1].disp.screen.y = (sint16)(sel);
}
