#include "include/vkSource/vkSource.hpp"

#include <iostream>

namespace vkCore
{
  vk::Device device = nullptr;

  auto initFenceUnique( vk::FenceCreateFlags flags = vk::FenceCreateFlagBits::eSignaled ) -> vk::UniqueFence;
  {
    vk::FenceCreateInfo createInfo( flags );

    UniqueFence fence = device.createFenceUnique( createInfo );
    if ( !fence )
    {
      std::cerr << "Failed to create unique fence." << std::endl;
    }

    return fence;
  }
}