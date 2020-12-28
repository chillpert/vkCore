#include <iostream>
#include <vulkan/vulkan.hpp>

// TODO
// initDescriptorSetsUnique does not allocate unique descriptor sets.

// Define the following three lines once in any .cpp source file.
// #define VULKAN_HPP_STORAGE_SHARED
// #define VULKAN_HPP_STORAGE_SHARED_EXPORT
// VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

#define VK_ASSERT( statement, message )              \
  if ( !statement )                                  \
  {                                                  \
    std::cerr << "vkCore: " << message << std::endl; \
    throw std::runtime_error( "vkCore: " #message ); \
  }

namespace vkCore
{
  namespace global
  {
    inline vk::Device device   = nullptr;
    inline uint32_t dataCopies = 2U;
  } // namespace global

  /// Returns a fence with a unique handle.
  auto initFenceUnique( vk::FenceCreateFlags flags = vk::FenceCreateFlagBits::eSignaled ) -> vk::UniqueFence;

  /// Returns a fence.
  auto initFence( vk::FenceCreateFlags flags = vk::FenceCreateFlagBits::eSignaled ) -> vk::Fence;

  /// Returns a semaphore with a unique handle.
  auto initSemaphoreUnique( vk::SemaphoreCreateFlags flags = { } ) -> vk::UniqueSemaphore;

  /// Returns a semaphore.
  auto initSemaphore( vk::SemaphoreCreateFlags flags = { } ) -> vk::Semaphore;

  /// Returns a command pool with a unique handle.
  /// @param queueFamilyIndex All command buffers allocated from this command pool must be submitted on queues from this queue family.
  auto initCommandPoolUnique( uint32_t queueFamilyIndex, vk::CommandPoolCreateFlags flags = { } ) -> vk::UniqueCommandPool;

  /// Returns a command pool.
  ///
  /// @see initCommandPoolUnique(queueFamilyIndex, flags)
  auto initCommandPool( uint32_t queueFamilyIndex, vk::CommandPoolCreateFlags flags = { } ) -> vk::CommandPool;

  /// Returns a unique descriptor pool.
  /// @param poolSizes A vector of descriptor pool sizes.
  /// @param maxSets The maximum amount of descriptor sets that can be allocated from this pool.
  auto initDescriptorPoolUnique( const std::vector<vk::DescriptorPoolSize>& poolSizes, uint32_t maxSets = 1U, vk::DescriptorPoolCreateFlags flags = { } ) -> vk::UniqueDescriptorPool;

  /// Returns a descriptor pool.
  ///
  /// @see initDescriptorPoolUnique(poolSizes, maxSets, flags)
  auto initDescriptorPool( const std::vector<vk::DescriptorPoolSize>& poolSizes, uint32_t maxSets = 1U, vk::DescriptorPoolCreateFlags flags = { } ) -> vk::DescriptorPool;

  /// Returns a vector of unique descriptor sets.
  ///
  /// The size of the vector is determined by global::dataCopies.
  /// @param pool A unique descriptor pool to allocate the sets from.
  /// @param layout The desired unique descriptor set layout.
  auto initDescriptorSetsUnique( const vk::UniqueDescriptorPool& pool, const vk::UniqueDescriptorSetLayout& layout ) -> std::vector<vk::DescriptorSet>;

  /// Returns a vector of descriptor sets.
  ///
  /// @see initDescriptorSetsUnique(pool, layout)
  auto initDescriptorSets( const vk::DescriptorPool& pool, const vk::DescriptorSetLayout& layout ) -> std::vector<vk::DescriptorSet>;

  // --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

  inline auto initFenceUnique( vk::FenceCreateFlags flags ) -> vk::UniqueFence
  {
    vk::FenceCreateInfo createInfo( flags );

    auto fence = global::device.createFenceUnique( createInfo );
    VK_ASSERT( fence, "Failed to create unique fence." );

    return std::move( fence );
  }

  inline auto initFence( vk::FenceCreateFlags flags ) -> vk::Fence
  {
    vk::FenceCreateInfo createInfo( flags );

    auto fence = global::device.createFence( createInfo );
    VK_ASSERT( fence, "Failed to create fence." );

    return fence;
  }

  inline auto initSemaphoreUnique( vk::SemaphoreCreateFlags flags ) -> vk::UniqueSemaphore
  {
    vk::SemaphoreCreateInfo createInfo( flags );

    auto semaphore = global::device.createSemaphoreUnique( createInfo );
    VK_ASSERT( semaphore, "Failed to create unique semaphore." );

    return std::move( semaphore );
  }

  inline auto initSemaphore( vk::SemaphoreCreateFlags flags ) -> vk::Semaphore
  {
    vk::SemaphoreCreateInfo createInfo( flags );

    auto semaphore = global::device.createSemaphore( createInfo );
    VK_ASSERT( semaphore, "Failed to create semaphore." );

    return semaphore;
  }

  inline auto initCommandPoolUnique( uint32_t queueFamilyIndex, vk::CommandPoolCreateFlags flags ) -> vk::UniqueCommandPool
  {
    vk::CommandPoolCreateInfo createInfo( flags, queueFamilyIndex );

    auto commandPool = global::device.createCommandPoolUnique( createInfo );
    VK_ASSERT( commandPool, "Failed to create unique command pool." );

    return std::move( commandPool );
  }

  inline auto initCommandPool( uint32_t queueFamilyIndex, vk::CommandPoolCreateFlags flags ) -> vk::CommandPool
  {
    vk::CommandPoolCreateInfo createInfo( flags, queueFamilyIndex );

    auto commandPool = global::device.createCommandPool( createInfo );
    VK_ASSERT( commandPool, "Failed to create command pool." );

    return commandPool;
  }

  inline auto initDescriptorPoolUnique( const std::vector<vk::DescriptorPoolSize>& poolSizes, uint32_t maxSets, vk::DescriptorPoolCreateFlags flags ) -> vk::UniqueDescriptorPool
  {
    vk::DescriptorPoolCreateInfo createInfo( flags,                                      // flags
                                             maxSets,                                    // maxSets
                                             static_cast<uint32_t>( poolSizes.size( ) ), // poolSizeCount
                                             poolSizes.data( ) );                        // pPoolSizes

    auto descriptorPool = global::device.createDescriptorPoolUnique( createInfo );
    VK_ASSERT( descriptorPool, "Failed to create unique descriptor pool." );

    return std::move( descriptorPool );
  }

  inline auto initDescriptorPool( const std::vector<vk::DescriptorPoolSize>& poolSizes, uint32_t maxSets, vk::DescriptorPoolCreateFlags flags ) -> vk::DescriptorPool
  {
    vk::DescriptorPoolCreateInfo createInfo( flags,                                      // flags
                                             maxSets,                                    // maxSets
                                             static_cast<uint32_t>( poolSizes.size( ) ), // poolSizeCount
                                             poolSizes.data( ) );                        // pPoolSizes

    auto descriptorPool = global::device.createDescriptorPool( createInfo );
    VK_ASSERT( descriptorPool, "Failed to create unique descriptor pool." );

    return descriptorPool;
  }

  inline auto initDescriptorSetsUnique( const vk::UniqueDescriptorPool& pool, const vk::UniqueDescriptorSetLayout& layout ) -> std::vector<vk::DescriptorSet>
  {
    std::vector<vk::DescriptorSetLayout> layouts( static_cast<size_t>( global::dataCopies ), layout.get( ) );

    vk::DescriptorSetAllocateInfo allocateInfo( pool.get( ),
                                                global::dataCopies,
                                                layouts.data( ) );

    auto sets = global::device.allocateDescriptorSets( allocateInfo );

    for ( auto set : sets )
    {
      VK_ASSERT( set, "Failed to create unique descriptor sets." );
    }

    return sets;
  }

  inline auto initDescriptorSets( const vk::DescriptorPool& pool, const vk::DescriptorSetLayout& layout ) -> std::vector<vk::DescriptorSet>
  {
    std::vector<vk::DescriptorSetLayout> layouts( static_cast<size_t>( global::dataCopies ), layout );

    vk::DescriptorSetAllocateInfo allocateInfo( pool,
                                                global::dataCopies,
                                                layouts.data( ) );

    auto sets = global::device.allocateDescriptorSets( allocateInfo );

    for ( auto set : sets )
    {
      VK_ASSERT( set, "Failed to create unique descriptor sets." );
    }

    return sets;
  }

} // namespace vkCore
