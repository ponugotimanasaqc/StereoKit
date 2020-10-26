#pragma once

#include "../stereokit.h"
#include "../libraries/sk_gpu.h"
#include "assets.h"
#include "shader.h"

namespace sk {

struct shaderargs_data_t {
	skg_bind_t   buffer_bind;
	size_t       buffer_size;
	skg_buffer_t buffer_gpu;
	bool         buffer_dirty;
	void        *buffer;
	tex_t       *textures;
	skg_bind_t  *texture_binds;
	int32_t      texture_count;
};

struct _material_t {
	asset_header_t    header;
	shader_t          shader;
	shaderargs_data_t args;
	transparency_     alpha_mode;
	cull_             cull;
	bool32_t          wireframe;
	int32_t           queue_offset;
	skg_pipeline_t    pipeline;
};

void   material_destroy   (material_t material);
size_t material_param_size(material_param_ type);

} // namespace sk