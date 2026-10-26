#include "stdafx.h"
#include "steamdeck_profile.h"
#include "system_config.h"
#include "Utilities/File.h"
#include "Utilities/StrUtil.h"

#include <cstdlib>

LOG_CHANNEL(sys_log, "SYS");

namespace
{
	using steamdeck_profile::pinned_setting;

	// Steam Deck: 1280x800 panel at 60 Hz (LCD), 4C/8T Zen 2, RDNA 2 iGPU sharing system memory.
	// Emulation is CPU-bound on this hardware, so render at the PS3's own resolution with no upscaling,
	// and keep RSX on its own single thread so it does not compete with the PPU/SPU threads.
	// The rest are RPCS3's defaults, pinned so that stale or per-game configs cannot move them.
	[[maybe_unused]] constexpr pinned_setting s_pinned[] =
	{
		{ "Video/Renderer", "Vulkan" },
		{ "Video/Resolution", "1280x720" },
		{ "Video/Aspect ratio", "16:9" },
		{ "Video/Frame limit", "Auto" },
		{ "Video/Second Frame Limit", "0" },
		{ "Video/MSAA", "Auto" },
		{ "Video/Shader Mode", "Async Recompiler with Shader Interpreter" },
		{ "Video/Shader Precision", "High" },
		{ "Video/VSync Mode", "Full" },
		{ "Video/Multithreaded RSX", "false" },
		{ "Video/Resolution Scale", "100" },
		{ "Video/Minimum Scalable Dimension", "16" },
		{ "Video/Anisotropic Filter Override", "0" },
		{ "Video/Output Scaling Mode", "Bilinear" },
		{ "Video/Stretch To Display Area", "false" },
		{ "Video/Use full RGB output range", "true" },
		{ "Video/Shader Compiler Threads", "0" },
		{ "Video/Vblank Rate", "60" },
		{ "Video/Vulkan/Exclusive Fullscreen Mode", "Automatic" },
		{ "Video/Vulkan/Asynchronous Texture Streaming", "false" },
		{ "Video/Vulkan/VRAM allocation limit (MB)", "65536" },
	};

	[[maybe_unused]] cfg::_base* find_node(std::string_view path)
	{
		cfg::_base* node = &g_cfg;

		for (const std::string& name : fmt::split(path, {"/"}))
		{
			if (node->get_type() != cfg::type::node)
			{
				return nullptr;
			}

			const auto& children = static_cast<cfg::node*>(node)->get_nodes();
			const auto it = std::find_if(children.cbegin(), children.cend(), [&](const cfg::_base* child) { return child->get_name() == name; });

			if (it == children.cend())
			{
				return nullptr;
			}

			node = *it;
		}

		return node;
	}
}

std::span<const pinned_setting> steamdeck_profile::pinned_settings()
{
#ifdef RPCS3_STEAMDECK_PROFILE
	return s_pinned;
#else
	return {};
#endif
}

bool steamdeck_profile::is_pinned(std::string_view path)
{
	const auto pinned = pinned_settings();
	return std::any_of(pinned.begin(), pinned.end(), [&](const pinned_setting& setting) { return setting.path == path; });
}

bool steamdeck_profile::is_pinned(const cfg::_base& node)
{
	if (pinned_settings().empty())
	{
		return false;
	}

	// The root node has no parent and does not appear in the path
	std::string path = node.get_name();

	for (const cfg::_base* parent = node.get_parent(); parent && parent->get_parent(); parent = parent->get_parent())
	{
		path = parent->get_name() + "/" + path;
	}

	return is_pinned(path);
}

void steamdeck_profile::apply()
{
	for (const auto& [path, value] : pinned_settings())
	{
		cfg::_base* node = find_node(path);

		if (!node)
		{
			sys_log.error("Steam Deck profile: setting not found: %s", path);
			continue;
		}

		const std::string old_value = node->to_string();

		if (!node->from_string(value))
		{
			sys_log.error("Steam Deck profile: invalid value '%s' for %s", value, path);
			continue;
		}

		if (const std::string new_value = node->to_string(); new_value != old_value)
		{
			sys_log.notice("Steam Deck profile: %s: '%s' -> '%s'", path, old_value, new_value);
		}
	}
}

void steamdeck_profile::select_vulkan_driver()
{
#if defined(RPCS3_STEAMDECK_PROFILE) && defined(__linux__)
	if (std::getenv("VK_DRIVER_FILES") || std::getenv("VK_ICD_FILENAMES"))
	{
		sys_log.notice("Steam Deck profile: Vulkan driver chosen by the environment, not overriding it");
		return;
	}

	// distrobox mounts the host at /run/host. Without it we are on the host, whose RADV already is the SteamOS one.
	if (!fs::is_file("/run/host/usr/lib/libvulkan_radeon.so"))
	{
		return;
	}

	if (!fs::is_file(RPCS3_HOST_RADV_ICD))
	{
		sys_log.error("Steam Deck profile: host RADV manifest not found: %s. Using the container's Vulkan driver.", RPCS3_HOST_RADV_ICD);
		return;
	}

	::setenv("VK_DRIVER_FILES", RPCS3_HOST_RADV_ICD, 0);
	sys_log.notice("Steam Deck profile: using the host SteamOS RADV (VK_DRIVER_FILES=%s)", RPCS3_HOST_RADV_ICD);
#endif
}
