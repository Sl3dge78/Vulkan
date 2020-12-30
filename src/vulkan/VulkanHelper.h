#pragma once

#include <fstream>
#include <optional>
#include <set>
#include <vector>

#include <vulkan/vulkan.h>

#include <SDL/SDL.h>

#include "Debug.h"

const std::vector<const char *> validation_layers = { "VK_LAYER_KHRONOS_validation" };
const std::vector<const char *> device_extensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
	VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
	VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
	VK_KHR_MAINTENANCE3_EXTENSION_NAME,
	VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
	VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME
};

struct QueueFamilyIndices {
	std::optional<Uint32> graphics_family;
	std::optional<Uint32> present_family;
	std::optional<Uint32> transfer_family;

	bool is_complete() {
		return graphics_family.has_value() && present_family.has_value() && transfer_family.has_value();
	}
};

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> present_modes;
};

std::vector<char> read_file(const std::string &path);

void check_vk_result(VkResult err);

VkFormat get_vk_format(SDL_PixelFormat *format);

uint32_t find_memory_type(VkPhysicalDevice physical_device, uint32_t type_filter, VkMemoryPropertyFlags properties);

void create_image_view(VkDevice device, VkImage &image, VkFormat format, VkImageAspectFlags aspect, VkImageView *image_view);

void create_buffer(VkDevice device, VkPhysicalDevice physical_device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &buffer_memory);

bool check_validation_layer_support();

std::vector<const char *> get_required_extensions(SDL_Window *window);

bool is_device_suitable(VkPhysicalDevice device, VkSurfaceKHR surface);
QueueFamilyIndices find_queue_families(VkPhysicalDevice device, VkSurfaceKHR surface);
bool check_device_extension_support(VkPhysicalDevice device);
SwapChainSupportDetails query_swap_chain_support(VkPhysicalDevice device, VkSurfaceKHR surface);

VkSurfaceFormatKHR choose_swapchain_surface_format(const std::vector<VkSurfaceFormatKHR> &available_formats);
VkPresentModeKHR choose_swapchain_present_mode(const std::vector<VkPresentModeKHR> &available_present_modes);
VkExtent2D choose_swapchain_extent(const VkSurfaceCapabilitiesKHR &capabilities, SDL_Window *window);

VkShaderModule create_shader_module(VkDevice device, const std::vector<char> &code);
