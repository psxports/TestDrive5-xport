#include <string.h>
#include "hd_post.h"

static struct
{
    VID_TARGET *scene, *bloom[2];
    VID_SHADER *extract_shader, *blur_shader, *resolve_shader;
    VID_MATERIAL *extract, *blur[2], *resolve[2];
    VID_GEOMETRY *triangle;
    sint32 width, height;
    VID_TARGET *tent_target;
    VID_SHADER *tent_shader;
    VID_MATERIAL *tent;
    sint32 tent_width, tent_height;
} post;

static const char vertex_source[] = "layout(location=0) in vec4 aPos;out vec2 vUV;out vec4 vColor;out float vDepth;out vec2 vFog;"
                                    "void main(){gl_Position=vec4(aPos.xy,0.0,1.0);vUV=aPos.xy*0.5+0.5;vColor=vec4(1.0);vDepth=0.0;vFog=vec2(0.0);}";

#define POST_INPUT \
    "uniform vec4 uDraw[2];vec2 point(){return mix(uDraw[1].xy,uDraw[1].zw,vUV);}" \
    "vec3 sampleScene(vec2 p){return texture(uTex,clamp(p,uDraw[1].xy+uDraw[0].xy*0.5,uDraw[1].zw-uDraw[0].xy*0.5)).rgb;}"

static const char extract_source[] = POST_INPUT "void main(){vec2 p=point(),d=uDraw[0].xy;vec3 c=0.25*(sampleScene(p+d)+sampleScene(p-d)+"
                                                "sampleScene(p+vec2(d.x,-d.y))+sampleScene(p+vec2(-d.x,d.y)));"
                                                "float peak=max(c.r,max(c.g,c.b));c*=max(peak-0.85,0.0)/max(peak,0.0001);outColor=vec4(min(c,vec3(8.0)),1.0);}";

static const char blur_source[] = POST_INPUT "void main(){vec2 p=point(),d=uDraw[0].xy*uDraw[0].zw;vec3 c=sampleScene(p)*0.227027;"
                                             "c+=(sampleScene(p+d*1.384615)+sampleScene(p-d*1.384615))*0.316216;"
                                             "c+=(sampleScene(p+d*3.230769)+sampleScene(p-d*3.230769))*0.070270;outColor=vec4(c,1.0);}";

static const char tent_source[] = "uniform vec4 uDraw[2];"
                                  "void main(){vec2 texel=uDraw[0].xy,axis=uDraw[0].zw,uv=vUV*uDraw[1].xy;"
                                  "float f=fract(dot(uv/texel,axis)-0.5),a=3.0-2.0*f,b=1.0+2.0*f;"
                                  "vec2 p0=uv+axis*texel*(-1.0-f+(2.0-f)/a),p1=uv+axis*texel*(1.0-f+f/b);"
                                  "vec2 lo=texel*0.5,hi=uDraw[1].xy-lo;"
                                  "outColor=(textureLod(uTex,clamp(p0,lo,hi),uDraw[1].z)*a+textureLod(uTex,clamp(p1,lo,hi),uDraw[1].z)*b)*0.25;}";

static const char resolve_source[] = POST_INPUT "uniform highp sampler2D uAux0;uniform highp sampler2D uAux1;\n"
                                                "float depthAt(vec2 p){return texture(uAux1,clamp(p,uDraw[1].xy+uDraw[0].xy*0.5,uDraw[1].zw-uDraw[0].xy*0.5)).r;}\n"
                                                "float luma(vec3 c){return dot(c,vec3(0.299,0.587,0.114));}\n"
                                                "void main(){vec2 p=point(),t=uDraw[0].xy;vec3 c=sampleScene(p);\n"
                                                " float m=luma(c),nw=luma(sampleScene(p+vec2(-t.x,t.y))),ne=luma(sampleScene(p+t)),sw=luma(sampleScene(p-t)),se=luma(sampleScene(p+vec2(t.x,-t.y)));\n"
                                                " float lo=min(m,min(min(nw,ne),min(sw,se))),hi=max(m,max(max(nw,ne),max(sw,se)));\n"
                                                " float z=depthAt(p),bend=abs(depthAt(p+t)+depthAt(p-t)+depthAt(p+vec2(t.x,-t.y))+depthAt(p+vec2(-t.x,t.y))-4.0*z);\n"
                                                " if((bend>max(0.0000001,0.002*(1.0-z))||hi>1.0)&&hi-lo>max(0.03125,hi*0.125)){\n"
                                                "  vec2 dir=vec2(-((nw+ne)-(sw+se)),(nw+sw)-(ne+se));float reduce=max((nw+ne+sw+se)*0.03125,0.0078125);\n"
                                                "  dir=clamp(dir/(min(abs(dir.x),abs(dir.y))+reduce),vec2(-8.0),vec2(8.0))*t;\n"
                                                "  vec3 a=0.5*(sampleScene(p-dir/6.0)+sampleScene(p+dir/6.0));vec3 b=0.5*a+0.25*(sampleScene(p-dir*0.5)+sampleScene(p+dir*0.5));float lb=luma(b);c=lb<lo||lb>hi?a:b;\n"
                                                " }\n"
                                                " vec2 halfTexel=0.5/vec2(textureSize(uAux0,0));vec3 glow=texture(uAux0,clamp(p,uDraw[1].xy+halfTexel,uDraw[1].zw-halfTexel)).rgb;\n"
                                                " outColor=vec4(clamp(c+0.08*glow,vec3(0.0),vec3(1.0)),1.0);\n"
                                                "}\n";

static VID_MATERIAL *create_material(VID_SHADER *shader, VID_TEXTURE *image)
{
    VID_MATERIAL *material = vid_mat_create();
    XASSERT(material != NULL);
    VID_MATERIAL_DATA *data = vid_mat_data(material);
    data->shader = shader;
    data->texture = image;
    data->filter = VID_FILTER_LINEAR;
    vid_mat_commit(material);
    return material;
}

void hd_post_prepare(void)
{
    if (post.scene != NULL)
        return;
    post.scene = vid_target_create_hdr_depth();
    post.bloom[0] = vid_target_create_hdr();
    post.bloom[1] = vid_target_create_hdr();
    post.extract_shader = vid_shader_create(vertex_source, extract_source);
    post.blur_shader = vid_shader_create(vertex_source, blur_source);
    post.resolve_shader = vid_shader_create(vertex_source, resolve_source);
    post.triangle = vid_mesh_create_3d(3);
    XASSERT(post.scene && post.bloom[0] && post.bloom[1] && post.extract_shader && post.blur_shader && post.resolve_shader && post.triangle);
    post.extract = create_material(post.extract_shader, vid_target_texture(post.scene));
    for (uint32 part = 0; part < 2; ++part)
        post.blur[part] = create_material(post.blur_shader, vid_target_texture(post.bloom[part]));
    for (uint32 view = 0; view < 2; ++view)
    {
        post.resolve[view] = vid_mat_create();
        XASSERT(post.resolve[view] != NULL);
        VID_MATERIAL_DATA *data = vid_mat_data(post.resolve[view]);
        data->shader = post.resolve_shader;
        data->texture = vid_target_texture(post.scene);
        data->aux[0] = vid_target_texture(post.bloom[0]);
        data->aux_filter[0] = VID_FILTER_LINEAR;
        data->aux[1] = vid_target_depth_texture(post.scene);
        data->filter = VID_FILTER_LINEAR;
        vid_mat_commit(post.resolve[view]);
    }
    VID_VERTEX3D *vertices = vid_mesh_data_3d(post.triangle);
    memset(vertices, 0, 3 * sizeof(*vertices));
    vertices[0].pos[0] = vertices[0].pos[1] = -1;
    vertices[1].pos[0] = 3;
    vertices[1].pos[1] = -1;
    vertices[2].pos[0] = -1;
    vertices[2].pos[1] = 3;
    vid_mesh_commit(post.triangle);
    vid_mesh_static(post.triangle);
    // allocate presentation-sized GPU storage during the existing loading stage
    sint32 width, height;
    vid_resize(&width, &height);
    if (width > 0 && height > 0)
        for (uint32 part = 0; part < 3; ++part)
        {
            sint32 w = part == 0 ? width : (width + 3) / 4;
            sint32 h = part == 0 ? height : (height + 3) / 4;
            sint32 begun = vid_begin(0, 0, 0);
            XASSERT(begun);
            if (!begun)
                break;
            sint32 selected = vid_target_select_offscreen_size(part == 0 ? post.scene : post.bloom[part - 1], w, h);
            XASSERT(selected);
            if (!selected)
            {
                vid_cancel();
                break;
            }
            sint32 completed = vid_end();
            XASSERT(completed);
        }
}

void hd_post_reset(void)
{
    vid_mat_free(post.tent);
    vid_shader_free(post.tent_shader);
    vid_target_free(post.tent_target);
    for (uint32 view = 0; view < 2; ++view)
        vid_mat_free(post.resolve[view]);
    vid_mat_free(post.extract);
    for (uint32 part = 0; part < 2; ++part)
        vid_mat_free(post.blur[part]);
    vid_shader_free(post.resolve_shader);
    vid_shader_free(post.extract_shader);
    vid_shader_free(post.blur_shader);
    vid_mesh_free(post.triangle);
    vid_target_free(post.scene);
    for (uint32 part = 0; part < 2; ++part)
        vid_target_free(post.bloom[part]);
    memset(&post, 0, sizeof(post));
}

void hd_post_reflection_prepare(sint32 width, sint32 height)
{
    sint32 half = width > 1 ? width / 2 : 1;
    if (post.tent_shader && half <= post.tent_width && height <= post.tent_height)
        return;
    if (!post.tent_shader)
    {
        post.tent_shader = vid_shader_create(vertex_source, tent_source);
        post.tent = create_material(post.tent_shader, NULL);
        post.tent_target = vid_target_create_hdr();
        XASSERT(post.tent_shader && post.tent && post.tent_target);
    }
    if (half > post.tent_width) post.tent_width = half;
    if (height > post.tent_height) post.tent_height = height;
    sint32 completed = vid_begin(0, 0, 0);
    XASSERT(completed);
    if (!completed) return;
    if (!vid_target_select_offscreen_size(post.tent_target, post.tent_width, post.tent_height))
    {
        vid_cancel();
        XASSERT(0);
        return;
    }
    completed = vid_end();
    XASSERT(completed);
}

sint32 hd_post_reflection_mips(VID_TARGET *target, sint32 width, sint32 height, uint32 levels)
{
    hd_post_reflection_prepare(width, height);
    for (uint32 level = 1; level < levels && (width > 1 || height > 1); ++level)
    {
        sint32 w = width > 1 ? width / 2 : 1, h = height > 1 ? height / 2 : 1;
        for (uint32 part = 0; part < 2; ++part)
        {
            if (!vid_begin(0, 0, 0)) return 0;
            sint32 selected = part == 0 ? vid_target_select_offscreen_size(post.tent_target, post.tent_width, post.tent_height) : vid_target_select_mip(target, level);
            if (!selected)
            {
                vid_cancel();
                return 0;
            }
            vid_viewport((VID_RECT){0, part == 0 ? post.tent_height - height : 0, w, part == 0 ? height : h});
            VID_MATERIAL_DATA *data = vid_mat_data(post.tent);
            data->texture = vid_target_texture(part == 0 ? target : post.tent_target);
            data->filter = part == 0 && level > 1 ? VID_FILTER_TRILINEAR : VID_FILTER_LINEAR;
            vid_mat_commit(post.tent);
            sint32 cw = part == 0 ? width : post.tent_width, ch = part == 0 ? height : post.tent_height;
            float uniforms[2][4] = {{1.0f / cw, 1.0f / ch, part == 0 ? 1.0f : 0.0f, part == 1 ? 1.0f : 0.0f}, {(float)(part == 0 ? width : w) / cw, (float)height / ch, part == 0 ? (float)(level - 1) : 0.0f}};
            vid_uniforms(&uniforms[0][0], 2);
            vid_pass(post.tent);
            vid_submit(post.triangle, NULL, 0, 3);
            if (!vid_end()) return 0;
        }
        width = w;
        height = h;
    }
    return 1;
}

static void submit(VID_MATERIAL *material, VID_RECT view, sint32 input_width, sint32 input_height, float dx, float dy)
{
    float uniforms[2][4] = {{1.0f / input_width, 1.0f / input_height, dx, dy}, {(float)view.x / post.width, (float)(post.height - view.y - view.h) / post.height, (float)(view.x + view.w) / post.width, (float)(post.height - view.y) / post.height}};
    vid_uniforms(&uniforms[0][0], 2);
    vid_pass(material);
    vid_submit(post.triangle, NULL, 0, 3);
}

sint32 hd_post_world_begin(sint32 width, sint32 height, const float clear[3])
{
    if (width <= 0 || height <= 0)
        return 0;
    hd_post_prepare();
    post.width = width;
    post.height = height;
    if (!vid_begin(clear[0], clear[1], clear[2]))
        return 0;
    if (!vid_target_select_offscreen_size(post.scene, width, height))
    {
        vid_cancel();
        return 0;
    }
    return 1;
}

sint32 hd_post_world_end(const VID_RECT *views, uint32 count)
{
    sint32 width = (post.width + 3) / 4, height = (post.height + 3) / 4;
    if (!vid_end())
        return 0;
    for (uint32 pass = 0; pass < 3; ++pass)
    {
        if (!vid_begin(0, 0, 0))
            return 0;
        if (!vid_target_select_offscreen_size(pass == 1 ? post.bloom[1] : post.bloom[0], width, height))
        {
            vid_cancel();
            return 0;
        }
        for (uint32 view = 0; view < count; ++view)
        {
            VID_RECT area = {views[view].x * width / post.width, views[view].y * height / post.height, (views[view].x + views[view].w) * width / post.width - views[view].x * width / post.width, (views[view].y + views[view].h) * height / post.height - views[view].y * height / post.height};
            if (area.w <= 0 || area.h <= 0)
                continue;
            vid_viewport(area);
            submit(pass == 0 ? post.extract : post.blur[pass - 1], views[view], pass == 0 ? post.width : width, pass == 0 ? post.height : height, pass == 1 ? 1.0f : 0.0f, pass == 2 ? 1.0f : 0.0f);
        }
        if (!vid_end())
            return 0;
    }
    return 1;
}

void hd_post_submit(VID_RECT view, uint32 index)
{
    vid_viewport(view);
    submit(post.resolve[index], view, post.width, post.height, 0, 0);
}
