#include <vulkan/vulkan.hpp>

#define VK_ASSERT( statement, message )  if ( statement )\
                                         {\
                                           std::cerr << "vkCore: " << message << std::endl:\
                                         }

namespace vkCore
{
  inline vk::Device device = nullptr;

  /// Creates a fence with a unique handle.
  inline auto initFenceUnique( vk::FenceCreateFlags flags = vk::FenceCreateFlagBits::eSignaled ) -> vk::UniqueFence;
  {
    vk::FenceCreateInfo createInfo( flags );

    UniqueFence fence = device.createFenceUnique( createInfo );
    VK_ASSERT( fence, "Failed to create fence." );

    return fence;
  }
}