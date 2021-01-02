#pragma once

#include <algorithm>
#include <array>
#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <vector>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_vulkan.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_vulkan.h"

#include "Debug.h"
#include "Input.h"
#include "UniformBufferObject.h"
#include "Vertex.h"
#include "VulkanFrame.h"
#include "VulkanHelper.h"

const Uint32 WINDOW_WIDTH = 1280;
const Uint32 WINDOW_HEIGHT = 720;
const Uint32 FRAME_RATE = 60;
const Uint32 MAX_FRAMES_IN_FLIGHT = 2;

struct FrameSemaphores {
	vk::UniqueSemaphore image_aquired;
	vk::UniqueSemaphore render_complete;
};

class VulkanApplication {
public:
	void run() {
		init_window();
		init_imgui();
		init_vulkan();
		load(vertices, indices);
		create_mesh_buffer();
		main_loop();
		cleanup();
	}

protected:
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	VtxUniformBufferObject vubo;
	FragUniformBufferObject fubo;

	vk::Extent2D swapchain_extent;
	SDL_Window *window = nullptr;

private:
	vk::DynamicLoader dynamic_loader;
	vk::UniqueInstance instance;
	vk::UniqueSurfaceKHR surface;
	vk::PhysicalDevice physical_device;
	vk::UniqueDevice device;

	vk::UniqueDebugUtilsMessengerEXT debug_messenger;

	vk::UniqueCommandPool graphics_command_pool;
	vk::UniqueCommandPool transfer_command_pool;

	QueueFamilyIndices queue_family_indices;
	vk::Queue graphics_queue;
	vk::Queue present_queue;

	vk::UniqueSwapchainKHR swapchain;
	vk::Format swapchain_format;

	std::vector<VulkanFrame> frames;

	vk::UniqueRenderPass render_pass;
	vk::PipelineLayout pipeline_layout;
	vk::UniquePipeline graphics_pipeline;

	std::vector<FrameSemaphores> frame_semaphores;
	size_t semaphore_index = 0;

	vk::Queue transfer_queue;

	vk::UniqueBuffer mesh_buffer;
	vk::UniqueDeviceMemory mesh_buffer_memory;

	uint32_t idx_offset;

	vk::UniqueDescriptorPool descriptor_pool;
	vk::UniqueDescriptorSetLayout descriptor_set_layout;

	vk::UniqueImage texture_image;
	vk::UniqueDeviceMemory texture_image_memory;
	vk::UniqueImageView texture_image_view;
	vk::UniqueSampler texture_sampler;

	vk::UniqueImage depth_image;
	vk::UniqueDeviceMemory depth_image_memory;
	vk::UniqueImageView depth_image_view;

	VkDescriptorPool imgui_descriptor_pool;

	bool framebuffer_rezised = false;
	bool wait_for_vsync = false;

	// Init & Cleanup
	void init_window();
	void init_vulkan();
	void cleanup();

	// Loop
	virtual void load(std::vector<Vertex> &vertices, std::vector<uint32_t> &indices) = 0;
	virtual void start() = 0;
	virtual void update(float delta_time) = 0;
	void main_loop();
	void draw_scene(VulkanFrame &frame);
	void draw_frame();

	// Device
	void create_instance();
	void pick_physical_device();
	void create_logical_device();

	// Swapchain
	void init_swapchain(bool reset = false);
	void create_swapchain();
	void cleanup_swapchain();

	// Rendering
	void create_render_pass();
	void create_graphics_pipeline();
	void create_sync_objects();

	// Scene
	void create_mesh_buffer();
	void create_descriptors();

	// Texture
	void create_texture_image();
	void create_image(const uint32_t w, const uint32_t h, const vk::Format fmt, const vk::ImageTiling tiling, const vk::ImageUsageFlags usage, const vk::MemoryPropertyFlags properties, vk::UniqueImage &image, vk::UniqueDeviceMemory &image_memory);
	void transition_image_layout(VkCommandBuffer c_buffer, VkImage image, VkImageLayout from, VkImageLayout to);
	void copy_buffer_to_image(VkCommandBuffer transfer_cbuffer, VkBuffer buffer, VkImage image, uint32_t w, uint32_t h);
	void create_texture_image_view();
	void create_texture_sampler();

	// Depth
	void create_depth_resources();
	vk::Format find_supported_format(const std::vector<vk::Format> &candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);
	vk::Format find_depth_format();

	// IMGUI
	void init_imgui();
	void cleanup_imgui();
	void create_imgui_context();
	void cleanup_imgui_context();
	void draw_ui(VulkanFrame &frame);

	void copy_buffer(vk::Buffer src, vk::Buffer dst, vk::DeviceSize size);

	// Cmd
	VkCommandBuffer begin_graphics_command_buffer();
	void end_graphics_command_buffer(VkCommandBuffer c_buffer);
	VkCommandBuffer begin_transfer_command_buffer();
	void end_transfer_command_buffer(VkCommandBuffer c_buffer);
};