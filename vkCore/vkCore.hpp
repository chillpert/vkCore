#include "external/tinyLogger/tinyLogger.hpp"
#include "vkCore/Initializers.hpp"

namespace vkCore
{
  inline vk::Device device = nullptr;

  /// Creates a fence with a unique handle.
  inline auto initFenceUnique( vk::FenceCreateFlags flags = vk::FenceCreateFlagBits::eSignaled ) -> vk::UniqueFence;
  {
    vk::FenceCreateInfo createInfo( flags );

    UniqueFence fence = device.createFenceUnique( createInfo );
    RX_ASSERT( fence.get( ), "Failed to create fence." );

    return fence;
  }
}