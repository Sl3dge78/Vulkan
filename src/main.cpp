#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#define TINYOBJLOADER_IMPLEMENTATION

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#include <deque>
#include <unordered_map>

#include <SDL/SDL.h>

#include "scene/Material.h"
#include "scene/Scene.h"
#include "vulkan/VulkanApplication.h"

// TODO : UI
// TODO : GLTF
//		- Materials
//		- Textures
// TODO : Update TLAS when an object has moved
// TODO - AMELIO : Virer VulkanFrame et le g�rer dans la swapchain directement ? ou virer swapchain et le gerer dans l'app? C'est de la complexit� inutile en trop

class Sl3dge : public VulkanApplication {
private:
	void load() override {
		scene->camera.load(this);
		auto viking_texture = scene->load_texture(this, "resources/textures/viking_room.png"); // 0
		/*
		auto viking_mesh = scene->load_mesh(this, "resources/models/viking_room.obj"); // 0
		auto viking_material = scene->create_material(0.1f, glm::vec3(1.0f, 1.0f, 1.0f), viking_texture);
		
		auto a = scene->create_instance(viking_mesh, viking_material);
		a->translate(glm::vec3(0.f, 1.f, 0.f));
		auto b = scene->create_instance(viking_mesh, viking_material);
		b->translate(glm::vec3(1.5f, 1.f, 0.f));
		b->rotate(3.14f, glm::vec3(0.f, 0.f, 1.f));
		*/
		auto sphere_mesh = scene->load_mesh(this, "resources/models/sphere.obj");
		auto sphere_material = scene->create_material(0.1f, glm::vec3(0.0f, 1.0f, 0.0f));
		auto sphere_a = scene->create_instance(sphere_mesh, sphere_material);

		auto plane_mesh = scene->load_mesh(this, "resources/models/plane.obj");
		auto plane_material = scene->create_material(0.1f, glm::vec3(.5f, .5f, .5f));
		auto plane_a = scene->create_instance(plane_mesh, plane_material);
	}
	void start() override {
		SDL_GetRelativeMouseState(nullptr, nullptr); // Called here to avoid the weird jump
		scene->camera.start();

		rtx_push_constants.clear_color = glm::vec4(0.1f, 0.1f, 0.1f, 1.f);
		rtx_push_constants.light_pos = glm::vec3(1.1f, 1.f, 1.f);
		rtx_push_constants.light_intensity = 0.5f;
		rtx_push_constants.light_type = 0;
	}
	void update(float delta_time) override {
		scene->camera.update(delta_time);

		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("Options")) {
				ImGui::MenuItem("Camera params", "", &scene->camera.show_window);
				ImGui::EndMenu();
			}
			ImGui::Separator();
			ImGui::Text("%.1f FPS", 1.f / delta_time);
		}
		ImGui::EndMainMenuBar();
	}
};

int main(int argc, char *argv[]) {
	Sl3dge app;
	app.run();

	return EXIT_SUCCESS;
}
