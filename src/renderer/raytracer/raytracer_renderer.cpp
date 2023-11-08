#include "raytracer_renderer.h"

#include "utils/resource_utils.h"

#include <iostream>

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

void cg::renderer::ray_tracing_renderer::init()
{
	render_target = std::make_shared<cg::resource<cg::unsigned_color>>(settings->width, settings->height);

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

	raytracer = std::make_shared<cg::renderer::raytracer<cg::vertex, cg::unsigned_color>>();
	raytracer->set_viewport(settings->width, settings->height);
	raytracer->set_render_target(render_target);
	raytracer->set_vertex_buffers(model->get_vertex_buffers());
	raytracer->set_index_buffers(model->get_index_buffers());
}

void cg::renderer::ray_tracing_renderer::destroy() {}

void cg::renderer::ray_tracing_renderer::update() {}

void cg::renderer::ray_tracing_renderer::render()
{
	raytracer->miss_shader = [](const ray& ray) {
		payload payload{};
		payload.color = {
				0.f,
				0.f,
				0.f};

		return payload;
	};

	std::random_device random_device;
	std::mt19937 random_generator(random_device());
	std::uniform_real_distribution<float> uni_dist(-1.f,
												   1.f);

	raytracer->closest_hit_shader = [&](const ray& ray, payload& payload,
										const triangle<cg::vertex>& triangle,
										size_t depth) {

		float3 position = ray.position +
						  ray.direction * payload.t;
		float3 normal = payload.bary.x * triangle.na +
						payload.bary.y * triangle.nb +
						payload.bary.z * triangle.nc;

		float3 res_color = triangle.emissive;

		float3 rand_direction{
				uni_dist(random_generator),
				uni_dist(random_generator),
				uni_dist(random_generator)};

		if (dot(rand_direction, normal) < 0.f) {
			rand_direction = -rand_direction;
		}

		cg::renderer::ray to_rand_direction(position, rand_direction);
		auto next_payload = raytracer->trace_ray(to_rand_direction, depth);
		res_color += next_payload.color.to_float3() * triangle.diffuse *
					 std::max(0.f, dot(normal, to_rand_direction.direction));

		payload.color = cg::color::from_float3(res_color);
		return payload;
	};

	raytracer->build_acceleration_structure();
	raytracer->clear_render_target({0, 0, 0});

	TIME_CALC(
		raytracer->ray_generation(
			camera->get_position(), camera->get_direction(),
			camera->get_right(), camera->get_up(),
			settings->raytracing_depth, settings->accumulation_num);, "Ray tracing took")

	cg::utils::save_resource(*render_target,
							 settings->result_path);
}