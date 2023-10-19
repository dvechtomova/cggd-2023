#include "rasterizer_renderer.h"

#include "utils/resource_utils.h"

#define CAMERA_SET(A) \
	camera->set_##A(settings->camera_##A);

#define TIME_CALC(A, DESC) \
	{                   															\
		auto start = std::chrono::high_resolution_clock::now(); 					\
	A                															\
		auto stop = std::chrono::high_resolution_clock::now();  					\
	std::chrono::duration<float, std::milli> clear_duration = stop - start; 	\
		std::cout << DESC << clear_duration.count() << "ms\n";                 		\
	}

void cg::renderer::rasterization_renderer::init()
{
	rasterizer = std::make_shared<cg::renderer::rasterizer<cg::vertex, cg::unsigned_color>>();
	rasterizer->set_viewport(settings->width, settings->height);

	render_target = std::make_shared<cg::resource<cg::unsigned_color>>(settings->width, settings->height);

	rasterizer->set_render_target(render_target);

	model = std::make_shared<cg::world::model>();
	model->load_obj(settings->model_path);

	camera = std::make_shared<cg::world::camera>();
	camera->set_height(static_cast<float>(settings->height));
	camera->set_width(static_cast<float>(settings->width));
	camera->set_position(float3{
			settings->camera_position[0],
			settings->camera_position[1],
			settings->camera_position[2]});

	CAMERA_SET(theta);
	CAMERA_SET(phi);
	CAMERA_SET(angle_of_view);
	CAMERA_SET(z_near);
	CAMERA_SET(z_far);

	depth_buffer = std::make_shared<cg::resource<float>>(settings->width, settings->height);
	rasterizer->set_render_target(render_target, depth_buffer);
}

void cg::renderer::rasterization_renderer::render()
{
	float4x4 matrix = mul(camera->get_projection_matrix(),camera->get_view_matrix(),model->get_world_matrix());
	rasterizer->vertex_shader = [&](float4 vertex, cg::vertex data) {
		auto processed = mul(matrix, vertex);
		return std::make_pair(processed, data);
	};

	rasterizer->pixel_shader = [](cg::vertex data, float z) {
			return cg::color{ data.ambient_r, data.ambient_g, data.ambient_b,
		};
	};

	TIME_CALC(
			rasterizer->clear_render_target({111, 5, 243});,
		    "Clearing took: "
	);

	TIME_CALC(
			for (size_t shape_id = 0; shape_id < model->get_index_buffers().size(); ++shape_id) {
				rasterizer->set_vertex_buffer(model->get_vertex_buffers()[shape_id]);
				rasterizer->set_index_buffer(model->get_index_buffers()[shape_id]);
				rasterizer->draw(
						model->get_index_buffers()[shape_id]->get_number_of_elements(),
						0);
			}, "Rendering took "
	);


	if (settings->grid != 0) {
		for (unsigned y = 0; y < settings->height; ++y) {
			for (unsigned x = 0; x < settings->width; ++x) {
				if (y % settings->grid == 0 || x % settings->grid == 0) {
					render_target->item(x, y) = {0, 0, 0};
				}
			}
		}
	}

	utils::save_resource(*render_target, settings->result_path);
}

void cg::renderer::rasterization_renderer::destroy() {}

void cg::renderer::rasterization_renderer::update() {}