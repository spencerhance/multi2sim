/*
 *  Multi2Sim
 *  Copyright (C) 2012  Rafael Ubal (ubal@ece.neu.edu)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <unistd.h>
#include <stdio.h>

#include "buffer.h"
#include "context.h"
#include "debug.h"
#include "linked-list.h"
#include "mhandle.h"
#include "program.h"
#include "shader.h"
#include "vertex-array.h"

/* 
 * Other libraries are responsible for initializing OpenGL context.
 * In Multi2Sim, it is initialized in glutInit() in glut runtime library.
 */ 
static unsigned int opengl_context_initialized;
struct opengl_context_t *opengl_ctx;


/*
 * Private Functions
 */

static struct opengl_context_state_t *opengl_context_state_create()
{
	struct opengl_context_state_t *state;

	/* Allocate */
	state = xcalloc(1, sizeof(struct opengl_context_state_t));

	/* Debug */
	opengl_debug("%s: [%p] created\n", __FUNCTION__, state);

	/* Return */
	return state;
}

static void opengl_context_state_free(struct opengl_context_state_t *state)
{
	/* Free */
	free(state);

	/* Debug */
	opengl_debug("%s: [%p] freed\n", __FUNCTION__, state);
}

static struct opengl_context_props_t *opengl_context_props_create()
{
	struct opengl_context_props_t *properties;

	/* Allocate */
	properties = xcalloc(1, sizeof(struct opengl_context_props_t));

	/* Initialize */
	opengl_context_props_set_version(0, 0, properties);
	properties->vendor = xstrdup("Multi2Sim");
	properties->renderer = xstrdup("OpenGL Emulator");

	properties->color = xcalloc(1, 4*sizeof(float));
	properties->depth = 1.0;
	properties->stencil = 0;

	/* Debug */
	opengl_debug("%s: [%p] created\n", __FUNCTION__, properties);

	/* Return */
	return properties;
}

static void opengl_context_props_free(struct opengl_context_props_t *props)
{
	/* Free */
	free(props->vendor);
	free(props->renderer);
	free(props->color);
	free(props);

	/* Debug */
	opengl_debug("%s: [%p] freed\n", __FUNCTION__, props);	
}

static struct opengl_context_t *opengl_context_create()
{
	struct opengl_context_t *context;

	/* Allocate */
	context = xcalloc(1, sizeof(struct opengl_context_t));
	context->state = opengl_context_state_create();
	context->props = opengl_context_props_create();

	context->buffer_binding_points = opengl_buffer_binding_points_create();
	context->idxed_buffer_binding_points = opengl_indexed_buffer_binding_points_create(MAX_INDEXED_TARGETS);

	/* FIXME: other repositories */
	context->vao_repo = opengl_vertex_array_obj_repo_create();

	/* Debug */
	opengl_debug("%s: OpenGL context [%p] created\n", __FUNCTION__, context);

	/* Return */
	return context;
}

static void opengl_context_free(struct opengl_context_t *context)
{
	/* Free */
	opengl_context_state_free(context->state);
	opengl_context_props_free(context->props);
	opengl_buffer_binding_points_free(context->buffer_binding_points);
	opengl_indexed_buffer_binding_points_free(context->idxed_buffer_binding_points);

	/* FIXME: other repositories */
	opengl_vertex_array_obj_repo_free(context->vao_repo);

	free(context);

	/* Debug */
	opengl_debug("%s: OpenGL context [%p] freed\n", __FUNCTION__, context);
}

/*
 * Public Functions
 */

void opengl_context_props_set_version(unsigned int major, unsigned int minor, 
	struct opengl_context_props_t *props)
{
	props->gl_major = major;
	props->gl_minor = minor;
	props->glsl_major = major;
	props->glsl_minor = minor;

	/* Debug */
	opengl_debug("%s: Opengl properties [%p] set, GL %d.%d, GLSL %d.%d\n",
		__FUNCTION__, props, major, minor, major, minor);
}

void opengl_context_init()
{
	if (!opengl_context_initialized)
	{
		/* Initialize OpenGL context */
		opengl_ctx = opengl_context_create();
		opengl_context_initialized = 1;

		/* Debug */
		opengl_debug("%s: OpenGL context [%p] initialized\n", __FUNCTION__, opengl_ctx);	
	}
	if (!buffer_repo)
		buffer_repo = opengl_buffer_obj_repo_create();
	if (!program_repo)
		program_repo = opengl_program_obj_repo_create();
	if (!shader_repo)
		shader_repo = opengl_shader_obj_repo_create();
}

void opengl_context_destroy()
{
	if (opengl_context_initialized)
	{
		opengl_context_initialized = 0;
		opengl_context_free(opengl_ctx);
	}
	if (buffer_repo)
		opengl_buffer_obj_repo_free(buffer_repo);
	if (program_repo)
		opengl_program_obj_repo_free(program_repo);
	if (shader_repo)
		opengl_shader_obj_repo_free(shader_repo);
}

/* 
 * OpenGL API functions 
 */

 const GLubyte *glGetString( GLenum name )
{
	opengl_debug("API call %s \n", __FUNCTION__);

	const GLubyte *str;

	switch(name)
	{

	case GL_VENDOR:
	{
		sprintf(opengl_ctx->props->info, "%s\n", opengl_ctx->props->vendor);
		str = (const GLubyte *)opengl_ctx->props->info;
		break;
	}

	case GL_RENDERER:
	{
		sprintf(opengl_ctx->props->info, "%s\n", opengl_ctx->props->renderer);
		str = (const GLubyte *)opengl_ctx->props->info;
		break;
	}

	case GL_VERSION:
	{
		sprintf(opengl_ctx->props->info, "Multi2Sim OpenGL %u.%u\n", 
			opengl_ctx->props->gl_major, opengl_ctx->props->gl_minor);
		str = (const GLubyte *)opengl_ctx->props->info;
		break;
	}

	case GL_SHADING_LANGUAGE_VERSION:
	{
		sprintf(opengl_ctx->props->info, "Multi2Sim OpenGL GLSL %u.%u\n", 
			opengl_ctx->props->gl_major, opengl_ctx->props->gl_minor);
		str = (const GLubyte *)opengl_ctx->props->info;
		break;
	}

	default:
		str = NULL;
		break;
	}

	/* Return */
	return str;
}

void glEnable( GLenum cap )
{
	/* Debug */
	opengl_debug("API call %s(%x)\n", __FUNCTION__, cap);

	switch(cap)
	{

	case GL_ALPHA_TEST:
	{
		opengl_ctx->state->enable_alpha_test = 1;
		opengl_debug("\tGL_ALPHA_TEST enabled!\n");
		break;
	}

	case GL_AUTO_NORMAL:
	{
		opengl_ctx->state->enable_auto_normal = 1;
		opengl_debug("\tGL_AUTO_NORMAL enabled!\n");
		break;
	}

	case GL_BLEND:
	{
		opengl_ctx->state->enable_blend = 1;
		opengl_debug("\tGL_BLEND enabled!\n");
		break;
	}

	case GL_CLIP_PLANE0:
	{
		opengl_ctx->state->enable_clip_plane0 = 1;
		opengl_debug("\tGL_CLIP_PLANE0 enabled!\n");
		break;
	}

	case GL_CLIP_PLANE1:
	{
		opengl_ctx->state->enable_clip_plane1 = 1;
		opengl_debug("\tGL_CLIP_PLANE1 enabled!\n");
		break;
	}

	case GL_CLIP_PLANE2:
	{
		opengl_ctx->state->enable_clip_plane2 = 1;
		opengl_debug("\tGL_CLIP_PLANE2 enabled!\n");
		break;
	}

	case GL_CLIP_PLANE3:
	{
		opengl_ctx->state->enable_clip_plane3 = 1;
		opengl_debug("\tGL_CLIP_PLANE3 enabled!\n");
		break;
	}

	case GL_CLIP_PLANE4:
	{
		opengl_ctx->state->enable_clip_plane4 = 1;
		opengl_debug("\tGL_CLIP_PLANE4 enabled!\n");
		break;
	}

	case GL_CLIP_PLANE5:
	{
		opengl_ctx->state->enable_clip_plane5 = 1;
		opengl_debug("\tGL_CLIP_PLANE5 enabled!\n");
		break;
	}

	case GL_COLOR_LOGIC_OP:
	{
		opengl_ctx->state->enable_color_logic_op = 1;
		opengl_debug("\tGL_COLOR_LOGIC_OP enabled!\n");
		break;
	}

	case GL_COLOR_MATERIAL:
	{
		opengl_ctx->state->enable_color_material = 1;
		opengl_debug("\tGL_COLOR_MATERIAL enabled!\n");
		break;
	}

	case GL_COLOR_SUM:
	{
		opengl_ctx->state->enable_color_sum = 1;
		opengl_debug("\tGL_COLOR_SUM enabled!\n");
		break;
	}

	case GL_COLOR_TABLE:
	{
		opengl_ctx->state->enable_color_table = 1;
		opengl_debug("\tGL_COLOR_TABLE enabled!\n");
		break;
	}

	case GL_CONVOLUTION_1D:
	{
		opengl_ctx->state->enable_convolution_1d = 1;
		opengl_debug("\tGL_CONVOLUTION_1D enabled!\n");
		break;
	}

	case GL_CONVOLUTION_2D:
	{
		opengl_ctx->state->enable_convolution_2d = 1;
		opengl_debug("\tGL_CONVOLUTION_2D enabled!\n");
		break;
	}

	case GL_CULL_FACE:
	{
		opengl_ctx->state->enable_cull_face = 1;
		opengl_debug("\tGL_CULL_FACE enabled!\n");
		break;
	}

	case GL_DEPTH_TEST:
	{
		opengl_ctx->state->enable_depth_test = 1;
		opengl_debug("\tGL_DEPTH_TEST enabled!\n");
		break;
	}

	case GL_DITHER:
	{
		opengl_ctx->state->enable_dither = 1;
		opengl_debug("\tGL_DITHER enabled!\n");
		break;
	}

	case GL_FOG:
	{
		opengl_ctx->state->enable_fog = 1;
		opengl_debug("\tGL_FOG enabled!\n");
		break;
	}

	case GL_HISTOGRAM:
	{
		opengl_ctx->state->enable_histogram = 1;
		opengl_debug("\tGL_HISTOGRAM enabled!\n");
		break;
	}

	case GL_INDEX_LOGIC_OP:
	{
		opengl_ctx->state->enable_index_logic_op = 1;
		opengl_debug("\tGL_INDEX_LOGIC_OP enabled!\n");
		break;
	}

	case GL_LIGHT0:
	{
		opengl_ctx->state->enable_light0 = 1;
		opengl_debug("\tGL_LIGHT0 enabled!\n");
		break;
	}

	case GL_LIGHT1:
	{
		opengl_ctx->state->enable_light1 = 1;
		opengl_debug("\tGL_LIGHT1 enabled!\n");
		break;
	}

	case GL_LIGHT2:
	{
		opengl_ctx->state->enable_light2 = 1;
		opengl_debug("\tGL_LIGHT2 enabled!\n");
		break;
	}

	case GL_LIGHT3:
	{
		opengl_ctx->state->enable_light3 = 1;
		opengl_debug("\tGL_LIGHT3 enabled!\n");
		break;
	}

	case GL_LIGHT4:
	{
		opengl_ctx->state->enable_light4 = 1;
		opengl_debug("\tGL_LIGHT4 enabled!\n");
		break;
	}

	case GL_LIGHT5:
	{
		opengl_ctx->state->enable_light5 = 1;
		opengl_debug("\tGL_LIGHT5 enabled!\n");
		break;
	}

	case GL_LIGHT6:
	{
		opengl_ctx->state->enable_light6 = 1;
		opengl_debug("\tGL_LIGHT6 enabled!\n");
		break;
	}

	case GL_LIGHT7:
	{
		opengl_ctx->state->enable_light7 = 1;
		opengl_debug("\tGL_LIGHT7 enabled!\n");
		break;
	}

	case GL_LIGHTING:
	{
		opengl_ctx->state->enable_lighting = 1;
		opengl_debug("\tGL_LIGHTING enabled!\n");
		break;
	}

	case GL_LINE_SMOOTH:
	{
		opengl_ctx->state->enable_line_smooth = 1;
		opengl_debug("\tGL_LINE_SMOOTH enabled!\n");
		break;
	}

	case GL_LINE_STIPPLE:
	{
		opengl_ctx->state->enable_line_stipple = 1;
		opengl_debug("\tGL_LINE_STIPPLE enabled!\n");
		break;
	}

	case GL_MAP1_COLOR_4:
	{
		opengl_ctx->state->enable_map1_color_4 = 1;
		opengl_debug("\tGL_MAP1_COLOR_4 enabled!\n");
		break;
	}

	case GL_MAP1_INDEX:
	{
		opengl_ctx->state->enable_map1_index = 1;
		opengl_debug("\tGL_MAP1_INDEX enabled!\n");
		break;
	}

	case GL_MAP1_NORMAL:
	{
		opengl_ctx->state->enable_map1_normal = 1;
		opengl_debug("\tGL_MAP1_NORMAL enabled!\n");
		break;
	}

	case GL_MAP1_TEXTURE_COORD_1:
	{
		opengl_ctx->state->enable_map1_texture_coord_1 = 1;
		opengl_debug("\tGL_MAP1_TEXTURE_COORD_1 enabled!\n");
		break;
	}

	case GL_MAP1_TEXTURE_COORD_2:
	{
		opengl_ctx->state->enable_map1_texture_coord_2 = 1;
		opengl_debug("\tGL_MAP1_TEXTURE_COORD_2 enabled!\n");
		break;
	}

	case GL_MAP1_TEXTURE_COORD_3:
	{
		opengl_ctx->state->enable_map1_texture_coord_3 = 1;
		opengl_debug("\tGL_MAP1_TEXTURE_COORD_3 enabled!\n");
		break;
	}

	case GL_MAP1_TEXTURE_COORD_4:
	{
		opengl_ctx->state->enable_map1_texture_coord_4 = 1;
		opengl_debug("\tGL_MAP1_TEXTURE_COORD_4 enabled!\n");
		break;
	}

	case GL_MAP1_VERTEX_3:
	{
		opengl_ctx->state->enable_map1_vertex_3 = 1;
		opengl_debug("\tGL_MAP1_VERTEX_3 enabled!\n");
		break;
	}

	case GL_MAP1_VERTEX_4:
	{
		opengl_ctx->state->enable_map1_vertex_4 = 1;
		opengl_debug("\tGL_MAP1_VERTEX_4 enabled!\n");
		break;
	}

	case GL_MAP2_COLOR_4:
	{
		opengl_ctx->state->enable_map2_color_4 = 1;
		opengl_debug("\tGL_MAP2_COLOR_4 enabled!\n");
		break;
	}

	case GL_MAP2_INDEX:
	{
		opengl_ctx->state->enable_map2_index = 1;
		opengl_debug("\tGL_MAP2_INDEX enabled!\n");
		break;
	}

	case GL_MAP2_NORMAL:
	{
		opengl_ctx->state->enable_map2_normal = 1;
		opengl_debug("\tGL_MAP2_NORMAL enabled!\n");
		break;
	}

	case GL_MAP2_TEXTURE_COORD_1:
	{
		opengl_ctx->state->enable_map2_texture_coord_1 = 1;
		opengl_debug("\tGL_MAP2_TEXTURE_COORD_1 enabled!\n");
		break;
	}

	case GL_MAP2_TEXTURE_COORD_2:
	{
		opengl_ctx->state->enable_map2_texture_coord_2 = 1;
		opengl_debug("\tGL_MAP2_TEXTURE_COORD_2 enabled!\n");
		break;
	}

	case GL_MAP2_TEXTURE_COORD_3:
	{
		opengl_ctx->state->enable_map2_texture_coord_3 = 1;
		opengl_debug("\tGL_MAP2_TEXTURE_COORD_3 enabled!\n");
		break;
	}

	case GL_MAP2_TEXTURE_COORD_4:
	{
		opengl_ctx->state->enable_map2_texture_coord_4 = 1;
		opengl_debug("\tGL_MAP2_TEXTURE_COORD_4 enabled!\n");
		break;
	}

	case GL_MAP2_VERTEX_3:
	{
		opengl_ctx->state->enable_map2_vertex_3 = 1;
		opengl_debug("\tGL_MAP2_VERTEX_3 enabled!\n");
		break;
	}

	case GL_MAP2_VERTEX_4:
	{
		opengl_ctx->state->enable_map2_vertex_4 = 1;
		opengl_debug("\tGL_MAP2_VERTEX_4 enabled!\n");
		break;
	}

	case GL_MINMAX:
	{
		opengl_ctx->state->enable_minmax = 1;
		opengl_debug("\tGL_MINMAX enabled!\n");
		break;
	}

	case GL_MULTISAMPLE:
	{
		opengl_ctx->state->enable_multisample = 1;
		opengl_debug("\tGL_MULTISAMPLE enabled!\n");
		break;
	}

	case GL_NORMALIZE:
	{
		opengl_ctx->state->enable_normalize = 1;
		opengl_debug("\tGL_NORMALIZE enabled!\n");
		break;
	}

	case GL_POINT_SMOOTH:
	{
		opengl_ctx->state->enable_point_smooth = 1;
		opengl_debug("\tGL_POINT_SMOOTH enabled!\n");
		break;
	}

	case GL_POINT_SPRITE:
	{
		opengl_ctx->state->enable_point_sprite = 1;
		opengl_debug("\tGL_POINT_SPRITE enabled!\n");
		break;
	}

	case GL_POLYGON_OFFSET_FILL:
	{
		opengl_ctx->state->enable_polygon_offset_fill = 1;
		opengl_debug("\tGL_POLYGON_OFFSET_FILL enabled!\n");
		break;
	}

	case GL_POLYGON_OFFSET_LINE:
	{
		opengl_ctx->state->enable_polygon_offset_line = 1;
		opengl_debug("\tGL_POLYGON_OFFSET_LINE enabled!\n");
		break;
	}

	case GL_POLYGON_OFFSET_POINT:
	{
		opengl_ctx->state->enable_polygon_offset_point = 1;
		opengl_debug("\tGL_POLYGON_OFFSET_POINT enabled!\n");
		break;
	}

	case GL_POLYGON_SMOOTH:
	{
		opengl_ctx->state->enable_polygon_smooth = 1;
		opengl_debug("\tGL_POLYGON_SMOOTH enabled!\n");
		break;
	}

	case GL_POLYGON_STIPPLE:
	{
		opengl_ctx->state->enable_polygon_stipple = 1;
		opengl_debug("\tGL_POLYGON_STIPPLE enabled!\n");
		break;
	}

	case GL_POST_COLOR_MATRIX_COLOR_TABLE:
	{
		opengl_ctx->state->enable_post_color_matrix_color_table = 1;
		opengl_debug("\tGL_POST_COLOR_MATRIX_COLOR_TABLE enabled!\n");
		break;
	}

	case GL_POST_CONVOLUTION_COLOR_TABLE:
	{
		opengl_ctx->state->enable_post_convolution_color_table = 1;
		opengl_debug("\tGL_POST_CONVOLUTION_COLOR_TABLE enabled!\n");
		break;
	}

	case GL_RESCALE_NORMAL:
	{
		opengl_ctx->state->enable_rescale_normal = 1;
		opengl_debug("\tGL_RESCALE_NORMAL enabled!\n");
		break;
	}

	case GL_SAMPLE_ALPHA_TO_COVERAGE:
	{
		opengl_ctx->state->enable_sample_alpha_to_coverage = 1;
		opengl_debug("\tGL_SAMPLE_ALPHA_TO_COVERAGE enabled!\n");
		break;
	}

	case GL_SAMPLE_ALPHA_TO_ONE:
	{
		opengl_ctx->state->enable_sample_alpha_to_one = 1;
		opengl_debug("\tGL_SAMPLE_ALPHA_TO_ONE enabled!\n");
		break;
	}

	case GL_SAMPLE_COVERAGE:
	{
		opengl_ctx->state->enable_sample_coverage = 1;
		opengl_debug("\tGL_SAMPLE_COVERAGE enabled!\n");
		break;
	}

	case GL_SEPARABLE_2D:
	{
		opengl_ctx->state->enable_separable_2d = 1;
		opengl_debug("\tGL_SEPARABLE_2D enabled!\n");
		break;
	}

	case GL_SCISSOR_TEST:
	{
		opengl_ctx->state->enable_scissor_test = 1;
		opengl_debug("\tGL_SCISSOR_TEST enabled!\n");
		break;
	}

	case GL_STENCIL_TEST:
	{
		opengl_ctx->state->enable_stencil_test = 1;
		opengl_debug("\tGL_STENCIL_TEST enabled!\n");
		break;
	}

	case GL_TEXTURE_1D:
	{
		opengl_ctx->state->enable_texture_1d = 1;
		opengl_debug("\tGL_TEXTURE_1D enabled!\n");
		break;
	}

	case GL_TEXTURE_2D:
	{
		opengl_ctx->state->enable_texture_2d = 1;
		opengl_debug("\tGL_TEXTURE_2D enabled!\n");
		break;
	}

	case GL_TEXTURE_3D:
	{
		opengl_ctx->state->enable_texture_3d = 1;
		opengl_debug("\tGL_TEXTURE_3D enabled!\n");
		break;
	}

	case GL_TEXTURE_CUBE_MAP:
	{
		opengl_ctx->state->enable_texture_cube_map = 1;
		opengl_debug("\tGL_TEXTURE_CUBE_MAP enabled!\n");
		break;
	}

	case GL_TEXTURE_GEN_Q:
	{
		opengl_ctx->state->enable_texture_gen_q = 1;
		opengl_debug("\tGL_TEXTURE_GEN_Q enabled!\n");
		break;
	}

	case GL_TEXTURE_GEN_R:
	{
		opengl_ctx->state->enable_texture_gen_r = 1;
		opengl_debug("\tGL_TEXTURE_GEN_R enabled!\n");
		break;
	}

	case GL_TEXTURE_GEN_S:
	{
		opengl_ctx->state->enable_texture_gen_s = 1;
		opengl_debug("\tGL_TEXTURE_GEN_S enabled!\n");
		break;
	}

	case GL_TEXTURE_GEN_T:
	{
		opengl_ctx->state->enable_texture_gen_t = 1;
		opengl_debug("\tGL_TEXTURE_GEN_T enabled!\n");
		break;
	}

	case GL_VERTEX_PROGRAM_POINT_SIZE:
	{
		opengl_ctx->state->enable_vertex_program_point_size = 1;
		opengl_debug("\tGL_VERTEX_PROGRAM_POINT_SIZE enabled!\n");
		break;
	}

	case GL_VERTEX_PROGRAM_TWO_SIDE:
	{
		opengl_ctx->state->enable_vertex_program_two_side = 1;
		opengl_debug("\tGL_VERTEX_PROGRAM_TWO_SIDE enabled!\n");
		break;
	}

	default:
		break;
	}
}

void glDisable( GLenum cap )
{
	/* Debug */
	opengl_debug("API call %s(%x)\n", __FUNCTION__, cap);

	switch(cap)
	{

	case GL_ALPHA_TEST:
	{
		opengl_ctx->state->enable_alpha_test = 0;
		opengl_debug("\tGL_ALPHA_TEST disabled!\n");
		break;
	}

	case GL_AUTO_NORMAL:
	{
		opengl_ctx->state->enable_auto_normal = 0;
		opengl_debug("\tGL_AUTO_NORMAL disabled!\n");
		break;
	}

	case GL_BLEND:
	{
		opengl_ctx->state->enable_blend = 0;
		opengl_debug("\tGL_BLEND disabled!\n");
		break;
	}

	case GL_CLIP_PLANE0:
	{
		opengl_ctx->state->enable_clip_plane0 = 0;
		opengl_debug("\tGL_CLIP_PLANE0 disabled!\n");
		break;
	}

	case GL_CLIP_PLANE1:
	{
		opengl_ctx->state->enable_clip_plane1 = 0;
		opengl_debug("\tGL_CLIP_PLANE1 disabled!\n");
		break;
	}

	case GL_CLIP_PLANE2:
	{
		opengl_ctx->state->enable_clip_plane2 = 0;
		opengl_debug("\tGL_CLIP_PLANE2 disabled!\n");
		break;
	}

	case GL_CLIP_PLANE3:
	{
		opengl_ctx->state->enable_clip_plane3 = 0;
		opengl_debug("\tGL_CLIP_PLANE3 disabled!\n");
		break;
	}

	case GL_CLIP_PLANE4:
	{
		opengl_ctx->state->enable_clip_plane4 = 0;
		opengl_debug("\tGL_CLIP_PLANE4 disabled!\n");
		break;
	}

	case GL_CLIP_PLANE5:
	{
		opengl_ctx->state->enable_clip_plane5 = 0;
		opengl_debug("\tGL_CLIP_PLANE5 disabled!\n");
		break;
	}

	case GL_COLOR_LOGIC_OP:
	{
		opengl_ctx->state->enable_color_logic_op = 0;
		opengl_debug("\tGL_COLOR_LOGIC_OP disabled!\n");
		break;
	}

	case GL_COLOR_MATERIAL:
	{
		opengl_ctx->state->enable_color_material = 0;
		opengl_debug("\tGL_COLOR_MATERIAL disabled!\n");
		break;
	}

	case GL_COLOR_SUM:
	{
		opengl_ctx->state->enable_color_sum = 0;
		opengl_debug("\tGL_COLOR_SUM disabled!\n");
		break;
	}

	case GL_COLOR_TABLE:
	{
		opengl_ctx->state->enable_color_table = 0;
		opengl_debug("\tGL_COLOR_TABLE disabled!\n");
		break;
	}

	case GL_CONVOLUTION_1D:
	{
		opengl_ctx->state->enable_convolution_1d = 0;
		opengl_debug("\tGL_CONVOLUTION_1D disabled!\n");
		break;
	}

	case GL_CONVOLUTION_2D:
	{
		opengl_ctx->state->enable_convolution_2d = 0;
		opengl_debug("\tGL_CONVOLUTION_2D disabled!\n");
		break;
	}

	case GL_CULL_FACE:
	{
		opengl_ctx->state->enable_cull_face = 0;
		opengl_debug("\tGL_CULL_FACE disabled!\n");
		break;
	}

	case GL_DEPTH_TEST:
	{
		opengl_ctx->state->enable_depth_test = 0;
		opengl_debug("\tGL_DEPTH_TEST disabled!\n");
		break;
	}

	case GL_DITHER:
	{
		opengl_ctx->state->enable_dither = 0;
		opengl_debug("\tGL_DITHER disabled!\n");
		break;
	}

	case GL_FOG:
	{
		opengl_ctx->state->enable_fog = 0;
		opengl_debug("\tGL_FOG disabled!\n");
		break;
	}

	case GL_HISTOGRAM:
	{
		opengl_ctx->state->enable_histogram = 0;
		opengl_debug("\tGL_HISTOGRAM disabled!\n");
		break;
	}

	case GL_INDEX_LOGIC_OP:
	{
		opengl_ctx->state->enable_index_logic_op = 0;
		opengl_debug("\tGL_INDEX_LOGIC_OP disabled!\n");
		break;
	}

	case GL_LIGHT0:
	{
		opengl_ctx->state->enable_light0 = 0;
		opengl_debug("\tGL_LIGHT0 disabled!\n");
		break;
	}

	case GL_LIGHT1:
	{
		opengl_ctx->state->enable_light1 = 0;
		opengl_debug("\tGL_LIGHT1 disabled!\n");
		break;
	}

	case GL_LIGHT2:
	{
		opengl_ctx->state->enable_light2 = 0;
		opengl_debug("\tGL_LIGHT2 disabled!\n");
		break;
	}

	case GL_LIGHT3:
	{
		opengl_ctx->state->enable_light3 = 0;
		opengl_debug("\tGL_LIGHT3 disabled!\n");
		break;
	}

	case GL_LIGHT4:
	{
		opengl_ctx->state->enable_light4 = 0;
		opengl_debug("\tGL_LIGHT4 disabled!\n");
		break;
	}

	case GL_LIGHT5:
	{
		opengl_ctx->state->enable_light5 = 0;
		opengl_debug("\tGL_LIGHT5 disabled!\n");
		break;
	}

	case GL_LIGHT6:
	{
		opengl_ctx->state->enable_light6 = 0;
		opengl_debug("\tGL_LIGHT6 disabled!\n");
		break;
	}

	case GL_LIGHT7:
	{
		opengl_ctx->state->enable_light7 = 0;
		opengl_debug("\tGL_LIGHT7 disabled!\n");
		break;
	}

	case GL_LIGHTING:
	{
		opengl_ctx->state->enable_lighting = 0;
		opengl_debug("\tGL_LIGHTING disabled!\n");
		break;
	}

	case GL_LINE_SMOOTH:
	{
		opengl_ctx->state->enable_line_smooth = 0;
		opengl_debug("\tGL_LINE_SMOOTH disabled!\n");
		break;
	}

	case GL_LINE_STIPPLE:
	{
		opengl_ctx->state->enable_line_stipple = 0;
		opengl_debug("\tGL_LINE_STIPPLE disabled!\n");
		break;
	}

	case GL_MAP1_COLOR_4:
	{
		opengl_ctx->state->enable_map1_color_4 = 0;
		opengl_debug("\tGL_MAP1_COLOR_4 disabled!\n");
		break;
	}

	case GL_MAP1_INDEX:
	{
		opengl_ctx->state->enable_map1_index = 0;
		opengl_debug("\tGL_MAP1_INDEX disabled!\n");
		break;
	}

	case GL_MAP1_NORMAL:
	{
		opengl_ctx->state->enable_map1_normal = 0;
		opengl_debug("\tGL_MAP1_NORMAL disabled!\n");
		break;
	}

	case GL_MAP1_TEXTURE_COORD_1:
	{
		opengl_ctx->state->enable_map1_texture_coord_1 = 0;
		opengl_debug("\tGL_MAP1_TEXTURE_COORD_1 disabled!\n");
		break;
	}

	case GL_MAP1_TEXTURE_COORD_2:
	{
		opengl_ctx->state->enable_map1_texture_coord_2 = 0;
		opengl_debug("\tGL_MAP1_TEXTURE_COORD_2 disabled!\n");
		break;
	}

	case GL_MAP1_TEXTURE_COORD_3:
	{
		opengl_ctx->state->enable_map1_texture_coord_3 = 0;
		opengl_debug("\tGL_MAP1_TEXTURE_COORD_3 disabled!\n");
		break;
	}

	case GL_MAP1_TEXTURE_COORD_4:
	{
		opengl_ctx->state->enable_map1_texture_coord_4 = 0;
		opengl_debug("\tGL_MAP1_TEXTURE_COORD_4 disabled!\n");
		break;
	}

	case GL_MAP1_VERTEX_3:
	{
		opengl_ctx->state->enable_map1_vertex_3 = 0;
		opengl_debug("\tGL_MAP1_VERTEX_3 disabled!\n");
		break;
	}

	case GL_MAP1_VERTEX_4:
	{
		opengl_ctx->state->enable_map1_vertex_4 = 0;
		opengl_debug("\tGL_MAP1_VERTEX_4 disabled!\n");
		break;
	}

	case GL_MAP2_COLOR_4:
	{
		opengl_ctx->state->enable_map2_color_4 = 0;
		opengl_debug("\tGL_MAP2_COLOR_4 disabled!\n");
		break;
	}

	case GL_MAP2_INDEX:
	{
		opengl_ctx->state->enable_map2_index = 0;
		opengl_debug("\tGL_MAP2_INDEX disabled!\n");
		break;
	}

	case GL_MAP2_NORMAL:
	{
		opengl_ctx->state->enable_map2_normal = 0;
		opengl_debug("\tGL_MAP2_NORMAL disabled!\n");
		break;
	}

	case GL_MAP2_TEXTURE_COORD_1:
	{
		opengl_ctx->state->enable_map2_texture_coord_1 = 0;
		opengl_debug("\tGL_MAP2_TEXTURE_COORD_1 disabled!\n");
		break;
	}

	case GL_MAP2_TEXTURE_COORD_2:
	{
		opengl_ctx->state->enable_map2_texture_coord_2 = 0;
		opengl_debug("\tGL_MAP2_TEXTURE_COORD_2 disabled!\n");
		break;
	}

	case GL_MAP2_TEXTURE_COORD_3:
	{
		opengl_ctx->state->enable_map2_texture_coord_3 = 0;
		opengl_debug("\tGL_MAP2_TEXTURE_COORD_3 disabled!\n");
		break;
	}

	case GL_MAP2_TEXTURE_COORD_4:
	{
		opengl_ctx->state->enable_map2_texture_coord_4 = 0;
		opengl_debug("\tGL_MAP2_TEXTURE_COORD_4 disabled!\n");
		break;
	}

	case GL_MAP2_VERTEX_3:
	{
		opengl_ctx->state->enable_map2_vertex_3 = 0;
		opengl_debug("\tGL_MAP2_VERTEX_3 disabled!\n");
		break;
	}

	case GL_MAP2_VERTEX_4:
	{
		opengl_ctx->state->enable_map2_vertex_4 = 0;
		opengl_debug("\tGL_MAP2_VERTEX_4 disabled!\n");
		break;
	}

	case GL_MINMAX:
	{
		opengl_ctx->state->enable_minmax = 0;
		opengl_debug("\tGL_MINMAX disabled!\n");
		break;
	}

	case GL_MULTISAMPLE:
	{
		opengl_ctx->state->enable_multisample = 0;
		opengl_debug("\tGL_MULTISAMPLE disabled!\n");
		break;
	}

	case GL_NORMALIZE:
	{
		opengl_ctx->state->enable_normalize = 0;
		opengl_debug("\tGL_NORMALIZE disabled!\n");
		break;
	}

	case GL_POINT_SMOOTH:
	{
		opengl_ctx->state->enable_point_smooth = 0;
		opengl_debug("\tGL_POINT_SMOOTH disabled!\n");
		break;
	}

	case GL_POINT_SPRITE:
	{
		opengl_ctx->state->enable_point_sprite = 0;
		opengl_debug("\tGL_POINT_SPRITE disabled!\n");
		break;
	}

	case GL_POLYGON_OFFSET_FILL:
	{
		opengl_ctx->state->enable_polygon_offset_fill = 0;
		opengl_debug("\tGL_POLYGON_OFFSET_FILL disabled!\n");
		break;
	}

	case GL_POLYGON_OFFSET_LINE:
	{
		opengl_ctx->state->enable_polygon_offset_line = 0;
		opengl_debug("\tGL_POLYGON_OFFSET_LINE disabled!\n");
		break;
	}

	case GL_POLYGON_OFFSET_POINT:
	{
		opengl_ctx->state->enable_polygon_offset_point = 0;
		opengl_debug("\tGL_POLYGON_OFFSET_POINT disabled!\n");
		break;
	}

	case GL_POLYGON_SMOOTH:
	{
		opengl_ctx->state->enable_polygon_smooth = 0;
		opengl_debug("\tGL_POLYGON_SMOOTH disabled!\n");
		break;
	}

	case GL_POLYGON_STIPPLE:
	{
		opengl_ctx->state->enable_polygon_stipple = 0;
		opengl_debug("\tGL_POLYGON_STIPPLE disabled!\n");
		break;
	}

	case GL_POST_COLOR_MATRIX_COLOR_TABLE:
	{
		opengl_ctx->state->enable_post_color_matrix_color_table = 0;
		opengl_debug("\tGL_POST_COLOR_MATRIX_COLOR_TABLE disabled!\n");
		break;
	}

	case GL_POST_CONVOLUTION_COLOR_TABLE:
	{
		opengl_ctx->state->enable_post_convolution_color_table = 0;
		opengl_debug("\tGL_POST_CONVOLUTION_COLOR_TABLE disabled!\n");
		break;
	}

	case GL_RESCALE_NORMAL:
	{
		opengl_ctx->state->enable_rescale_normal = 0;
		opengl_debug("\tGL_RESCALE_NORMAL disabled!\n");
		break;
	}

	case GL_SAMPLE_ALPHA_TO_COVERAGE:
	{
		opengl_ctx->state->enable_sample_alpha_to_coverage = 0;
		opengl_debug("\tGL_SAMPLE_ALPHA_TO_COVERAGE disabled!\n");
		break;
	}

	case GL_SAMPLE_ALPHA_TO_ONE:
	{
		opengl_ctx->state->enable_sample_alpha_to_one = 0;
		opengl_debug("\tGL_SAMPLE_ALPHA_TO_ONE disabled!\n");
		break;
	}

	case GL_SAMPLE_COVERAGE:
	{
		opengl_ctx->state->enable_sample_coverage = 0;
		opengl_debug("\tGL_SAMPLE_COVERAGE disabled!\n");
		break;
	}

	case GL_SEPARABLE_2D:
	{
		opengl_ctx->state->enable_separable_2d = 0;
		opengl_debug("\tGL_SEPARABLE_2D disabled!\n");
		break;
	}

	case GL_SCISSOR_TEST:
	{
		opengl_ctx->state->enable_scissor_test = 0;
		opengl_debug("\tGL_SCISSOR_TEST disabled!\n");
		break;
	}

	case GL_STENCIL_TEST:
	{
		opengl_ctx->state->enable_stencil_test = 0;
		opengl_debug("\tGL_STENCIL_TEST disabled!\n");
		break;
	}

	case GL_TEXTURE_1D:
	{
		opengl_ctx->state->enable_texture_1d = 0;
		opengl_debug("\tGL_TEXTURE_1D disabled!\n");
		break;
	}

	case GL_TEXTURE_2D:
	{
		opengl_ctx->state->enable_texture_2d = 0;
		opengl_debug("\tGL_TEXTURE_2D disabled!\n");
		break;
	}

	case GL_TEXTURE_3D:
	{
		opengl_ctx->state->enable_texture_3d = 0;
		opengl_debug("\tGL_TEXTURE_3D disabled!\n");
		break;
	}

	case GL_TEXTURE_CUBE_MAP:
	{
		opengl_ctx->state->enable_texture_cube_map = 0;
		opengl_debug("\tGL_TEXTURE_CUBE_MAP disabled!\n");
		break;
	}

	case GL_TEXTURE_GEN_Q:
	{
		opengl_ctx->state->enable_texture_gen_q = 0;
		opengl_debug("\tGL_TEXTURE_GEN_Q disabled!\n");
		break;
	}

	case GL_TEXTURE_GEN_R:
	{
		opengl_ctx->state->enable_texture_gen_r = 0;
		opengl_debug("\tGL_TEXTURE_GEN_R disabled!\n");
		break;
	}

	case GL_TEXTURE_GEN_S:
	{
		opengl_ctx->state->enable_texture_gen_s = 0;
		opengl_debug("\tGL_TEXTURE_GEN_S disabled!\n");
		break;
	}

	case GL_TEXTURE_GEN_T:
	{
		opengl_ctx->state->enable_texture_gen_t = 0;
		opengl_debug("\tGL_TEXTURE_GEN_T disabled!\n");
		break;
	}

	case GL_VERTEX_PROGRAM_POINT_SIZE:
	{
		opengl_ctx->state->enable_vertex_program_point_size = 0;
		opengl_debug("\tGL_VERTEX_PROGRAM_POINT_SIZE disabled!\n");
		break;
	}

	case GL_VERTEX_PROGRAM_TWO_SIDE:
	{
		opengl_ctx->state->enable_vertex_program_two_side = 0;
		opengl_debug("\tGL_VERTEX_PROGRAM_TWO_SIDE disabled!\n");
		break;
	}

	default:
		break;
	}
}

void glClear( GLbitfield mask )
{
	/* Debug */
	opengl_debug("API call %s(%x)\n", __FUNCTION__, mask);

	/* FIXME */
	if (mask & ~(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_ACCUM_BUFFER_BIT))
		opengl_debug("\tInvalid mask!\n");

	if ((mask & GL_COLOR_BUFFER_BIT) == GL_COLOR_BUFFER_BIT) 
	{
		opengl_debug("\tColor buffer cleared\n");
		/* Clear color buffer */
	}

	if ((mask & GL_DEPTH_BUFFER_BIT) == GL_DEPTH_BUFFER_BIT) 
	{
		opengl_debug("\tDepth buffer cleared\n");
		/* Clear depth buffer */
	}

	if ((mask & GL_STENCIL_BUFFER_BIT) == GL_STENCIL_BUFFER_BIT)
	{
		opengl_debug("\tStencil buffer cleared\n");
		/* Clear stencil buffer */
	}

}

void glClearColor( GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha )
{
	/* Debug */
	opengl_debug("API call %s(%f, %f, %f, %f)\n", 
		__FUNCTION__, red, green, blue, alpha);

	/* Specify a clear value for the color buffers */
	opengl_ctx->props->color[0] = red;
	opengl_ctx->props->color[0] = green;
	opengl_ctx->props->color[0] = blue;
	opengl_ctx->props->color[0] = alpha;
}

void glClearDepth( GLclampd depth )
{
	/* Debug */
	opengl_debug("API call %s(%f)\n", __FUNCTION__, depth);

	/* Specify a clear value for the depth buffer */
	opengl_ctx->props->depth = depth;
}

void glClearStencil( GLint s )
{
	/* Debug */
	opengl_debug("API call %s(%d)\n", __FUNCTION__, s);

	/* Specify a clear value for the stencil buffer */
	opengl_ctx->props->stencil = s;
}

void glViewport( GLint x, GLint y, GLsizei width, GLsizei height )
{
	/* Debug */
	opengl_debug("API call %s(%d, %d, %d, %d)\n", 
		__FUNCTION__, x, y, width, height);

	/* Set the viewport */
	opengl_ctx->props->vp_x = x;
	opengl_ctx->props->vp_y = y;
	opengl_ctx->props->vp_w = width;
	opengl_ctx->props->vp_h = height;
}