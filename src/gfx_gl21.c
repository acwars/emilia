#define _GNU_SOURCE

#include "../include/gfx_gl21.h"
#include "../include/vt.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "../include/freetype.h"
#include "../include/freetype_errors.h"
#include <GL/gl.h>

#include "../include/map.h"
#include "../include/shaders_gl21.h"
#include "../include/util.h"
#include "../include/wc_width.h"

DEF_PAIR(GLuint);

#define NUM_BUCKETS 513

#ifndef ATLAS_SIZE_LIMIT
#define ATLAS_SIZE_LIMIT INT32_MAX
#endif

#ifndef DIM_COLOR_BLEND_FACTOR
#define DIM_COLOR_BLEND_FACTOR 0.4f
#endif

#ifndef N_RECYCLED_TEXTURES
#define N_RECYCLED_TEXTURES 5
#endif

#define PROXY_INDEX_TEXTURE           0
#define PROXY_INDEX_TEXTURE_BLINK     1
#define PROXY_INDEX_DEPTHBUFFER       2
#define PROXY_INDEX_DEPTHBUFFER_BLINK 3

#define IMG_PROXY_INDEX_TEXTURE_ID 0

#define IMG_VIEW_PROXY_INDEX_VBO_ID 0

#define SIXEL_PROXY_INDEX_TEXTURE_ID 0
#define SIXEL_PROXY_INDEX_VBO_ID     1

#define BOUND_RESOURCES_NONE      0
#define BOUND_RESOURCES_BG        1
#define BOUND_RESOURCES_FONT      2
#define BOUND_RESOURCES_LINES     3
#define BOUND_RESOURCES_IMAGE     4
#define BOUND_RESOURCES_FONT_MONO 5

#include "../include/gl_ext.h"

static PFNGLBUFFERSUBDATAARBPROC        glBufferSubData;
static PFNGLUNIFORM4FPROC               glUniform4f;
static PFNGLUNIFORM3FPROC               glUniform3f;
static PFNGLUNIFORM2FPROC               glUniform2f;
static PFNGLBUFFERDATAPROC              glBufferData;
static PFNGLDELETEPROGRAMPROC           glDeleteProgram;
static PFNGLUSEPROGRAMPROC              glUseProgram;
static PFNGLGETUNIFORMLOCATIONPROC      glGetUniformLocation;
static PFNGLGETATTRIBLOCATIONPROC       glGetAttribLocation;
static PFNGLDELETESHADERPROC            glDeleteShader;
static PFNGLDETACHSHADERPROC            glDetachShader;
static PFNGLGETPROGRAMINFOLOGPROC       glGetProgramInfoLog;
static PFNGLGETPROGRAMIVPROC            glGetProgramiv;
static PFNGLLINKPROGRAMPROC             glLinkProgram;
static PFNGLATTACHSHADERPROC            glAttachShader;
static PFNGLCOMPILESHADERPROC           glCompileShader;
static PFNGLSHADERSOURCEPROC            glShaderSource;
static PFNGLCREATESHADERPROC            glCreateShader;
static PFNGLCREATEPROGRAMPROC           glCreateProgram;
static PFNGLGETSHADERINFOLOGPROC        glGetShaderInfoLog;
static PFNGLGETSHADERIVPROC             glGetShaderiv;
static PFNGLDELETEBUFFERSPROC           glDeleteBuffers;
static PFNGLVERTEXATTRIBPOINTERPROC     glVertexAttribPointer;
static PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
static PFNGLBINDBUFFERPROC              glBindBuffer;
static PFNGLGENBUFFERSPROC              glGenBuffers;
static PFNGLDELETEFRAMEBUFFERSPROC      glDeleteFramebuffers;
static PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer;
static PFNGLRENDERBUFFERSTORAGEPROC     glRenderbufferStorage;
static PFNGLBINDBUFFERPROC              glBindBuffer;
static PFNGLGENBUFFERSPROC              glGenBuffers;
static PFNGLDELETEFRAMEBUFFERSPROC      glDeleteFramebuffers;
static PFNGLFRAMEBUFFERTEXTURE2DPROC    glFramebufferTexture2D;
static PFNGLBINDFRAMEBUFFERPROC         glBindFramebuffer;
static PFNGLBINDRENDERBUFFERPROC        glBindRenderbuffer;
static PFNGLGENRENDERBUFFERSPROC        glGenRenderbuffers;
static PFNGLDELETERENDERBUFFERSPROC     glDeleteRenderbuffers;
static PFNGLGENFRAMEBUFFERSPROC         glGenFramebuffers;
static PFNGLGENERATEMIPMAPPROC          glGenerateMipmap;
static PFNGLBLENDFUNCSEPARATEPROC       glBlendFuncSeparate;

#ifdef DEBUG
static PFNGLDEBUGMESSAGECALLBACKPROC   glDebugMessageCallback;
static PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus;
#endif

static void maybe_load_gl_exts(void* loader,
                               void* (*loader_func)(void* loader, const char* proc_name))
{
    static bool loaded = false;
    if (loaded)
        return;

    glBufferSubData           = loader_func(loader, "glBufferSubData");
    glUniform4f               = loader_func(loader, "glUniform4f");
    glUniform3f               = loader_func(loader, "glUniform3f");
    glUniform2f               = loader_func(loader, "glUniform2f");
    glBufferData              = loader_func(loader, "glBufferData");
    glDeleteProgram           = loader_func(loader, "glDeleteProgram");
    glUseProgram              = loader_func(loader, "glUseProgram");
    glGetUniformLocation      = loader_func(loader, "glGetUniformLocation");
    glGetAttribLocation       = loader_func(loader, "glGetAttribLocation");
    glDeleteShader            = loader_func(loader, "glDeleteShader");
    glDetachShader            = loader_func(loader, "glDetachShader");
    glGetProgramInfoLog       = loader_func(loader, "glGetProgramInfoLog");
    glGetProgramiv            = loader_func(loader, "glGetProgramiv");
    glLinkProgram             = loader_func(loader, "glLinkProgram");
    glAttachShader            = loader_func(loader, "glAttachShader");
    glCompileShader           = loader_func(loader, "glCompileShader");
    glShaderSource            = loader_func(loader, "glShaderSource");
    glCreateShader            = loader_func(loader, "glCreateShader");
    glCreateProgram           = loader_func(loader, "glCreateProgram");
    glGetShaderInfoLog        = loader_func(loader, "glGetShaderInfoLog");
    glGetShaderiv             = loader_func(loader, "glGetShaderiv");
    glDeleteBuffers           = loader_func(loader, "glDeleteBuffers");
    glVertexAttribPointer     = loader_func(loader, "glVertexAttribPointer");
    glEnableVertexAttribArray = loader_func(loader, "glEnableVertexAttribArray");
    glBindBuffer              = loader_func(loader, "glBindBuffer");
    glGenBuffers              = loader_func(loader, "glGenBuffers");
    glDeleteFramebuffers      = loader_func(loader, "glDeleteFramebuffers");
    glFramebufferRenderbuffer = loader_func(loader, "glFramebufferRenderbuffer");
    glBindBuffer              = loader_func(loader, "glBindBuffer");
    glGenBuffers              = loader_func(loader, "glGenBuffers");
    glDeleteFramebuffers      = loader_func(loader, "glDeleteFramebuffers");
    glFramebufferTexture2D    = loader_func(loader, "glFramebufferTexture2D");
    glBindFramebuffer         = loader_func(loader, "glBindFramebuffer");
    glRenderbufferStorage     = loader_func(loader, "glRenderbufferStorage");
    glDeleteRenderbuffers     = loader_func(loader, "glDeleteRenderbuffers");
    glBindRenderbuffer        = loader_func(loader, "glBindRenderbuffer");
    glGenRenderbuffers        = loader_func(loader, "glGenRenderbuffers");
    glGenFramebuffers         = loader_func(loader, "glGenFramebuffers");
    glGenerateMipmap          = loader_func(loader, "glGenerateMipmap");
    glBlendFuncSeparate       = loader_func(loader, "glBlendFuncSeparate");
#ifdef DEBUG
    glDebugMessageCallback   = loader_func(loader, "glDebugMessageCallback");
    glCheckFramebufferStatus = loader_func(loader, "glCheckFramebufferStatus");
#endif

    loaded = true;
}

#include "../include/gl.h"

enum GlyphColor
{
    GLYPH_COLOR_MONO,
    GLYPH_COLOR_LCD,
    GLYPH_COLOR_COLOR,
};

DEF_VECTOR(Texture, Texture_destroy);

static inline size_t Rune_hash(const Rune* self)
{
    return self->code;
}

static inline size_t Rune_eq(const Rune* self, const Rune* other)
{
    bool combinable_eq = true;
    for (int i = 0; i < VT_RUNE_MAX_COMBINE; ++i) {
        if (self->combine[i] == other->combine[i]) {
            if (!self->combine[i])
                break;
            continue;
        } else {
            combinable_eq = false;
            break;
        }
    }
    return self->code == other->code && self->style == other->style && combinable_eq;
}

#define ATLAS_RENDERABLE_START ' '
#define ATLAS_RENDERABLE_END   CHAR_MAX

typedef struct
{
    float x, y;
} vertex_t;

DEF_VECTOR(float, NULL);
DEF_VECTOR(Vector_float, Vector_destroy_float);

DEF_VECTOR(vertex_t, NULL);

typedef struct
{
    GLuint color_tex;
    GLuint depth_rb;
} LineTexture;

static void LineTexture_destroy(LineTexture* self)
{
    if (self->color_tex) {
        ASSERT(self->depth_rb, "deleted texture has depth renderbuffer");
        glDeleteTextures(1, &self->color_tex);
        glDeleteRenderbuffers(1, &self->depth_rb);
        self->color_tex = 0;
        self->depth_rb  = 0;
    }
}

struct _GfxOpenGL21;

typedef struct
{
    uint32_t           page_id;
    GLuint             texture_id;
    GLenum             internal_format;
    enum TextureFormat texture_format;
    uint32_t           width_px, height_px;
    uint32_t           current_line_height_px, current_offset_y, current_offset_x;
    float              sx, sy;
} GlyphAtlasPage;

typedef struct
{
    uint8_t page_id;
    bool    can_scale;
    GLuint  texture_id;

    float   left, top;
    int32_t height, width;
    float   tex_coords[4];
} GlyphAtlasEntry;

static void GlyphAtlasPage_destroy(GlyphAtlasPage* self)
{
    if (self->texture_id) {
        glDeleteTextures(1, &self->texture_id);
        self->texture_id = 0;
    }
}

DEF_VECTOR(GlyphAtlasPage, GlyphAtlasPage_destroy);
DEF_MAP(Rune, GlyphAtlasEntry, Rune_hash, Rune_eq, NULL);

typedef struct
{
    Vector_GlyphAtlasPage    pages;
    GlyphAtlasPage*          current_rgb_page;
    GlyphAtlasPage*          current_rgba_page;
    GlyphAtlasPage*          current_grayscale_page;
    Map_Rune_GlyphAtlasEntry entry_map;
    uint32_t                 page_size_px;
} GlyphAtlas;

typedef struct _GfxOpenGL21
{
    GLint max_tex_res;

    Vector_vertex_t vec_vertex_buffer;
    Vector_vertex_t vec_vertex_buffer2;

    VBO flex_vbo;

    GLuint full_framebuffer_quad_vbo;
    GLuint line_quads_vbo;

    /* pen position to begin drawing font */
    float pen_begin_y;
    int   pen_begin_pixels_y;
    int   pen_begin_pixels_x;

    uint32_t win_w, win_h;
    float    line_height, glyph_width;
    uint16_t line_height_pixels, glyph_width_pixels;
    size_t   max_cells_in_line;
    float    sx, sy;
    uint32_t gw;

    /* padding offset from the top right corner */
    uint8_t pixel_offset_x;
    uint8_t pixel_offset_y;

    GLuint line_framebuffer;

    Shader solid_fill_shader;
    Shader font_shader;
    Shader font_shader_blend;
    Shader font_shader_gray;
    Shader line_shader;
    Shader image_shader;
    Shader image_tint_shader;

    ColorRGB  color;
    ColorRGBA bg_color;

    GlyphAtlas          glyph_atlas;
    Vector_Vector_float float_vec;

    // keep textures for reuse in order of length
    LineTexture recycled_textures[5];

    Texture squiggle_texture;

    TimePoint blink_switch;
    TimePoint blink_switch_text;
    TimePoint action;
    TimePoint inactive;

    bool is_main_font_rgb;

    Freetype* freetype;

    int_fast8_t bound_resources;
} GfxOpenGL21;

#define gfxOpenGL21(gfx) ((GfxOpenGL21*)&gfx->extend_data)
#define gfxBase(gfxGl21) ((Gfx*)(((uint8_t*)(gfxGl21)) - offsetof(Gfx, extend_data)))

Pair_GLuint GfxOpenGL21_pop_recycled(GfxOpenGL21* self);
void        GfxOpenGL21_load_font(Gfx* self);
void        GfxOpenGL21_destroy_recycled(GfxOpenGL21* self);

void          GfxOpenGL21_destroy(Gfx* self);
void          GfxOpenGL21_draw(Gfx* self, const Vt* vt, Ui* ui);
Pair_uint32_t GfxOpenGL21_get_char_size(Gfx* self);
void          GfxOpenGL21_resize(Gfx* self, uint32_t w, uint32_t h);
void          GfxOpenGL21_init_with_context_activated(Gfx* self);
bool          GfxOpenGL21_set_focus(Gfx* self, bool focus);
Pair_uint32_t GfxOpenGL21_pixels(Gfx* self, uint32_t c, uint32_t r);
void          GfxOpenGL21_reload_font(Gfx* self);
void          GfxOpenGL21_destroy_proxy(Gfx* self, uint32_t* proxy);
void          GfxOpenGL21_destroy_image_proxy(Gfx* self, uint32_t* proxy);
void          GfxOpenGL21_destroy_image_view_proxy(Gfx* self, uint32_t* proxy);
void          GfxOpenGL21_destroy_sixel_proxy(Gfx* self, uint32_t* proxy);
static void   GfxOpenGL21_regenerate_line_quad_vbo(GfxOpenGL21* gfx);

static struct IGfx gfx_interface_opengl21 = {
    .draw                        = GfxOpenGL21_draw,
    .resize                      = GfxOpenGL21_resize,
    .get_char_size               = GfxOpenGL21_get_char_size,
    .init_with_context_activated = GfxOpenGL21_init_with_context_activated,
    .reload_font                 = GfxOpenGL21_reload_font,
    .pixels                      = GfxOpenGL21_pixels,
    .destroy                     = GfxOpenGL21_destroy,
    .destroy_proxy               = GfxOpenGL21_destroy_proxy,
    .destroy_image_proxy         = GfxOpenGL21_destroy_image_proxy,
    .destroy_image_view_proxy    = GfxOpenGL21_destroy_image_view_proxy,
    .destroy_sixel_proxy         = GfxOpenGL21_destroy_sixel_proxy,
};

Gfx* Gfx_new_OpenGL21(Freetype* freetype)
{
    Gfx* self                   = calloc(1, sizeof(Gfx) + sizeof(GfxOpenGL21) - sizeof(uint8_t));
    self->interface             = &gfx_interface_opengl21;
    gfxOpenGL21(self)->freetype = freetype;
    gfxOpenGL21(self)->is_main_font_rgb = !(freetype->primary_output_type == FT_OUTPUT_GRAYSCALE);
    GfxOpenGL21_load_font(self);
    return self;
}

#define ARRAY_BUFFER_SUB_OR_SWAP(_buf, _size, _newsize)                                            \
    if ((_newsize) > _size) {                                                                      \
        _size = (_newsize);                                                                        \
        glBufferData(GL_ARRAY_BUFFER, (_newsize), (_buf), GL_STREAM_DRAW);                         \
    } else {                                                                                       \
        glBufferSubData(GL_ARRAY_BUFFER, 0, (_newsize), (_buf));                                   \
    }

static GlyphAtlasPage GlyphAtlasPage_new(GfxOpenGL21*       gfx,
                                         uint32_t           page_id,
                                         bool               filter,
                                         GLenum             internal_texture_format,
                                         enum TextureFormat texture_format,
                                         GLint              width_px,
                                         GLint              height_px)
{
    GlyphAtlasPage self;
    self.page_id                = page_id;
    self.current_offset_x       = 0;
    self.current_offset_y       = 0;
    self.current_line_height_px = 0;
    self.width_px               = MIN(gfx->max_tex_res, width_px);
    self.height_px              = MIN(gfx->max_tex_res, height_px);
    self.sx                     = 2.0 / self.width_px;
    self.sy                     = 2.0 / self.height_px;
    self.texture_format         = texture_format;
    self.internal_format        = internal_texture_format;
    self.texture_id             = 0;

    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &self.texture_id);
    glBindTexture(GL_TEXTURE_2D, self.texture_id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D,
                    GL_TEXTURE_MIN_FILTER,
                    filter ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter ? GL_LINEAR : GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 self.internal_format,
                 self.width_px,
                 self.height_px,
                 0,
                 self.internal_format,
                 GL_UNSIGNED_BYTE,
                 0);

    if (filter) {
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    return self;
}

static inline bool GlyphAtlasPage_can_push(GlyphAtlasPage* self, FreetypeOutput* glyph)
{
    return self->current_offset_y + MAX((uint32_t)glyph->height, self->current_line_height_px) + 1 <
           self->height_px;
}

static inline bool GlyphAtlasPage_can_push_tex(GlyphAtlasPage* self, Texture tex)
{
    return self->current_offset_y + MAX(tex.h, self->current_line_height_px) + 1 < self->height_px;
}

static GlyphAtlasEntry GlyphAtlasPage_push_tex(GfxOpenGL21*    gfx,
                                               GlyphAtlasPage* self,
                                               FreetypeOutput* glyph,
                                               Texture         tex)
{
    ASSERT(GlyphAtlasPage_can_push_tex(self, tex), "does not overflow");
    if (self->current_offset_x + glyph->width >= self->width_px) {
        self->current_offset_y += (self->current_line_height_px + 1);
        self->current_offset_x       = 0;
        self->current_line_height_px = 0;
    }
    self->current_line_height_px = MAX(self->current_line_height_px, (uint32_t)glyph->height);
    if (unlikely(glyph->type == FT_OUTPUT_COLOR_BGRA))
        glGenerateMipmap(GL_TEXTURE_2D);

    GLint     old_fb;
    GLint     old_shader;
    GLboolean old_depth_test;
    GLboolean old_scissor_test;
    GLint     old_viewport[4];
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &old_fb);
    glGetIntegerv(GL_CURRENT_PROGRAM, &old_shader);
    glGetBooleanv(GL_DEPTH_TEST, &old_depth_test);
    glGetBooleanv(GL_SCISSOR_TEST, &old_scissor_test);
    glGetIntegerv(GL_VIEWPORT, old_viewport);

    GLuint tmp_fb;
    glGenFramebuffers(1, &tmp_fb);
    glBindFramebuffer(GL_FRAMEBUFFER, tmp_fb);
    glFramebufferTexture2D(GL_FRAMEBUFFER,
                           GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D,
                           self->texture_id,
                           0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
    glViewport(0, 0, self->width_px, self->height_px);

    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    GLuint tmp_vbo;
    glGenBuffers(1, &tmp_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, tmp_vbo);
    glUseProgram(gfx->image_shader.id);
    glBindTexture(GL_TEXTURE_2D, tex.id);

    float sx             = 2.0f / self->width_px;
    float sy             = 2.0f / self->height_px;
    float w              = tex.w * sx;
    float h              = tex.h * sy;
    float x              = -1.0f + self->current_offset_x * sx;
    float y              = -1.0f + self->current_offset_y * sy + h;
    float vbo_data[4][4] = {
        { x, y, 0.0f, 1.0f },
        { x + w, y, 1.0f, 1.0f },
        { x + w, y - h, 1.0f, 0.0f },
        { x, y - h, 0.0f, 0.0f },
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(float[4][4]), vbo_data, GL_STREAM_DRAW);
    glVertexAttribPointer(gfx->image_shader.attribs->location, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glDrawArrays(GL_QUADS, 0, 4);

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDeleteFramebuffers(1, &tmp_fb);
    glDeleteBuffers(1, &tmp_vbo);

    /* restore initial state */
    glUseProgram(old_shader);
    glBindFramebuffer(GL_FRAMEBUFFER, old_fb);
    glViewport(old_viewport[0], old_viewport[1], old_viewport[2], old_viewport[3]);
    if (!old_depth_test) {
        glDisable(GL_DEPTH_TEST);
    }
    if (old_scissor_test) {
        glEnable(GL_SCISSOR_TEST);
    }

    GlyphAtlasEntry retval = {
        .page_id    = self->page_id,
        .can_scale  = glyph->type == FT_OUTPUT_COLOR_BGRA,
        .texture_id = self->texture_id,
        .left       = MIN(0, glyph->left),
        .top        = tex.h,
        .height     = tex.h,
        .width      = tex.w,
        .tex_coords = { (float)self->current_offset_x / self->width_px,
                        1.0f - (((float)self->height_px - (float)self->current_offset_y) /
                                self->height_px),
                        (float)self->current_offset_x / self->width_px +
                          (float)tex.w / self->width_px,
                        1.0f - (((float)self->height_px - (float)self->current_offset_y) /
                                  self->height_px -
                                (float)tex.h / self->height_px) },
    };
    self->current_offset_x += tex.w;
    return retval;
}

static GlyphAtlasEntry GlyphAtlasPage_push(GlyphAtlasPage* self, FreetypeOutput* glyph)
{
    ASSERT(GlyphAtlasPage_can_push(self, glyph), "does not overflow");
    if (self->current_offset_x + glyph->width >= self->width_px) {
        self->current_offset_y += (self->current_line_height_px + 1);
        self->current_offset_x       = 0;
        self->current_line_height_px = 0;
    }
    self->current_line_height_px = MAX(self->current_line_height_px, (uint32_t)glyph->height);
    GLenum format;
    switch (glyph->type) {
        case FT_OUTPUT_BGR_H:
        case FT_OUTPUT_BGR_V:
            format = GL_BGR;
            break;
        case FT_OUTPUT_RGB_H:
        case FT_OUTPUT_RGB_V:
            format = GL_RGB;
            break;
        case FT_OUTPUT_GRAYSCALE:
            format = GL_RED;
            break;
        case FT_OUTPUT_COLOR_BGRA:
            format = GL_BGRA;
            break;
        default:
            ASSERT_UNREACHABLE
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, glyph->alignment);
    glBindTexture(GL_TEXTURE_2D, self->texture_id);
    glTexSubImage2D(GL_TEXTURE_2D,
                    0,
                    self->current_offset_x,
                    self->current_offset_y,
                    glyph->width,
                    glyph->height,
                    format,
                    GL_UNSIGNED_BYTE,
                    glyph->pixels);
    if (unlikely(glyph->type == FT_OUTPUT_COLOR_BGRA))
        glGenerateMipmap(GL_TEXTURE_2D);
    GlyphAtlasEntry retval = {
        .page_id    = self->page_id,
        .can_scale  = glyph->type == FT_OUTPUT_COLOR_BGRA,
        .texture_id = self->texture_id,
        .left       = glyph->left,
        .top        = glyph->top,
        .height     = glyph->height,
        .width      = glyph->width,
        .tex_coords = { (float)self->current_offset_x / self->width_px,
                        1.0f - (((float)self->height_px - (float)self->current_offset_y) /
                                self->height_px),
                        (float)self->current_offset_x / self->width_px +
                          (float)glyph->width / self->width_px,
                        1.0f - (((float)self->height_px - (float)self->current_offset_y) /
                                  self->height_px -
                                (float)glyph->height / self->height_px) },
    };
    self->current_offset_x += glyph->width;
    return retval;
}

static GlyphAtlas GlyphAtlas_new(uint32_t page_size_px)
{
    GlyphAtlas self;
    self.pages                  = Vector_new_with_capacity_GlyphAtlasPage(3);
    self.entry_map              = Map_new_Rune_GlyphAtlasEntry(1024);
    self.current_rgba_page      = NULL;
    self.current_rgb_page       = NULL;
    self.current_grayscale_page = NULL;
    self.page_size_px           = page_size_px;
    return self;
}

static void GlyphAtlas_destroy(GlyphAtlas* self)
{
    Vector_destroy_GlyphAtlasPage(&self->pages);
    Map_destroy_Rune_GlyphAtlasEntry(&self->entry_map);
}

__attribute__((cold)) GlyphAtlasEntry* GlyphAtlas_get_combined(GfxOpenGL21* gfx,
                                                               GlyphAtlas*  self,
                                                               const Rune*  rune)
{
    enum FreetypeFontStyle style = FT_STYLE_REGULAR;
    switch (rune->style) {
        case VT_RUNE_BOLD:
            style = FT_STYLE_BOLD;
            break;
        case VT_RUNE_ITALIC:
            style = FT_STYLE_ITALIC;
            break;
        case VT_RUNE_BOLD_ITALIC:
            style = FT_STYLE_BOLD_ITALIC;
            break;
        default:;
    }
    FreetypeOutput *output, *base_output;
    output = base_output = Freetype_load_and_render_glyph(gfx->freetype, rune->code, style);

    if (!output)
        return NULL;

    GLenum internal_format;
    GLenum load_format;
    bool   scale = false;
    switch (output->type) {
        case FT_OUTPUT_RGB_H:
            internal_format = GL_RGB;
            load_format     = GL_RGB;
            break;
        case FT_OUTPUT_BGR_H:
            internal_format = GL_RGB;
            load_format     = GL_BGR;
            break;
        case FT_OUTPUT_RGB_V:
            internal_format = GL_RGB;
            load_format     = GL_RGB;
            break;
        case FT_OUTPUT_BGR_V:
            internal_format = GL_RGB;
            load_format     = GL_BGR;
            break;
        case FT_OUTPUT_GRAYSCALE:
            internal_format = GL_RED;
            load_format     = GL_RED;
            break;
        case FT_OUTPUT_COLOR_BGRA:
            scale           = true;
            internal_format = GL_RGBA;
            load_format     = GL_BGRA;
            break;
        default:
            ASSERT_UNREACHABLE
    }

    GLint     old_fb;
    GLint     old_shader;
    GLboolean old_depth_test;
    GLboolean old_scissor_test;
    GLint     old_viewport[4];
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &old_fb);
    glGetIntegerv(GL_CURRENT_PROGRAM, &old_shader);
    glGetBooleanv(GL_DEPTH_TEST, &old_depth_test);
    glGetBooleanv(GL_SCISSOR_TEST, &old_scissor_test);
    glGetIntegerv(GL_VIEWPORT, old_viewport);

    Texture tex = {
        .id = 0,
        .w  = MAX(gfx->glyph_width_pixels, output->width),
        .h  = MAX(gfx->line_height_pixels, output->height),
    };

    float scalex = 2.0 / tex.w;
    float scaley = 2.0 / tex.h;

    glDisable(GL_SCISSOR_TEST);
    glGenTextures(1, &tex.id);
    glBindTexture(GL_TEXTURE_2D, tex.id);
    glTexParameteri(GL_TEXTURE_2D,
                    GL_TEXTURE_MIN_FILTER,
                    scale ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, scale ? GL_LINEAR : GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 internal_format,
                 tex.w,
                 tex.h,
                 0,
                 load_format,
                 GL_UNSIGNED_BYTE,
                 NULL);
    GLuint tmp_rb;
    glGenRenderbuffers(1, &tmp_rb);
    glBindRenderbuffer(GL_RENDERBUFFER, tmp_rb);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, tex.w, tex.h);
    GLuint tmp_fb;
    glGenFramebuffers(1, &tmp_fb);
    glBindFramebuffer(GL_FRAMEBUFFER, tmp_fb);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex.id, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, tmp_rb);
    glViewport(0, 0, tex.w, tex.h);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glDepthRange(0.0f, 1.0f);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    GLuint tmp_vbo;
    glGenBuffers(1, &tmp_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, tmp_vbo);
    glUseProgram(gfx->font_shader_blend.id);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float[4][4]), NULL, GL_STREAM_DRAW);
    glVertexAttribPointer(gfx->font_shader_blend.attribs->location, 4, GL_FLOAT, GL_FALSE, 0, 0);

    for (uint32_t i = 0; i < VT_RUNE_MAX_COMBINE + 1; ++i) {
        char32_t c = i ? rune->combine[i - 1] : rune->code;
        if (!c) {
            break;
        }

        if (i) {
            output = Freetype_load_and_render_glyph(gfx->freetype, c, style);
        }

        if (!output) {
            WRN("Missing combining glyph u+%X\n", c);
            continue;
        }

        GLuint tmp_tex;
        glGenTextures(1, &tmp_tex);
        glBindTexture(GL_TEXTURE_2D, tmp_tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     internal_format,
                     output->width,
                     output->height,
                     0,
                     load_format,
                     GL_UNSIGNED_BYTE,
                     output->pixels);
        gl_check_error();

        float l = scalex * output->left;
        float t = scaley * output->top;
        float w = scalex * output->width;
        float h = scaley * output->height;

        float x = -1.0 + (i ? ((tex.w - output->width) / 2 * scalex) : l);
        float y = 1.0 - t + h;
        y       = CLAMP(y, -1.0 + h, 1.0);

        float vbo_data[4][4] = {
            { x, y, 0.0f, 1.0f },
            { x + w, y, 1.0f, 1.0f },
            { x + w, y - h, 1.0f, 0.0f },
            { x, y - h, 0.0f, 0.0f },
        };
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vbo_data), vbo_data);
        glDrawArrays(GL_QUADS, 0, 4);

        glDeleteTextures(1, &tmp_tex);
        gl_check_error();
    }

    glDeleteFramebuffers(1, &tmp_fb);
    glDeleteRenderbuffers(1, &tmp_rb);
    glDeleteBuffers(1, &tmp_vbo);

    /* restore initial state */
    glUseProgram(old_shader);
    glBindFramebuffer(GL_FRAMEBUFFER, old_fb);
    glViewport(old_viewport[0], old_viewport[1], old_viewport[2], old_viewport[3]);

    if (!old_depth_test) {
        glDisable(GL_DEPTH_TEST);
    }

    if (old_scissor_test) {
        glEnable(GL_SCISSOR_TEST);
    }

    GlyphAtlasPage* tgt_page;

    if (!output) {
        return NULL;
    }
    switch (output->type) {
        case FT_OUTPUT_RGB_H:
        case FT_OUTPUT_BGR_H:
        case FT_OUTPUT_RGB_V:
        case FT_OUTPUT_BGR_V:
            tgt_page = self->current_rgb_page;
            if (unlikely(!tgt_page || !GlyphAtlasPage_can_push_tex(tgt_page, tex))) {
                Vector_push_GlyphAtlasPage(&self->pages,
                                           GlyphAtlasPage_new(gfx,
                                                              self->pages.size,
                                                              false,
                                                              GL_RGB,
                                                              TEX_FMT_RGB,
                                                              self->page_size_px,
                                                              self->page_size_px));
                tgt_page = self->current_rgb_page = Vector_last_GlyphAtlasPage(&self->pages);
            }
            break;

        case FT_OUTPUT_GRAYSCALE:
            tgt_page = self->current_grayscale_page;
            if (unlikely(!tgt_page || !GlyphAtlasPage_can_push_tex(tgt_page, tex))) {
                Vector_push_GlyphAtlasPage(&self->pages,
                                           GlyphAtlasPage_new(gfx,
                                                              self->pages.size,
                                                              false,
                                                              GL_RED,
                                                              TEX_FMT_MONO,
                                                              self->page_size_px,
                                                              self->page_size_px));
                tgt_page = self->current_grayscale_page = Vector_last_GlyphAtlasPage(&self->pages);
            }
            break;

        case FT_OUTPUT_COLOR_BGRA:
            tgt_page = self->current_rgba_page;
            if (unlikely(!tgt_page || !GlyphAtlasPage_can_push_tex(tgt_page, tex))) {
                Vector_push_GlyphAtlasPage(&self->pages,
                                           GlyphAtlasPage_new(gfx,
                                                              self->pages.size,
                                                              true,
                                                              GL_RGBA,
                                                              TEX_FMT_RGBA,
                                                              self->page_size_px,
                                                              self->page_size_px));
                tgt_page = self->current_rgba_page = Vector_last_GlyphAtlasPage(&self->pages);
            }
            break;
        default:
            ASSERT_UNREACHABLE;
    }

    Rune key = *rune;
    if (output->style == FT_STYLE_NONE)
        key.style = VT_RUNE_UNSTYLED;

    return Map_insert_Rune_GlyphAtlasEntry(
      &self->entry_map,
      key,
      GlyphAtlasPage_push_tex(gfx, tgt_page, base_output, tex));
}

__attribute__((hot, always_inline)) static inline GlyphAtlasEntry*
GlyphAtlas_get_regular(GfxOpenGL21* gfx, GlyphAtlas* self, const Rune* rune)
{
    enum FreetypeFontStyle style = FT_STYLE_REGULAR;
    switch (rune->style) {
        case VT_RUNE_BOLD:
            style = FT_STYLE_BOLD;
            break;
        case VT_RUNE_ITALIC:
            style = FT_STYLE_ITALIC;
            break;
        case VT_RUNE_BOLD_ITALIC:
            style = FT_STYLE_BOLD_ITALIC;
            break;
        default:;
    }

    FreetypeOutput* output = Freetype_load_and_render_glyph(gfx->freetype, rune->code, style);
    if (!output) {
        WRN("Missing glyph u+%X\n", rune->code);
        return NULL;
    }
    GlyphAtlasPage* tgt_page;
    switch (output->type) {
        case FT_OUTPUT_RGB_H:
        case FT_OUTPUT_BGR_H:
        case FT_OUTPUT_RGB_V:
        case FT_OUTPUT_BGR_V:
            tgt_page = self->current_rgb_page;
            if (unlikely(!tgt_page || !GlyphAtlasPage_can_push(tgt_page, output))) {
                Vector_push_GlyphAtlasPage(&self->pages,
                                           GlyphAtlasPage_new(gfx,
                                                              self->pages.size,
                                                              false,
                                                              GL_RGB,
                                                              TEX_FMT_RGB,
                                                              self->page_size_px,
                                                              self->page_size_px));
                tgt_page = self->current_rgb_page = Vector_last_GlyphAtlasPage(&self->pages);
            }
            break;
        case FT_OUTPUT_GRAYSCALE:
            tgt_page = self->current_grayscale_page;
            if (unlikely(!tgt_page || !GlyphAtlasPage_can_push(tgt_page, output))) {
                Vector_push_GlyphAtlasPage(&self->pages,
                                           GlyphAtlasPage_new(gfx,
                                                              self->pages.size,
                                                              false,
                                                              GL_RED,
                                                              TEX_FMT_MONO,
                                                              self->page_size_px,
                                                              self->page_size_px));
                tgt_page = self->current_grayscale_page = Vector_last_GlyphAtlasPage(&self->pages);
            }
            break;
        case FT_OUTPUT_COLOR_BGRA:
            tgt_page = self->current_rgba_page;
            if (unlikely(!tgt_page || !GlyphAtlasPage_can_push(tgt_page, output))) {
                Vector_push_GlyphAtlasPage(&self->pages,
                                           GlyphAtlasPage_new(gfx,
                                                              self->pages.size,
                                                              true,
                                                              GL_RGBA,
                                                              TEX_FMT_RGBA,
                                                              self->page_size_px,
                                                              self->page_size_px));
                tgt_page = self->current_rgba_page = Vector_last_GlyphAtlasPage(&self->pages);
            }
            break;
        default:
            ASSERT_UNREACHABLE;
    }
    Rune key = *rune;
    if (output->style == FT_STYLE_NONE)
        key.style = VT_RUNE_UNSTYLED;
    return Map_insert_Rune_GlyphAtlasEntry(&self->entry_map,
                                           key,
                                           GlyphAtlasPage_push(tgt_page, output));
}

__attribute__((hot)) static GlyphAtlasEntry* GlyphAtlas_get(GfxOpenGL21* gfx,
                                                            GlyphAtlas*  self,
                                                            const Rune*  rune)
{
    GlyphAtlasEntry* entry = Map_get_Rune_GlyphAtlasEntry(&self->entry_map, rune);

    if (likely(entry)) {
        return entry;
    }

    Rune alt = *rune;

    if (!settings.has_bold_fonts && rune->style == VT_RUNE_BOLD) {
        alt.style = VT_RUNE_NORMAL;
        entry     = Map_get_Rune_GlyphAtlasEntry(&self->entry_map, &alt);
        if (likely(entry)) {
            return entry;
        }
    }
    if (!settings.has_italic_fonts && rune->style == VT_RUNE_ITALIC) {
        alt.style = VT_RUNE_NORMAL;
        entry     = Map_get_Rune_GlyphAtlasEntry(&self->entry_map, &alt);
        if (likely(entry)) {
            return entry;
        }
    }
    if (!settings.has_bold_italic_fonts && rune->style == VT_RUNE_BOLD_ITALIC) {
        alt.style = settings.has_bold_fonts     ? VT_RUNE_BOLD
                    : settings.has_italic_fonts ? VT_RUNE_ITALIC
                                                : VT_RUNE_NORMAL;
        entry     = Map_get_Rune_GlyphAtlasEntry(&self->entry_map, &alt);
        if (likely(entry)) {
            return entry;
        }
    }

    alt.style = VT_RUNE_UNSTYLED;
    entry     = Map_get_Rune_GlyphAtlasEntry(&self->entry_map, &alt);
    if (likely(entry)) {
        return entry;
    }

    if (unlikely(rune->combine[0])) {
        return GlyphAtlas_get_combined(gfx, self, rune);
    } else {
        return GlyphAtlas_get_regular(gfx, self, rune);
    }
}

// Generate a sinewave image and store it as an OpenGL texture
__attribute__((cold)) static Texture create_squiggle_texture(uint32_t w,
                                                             uint32_t h,
                                                             uint32_t thickness)
{
    const double MSAA = 4;
    w *= MSAA;
    h *= MSAA;

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    uint8_t* fragments                 = calloc(w * h * 4, 1);
    double   pixel_size                = 2.0 / h;
    double   stroke_width              = thickness * pixel_size * (MSAA / 1.3);
    double   stroke_fade               = pixel_size * MSAA * 2;
    double   distance_limit_full_alpha = POW2(stroke_width / 1.0);
    double   distance_limit_zero_alpha = POW2(stroke_width / 1.0 + stroke_fade);

    for (uint_fast32_t x = 0; x < w; ++x)
        for (uint_fast32_t y = 0; y < h; ++y) {
#define DISTANCE_SQR(_x, _y, _x2, _y2) (pow((_x2) - (_x), 2) + pow((_y2) - (_y), 2))
            uint8_t* fragment = &fragments[(y * w + x) * 4];
            double   x_frag   = (double)x / (double)w * 2.0 * M_PI;
            double y_frag = (double)y / (double)h * (2.0 + stroke_width * 2.0 + stroke_fade * 2.0) -
                            1.0 - stroke_width - stroke_fade;
            double y_curve          = sin(x_frag);
            double dx_frag          = cos(x_frag);
            double y_dist           = y_frag - y_curve;
            double closest_distance = DISTANCE_SQR(x_frag, y_frag, x_frag, y_curve);
            double step             = dx_frag * y_dist < 0.0 ? 0.001 : -0.001;

            for (double i = x_frag + step;; i += step / 2.0) {
                double i_distance = DISTANCE_SQR(x_frag, y_frag, i, sin(i));
                if (likely(i_distance <= closest_distance)) {
                    closest_distance = i_distance;
                } else {
                    break;
                }
            }

            fragments[3] = 0;

            if (closest_distance <= distance_limit_full_alpha) {
                fragment[0] = fragment[1] = fragment[2] = fragment[3] = UINT8_MAX;
            } else if (closest_distance < distance_limit_zero_alpha) {
                double alpha = (1.0 - (closest_distance - distance_limit_full_alpha) /
                                        (distance_limit_zero_alpha - distance_limit_full_alpha));

                fragment[0] = fragment[1] = fragment[2] = UINT8_MAX;
                fragment[3]                             = CLAMP(alpha * UINT8_MAX, 0, UINT8_MAX);
            }
        }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, fragments);

    free(fragments);

    return (Texture){ .id = tex, .format = TEX_FMT_RGBA, .w = w / MSAA, .h = h / MSAA };
}

void GfxOpenGL21_resize(Gfx* self, uint32_t w, uint32_t h)
{
    GfxOpenGL21* gl21 = gfxOpenGL21(self);
    GfxOpenGL21_destroy_recycled(gl21);

    gl21->win_w = w;
    gl21->win_h = h;

    gl21->sx = 2.0f / gl21->win_w;
    gl21->sy = 2.0f / gl21->win_h;

    gl21->line_height_pixels = gl21->freetype->line_height_pixels + settings.padd_glyph_y;
    gl21->glyph_width_pixels = gl21->freetype->glyph_width_pixels + settings.padd_glyph_x;
    gl21->gw                 = gl21->freetype->gw;

    FreetypeOutput* output =
      Freetype_load_ascii_glyph(gl21->freetype, settings.center_char, FT_STYLE_REGULAR);

    if (!output) {
        ERR("Failed to load character metrics, is font set up correctly?");
    }

    uint32_t hber = output->ft_slot->metrics.horiBearingY / 64 / 2 / 2 + 1;

    gl21->pen_begin_y = gl21->sy * (gl21->line_height_pixels / 2.0) + gl21->sy * hber;
    gl21->pen_begin_pixels_y =
      (float)(gl21->line_height_pixels / 1.75) + (float)hber + settings.offset_glyph_y;
    gl21->pen_begin_pixels_x = settings.offset_glyph_x;

    uint32_t height         = (gl21->line_height_pixels + settings.padd_glyph_y) * 64;
    gl21->line_height       = (float)height * gl21->sy / 64.0;
    gl21->glyph_width       = gl21->glyph_width_pixels * gl21->sx;
    gl21->max_cells_in_line = gl21->win_w / gl21->glyph_width_pixels;

    glViewport(0, 0, gl21->win_w, gl21->win_h);
    GfxOpenGL21_regenerate_line_quad_vbo(gl21);
}

Pair_uint32_t GfxOpenGL21_get_char_size(Gfx* self)
{
    GfxOpenGL21* gl21 = gfxOpenGL21(self);

    int32_t cols = MAX((gl21->win_w - 2 * settings.padding) /
                         (gfxOpenGL21(self)->freetype->glyph_width_pixels + settings.padd_glyph_x),
                       0);
    int32_t rows = MAX((gl21->win_h - 2 * settings.padding) /
                         (gfxOpenGL21(self)->freetype->line_height_pixels + settings.padd_glyph_y),
                       0);

    return (Pair_uint32_t){ .first = cols, .second = rows };
}

Pair_uint32_t GfxOpenGL21_pixels(Gfx* self, uint32_t c, uint32_t r)
{
    float x, y;
    x = c * (gfxOpenGL21(self)->freetype->glyph_width_pixels + settings.padd_glyph_x);
    y = r * (gfxOpenGL21(self)->freetype->line_height_pixels + settings.padd_glyph_y);
    return (Pair_uint32_t){ .first = x + 2 * settings.padding, .second = y + 2 * settings.padding };
}

void GfxOpenGL21_load_font(Gfx* self) {}

void GfxOpenGL21_init_with_context_activated(Gfx* self)
{
    GfxOpenGL21* gl21 = gfxOpenGL21(self);

    ASSERT(self->callbacks.user_data, "callback user data defined");
    ASSERT(self->callbacks.load_extension_proc_address, "callback func defined");

    maybe_load_gl_exts(self->callbacks.user_data, self->callbacks.load_extension_proc_address);

#ifdef DEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(on_gl_error, NULL);
#endif

    if (unlikely(settings.debug_gfx)) {
        fprintf(stderr, "GL_VENDOR = %s\n", glGetString(GL_VENDOR));
        fprintf(stderr, "GL_RENDERER = %s\n", glGetString(GL_RENDERER));
        fprintf(stderr, "GL_VERSION = %s\n", glGetString(GL_VERSION));
        fprintf(stderr,
                "GL_SHADING_LANGUAGE_VERSION = %s\n",
                glGetString(GL_SHADING_LANGUAGE_VERSION));
    }

    gl21->float_vec = Vector_new_with_capacity_Vector_float(3);
    Vector_push_Vector_float(&gl21->float_vec, Vector_new_float());

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    glDisable(GL_FRAMEBUFFER_SRGB);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glClearColor(ColorRGBA_get_float(settings.bg, 0),
                 ColorRGBA_get_float(settings.bg, 1),
                 ColorRGBA_get_float(settings.bg, 2),
                 ColorRGBA_get_float(settings.bg, 3));

    gl21->solid_fill_shader = Shader_new(solid_fill_vs_src, solid_fill_fs_src, "pos", "clr", NULL);
    gl21->font_shader = Shader_new(font_vs_src, font_fs_src, "coord", "tex", "clr", "bclr", NULL);
    gl21->font_shader_gray =
      Shader_new(font_vs_src, font_gray_fs_src, "coord", "tex", "clr", "bclr", NULL);
    gl21->font_shader_blend =
      Shader_new(font_vs_src, font_depth_blend_fs_src, "coord", "tex", NULL);
    gl21->line_shader = Shader_new(line_vs_src, line_fs_src, "pos", "clr", NULL);
    gfxOpenGL21(self)->image_shader =
      Shader_new(image_rgb_vs_src, image_rgb_fs_src, "coord", "tex", "offset", NULL);
    gl21->image_tint_shader =
      Shader_new(image_rgb_vs_src, image_tint_rgb_fs_src, "coord", "tex", "tint", "offset", NULL);

    gl21->flex_vbo = VBO_new(4, 1, gl21->font_shader.attribs);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 4, NULL, GL_STREAM_DRAW);

    GLuint new_vbos[2];
    glGenBuffers(2, new_vbos);
    gl21->full_framebuffer_quad_vbo = new_vbos[0];
    gl21->line_quads_vbo            = new_vbos[1];

    glBindBuffer(GL_ARRAY_BUFFER, gl21->full_framebuffer_quad_vbo);
    static const float vertex_data[] = {
        1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f,
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data, GL_STATIC_DRAW);
    GfxOpenGL21_regenerate_line_quad_vbo(gl21);

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &gl21->max_tex_res);

    gl21->color    = settings.fg;
    gl21->bg_color = settings.bg;

    gl21->glyph_atlas = GlyphAtlas_new(1024);

    Shader_use(&gl21->font_shader);
    glUniform3f(gl21->font_shader.uniforms[1].location,
                ColorRGB_get_float(settings.fg, 0),
                ColorRGB_get_float(settings.fg, 1),
                ColorRGB_get_float(settings.fg, 2));

    glGenFramebuffers(1, &gl21->line_framebuffer);

    gl21->blink_switch       = TimePoint_ms_from_now(settings.cursor_blink_interval_ms);
    gl21->blink_switch_text  = TimePoint_now();
    gl21->vec_vertex_buffer  = Vector_new_vertex_t();
    gl21->vec_vertex_buffer2 = Vector_new_vertex_t();

    Freetype* ft = gl21->freetype;

    gl21->line_height_pixels = ft->line_height_pixels + settings.padd_glyph_y;
    gl21->glyph_width_pixels = ft->glyph_width_pixels + settings.padd_glyph_x;
    uint32_t t_height        = CLAMP(gl21->line_height_pixels / 8.0 + 2, 4, UINT8_MAX);
    gl21->squiggle_texture =
      create_squiggle_texture(t_height * M_PI / 2.0, t_height, CLAMP((t_height / 4), 1, 20));
}

void GfxOpenGL21_reload_font(Gfx* self)
{
    GfxOpenGL21* gl21 = gfxOpenGL21(self);

    GfxOpenGL21_load_font(self);
    GfxOpenGL21_resize(self, gl21->win_w, gl21->win_h);

    GlyphAtlas_destroy(&gl21->glyph_atlas);
    gl21->glyph_atlas = GlyphAtlas_new(1024);

    GfxOpenGL21_regenerate_line_quad_vbo(gl21);

    // regenerate the squiggle texture
    glDeleteTextures(1, &gl21->squiggle_texture.id);
    uint32_t t_height = CLAMP(gl21->line_height_pixels / 8.0 + 2, 4, UINT8_MAX);
    gl21->squiggle_texture =
      create_squiggle_texture(t_height * M_PI / 2.0, t_height, CLAMP(t_height / 4, 1, 20));
}

static void GfxOpenGL21_regenerate_line_quad_vbo(GfxOpenGL21* gfx)
{
    uint32_t     n_lines  = GfxOpenGL21_get_char_size(gfxBase(gfx)).second;
    Vector_float transfer = Vector_new_with_capacity_float(n_lines * 4 * 4);
    for (uint32_t i = 0; i < n_lines; ++i) {
        float tex_end_x   = -1.0f + gfx->max_cells_in_line * gfx->glyph_width_pixels * gfx->sx;
        float tex_begin_y = 1.0f - gfx->line_height_pixels * (i + 1) * gfx->sy;
        float buf[]       = {
            -1.0f,     tex_begin_y + gfx->line_height,
            0.0f,      0.0f,
            -1.0,      tex_begin_y,
            0.0f,      1.0f,
            tex_end_x, tex_begin_y,
            1.0f,      1.0f,
            tex_end_x, tex_begin_y + gfx->line_height,
            1.0f,      0.0f,
        };
        Vector_pushv_float(&transfer, buf, ARRAY_SIZE(buf));
    }

    glBindBuffer(GL_ARRAY_BUFFER, gfx->line_quads_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * transfer.size, transfer.buf, GL_STATIC_DRAW);
    Vector_destroy_float(&transfer);
}

static void GfxOpenGL21_draw_line_quads(GfxOpenGL21*  gfx,
                                        Ui*           ui,
                                        VtLine* const vt_line,
                                        uint32_t      quad_index)
{
    if (likely(vt_line->proxy.data[PROXY_INDEX_TEXTURE] ||
               (vt_line->proxy.data[PROXY_INDEX_TEXTURE_BLINK]))) {
        uint8_t tidx = PROXY_INDEX_TEXTURE;
        if (unlikely(vt_line->proxy.data[PROXY_INDEX_TEXTURE_BLINK] && !ui->draw_text_blinking)) {
            tidx = PROXY_INDEX_TEXTURE_BLINK;
        }

        GLuint tex_id = vt_line->proxy.data[tidx];
        glBindTexture(GL_TEXTURE_2D, tex_id);
        glDrawArrays(GL_QUADS, quad_index * 4, 4);
    }
}

typedef struct
{
    GfxOpenGL21*      gl21;
    const Vt* const   vt;
    VtLine*           vt_line;
    VtLineProxy*      proxy;
    vt_line_damage_t* damage;
    size_t            visual_index;
    uint16_t          cnd_cursor_column;
    bool              is_for_cursor;
    bool              is_for_blinking;
} line_render_pass_args_t;

typedef struct
{
    uint16_t render_range_begin;
    uint16_t render_range_end;
} line_render_subpass_args_t;

typedef struct
{
    line_render_subpass_args_t* prepared_subpass;
    line_render_pass_args_t     args;
    GLuint                      final_texture;
    GLuint                      final_depthbuffer;
    line_render_subpass_args_t  subpass_args[2];
    uint16_t                    length;
    uint8_t                     n_queued_subpasses;
    bool                        has_blinking_chars;
    bool                        has_underlined_chars;
    bool                        is_reusing;
    uint32_t                    texture_width;
    uint32_t                    texture_height;
} line_render_pass_t;

static void line_reder_pass_run(line_render_pass_t* self);

static bool should_create_line_render_pass(const line_render_pass_args_t* args)
{
    vt_line_damage_t* dmg = OR(args->damage, &args->vt_line->damage);
    return !likely(!args->vt_line->data.size || (dmg->type == VT_LINE_DAMAGE_NONE));
}

static line_render_pass_t create_line_render_pass(const line_render_pass_args_t* args)
{
    line_render_pass_t rp = { .args                 = *args,
                              .final_texture        = 0,
                              .final_depthbuffer    = 0,
                              .prepared_subpass     = NULL,
                              .length               = args->vt_line->data.size,
                              .has_underlined_chars = false,
                              .has_blinking_chars   = false,
                              .is_reusing           = false,
                              .texture_width =
                                args->gl21->max_cells_in_line * args->gl21->glyph_width_pixels,
                              .texture_height     = args->gl21->line_height_pixels,
                              .n_queued_subpasses = 0,
                              .subpass_args       = {
                                {
                                  .render_range_begin = 0,
                                  .render_range_end   = 0,
                                },
                                {
                                  .render_range_begin = 0,
                                  .render_range_end   = 0,
                                },
                              } };

    if (!rp.args.proxy) {
        rp.args.proxy = &rp.args.vt_line->proxy;
    }

    if (!rp.args.damage) {
        rp.args.damage = &rp.args.vt_line->damage;
    }

    return rp;
}

static void line_render_pass_try_to_recover_proxies(line_render_pass_t* self)
{
    size_t proxy_tex_idx   = PROXY_INDEX_TEXTURE;
    size_t proxy_depth_idx = PROXY_INDEX_DEPTHBUFFER;

    if (unlikely(self->args.is_for_blinking)) {
        proxy_tex_idx   = PROXY_INDEX_TEXTURE_BLINK;
        proxy_depth_idx = PROXY_INDEX_DEPTHBUFFER_BLINK;
    }

    GLuint recovered_texture     = self->args.proxy->data[proxy_tex_idx];
    GLuint recovered_depthbuffer = self->args.proxy->data[proxy_depth_idx];

    self->is_reusing        = recovered_texture;
    self->final_texture     = recovered_texture;
    self->final_depthbuffer = recovered_depthbuffer;
}

static void line_render_pass_set_up_subpasses(line_render_pass_t* self)
{
#define CURSOR_OVERPAINT_FWD  3
#define CURSOR_OVERPAINT_BACK 4

    if (self->args.is_for_cursor) {
        uint16_t range_begin_idx = self->args.cnd_cursor_column;
        uint16_t range_end_idx   = range_begin_idx;

        for (uint_fast8_t i = 0; i < CURSOR_OVERPAINT_BACK && range_begin_idx; ++i) {
            --range_begin_idx;
        }

        for (uint_fast8_t i = 0; i < CURSOR_OVERPAINT_FWD && range_end_idx < self->length; ++i) {
            ++range_end_idx;
        }

        self->subpass_args[0] = (line_render_subpass_args_t){
            .render_range_begin = range_begin_idx,
            .render_range_end   = range_end_idx,
        };

        self->n_queued_subpasses = 1;

    } else /* not for cursor */ {
        const VtLine* const ln = self->args.vt_line;

        switch (self->args.damage->type) {
            case VT_LINE_DAMAGE_RANGE: {
                uint16_t range_begin_idx = self->args.damage->front;
                uint16_t range_end_idx   = self->args.damage->end + 1;

                while (range_begin_idx) {
                    char32_t this_char = ln->data.buf[range_begin_idx].rune.code;
                    char32_t prev_char = ln->data.buf[range_begin_idx - 1].rune.code;

                    if (this_char == ' ' && !unicode_is_ambiguous_width(prev_char) &&
                        wc_width(prev_char) < 2) {
                        break;
                    }
                    --range_begin_idx;
                }

                while (range_end_idx < self->args.vt_line->data.size && range_end_idx) {
                    char32_t this_char = ln->data.buf[range_end_idx].rune.code;
                    char32_t prev_char = ln->data.buf[range_end_idx - 1].rune.code;

                    ++range_end_idx;
                    if (this_char == ' ' && !unicode_is_ambiguous_width(prev_char) &&
                        wc_width(prev_char) < 2) {
                        break;
                    }
                }

                self->subpass_args[0] = (line_render_subpass_args_t){
                    .render_range_begin = range_begin_idx,
                    .render_range_end   = range_end_idx,
                };

                self->n_queued_subpasses = 1;
            } break;

            case VT_LINE_DAMAGE_SHIFT:
                // TODO:
            case VT_LINE_DAMAGE_FULL: {
                self->subpass_args[0] = (line_render_subpass_args_t){
                    .render_range_begin = 0,
                    .render_range_end   = self->length,
                };

                self->n_queued_subpasses = 1;
            } break;

            default:
                ASSERT_UNREACHABLE
        }
    }
}

static void line_render_pass_finalize(line_render_pass_t* self)
{
    // set proxy data to generated texture
    if (unlikely(self->args.is_for_blinking)) {
        self->args.proxy->data[PROXY_INDEX_TEXTURE_BLINK]     = self->final_texture;
        self->args.proxy->data[PROXY_INDEX_DEPTHBUFFER_BLINK] = self->final_depthbuffer;
        self->args.damage->type                               = VT_LINE_DAMAGE_NONE;
        self->args.damage->shift                              = 0;
        self->args.damage->front                              = 0;
        self->args.damage->end                                = 0;
    } else {
        self->args.proxy->data[PROXY_INDEX_TEXTURE]     = self->final_texture;
        self->args.proxy->data[PROXY_INDEX_DEPTHBUFFER] = self->final_depthbuffer;
        if (!self->has_blinking_chars) {
            self->args.damage->type  = VT_LINE_DAMAGE_NONE;
            self->args.damage->shift = 0;
            self->args.damage->front = 0;
            self->args.damage->end   = 0;
        }
    }

    if (unlikely(settings.debug_gfx)) {
        static float debug_tint = 0.0f;
        glDisable(GL_SCISSOR_TEST);
        glBindTexture(GL_TEXTURE_2D, 0);
        Shader_use(&self->args.gl21->solid_fill_shader);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glColor4f(fabs(sin(debug_tint)), fabs(cos(debug_tint)), sin(debug_tint), 0.1);
        glUniform4f(self->args.gl21->solid_fill_shader.uniforms[0].location,
                    fabs(sin(debug_tint)),
                    fabs(cos(debug_tint)),
                    sin(debug_tint),
                    0.1);
        glBindBuffer(GL_ARRAY_BUFFER, self->args.gl21->full_framebuffer_quad_vbo);
        glVertexAttribPointer(self->args.gl21->solid_fill_shader.attribs->location,
                              2,
                              GL_FLOAT,
                              GL_FALSE,
                              0,
                              0);
        glDrawArrays(GL_QUADS, 0, 4);

        glDisable(GL_BLEND);
        debug_tint += 0.5f;
        if (debug_tint > M_PI) {
            debug_tint -= M_PI;
        }
    }

    glDisable(GL_DEPTH_TEST);

    glBindFramebuffer(GL_FRAMEBUFFER, self->args.gl21->line_framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    gl_check_error();

    glViewport(0, 0, self->args.gl21->win_w, self->args.gl21->win_h);

    // There are no blinking characters, but their resources still exist
    if (!self->has_blinking_chars && self->args.proxy->data[PROXY_INDEX_TEXTURE_BLINK]) {
        // TODO: recycle?
        glDeleteTextures(1, (GLuint*)&self->args.proxy->data[PROXY_INDEX_TEXTURE_BLINK]);
        self->args.proxy->data[PROXY_INDEX_TEXTURE_BLINK] = 0;

        ASSERT(self->args.proxy->data[PROXY_INDEX_DEPTHBUFFER_BLINK],
               "deleted proxy texture has depth rb");
        glDeleteRenderbuffers(1, (GLuint*)&self->args.proxy->data[PROXY_INDEX_DEPTHBUFFER_BLINK]);
        self->args.proxy->data[PROXY_INDEX_DEPTHBUFFER_BLINK] = 0;
    }
}

static void line_render_pass_set_up_framebuffer(line_render_pass_t* self)
{
    if (self->is_reusing) {
        glBindFramebuffer(GL_FRAMEBUFFER, self->args.gl21->line_framebuffer);

        glBindTexture(GL_TEXTURE_2D, self->final_texture);
        glFramebufferTexture2D(GL_FRAMEBUFFER,
                               GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D,
                               self->final_texture,
                               0);
        glBindRenderbuffer(GL_RENDERBUFFER, self->final_depthbuffer);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                                  GL_DEPTH_ATTACHMENT,
                                  GL_RENDERBUFFER,
                                  self->final_depthbuffer);

        glViewport(0, 0, self->texture_width, self->texture_height);

        gl_check_error();
    } else {
        if (!self->args.is_for_blinking) {
            GfxOpenGL21_destroy_proxy(
              (void*)((uint8_t*)self->args.gl21 - offsetof(Gfx, extend_data)),
              self->args.proxy->data);
        }
        if (!self->args.vt_line->data.size) {
            return;
        }

        GLuint recycle_tex_id = self->args.gl21->recycled_textures[0].color_tex;

        if (recycle_tex_id) {
            Pair_GLuint recycled = GfxOpenGL21_pop_recycled(self->args.gl21);
            ASSERT(recycled.second, "recovered texture has a depth rb");
            self->final_texture     = recycled.first;
            self->final_depthbuffer = recycled.second;
            glBindFramebuffer(GL_FRAMEBUFFER, self->args.gl21->line_framebuffer);
            glBindTexture(GL_TEXTURE_2D, self->final_texture);

            glFramebufferTexture2D(GL_FRAMEBUFFER,
                                   GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_2D,
                                   self->final_texture,
                                   0);

            glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                                      GL_DEPTH_ATTACHMENT,
                                      GL_RENDERBUFFER,
                                      self->final_depthbuffer);
            gl_check_error();

        } else {
            // Generate new framebuffer attachments
            glGenTextures(1, &self->final_texture);
            glBindTexture(GL_TEXTURE_2D, self->final_texture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexImage2D(GL_TEXTURE_2D,
                         0,
                         GL_RGBA,
                         self->texture_width,
                         self->texture_height,
                         0,
                         GL_RGBA,
                         GL_UNSIGNED_BYTE,
                         0);

            glGenRenderbuffers(1, &self->final_depthbuffer);
            glBindRenderbuffer(GL_RENDERBUFFER, self->final_depthbuffer);
            glRenderbufferStorage(GL_RENDERBUFFER,
                                  GL_DEPTH_COMPONENT,
                                  self->texture_width,
                                  self->texture_height);

            glBindFramebuffer(GL_FRAMEBUFFER, self->args.gl21->line_framebuffer);
            glFramebufferTexture2D(GL_FRAMEBUFFER,
                                   GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_2D,
                                   self->final_texture,
                                   0);

            glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                                      GL_DEPTH_ATTACHMENT,
                                      GL_RENDERBUFFER,
                                      self->final_depthbuffer);
            gl_check_error();
        }
    }

    assert_farmebuffer_complete();
}

typedef struct
{
    line_render_subpass_args_t* args;
} line_render_subpass_t;

static line_render_subpass_t line_render_pass_create_subpass(line_render_pass_t*         self,
                                                             line_render_subpass_args_t* args)
{
    line_render_subpass_t sub = {
        .args = args,
    };

    return sub;
}

static void line_reder_pass_run_cell_subpass(line_render_pass_t*    pass,
                                             line_render_subpass_t* subpass)
{
    const double scalex = 2.0 / pass->texture_width;
    const double scaley = 2.0 / pass->texture_height;

    GLint bg_pixels_begin = subpass->args->render_range_begin * pass->args.gl21->glyph_width_pixels,
          bg_pixels_end;
    ColorRGBA active_bg_color =
      pass->args.is_for_cursor ? Vt_rune_cursor_bg(pass->args.vt, NULL) : pass->args.vt->colors.bg;
    VtRune* each_rune = pass->args.vt_line->data.buf + subpass->args->render_range_begin;
    VtRune* same_bg_block_begin_rune = each_rune;
    VtRune* cursor_rune              = Vt_cursor_cell(pass->args.vt);

    for (uint16_t idx_each_rune = subpass->args->render_range_begin;
         idx_each_rune <= subpass->args->render_range_end;) {
        each_rune = pass->args.vt_line->data.buf + idx_each_rune;
        if (likely(idx_each_rune != subpass->args->render_range_end)) {
            if (unlikely(each_rune->blinkng)) {
                pass->has_blinking_chars = true;
            }
            if (!pass->has_underlined_chars &&
                unlikely(each_rune->underlined || each_rune->strikethrough ||
                         each_rune->doubleunderline || each_rune->curlyunderline ||
                         each_rune->overline || each_rune->hyperlink_idx)) {
                pass->has_underlined_chars = true;
            }
        }

        if (idx_each_rune == subpass->args->render_range_end ||
            !ColorRGBA_eq(Vt_rune_final_bg(pass->args.vt,
                                           pass->args.is_for_cursor ? cursor_rune : each_rune,
                                           idx_each_rune,
                                           pass->args.visual_index,
                                           pass->args.is_for_cursor),
                          active_bg_color)) {
            int32_t extra_width = 0;

            if (idx_each_rune > 1) {
                extra_width =
                  MAX(wc_width(pass->args.vt_line->data.buf[idx_each_rune - 1].rune.code) - 2, 0);
            }

            bg_pixels_end = (idx_each_rune + extra_width) * pass->args.gl21->glyph_width_pixels;

            glEnable(GL_SCISSOR_TEST);
            glScissor(bg_pixels_begin, 0, bg_pixels_end - bg_pixels_begin, pass->texture_height);

            glClearColor(ColorRGBA_get_float(active_bg_color, 0),
                         ColorRGBA_get_float(active_bg_color, 1),
                         ColorRGBA_get_float(active_bg_color, 2),
                         ColorRGBA_get_float(active_bg_color, 3));

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            { // for each block of characters with the same background color
                ColorRGB      active_fg_color              = settings.fg;
                const VtRune* same_colors_block_begin_rune = same_bg_block_begin_rune;

                for (const VtRune* each_rune_same_bg = same_bg_block_begin_rune;
                     each_rune_same_bg != each_rune + 1;
                     ++each_rune_same_bg) {

                    if (each_rune_same_bg == each_rune ||
                        !ColorRGB_eq(
                          Vt_rune_final_fg(pass->args.vt,
                                           each_rune_same_bg,
                                           each_rune_same_bg - pass->args.vt_line->data.buf,
                                           pass->args.visual_index,
                                           active_bg_color,
                                           pass->args.is_for_cursor),
                          active_fg_color)) {

                        /* Dummy value with we can point to to filter out a character */
                        VtRune        same_color_blank_space;
                        const VtRune* each_rune_filtered_visible;

                        for (Vector_float* i = NULL;
                             (i = Vector_iter_Vector_float(&pass->args.gl21->float_vec, i));) {
                            Vector_clear_float(i);
                        }

                        for (const VtRune* each_rune_same_colors = same_colors_block_begin_rune;
                             each_rune_same_colors != each_rune_same_bg;
                             ++each_rune_same_colors) {
                            size_t column = each_rune_same_colors - pass->args.vt_line->data.buf;

                            /* Filter out stuff that should be hidden on this pass */
                            if (unlikely(
                                  (pass->args.is_for_blinking && each_rune_same_colors->blinkng) ||
                                  each_rune_same_colors->hidden)) {
                                same_color_blank_space           = *each_rune_same_colors;
                                same_color_blank_space.rune.code = ' ';
                                each_rune_filtered_visible       = &same_color_blank_space;
                            } else {
                                each_rune_filtered_visible = each_rune_same_colors;
                            }

                            if (each_rune_filtered_visible->rune.code > ' ') {
                                GlyphAtlasEntry* entry =
                                  GlyphAtlas_get(pass->args.gl21,
                                                 &pass->args.gl21->glyph_atlas,
                                                 &each_rune_filtered_visible->rune);
                                if (!entry) {
                                    continue;
                                }

                                float h = (float)entry->height * scaley;
                                float w = (float)entry->width * scalex;
                                float t = entry->top * scaley;
                                float l = entry->left * scalex;

                                if (unlikely(h > 2.0f && entry->can_scale)) {
                                    float s = h / 2.0f;
                                    h /= s;
                                    w /= s;
                                    t /= s;
                                    l /= s;
                                }

                                float x3 =
                                  -1.0 +
                                  (double)column * pass->args.gl21->glyph_width_pixels * scalex +
                                  l + pass->args.gl21->pen_begin_pixels_x * scalex;
                                float y3 = -1.0 + pass->args.gl21->pen_begin_pixels_y * scaley - t;

                                float buf[] = {
                                    x3,     y3,     entry->tex_coords[0], entry->tex_coords[1],
                                    x3 + w, y3,     entry->tex_coords[2], entry->tex_coords[1],
                                    x3 + w, y3 + h, entry->tex_coords[2], entry->tex_coords[3],
                                    x3,     y3 + h, entry->tex_coords[0], entry->tex_coords[3],
                                };

                                while (pass->args.gl21->float_vec.size <= entry->page_id) {
                                    Vector_push_Vector_float(&pass->args.gl21->float_vec,
                                                             Vector_new_float());
                                }

                                Vector_pushv_float(&pass->args.gl21->float_vec.buf[entry->page_id],
                                                   buf,
                                                   ARRAY_SIZE(buf));
                            }
                        }
                        GLint clip_begin =
                          (same_colors_block_begin_rune - pass->args.vt_line->data.buf) *
                          pass->args.gl21->glyph_width_pixels;
                        GLsizei clip_end = (each_rune_same_bg - pass->args.vt_line->data.buf) *
                                           pass->args.gl21->glyph_width_pixels;

                        glEnable(GL_SCISSOR_TEST);
                        glScissor(clip_begin, 0, clip_end - clip_begin, pass->texture_height);

                        // actual drawing
                        for (size_t i = 0; i < pass->args.gl21->glyph_atlas.pages.size; ++i) {
                            Vector_float*   v    = &pass->args.gl21->float_vec.buf[i];
                            GlyphAtlasPage* page = &pass->args.gl21->glyph_atlas.pages.buf[i];

                            glBindTexture(GL_TEXTURE_2D, page->texture_id);
                            glBindBuffer(GL_ARRAY_BUFFER, pass->args.gl21->flex_vbo.vbo);

                            size_t newsize = v->size * sizeof(float);
                            ARRAY_BUFFER_SUB_OR_SWAP(v->buf,
                                                     pass->args.gl21->flex_vbo.size,
                                                     newsize);

                            switch (page->texture_format) {
                                case TEX_FMT_RGB:
                                    if (pass->args.gl21->bound_resources != BOUND_RESOURCES_FONT) {
                                        pass->args.gl21->bound_resources = BOUND_RESOURCES_FONT;
                                        glUseProgram(pass->args.gl21->font_shader.id);
                                    }
                                    glVertexAttribPointer(
                                      pass->args.gl21->font_shader.attribs->location,
                                      4,
                                      GL_FLOAT,
                                      GL_FALSE,
                                      0,
                                      0);
                                    glUniform3f(pass->args.gl21->font_shader.uniforms[1].location,
                                                ColorRGB_get_float(active_fg_color, 0),
                                                ColorRGB_get_float(active_fg_color, 1),
                                                ColorRGB_get_float(active_fg_color, 2));
                                    glUniform4f(pass->args.gl21->font_shader.uniforms[2].location,
                                                ColorRGBA_get_float(active_bg_color, 0),
                                                ColorRGBA_get_float(active_bg_color, 1),
                                                ColorRGBA_get_float(active_bg_color, 2),
                                                ColorRGBA_get_float(active_bg_color, 3));
                                    break;

                                case TEX_FMT_MONO:
                                    if (pass->args.gl21->bound_resources !=
                                        BOUND_RESOURCES_FONT_MONO) {
                                        pass->args.gl21->bound_resources =
                                          BOUND_RESOURCES_FONT_MONO;
                                        glUseProgram(pass->args.gl21->font_shader_gray.id);
                                    }
                                    glVertexAttribPointer(
                                      pass->args.gl21->font_shader_gray.attribs->location,
                                      4,
                                      GL_FLOAT,
                                      GL_FALSE,
                                      0,
                                      0);
                                    glUniform3f(
                                      pass->args.gl21->font_shader_gray.uniforms[1].location,
                                      ColorRGB_get_float(active_fg_color, 0),
                                      ColorRGB_get_float(active_fg_color, 1),
                                      ColorRGB_get_float(active_fg_color, 2));

                                    glUniform4f(
                                      pass->args.gl21->font_shader_gray.uniforms[2].location,
                                      ColorRGBA_get_float(active_bg_color, 0),
                                      ColorRGBA_get_float(active_bg_color, 1),
                                      ColorRGBA_get_float(active_bg_color, 2),
                                      ColorRGBA_get_float(active_bg_color, 3));
                                    break;
                                case TEX_FMT_RGBA:
                                    if (pass->args.gl21->bound_resources != BOUND_RESOURCES_IMAGE) {
                                        pass->args.gl21->bound_resources = BOUND_RESOURCES_IMAGE;
                                        glUseProgram(pass->args.gl21->image_shader.id);
                                    }

                                    glEnable(GL_BLEND);
                                    glBlendFuncSeparate(GL_ONE,
                                                        GL_ONE_MINUS_SRC_COLOR,
                                                        GL_ONE,
                                                        GL_ONE);
                                    glVertexAttribPointer(
                                      pass->args.gl21->image_shader.attribs->location,
                                      4,
                                      GL_FLOAT,
                                      GL_FALSE,
                                      0,
                                      0);
                                default:;
                            }

                            glDrawArrays(GL_QUADS, 0, v->size / 4);
                            glDisable(GL_BLEND);
                        }
                        // end drawing

                        glDisable(GL_SCISSOR_TEST);

                        if (each_rune_same_bg != each_rune) {
                            same_colors_block_begin_rune = each_rune_same_bg;
                            // update active fg color;
                            if (settings.highlight_change_fg &&
                                unlikely(Vt_is_cell_selected(pass->args.vt,
                                                             each_rune_same_bg -
                                                               pass->args.vt_line->data.buf,
                                                             pass->args.visual_index))) {
                                active_fg_color = pass->args.vt->colors.highlight.fg;
                            } else {
                                active_fg_color =
                                  Vt_rune_final_fg_apply_dim(pass->args.vt,
                                                             each_rune_same_bg,
                                                             active_bg_color,
                                                             pass->args.is_for_cursor);
                            }
                        }

                    } // end for each block with the same color
                }     // end for each char
            }         // end for each block with the same bg

            bg_pixels_begin = (idx_each_rune + extra_width) * pass->args.gl21->glyph_width_pixels;

            int clip_begin = idx_each_rune * pass->args.gl21->glyph_width_pixels;
            glEnable(GL_SCISSOR_TEST);
            glScissor(clip_begin, 0, pass->texture_width, pass->texture_height);

            if (idx_each_rune != subpass->args->render_range_end) {
                same_bg_block_begin_rune = each_rune;
                active_bg_color          = Vt_rune_final_bg(pass->args.vt,
                                                   each_rune,
                                                   idx_each_rune,
                                                   pass->args.visual_index,
                                                   pass->args.is_for_cursor);
            }
        } // end if bg color changed

        int w = likely(idx_each_rune != subpass->args->render_range_end)
                  ? wc_width(pass->args.vt_line->data.buf[idx_each_rune].rune.code)
                  : 1;

        idx_each_rune = CLAMP(idx_each_rune + (unlikely(w > 1) ? w : 1),
                              subpass->args->render_range_begin,
                              (uint16_t)(pass->args.vt_line->data.size + 1));
    }
}

static void line_reder_pass_run_line_subpass(const line_render_pass_t* pass,
                                             line_render_subpass_t*    subpass)
{
    /* Scale from pixels to GL coordinates */
    const double scalex = 2.0f / pass->texture_width;
    const double scaley = 2.0f / pass->texture_height;

    // draw lines
    static float begin[6]   = { -1, -1, -1, -1, -1 };
    static float end[6]     = { 1, 1, 1, 1, 1, 1 };
    static bool  drawing[6] = { 0 };

    if (unlikely(subpass->args->render_range_begin)) {
        const float init_coord = unlikely(subpass->args->render_range_end)
                                   ? -1.0f + pass->args.gl21->glyph_width_pixels * scalex *
                                               subpass->args->render_range_begin
                                   : 0.0f;

        for (uint_fast8_t i = 0; i < ARRAY_SIZE(begin); ++i) {
            begin[i] = init_coord;
        }
    }

    // lines are drawn in the same color as the character, unless the line color was explicitly set
    ColorRGB line_color =
      Vt_rune_ln_clr(pass->args.vt,
                     &pass->args.vt_line->data.buf[subpass->args->render_range_begin]);
    glDisable(GL_SCISSOR_TEST);
    glEnable(GL_BLEND);

    for (const VtRune* each_rune = pass->args.vt_line->data.buf + subpass->args->render_range_begin;
         each_rune <= pass->args.vt_line->data.buf + subpass->args->render_range_end;
         ++each_rune) {

        /* text column where this should be drawn */
        size_t   column = each_rune - pass->args.vt_line->data.buf;
        ColorRGB nc     = { 0 };
        if (each_rune != pass->args.vt_line->data.buf + subpass->args->render_range_end) {
            nc = Vt_rune_ln_clr(pass->args.vt, each_rune);
        }
        // State has changed
        if (!ColorRGB_eq(line_color, nc) ||
            each_rune == pass->args.vt_line->data.buf + subpass->args->render_range_end ||
            each_rune->underlined != drawing[0] || each_rune->doubleunderline != drawing[1] ||
            each_rune->strikethrough != drawing[2] || each_rune->overline != drawing[3] ||
            each_rune->curlyunderline != drawing[4] ||
            (each_rune->hyperlink_idx != 0) != drawing[5]) {

            if (each_rune == pass->args.vt_line->data.buf + subpass->args->render_range_end) {
                for (uint_fast8_t tmp = 0; tmp < ARRAY_SIZE(end); tmp++) {
                    end[tmp] =
                      -1.0f + (float)column * scalex * (float)pass->args.gl21->glyph_width_pixels;
                }
            } else {
#define L_SET_BOUNDS_END(what, index)                                                              \
    if (drawing[index]) {                                                                          \
        end[index] = -1.0f + (float)column * scalex * (float)pass->args.gl21->glyph_width_pixels;  \
    }
                L_SET_BOUNDS_END(z->underlined, 0);
                L_SET_BOUNDS_END(z->doubleunderline, 1);
                L_SET_BOUNDS_END(z->strikethrough, 2);
                L_SET_BOUNDS_END(z->overline, 3);
                L_SET_BOUNDS_END(z->curlyunderline, 4);
                L_SET_BOUNDS_END(z->hyperlink_idx, 5);
            }

            Vector_clear_vertex_t(&pass->args.gl21->vec_vertex_buffer);
            Vector_clear_vertex_t(&pass->args.gl21->vec_vertex_buffer2);

            if (drawing[0]) {
                Vector_push_vertex_t(&pass->args.gl21->vec_vertex_buffer,
                                     (vertex_t){ begin[0], 1.0f - scaley });
                Vector_push_vertex_t(&pass->args.gl21->vec_vertex_buffer,
                                     (vertex_t){ end[0], 1.0f - scaley });
            }
            if (drawing[1]) {
                Vector_push_vertex_t(&pass->args.gl21->vec_vertex_buffer,
                                     (vertex_t){ begin[1], 1.0f });
                Vector_push_vertex_t(&pass->args.gl21->vec_vertex_buffer,
                                     (vertex_t){ end[1], 1.0f });
                Vector_push_vertex_t(&pass->args.gl21->vec_vertex_buffer,
                                     (vertex_t){ begin[1], 1.0f - 2 * scaley });
                Vector_push_vertex_t(&pass->args.gl21->vec_vertex_buffer,
                                     (vertex_t){ end[1], 1.0f - 2 * scaley });
            }
            if (drawing[2]) {
                Vector_push_vertex_t(&pass->args.gl21->vec_vertex_buffer,
                                     (vertex_t){ begin[2], .2f });
                Vector_push_vertex_t(&pass->args.gl21->vec_vertex_buffer,
                                     (vertex_t){ end[2], .2f });
            }
            if (drawing[3]) {
                Vector_push_vertex_t(&pass->args.gl21->vec_vertex_buffer,
                                     (vertex_t){ begin[3], -1.0f + scaley });
                Vector_push_vertex_t(&pass->args.gl21->vec_vertex_buffer,
                                     (vertex_t){ end[3], -1.0f + scaley });
            }
            if (drawing[4]) {
                float   cw      = pass->args.gl21->glyph_width_pixels * scalex;
                int32_t n_cells = round((end[4] - begin[4]) / cw);
                float   t_y     = 1.0f - pass->args.gl21->squiggle_texture.h * scaley;

                Vector_push_vertex_t(&pass->args.gl21->vec_vertex_buffer2,
                                     (vertex_t){ begin[4], t_y });
                Vector_push_vertex_t(&pass->args.gl21->vec_vertex_buffer2,
                                     (vertex_t){ 0.0f, 0.0f });

                Vector_push_vertex_t(&pass->args.gl21->vec_vertex_buffer2,
                                     (vertex_t){ begin[4], 1.0f });
                Vector_push_vertex_t(&pass->args.gl21->vec_vertex_buffer2,
                                     (vertex_t){ 0.0f, 1.0f });

                Vector_push_vertex_t(&pass->args.gl21->vec_vertex_buffer2,
                                     (vertex_t){ end[4], 1.0f });
                Vector_push_vertex_t(&pass->args.gl21->vec_vertex_buffer2,
                                     (vertex_t){ 1.0f * n_cells, 1.0f });

                Vector_push_vertex_t(&pass->args.gl21->vec_vertex_buffer2,
                                     (vertex_t){ end[4], t_y });
                Vector_push_vertex_t(&pass->args.gl21->vec_vertex_buffer2,
                                     (vertex_t){ 1.0f * n_cells, 0.0f });
            }
            if (drawing[5] && !drawing[0]) {
                for (float i = begin[5]; i < (end[5] - scalex * 0.5f);
                     i += (scalex * pass->args.gl21->glyph_width_pixels)) {
                    float j = i + scalex * pass->args.gl21->glyph_width_pixels / 2.0;
                    Vector_push_vertex_t(&pass->args.gl21->vec_vertex_buffer,
                                         (vertex_t){ i, 1.0f - scaley });
                    Vector_push_vertex_t(&pass->args.gl21->vec_vertex_buffer,
                                         (vertex_t){ j, 1.0f - scaley });
                }
            }

            if (pass->args.gl21->vec_vertex_buffer.size) {
                if (pass->args.gl21->bound_resources != BOUND_RESOURCES_LINES) {
                    pass->args.gl21->bound_resources = BOUND_RESOURCES_LINES;
                    Shader_use(&pass->args.gl21->line_shader);
                    glBindBuffer(GL_ARRAY_BUFFER, pass->args.gl21->flex_vbo.vbo);
                    glVertexAttribPointer(pass->args.gl21->line_shader.attribs->location,
                                          2,
                                          GL_FLOAT,
                                          GL_FALSE,
                                          0,
                                          0);
                }
                glUniform3f(pass->args.gl21->line_shader.uniforms[1].location,
                            ColorRGB_get_float(line_color, 0),
                            ColorRGB_get_float(line_color, 1),
                            ColorRGB_get_float(line_color, 2));
                size_t new_size = sizeof(vertex_t) * pass->args.gl21->vec_vertex_buffer.size;
                ARRAY_BUFFER_SUB_OR_SWAP(pass->args.gl21->vec_vertex_buffer.buf,
                                         pass->args.gl21->flex_vbo.size,
                                         new_size);
                glDrawArrays(GL_LINES, 0, pass->args.gl21->vec_vertex_buffer.size);
            }
            if (pass->args.gl21->vec_vertex_buffer2.size) {
                pass->args.gl21->bound_resources = BOUND_RESOURCES_NONE;
                Shader_use(&pass->args.gl21->image_tint_shader);
                glBindTexture(GL_TEXTURE_2D, pass->args.gl21->squiggle_texture.id);
                glUniform3f(pass->args.gl21->image_tint_shader.uniforms[1].location,
                            ColorRGB_get_float(line_color, 0),
                            ColorRGB_get_float(line_color, 1),
                            ColorRGB_get_float(line_color, 2));
                glBindBuffer(GL_ARRAY_BUFFER, pass->args.gl21->flex_vbo.vbo);
                glVertexAttribPointer(pass->args.gl21->font_shader.attribs->location,
                                      4,
                                      GL_FLOAT,
                                      GL_FALSE,
                                      0,
                                      0);
                size_t new_size = sizeof(vertex_t) * pass->args.gl21->vec_vertex_buffer2.size;
                ARRAY_BUFFER_SUB_OR_SWAP(pass->args.gl21->vec_vertex_buffer2.buf,
                                         pass->args.gl21->flex_vbo.size,
                                         new_size);
                glDrawArrays(GL_QUADS, 0, pass->args.gl21->vec_vertex_buffer2.size / 2);
            }

#define L_SET_BOUNDS_BEGIN(what, index)                                                            \
    if (what) {                                                                                    \
        begin[index] =                                                                             \
          -1.0f + (float)column * scalex * (float)pass->args.gl21->glyph_width_pixels;             \
    }

            if (each_rune != pass->args.vt_line->data.buf + subpass->args->render_range_end) {
                L_SET_BOUNDS_BEGIN(each_rune->underlined, 0);
                L_SET_BOUNDS_BEGIN(each_rune->doubleunderline, 1);
                L_SET_BOUNDS_BEGIN(each_rune->strikethrough, 2);
                L_SET_BOUNDS_BEGIN(each_rune->overline, 3);
                L_SET_BOUNDS_BEGIN(each_rune->curlyunderline, 4);
                L_SET_BOUNDS_BEGIN(each_rune->hyperlink_idx, 5);
                drawing[0] = each_rune->underlined;
                drawing[1] = each_rune->doubleunderline;
                drawing[2] = each_rune->strikethrough;
                drawing[3] = each_rune->overline;
                drawing[4] = each_rune->curlyunderline;
                drawing[5] = each_rune->hyperlink_idx != 0;
            } else {
                memset(drawing, false, sizeof(drawing));
            }

            line_color = nc;
        }
    }
}

static void line_reder_subpass_run_clear_stage(const line_render_pass_t* self,
                                               line_render_subpass_t*    subpass)
{
    glViewport(0, 0, self->texture_width, self->texture_height);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);

    if (self->args.is_for_cursor) {
        ColorRGBA clr = Vt_rune_cursor_bg(self->args.vt, NULL);
        glClearColor(ColorRGBA_get_float(clr, 0),
                     ColorRGBA_get_float(clr, 1),
                     ColorRGBA_get_float(clr, 2),
                     ColorRGBA_get_float(clr, 3));
    } else {
        glClearColor(ColorRGBA_get_float(self->args.vt->colors.bg, 0),
                     ColorRGBA_get_float(self->args.vt->colors.bg, 1),
                     ColorRGBA_get_float(self->args.vt->colors.bg, 2),
                     ColorRGBA_get_float(self->args.vt->colors.bg, 3));
    }

    if (self->args.damage->type == VT_LINE_DAMAGE_RANGE) {
        glEnable(GL_SCISSOR_TEST);
        size_t begin_px = self->args.gl21->glyph_width_pixels * self->args.damage->front;
        size_t width_px = ((self->args.damage->end + 1) - self->args.damage->front) *
                          self->args.gl21->glyph_width_pixels;
        glScissor(begin_px, 0, width_px, self->texture_height);
    } else {
        glDisable(GL_SCISSOR_TEST);
    }
    // TODO: VT_LINE_DAMAGE_SHIFT

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_DEPTH_TEST);

    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glDepthRange(0.0f, 1.0f);
}

static void line_reder_pass_run_subpass(line_render_pass_t* pass, line_render_subpass_t* subpass)
{
    line_reder_subpass_run_clear_stage(pass, subpass);
    line_reder_pass_run_cell_subpass(pass, subpass);

    if (pass->has_underlined_chars) {
        line_reder_pass_run_line_subpass(pass, subpass);
    }
}

static void line_reder_pass_run(line_render_pass_t* self)
{
    line_render_pass_try_to_recover_proxies(self);
    line_render_pass_set_up_framebuffer(self);
    line_render_pass_set_up_subpasses(self);

    for (int i = 0; i < self->n_queued_subpasses; ++i) {
        line_render_subpass_t sub = line_render_pass_create_subpass(self, &self->subpass_args[i]);
        line_reder_pass_run_subpass(self, &sub);
    }
}

static void _GfxOpenGL21_draw_block_cursor(GfxOpenGL21* gfx,
                                           const Vt*    vt,
                                           const Ui*    ui,
                                           ColorRGBA    clr,
                                           size_t       row)
{
    ((vt_line_damage_t*)&ui->cursor_damage)->type = VT_LINE_DAMAGE_FULL; // test
    VtLine* vt_line                               = Vt_cursor_line(vt);

    line_render_pass_args_t rp_args = {
        .gl21              = gfx,
        .vt                = vt,
        .vt_line           = vt_line,
        .proxy             = (VtLineProxy*)&ui->cursor_proxy,
        .damage            = (vt_line_damage_t*)&ui->cursor_damage,
        .visual_index      = Vt_visual_cursor_row(vt),
        .cnd_cursor_column = ui->cursor->col,
        .is_for_cursor     = true,
        .is_for_blinking   = false,
    };

    if (should_create_line_render_pass(&rp_args)) {
        gfx->bound_resources  = BOUND_RESOURCES_NONE;
        line_render_pass_t rp = create_line_render_pass(&rp_args);
        line_reder_pass_run(&rp);

        if (rp.has_blinking_chars) {
            gfxBase(gfx)->has_blinking_text = true;
            rp_args.is_for_blinking         = true;
            gfx->bound_resources            = BOUND_RESOURCES_NONE;
            line_render_pass_t rp_b         = create_line_render_pass(&rp_args);
            line_reder_pass_run(&rp_b);
            rp_b.has_blinking_chars = true;
            line_render_pass_finalize(&rp_b);
            rp_args.is_for_blinking = false;
        }

        line_render_pass_finalize(&rp);
    }

    glViewport(0, 0, gfx->win_w, gfx->win_h);
    double dbl_col = ui->cursor_cell_fraction;
    {
        glEnable(GL_SCISSOR_TEST);
        GLint x   = dbl_col * gfx->glyph_width_pixels + gfx->pixel_offset_x,
              y   = gfx->win_h - (row + 1) * gfx->line_height_pixels - gfx->pixel_offset_y;
        GLsizei w = gfx->glyph_width_pixels, h = gfx->line_height_pixels;
        glScissor(x, y, w, h);
    }

    glClearColor(ColorRGBA_get_float(clr, 0),
                 ColorRGBA_get_float(clr, 1),
                 ColorRGBA_get_float(clr, 2),
                 ColorRGBA_get_float(clr, 3));

    glClear(GL_COLOR_BUFFER_BIT);

    float tex_begin_x = -1.0f + (gfx->pixel_offset_x) * gfx->sx;
    float tex_end_x =
      -1.0f + (gfx->max_cells_in_line * gfx->glyph_width_pixels + gfx->pixel_offset_x) * gfx->sx;
    float tex_begin_y = 1.0f - gfx->line_height_pixels * (Vt_visual_cursor_row(vt) + 1) * gfx->sy -
                        gfx->pixel_offset_y * gfx->sy;

    float buf[] = {
        tex_begin_x, tex_begin_y + gfx->line_height,
        0.0f,        0.0f,
        tex_begin_x, tex_begin_y,
        0.0f,        1.0f,
        tex_end_x,   tex_begin_y,
        1.0f,        1.0f,
        tex_end_x,   tex_begin_y + gfx->line_height,
        1.0f,        0.0f,
    };

    Shader_use(&gfx->image_shader);
    glBindTexture(GL_TEXTURE_2D,
                  ui->cursor_proxy.data[PROXY_INDEX_TEXTURE_BLINK] && !ui->draw_text_blinking
                    ? ui->cursor_proxy.data[PROXY_INDEX_TEXTURE_BLINK]
                    : ui->cursor_proxy.data[PROXY_INDEX_TEXTURE]);
    glUniform2f(gfx->image_shader.uniforms[1].location, 0, 0);

    glVertexAttribPointer(gfx->image_shader.attribs->location, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, gfx->flex_vbo.vbo);
    size_t newsize = sizeof(buf);
    ARRAY_BUFFER_SUB_OR_SWAP(buf, gfx->flex_vbo.size, newsize);

    glDrawArrays(GL_QUADS, 0, 4);
}

static void GfxOpenGL21_draw_cursor(GfxOpenGL21* gfx, const Vt* vt, const Ui* ui)
{
    bool show_blink = !settings.enable_cursor_blink || !ui->window_in_focus ||
                      !ui->cursor->blinking || (ui->cursor->blinking && ui->draw_cursor_blinking);

    if (show_blink && !ui->cursor->hidden) {
        bool   filled_block = false;
        size_t row = ui->cursor->row - Vt_visual_top_line(vt), st_col = ui->cursor->col;
        double col = ui->cursor_cell_fraction;

        if (row >= Vt_row(vt))
            return;

        Vector_clear_vertex_t(&gfx->vec_vertex_buffer);
        switch (ui->cursor->type) {
            case CURSOR_BEAM:
                Vector_pushv_vertex_t(
                  &gfx->vec_vertex_buffer,
                  (vertex_t[2]){ { .x = -1.0f + (1 + col * gfx->glyph_width_pixels) * gfx->sx,
                                   .y = 1.0f - row * gfx->line_height_pixels * gfx->sy },
                                 { .x = -1.0f + (1 + col * gfx->glyph_width_pixels) * gfx->sx,
                                   .y = 1.0f - (row + 1) * gfx->line_height_pixels * gfx->sy } },
                  2);
                break;
            case CURSOR_UNDERLINE:
                Vector_pushv_vertex_t(
                  &gfx->vec_vertex_buffer,
                  (vertex_t[2]){ { .x = -1.0f + col * gfx->glyph_width_pixels * gfx->sx,
                                   .y = 1.0f - ((row + 1) * gfx->line_height_pixels) * gfx->sy },
                                 { .x = -1.0f + (col + 1) * gfx->glyph_width_pixels * gfx->sx,
                                   .y = 1.0f - ((row + 1) * gfx->line_height_pixels) * gfx->sy } },
                  2);
                break;
            case CURSOR_BLOCK:
                if (!ui->window_in_focus)
                    Vector_pushv_vertex_t(
                      &gfx->vec_vertex_buffer,
                      (vertex_t[4]){
                        { .x = -1.0f + (col * gfx->glyph_width_pixels) * gfx->sx + 0.9f * gfx->sx,
                          .y = 1.0f - ((row + 1) * gfx->line_height_pixels) * gfx->sy +
                               0.5f * gfx->sy },
                        { .x = -1.0f + ((col + 1) * gfx->glyph_width_pixels) * gfx->sx,
                          .y = 1.0f - ((row + 1) * gfx->line_height_pixels) * gfx->sy +
                               0.5f * gfx->sy },
                        { .x = -1.0f + ((col + 1) * gfx->glyph_width_pixels) * gfx->sx,
                          .y = 1.0f - (row * gfx->line_height_pixels) * gfx->sy - 0.5f * gfx->sy },
                        { .x = -1.0f + (col * gfx->glyph_width_pixels) * gfx->sx + 0.9f * gfx->sx,
                          .y = 1.0f - (row * gfx->line_height_pixels) * gfx->sy } },
                      4);
                else
                    filled_block = true;
                break;
        }

        VtRune* cursor_rune = NULL;
        if (vt->lines.size > ui->cursor->row && vt->lines.buf[ui->cursor->row].data.size > st_col) {
            cursor_rune = &vt->lines.buf[ui->cursor->row].data.buf[st_col];
        }
        ColorRGBA clr = Vt_rune_cursor_bg(vt, cursor_rune);

        if (!filled_block) {
            Shader_use(&gfx->line_shader);
            glBindTexture(GL_TEXTURE_2D, 0);

            glBindBuffer(GL_ARRAY_BUFFER, gfx->flex_vbo.vbo);
            glVertexAttribPointer(gfx->line_shader.attribs->location, 2, GL_FLOAT, GL_FALSE, 0, 0);

            glUniform3f(gfx->line_shader.uniforms[1].location,
                        ColorRGBA_get_float(clr, 0),
                        ColorRGBA_get_float(clr, 1),
                        ColorRGBA_get_float(clr, 2));

            size_t newsize = gfx->vec_vertex_buffer.size * sizeof(vertex_t);
            ARRAY_BUFFER_SUB_OR_SWAP(gfx->vec_vertex_buffer.buf, gfx->flex_vbo.size, newsize);
            glDrawArrays(gfx->vec_vertex_buffer.size == 2 ? GL_LINES : GL_LINE_LOOP,
                         0,
                         gfx->vec_vertex_buffer.size);
        } else {
            _GfxOpenGL21_draw_block_cursor(gfx, vt, ui, clr, row);
        }

        glDisable(GL_SCISSOR_TEST);
        glDisable(GL_BLEND);
    }
}

__attribute__((cold)) static void GfxOpenGL21_draw_unicode_input(GfxOpenGL21* gfx, const Vt* vt)
{
    size_t begin = MIN(vt->cursor.col, vt->ws.ws_col - vt->unicode_input.buffer.size - 1);
    size_t row   = vt->cursor.row - Vt_visual_top_line(vt);
    size_t col   = begin;
    glEnable(GL_SCISSOR_TEST);
    {
        glScissor(col * gfx->glyph_width_pixels + gfx->pixel_offset_x,
                  gfx->win_h - (row + 1) * gfx->line_height_pixels - gfx->pixel_offset_y,
                  gfx->glyph_width_pixels * (vt->unicode_input.buffer.size + 1),
                  gfx->line_height_pixels);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glBindBuffer(GL_ARRAY_BUFFER, gfx->flex_vbo.vbo);
        Rune rune = {
            .code    = 'u',
            .combine = { 0 },
            .style   = VT_RUNE_NORMAL,
        };
        GlyphAtlasEntry* entry = GlyphAtlas_get(gfx, &gfx->glyph_atlas, &rune);
        float            h     = (float)entry->height * gfx->sy;
        float            w     = (float)entry->width * gfx->sx;
        float            t     = entry->top * gfx->sy;
        float            l     = entry->left * gfx->sx;
        float            x3    = -1.0f + (float)col * gfx->glyph_width_pixels * gfx->sx + l +
                   gfx->pen_begin_pixels_x * gfx->sx;
        float y3 = 1.0f - (float)(row)*gfx->line_height_pixels * gfx->sy -
                   gfx->pen_begin_pixels_y * gfx->sy + t;
        float buf[] = {
            x3,     y3,     entry->tex_coords[0], entry->tex_coords[1],
            x3 + w, y3,     entry->tex_coords[2], entry->tex_coords[1],
            x3 + w, y3 - h, entry->tex_coords[2], entry->tex_coords[3],
            x3,     y3 - h, entry->tex_coords[0], entry->tex_coords[3],
        };
        GlyphAtlasPage* page = &gfx->glyph_atlas.pages.buf[entry->page_id];
        glBindTexture(GL_TEXTURE_2D, page->texture_id);
        glBindBuffer(GL_ARRAY_BUFFER, gfx->flex_vbo.vbo);
        size_t newsize = sizeof(buf);
        ARRAY_BUFFER_SUB_OR_SWAP(buf, gfx->flex_vbo.size, newsize);
        switch (page->texture_format) {
            case TEX_FMT_RGB: {
                glUseProgram(gfx->font_shader.id);
                GLuint loc = gfx->font_shader.attribs->location;
                glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, 0);
                glUniform3f(gfx->font_shader.uniforms[1].location, 0.0f, 0.0f, 0.0f);
                glUniform4f(gfx->font_shader.uniforms[2].location, 1.0f, 1.0f, 1.0f, 1.0f);
            } break;
            case TEX_FMT_MONO: {
                glUseProgram(gfx->font_shader_gray.id);
                GLuint loc = gfx->font_shader_gray.attribs->location;
                glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, 0);
                glUniform3f(gfx->font_shader_gray.uniforms[1].location, 0.0f, 0.0f, 0.0f);
                glUniform4f(gfx->font_shader_gray.uniforms[2].location, 1.0f, 1.0f, 1.0f, 1.0f);
            } break;
            default:
                ASSERT_UNREACHABLE;
        }
        glDrawArrays(GL_QUADS, 0, 4);
    }

    for (size_t i = 0; i < vt->unicode_input.buffer.size; ++i) {
        ++col;
        glBindBuffer(GL_ARRAY_BUFFER, gfx->flex_vbo.vbo);
        Rune rune = (Rune){
            .code    = vt->unicode_input.buffer.buf[i],
            .combine = { 0 },
            .style   = VT_RUNE_NORMAL,
        };

        GlyphAtlasEntry* entry = GlyphAtlas_get(gfx, &gfx->glyph_atlas, &rune);
        float            h     = (float)entry->height * gfx->sy;
        float            w     = (float)entry->width * gfx->sx;
        float            t     = entry->top * gfx->sy;
        float            l     = entry->left * gfx->sx;
        float            x3    = -1.0f + (float)col * gfx->glyph_width_pixels * gfx->sx + l +
                   gfx->pen_begin_pixels_x * gfx->sx;
        float y3 = 1.0f - (float)(row)*gfx->line_height_pixels * gfx->sy -
                   gfx->pen_begin_pixels_y * gfx->sy + t;
        float buf[] = {
            x3,     y3,     entry->tex_coords[0], entry->tex_coords[1],
            x3 + w, y3,     entry->tex_coords[2], entry->tex_coords[1],
            x3 + w, y3 - h, entry->tex_coords[2], entry->tex_coords[3],
            x3,     y3 - h, entry->tex_coords[0], entry->tex_coords[3],
        };
        GlyphAtlasPage* page = &gfx->glyph_atlas.pages.buf[entry->page_id];
        glBindTexture(GL_TEXTURE_2D, page->texture_id);
        glBindBuffer(GL_ARRAY_BUFFER, gfx->flex_vbo.vbo);
        size_t newsize = sizeof(buf);
        ARRAY_BUFFER_SUB_OR_SWAP(buf, gfx->flex_vbo.size, newsize);

        switch (page->texture_format) {
            case TEX_FMT_RGB: {
                glUseProgram(gfx->font_shader.id);
                GLuint loc = gfx->font_shader.attribs->location;
                glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, 0);
                glUniform3f(gfx->font_shader.uniforms[1].location, 0.0f, 0.0f, 0.0f);
                glUniform4f(gfx->font_shader.uniforms[2].location, 1.0f, 1.0f, 1.0f, 1.0f);
            } break;
            case TEX_FMT_MONO: {
                glUseProgram(gfx->font_shader_gray.id);
                GLuint loc = gfx->font_shader_gray.attribs->location;
                glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, 0);
                glUniform3f(gfx->font_shader_gray.uniforms[1].location, 0.0f, 0.0f, 0.0f);
                glUniform4f(gfx->font_shader_gray.uniforms[2].location, 1.0f, 1.0f, 1.0f, 1.0f);
            } break;
            default:
                ASSERT_UNREACHABLE;
        }
        glDrawArrays(GL_QUADS, 0, 4);
    }
    glDisable(GL_SCISSOR_TEST);
}

static void GfxOpenGL21_draw_scrollbar(GfxOpenGL21* self, const Scrollbar* scrollbar)
{
    glEnable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    Shader_use(&self->solid_fill_shader);
    glUniform4f(self->solid_fill_shader.uniforms[0].location,
                1.0f,
                1.0f,
                1.0f,
                scrollbar->dragging ? 0.8f : scrollbar->opacity * 0.5f);
    glViewport(0, 0, self->win_w, self->win_h);
    glBindTexture(GL_TEXTURE_2D, 0);

    float length = scrollbar->length;
    float begin  = scrollbar->top;
    float width  = self->sx * scrollbar->width;

    float slide         = (1.0f - scrollbar->opacity) * scrollbar->width * self->sx;
    float vertex_data[] = {
        1.0f - width + slide,
        1.0f - begin,
        1.0f,
        1.0f - begin,
        1.0f,
        1.0f - length - begin,
        1.0f - width + slide,
        1.0f - length - begin,
    };

    glBindBuffer(GL_ARRAY_BUFFER, self->flex_vbo.vbo);
    ARRAY_BUFFER_SUB_OR_SWAP(vertex_data, self->flex_vbo.size, (sizeof vertex_data));
    glVertexAttribPointer(self->solid_fill_shader.attribs->location, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glDrawArrays(GL_QUADS, 0, 4);
}

static void GfxOpenGL21_draw_hovered_link(GfxOpenGL21* self, const Vt* vt, const Ui* ui)
{
    Vector_clear_vertex_t(&self->vec_vertex_buffer);

    if (ui->hovered_link.start_line_idx == ui->hovered_link.end_line_idx) {
        size_t yidx = (ui->hovered_link.start_line_idx + 1) - Vt_visual_top_line(vt);

        float x = -1.0f + (ui->pixel_offset_x +
                           self->glyph_width_pixels * ui->hovered_link.start_cell_idx) *
                            self->sx;
        float y = 1.0f - (ui->pixel_offset_y + self->line_height_pixels * yidx - 1) * self->sy;

        Vector_push_vertex_t(&self->vec_vertex_buffer, (vertex_t){ x, y });
        x = -1.0f +
            (ui->pixel_offset_x + self->glyph_width_pixels * (ui->hovered_link.end_cell_idx + 1)) *
              self->sx;
        Vector_push_vertex_t(&self->vec_vertex_buffer, (vertex_t){ x, y });
    } else {
        size_t yidx = (ui->hovered_link.start_line_idx + 1) - Vt_visual_top_line(vt);
        float  x    = -1.0f + (ui->pixel_offset_x +
                           self->glyph_width_pixels * ui->hovered_link.start_cell_idx) *
                            self->sx;
        float y = 1.0f - (ui->pixel_offset_y + self->line_height_pixels * yidx - 1) * self->sy;
        Vector_push_vertex_t(&self->vec_vertex_buffer, (vertex_t){ x, y });
        x = -1.0f + (ui->pixel_offset_x + self->glyph_width_pixels * Vt_col(vt)) * self->sx;
        Vector_push_vertex_t(&self->vec_vertex_buffer, (vertex_t){ x, y });

        for (size_t row = ui->hovered_link.start_line_idx + 1; row < ui->hovered_link.end_line_idx;
             ++row) {
            yidx = (row + 1) - Vt_visual_top_line(vt);
            y    = 1.0f - (ui->pixel_offset_y + self->line_height_pixels * yidx - 1) * self->sy;
            x    = -1.0f + ui->pixel_offset_x * self->sx;
            Vector_push_vertex_t(&self->vec_vertex_buffer, (vertex_t){ x, y });
            x =
              -1.0f + (ui->pixel_offset_x + self->glyph_width_pixels * (Vt_col(vt) - 1)) * self->sx;
            Vector_push_vertex_t(&self->vec_vertex_buffer, (vertex_t){ x, y });
        }
        yidx = (ui->hovered_link.end_line_idx + 1) - Vt_visual_top_line(vt);
        y    = 1.0f - (ui->pixel_offset_y + self->line_height_pixels * yidx - 1) * self->sy;
        x    = -1.0f + ui->pixel_offset_x * self->sx;
        Vector_push_vertex_t(&self->vec_vertex_buffer, (vertex_t){ x, y });
        x = -1.0f +
            (ui->pixel_offset_x + self->glyph_width_pixels * (ui->hovered_link.end_cell_idx + 1)) *
              self->sx;
        Vector_push_vertex_t(&self->vec_vertex_buffer, (vertex_t){ x, y });
    }

    glViewport(0, 0, self->win_w, self->win_h);
    glBindTexture(GL_TEXTURE_2D, 0);
    Shader_use(&self->line_shader);
    glBindBuffer(GL_ARRAY_BUFFER, self->flex_vbo.vbo);
    glVertexAttribPointer(self->line_shader.attribs->location, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glUniform3f(self->line_shader.uniforms[1].location,
                ColorRGB_get_float(vt->colors.fg, 0),
                ColorRGB_get_float(vt->colors.fg, 1),
                ColorRGB_get_float(vt->colors.fg, 2));
    size_t new_size = sizeof(vertex_t) * self->vec_vertex_buffer.size;
    ARRAY_BUFFER_SUB_OR_SWAP(self->vec_vertex_buffer.buf, self->flex_vbo.size, new_size);
    glDrawArrays(GL_LINES, 0, self->vec_vertex_buffer.size);
}

static void GfxOpenGL21_draw_overlays(GfxOpenGL21* self, const Vt* vt, const Ui* ui)
{
    if (vt->unicode_input.active) {
        GfxOpenGL21_draw_unicode_input(self, vt);
    } else {
        GfxOpenGL21_draw_cursor(self, vt, ui);
    }

    if (ui->scrollbar.visible) {
        GfxOpenGL21_draw_scrollbar(self, &ui->scrollbar);
    }

    if (ui->hovered_link.active) {
        GfxOpenGL21_draw_hovered_link(self, vt, ui);
    }
}

static void GfxOpenGL21_draw_flash(GfxOpenGL21* self, double fraction)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_SCISSOR_TEST);
    glBindTexture(GL_TEXTURE_2D, 0);
    Shader_use(&self->solid_fill_shader);
    glUniform4f(self->solid_fill_shader.uniforms[0].location,
                ColorRGBA_get_float(settings.bell_flash, 0),
                ColorRGBA_get_float(settings.bell_flash, 1),
                ColorRGBA_get_float(settings.bell_flash, 2),
                (double)ColorRGBA_get_float(settings.bell_flash, 3) * fraction);
    glBindBuffer(GL_ARRAY_BUFFER, self->full_framebuffer_quad_vbo);
    glVertexAttribPointer(self->solid_fill_shader.attribs->location, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glDrawArrays(GL_QUADS, 0, 4);
}

static void GfxOpenGL21_draw_tint(GfxOpenGL21* self)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_SCISSOR_TEST);
    glBindTexture(GL_TEXTURE_2D, 0);
    Shader_use(&self->solid_fill_shader);
    glUniform4f(self->solid_fill_shader.uniforms[0].location,
                ColorRGBA_get_float(settings.dim_tint, 0),
                ColorRGBA_get_float(settings.dim_tint, 1),
                ColorRGBA_get_float(settings.dim_tint, 2),
                ColorRGBA_get_float(settings.dim_tint, 3));
    glBindBuffer(GL_ARRAY_BUFFER, self->full_framebuffer_quad_vbo);
    glVertexAttribPointer(self->solid_fill_shader.attribs->location, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glDrawArrays(GL_QUADS, 0, 4);
}

static void GfxOpenGL21_load_image(GfxOpenGL21* self, VtImageSurface* surface)
{
    if (surface->state != VT_IMAGE_SURFACE_READY ||
        surface->proxy.data[IMG_PROXY_INDEX_TEXTURE_ID]) {
        return;
    }

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 surface->bytes_per_pixel == 3 ? GL_RGB : GL_RGBA,
                 surface->width,
                 surface->height,
                 0,
                 surface->bytes_per_pixel == 3 ? GL_RGB : GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 surface->fragments.buf);
    glGenerateMipmap(GL_TEXTURE_2D);
    surface->proxy.data[IMG_PROXY_INDEX_TEXTURE_ID] = tex;
}

static void GfxOpenGL21_load_image_view(GfxOpenGL21* self, VtImageSurfaceView* view)
{
    if (view->proxy.data[IMG_VIEW_PROXY_INDEX_VBO_ID]) {
        return;
    }

    VtImageSurface* surf = RcPtr_get_VtImageSurface(&view->source_image_surface);

    float w = self->sx * (view->cell_scale_rect.first
                            ? (view->cell_scale_rect.first * self->glyph_width_pixels)
                            : OR(view->sample_dims_px.first, surf->width));
    float h = self->sy * (view->cell_scale_rect.second
                            ? (view->cell_scale_rect.second * self->line_height_pixels)
                            : OR(view->sample_dims_px.second, surf->height));

    float sample_x = (float)view->anchor_offset_px.first / surf->width;
    float sample_y = (float)view->anchor_offset_px.second / surf->height;
    float sample_w =
      view->sample_dims_px.first ? ((float)view->sample_dims_px.first / surf->width) : 1.0f;
    float sample_h =
      view->sample_dims_px.second ? ((float)view->sample_dims_px.second / surf->height) : 1.0f;

    // set the origin points to the top left corner of framebuffer and image
    float vertex_data[][4] = {
        { -1.0f, 1.0f - h, sample_x, sample_y + sample_h },
        { -1.0f + w, 1.0f - h, sample_x + sample_w, sample_y + sample_h },
        { -1.0f + w, 1, sample_x + sample_w, sample_y },
        { -1.0f, 1, sample_x, sample_y },
    };

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data, GL_STATIC_DRAW);
    view->proxy.data[IMG_VIEW_PROXY_INDEX_VBO_ID] = vbo;
}

static void GfxOpenGL21_draw_image_view(GfxOpenGL21* self, const Vt* vt, VtImageSurfaceView* view)
{
    if (!Vt_ImageSurfaceView_is_visual_visible(vt, view)) {
        return;
    }

    VtImageSurface* surf = RcPtr_get_VtImageSurface(&view->source_image_surface);
    GfxOpenGL21_load_image(self, surf);
    GfxOpenGL21_load_image_view(self, view);

    GLuint vbo = view->proxy.data[IMG_VIEW_PROXY_INDEX_VBO_ID];
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glEnable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);

    Shader_use(&self->image_shader);
    glBindTexture(GL_TEXTURE_2D, surf->proxy.data[IMG_PROXY_INDEX_TEXTURE_ID]);

    int64_t y_index = view->anchor_global_index - Vt_visual_top_line(vt);

    float offset_x =
      self->sx * (view->anchor_cell_idx * self->glyph_width_pixels + view->anchor_offset_px.first);
    float offset_y =
      -self->sy * (y_index * self->line_height_pixels + view->anchor_offset_px.second);

    glVertexAttribPointer(self->image_shader.attribs->location, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glUniform2f(self->image_shader.uniforms[1].location, offset_x, offset_y);
    glDrawArrays(GL_QUADS, 0, 4);
}

void GfxOpenGL21_load_sixel(GfxOpenGL21* self, Vt* vt, VtSixelSurface* srf)
{
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGB,
                 srf->width,
                 srf->height,
                 0,
                 GL_RGB,
                 GL_UNSIGNED_BYTE,
                 srf->fragments.buf);
    srf->proxy.data[SIXEL_PROXY_INDEX_TEXTURE_ID] = tex;

    float w                = self->sx * srf->width;
    float h                = self->sy * srf->height;
    float vertex_data[][4] = {
        { -1.0f, 1.0f - h, 0.0f, 1.0f },
        { -1.0f + w, 1.0f - h, 1.0f, 1.0f },
        { -1.0f + w, 1, 1.0f, 0.0f },
        { -1.0f, 1, 0.0f, 0.0f },
    };

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data, GL_STATIC_DRAW);
    srf->proxy.data[SIXEL_PROXY_INDEX_VBO_ID] = vbo;
}

void GfxOpenGL21_draw_sixel(GfxOpenGL21* self, Vt* vt, VtSixelSurface* srf)
{
    if (unlikely(!srf->proxy.data[SIXEL_PROXY_INDEX_TEXTURE_ID])) {
        GfxOpenGL21_load_sixel(self, vt, srf);
    }
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
    Shader_use(&self->image_shader);
    glBindTexture(GL_TEXTURE_2D, srf->proxy.data[SIXEL_PROXY_INDEX_TEXTURE_ID]);

    int64_t y_index  = srf->anchor_global_index - Vt_visual_top_line(vt);
    float   offset_x = self->sx * (srf->anchor_cell_idx * self->glyph_width_pixels);
    float   offset_y = -self->sy * (y_index * self->line_height_pixels);

    glBindBuffer(GL_ARRAY_BUFFER, srf->proxy.data[SIXEL_PROXY_INDEX_VBO_ID]);
    glVertexAttribPointer(self->image_shader.attribs->location, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glUniform2f(self->image_shader.uniforms[1].location, offset_x, offset_y);
    glDrawArrays(GL_QUADS, 0, 4);
}

void GfxOpenGL21_draw_sixels(GfxOpenGL21* self, Vt* vt)
{
    for (RcPtr_VtSixelSurface* i = NULL;
         (i = Vector_iter_RcPtr_VtSixelSurface(&vt->scrolled_sixels, i));) {
        VtSixelSurface* ptr = RcPtr_get_VtSixelSurface(i);
        if (!ptr) {
            continue;
        }

        uint32_t six_ycells = Vt_pixels_to_cells(vt, 0, ptr->height).second + 1;
        if (ptr->anchor_global_index < Vt_visual_bottom_line(vt) &&
            ptr->anchor_global_index + six_ycells > Vt_visual_top_line(vt)) {
            GfxOpenGL21_draw_sixel(self, vt, ptr);
        }
    }
}

void GfxOpenGL21_draw_images(GfxOpenGL21* self, const Vt* vt, bool up_to_zero_z)
{
    for (VtLine* l = NULL; (l = Vector_iter_VtLine((Vector_VtLine*)&vt->lines, l));) {
        if (l->graphic_attachments && l->graphic_attachments->images) {
            for (RcPtr_VtImageSurfaceView* i = NULL;
                 (i = Vector_iter_RcPtr_VtImageSurfaceView(l->graphic_attachments->images, i));) {
                VtImageSurfaceView* view = RcPtr_get_VtImageSurfaceView(i);
                VtImageSurface*     surf = RcPtr_get_VtImageSurface(&view->source_image_surface);
                if (surf->state == VT_IMAGE_SURFACE_READY &&
                    ((view->z_layer >= 0 && !up_to_zero_z) ||
                     (view->z_layer < 0 && up_to_zero_z))) {
                    GfxOpenGL21_draw_image_view(self, vt, view);
                }
            }
        }
    }
}

void GfxOpenGL21_draw(Gfx* self, const Vt* vt, Ui* ui)
{
    GfxOpenGL21* gfx    = gfxOpenGL21(self);
    gfx->pixel_offset_x = ui->pixel_offset_x;
    gfx->pixel_offset_y = ui->pixel_offset_y;

    VtLine *begin, *end;
    Vt_get_visible_lines(vt, &begin, &end);
    glDisable(GL_SCISSOR_TEST);
    glViewport(0, 0, gfx->win_w, gfx->win_h);
    glClearColor(ColorRGBA_get_float(vt->colors.bg, 0),
                 ColorRGBA_get_float(vt->colors.bg, 1),
                 ColorRGBA_get_float(vt->colors.bg, 2),
                 ColorRGBA_get_float(vt->colors.bg, 3));

    glClear(GL_COLOR_BUFFER_BIT);

    for (VtLine* i = begin; i < end; ++i) {
        line_render_pass_args_t rp_args = {
            .gl21            = gfx,
            .vt              = vt,
            .vt_line         = i,
            .proxy           = NULL,
            .damage          = NULL,
            .visual_index    = i - begin,
            .is_for_cursor   = false,
            .is_for_blinking = false,
        };
        if (should_create_line_render_pass(&rp_args)) {
            gfx->bound_resources  = BOUND_RESOURCES_NONE;
            line_render_pass_t rp = create_line_render_pass(&rp_args);
            line_reder_pass_run(&rp);

            if (unlikely(rp.has_blinking_chars)) {
                self->has_blinking_text = true;
                gfx->bound_resources    = BOUND_RESOURCES_NONE;
                rp_args.is_for_blinking = true;
                line_render_pass_t rp_b = create_line_render_pass(&rp_args);
                line_reder_pass_run(&rp_b);
                rp_b.has_blinking_chars = true;
                line_render_pass_finalize(&rp_b);
                rp_args.is_for_blinking = false;
            }

            line_render_pass_finalize(&rp);
        }
    }

    glDisable(GL_BLEND);
    glEnable(GL_SCISSOR_TEST);
    Pair_uint32_t chars = Gfx_get_char_size(self);
    if (vt->scrolling_visual) {
        glScissor(gfx->pixel_offset_x,
                  gfx->pixel_offset_y,
                  chars.first * gfx->glyph_width_pixels,
                  gfx->win_h);
    } else {
        glScissor(gfx->pixel_offset_x,
                  gfxOpenGL21(self)->win_h - chars.second * gfx->line_height_pixels -
                    gfx->pixel_offset_y,
                  chars.first * gfx->glyph_width_pixels,
                  chars.second * gfx->line_height_pixels);
    }

    GfxOpenGL21_draw_images(gfx, vt, true);
    glBindBuffer(GL_ARRAY_BUFFER, gfx->line_quads_vbo);
    Shader_use(&gfx->image_shader);
    glUniform2f(gfx->image_shader.uniforms[1].location, 0, 0);
    glVertexAttribPointer(gfx->image_shader.attribs->location, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glViewport(gfx->pixel_offset_x, -gfx->pixel_offset_y, gfx->win_w, gfx->win_h);

    for (VtLine* i = begin; i < end; ++i) {
        GfxOpenGL21_draw_line_quads(gfx, ui, i, i - begin);
    }

    GfxOpenGL21_draw_images(gfx, vt, false);
    GfxOpenGL21_draw_sixels(gfx, (Vt*)vt);
    GfxOpenGL21_draw_overlays(gfx, vt, ui);

    if (ui->flash_fraction != 0.0) {
        glViewport(0, 0, gfx->win_w, gfx->win_h);
        GfxOpenGL21_draw_flash(gfx, ui->flash_fraction);
    }

    if (unlikely(ui->draw_out_of_focus_tint && settings.dim_tint.a)) {
        GfxOpenGL21_draw_tint(gfx);
    }

    if (unlikely(settings.debug_gfx)) {
        static bool repaint_indicator_visible = true;
        if (repaint_indicator_visible) {
            Shader_use(&gfx->solid_fill_shader);
            glBindTexture(GL_TEXTURE_2D, 0);
            float vertex_data[] = { -1.0f, 1.0f,  -1.0f + gfx->sx * 50.0f,
                                    1.0f,  -1.0f, 1.0f - gfx->sy * 50.0f };
            glBindBuffer(GL_ARRAY_BUFFER, gfx->flex_vbo.vbo);
            ARRAY_BUFFER_SUB_OR_SWAP(vertex_data, gfx->flex_vbo.size, (sizeof vertex_data));
            glVertexAttribPointer(gfx->solid_fill_shader.attribs->location,
                                  2,
                                  GL_FLOAT,
                                  GL_FALSE,
                                  0,
                                  0);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }
        repaint_indicator_visible = !repaint_indicator_visible;
    }
}

void GfxOpenGL21_destroy_recycled(GfxOpenGL21* self)
{
    for (uint_fast8_t i = 0; i < ARRAY_SIZE(self->recycled_textures); ++i) {
        if (self->recycled_textures[i].color_tex) {
            glDeleteTextures(1, &self->recycled_textures[i].color_tex);
            glDeleteRenderbuffers(1, &self->recycled_textures[i].depth_rb);
        }
        self->recycled_textures[i].color_tex = 0;
        self->recycled_textures[i].depth_rb  = 0;
    }
}

void GfxOpenGL21_push_recycled(GfxOpenGL21* self, GLuint tex_id, GLuint rb_id)
{
    for (uint_fast8_t insert_point = 0; insert_point < N_RECYCLED_TEXTURES; ++insert_point) {
        if (!self->recycled_textures[insert_point].color_tex) {
            if (likely(ARRAY_LAST(self->recycled_textures).color_tex)) {
                ASSERT(ARRAY_LAST(self->recycled_textures).depth_rb,
                       "deleted texture has depth rb");
                glDeleteTextures(1, &ARRAY_LAST(self->recycled_textures).color_tex);
                glDeleteRenderbuffers(1, &ARRAY_LAST(self->recycled_textures).depth_rb);
            }

            void*  src = self->recycled_textures + insert_point;
            void*  dst = self->recycled_textures + insert_point + 1;
            size_t n = sizeof(*self->recycled_textures) * (N_RECYCLED_TEXTURES - insert_point - 1);
            memmove(dst, src, n);
            self->recycled_textures[insert_point].color_tex = tex_id;
            self->recycled_textures[insert_point].depth_rb  = rb_id;
            return;
        }
    }

    glDeleteTextures(1, &tex_id);
    glDeleteRenderbuffers(1, &rb_id);
}

Pair_GLuint GfxOpenGL21_pop_recycled(GfxOpenGL21* self)
{
    Pair_GLuint ret = { .first  = self->recycled_textures[0].color_tex,
                        .second = self->recycled_textures[0].depth_rb };

    memmove(self->recycled_textures,
            self->recycled_textures + 1,
            sizeof(*self->recycled_textures) * (N_RECYCLED_TEXTURES - 1));

    ARRAY_LAST(self->recycled_textures).color_tex = 0;
    ARRAY_LAST(self->recycled_textures).depth_rb  = 0;
    return ret;
}

void GfxOpenGL21_destroy_image_proxy(Gfx* self, uint32_t* proxy)
{
    if (proxy[IMG_PROXY_INDEX_TEXTURE_ID]) {
        glDeleteTextures(1, &proxy[IMG_PROXY_INDEX_TEXTURE_ID]);
        proxy[IMG_PROXY_INDEX_TEXTURE_ID] = 0;
    }
}

void GfxOpenGL21_destroy_sixel_proxy(Gfx* self, uint32_t* proxy)
{
    if (proxy[SIXEL_PROXY_INDEX_TEXTURE_ID]) {
        glDeleteTextures(1, &proxy[SIXEL_PROXY_INDEX_TEXTURE_ID]);
        glDeleteBuffers(1, &proxy[SIXEL_PROXY_INDEX_VBO_ID]);
        proxy[SIXEL_PROXY_INDEX_TEXTURE_ID] = 0;
        proxy[SIXEL_PROXY_INDEX_VBO_ID]     = 0;
    }
}

void GfxOpenGL21_destroy_image_view_proxy(Gfx* self, uint32_t* proxy)
{
    if (proxy[IMG_VIEW_PROXY_INDEX_VBO_ID]) {
        glDeleteBuffers(1, proxy);
        proxy[IMG_VIEW_PROXY_INDEX_VBO_ID] = 0;
    }
}

__attribute__((hot)) void GfxOpenGL21_destroy_proxy(Gfx* self, uint32_t* proxy)
{
    if (unlikely(proxy[PROXY_INDEX_TEXTURE])) {
        GfxOpenGL21_push_recycled(gfxOpenGL21(self),
                                  proxy[PROXY_INDEX_TEXTURE],
                                  proxy[PROXY_INDEX_DEPTHBUFFER]);

        if (unlikely(proxy[PROXY_INDEX_TEXTURE_BLINK])) {
            GfxOpenGL21_push_recycled(gfxOpenGL21(self),
                                      proxy[PROXY_INDEX_TEXTURE_BLINK],
                                      proxy[PROXY_INDEX_DEPTHBUFFER_BLINK]);
        }
    } else if (likely(proxy[PROXY_INDEX_TEXTURE])) {
        /* delete starting from first */
        ASSERT(proxy[PROXY_INDEX_DEPTHBUFFER], "deleted proxy texture has a renderbuffer");

        int del_num = unlikely(proxy[PROXY_INDEX_TEXTURE_BLINK]) ? 2 : 1;

        if (del_num == 2) {
            ASSERT(proxy[PROXY_INDEX_DEPTHBUFFER_BLINK],
                   "deleted proxy texture has a renderbuffer");
        }
        glDeleteTextures(del_num, (GLuint*)&proxy[PROXY_INDEX_TEXTURE]);
        glDeleteRenderbuffers(del_num, (GLuint*)&proxy[PROXY_INDEX_DEPTHBUFFER]);

    } else if (unlikely(proxy[PROXY_INDEX_TEXTURE_BLINK])) {
        ASSERT_UNREACHABLE;
        glDeleteTextures(1, (GLuint*)&proxy[PROXY_INDEX_TEXTURE_BLINK]);
        glDeleteRenderbuffers(1, (GLuint*)&proxy[PROXY_INDEX_DEPTHBUFFER_BLINK]);
    }
    proxy[PROXY_INDEX_TEXTURE]           = 0;
    proxy[PROXY_INDEX_TEXTURE_BLINK]     = 0;
    proxy[PROXY_INDEX_DEPTHBUFFER]       = 0;
    proxy[PROXY_INDEX_DEPTHBUFFER_BLINK] = 0;
}

void GfxOpenGL21_destroy(Gfx* self)
{
    glUseProgram(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    GfxOpenGL21_destroy_recycled(gfxOpenGL21(self));
    glDeleteTextures(1, &gfxOpenGL21(self)->squiggle_texture.id);
    glDeleteFramebuffers(1, &gfxOpenGL21(self)->line_framebuffer);
    VBO_destroy(&gfxOpenGL21(self)->flex_vbo);
    glDeleteBuffers(1, &gfxOpenGL21(self)->line_quads_vbo);
    glDeleteBuffers(1, &gfxOpenGL21(self)->full_framebuffer_quad_vbo);
    Shader_destroy(&gfxOpenGL21(self)->solid_fill_shader);
    Shader_destroy(&gfxOpenGL21(self)->font_shader);
    Shader_destroy(&gfxOpenGL21(self)->font_shader_gray);
    Shader_destroy(&gfxOpenGL21(self)->font_shader_blend);
    Shader_destroy(&gfxOpenGL21(self)->line_shader);
    Shader_destroy(&gfxOpenGL21(self)->image_shader);
    Shader_destroy(&gfxOpenGL21(self)->image_tint_shader);
    GlyphAtlas_destroy(&gfxOpenGL21(self)->glyph_atlas);
    Vector_destroy_vertex_t(&(gfxOpenGL21(self)->vec_vertex_buffer));
    Vector_destroy_vertex_t(&(gfxOpenGL21(self)->vec_vertex_buffer2));
    Vector_destroy_Vector_float(&(gfxOpenGL21(self))->float_vec);
}
