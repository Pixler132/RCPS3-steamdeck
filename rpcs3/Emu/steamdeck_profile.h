#pragma once

#include <span>
#include <string_view>

namespace cfg
{
	class _base;
}

// Steam Deck build profile (CMake option USE_STEAMDECK_PROFILE).
// Pins the hardware-facing video settings to values chosen for the Deck and loads the SteamOS Vulkan driver.
// Game compatibility settings (buffer reads/writes, strict modes, CPU/SPU accuracy) stay adjustable.
// With the option off, every function here is a no-op and nothing is pinned.
namespace steamdeck_profile
{
	struct pinned_setting
	{
		std::string_view path; // Config location joined with '/', e.g. "Video/Renderer"
		std::string_view value;
	};

	std::span<const pinned_setting> pinned_settings();

	bool is_pinned(std::string_view path);
	bool is_pinned(const cfg::_base& node);

	// Overwrite the pinned settings in g_cfg. Runs before the other fixups, after every config source is applied.
	void apply();

	// Inside a container, point the Vulkan loader at the host's SteamOS RADV unless a driver was chosen explicitly.
	// Must run before anything initializes Vulkan.
	void select_vulkan_driver();
}
