#include <vulkan/vulkan.hpp> 

namespace vkCore
{
  /// Creates a fence with a unique handle.
  auto initFenceUnique( vk::FenceCreateFlags flags = vk::FenceCreateFlagBits::eSignaled ) -> vk::UniqueFence;
}