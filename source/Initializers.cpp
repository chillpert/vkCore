#include "vkCore/Initializers.hpp"
#include "vkCore/vkCore.hpp"

namespace vkCore
{
  auto initFenceUnique( vk::FenceCreateFlags flags ) -> UniqueFence
  {
    vk::FenceCreateInfo createInfo( flags );

    UniqueFence fence = device.createFenceUnique( createInfo );
    RX_ASSERT( fence.get( ), "Failed to create fence." );

    return fence;
  }
}