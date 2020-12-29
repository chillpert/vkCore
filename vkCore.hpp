#pragma once

#include <fstream>
#include <iomanip>
#include <iostream>
#include <vulkan/vulkan.hpp>

// Define the following three lines once in any .cpp source file.
// #define VULKAN_HPP_STORAGE_SHARED
// #define VULKAN_HPP_STORAGE_SHARED_EXPORT
// VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

#define VK_CORE_ASSERT( statement, message )         \
  if ( !statement )                                  \
  {                                                  \
    std::cerr << "vkCore: " << message << std::endl; \
    throw std::runtime_error( "vkCore: " #message ); \
  }

#define VK_CORE_ENABLE_UNIQUE

#define VK_CORE_ASSERT_COMPONENTS

#ifdef VK_CORE_ASSERT_COMPONENTS
  #define VK_CORE_ASSERT_DEVICE VK_CORE_ASSERT( global::device, "Invalid device." )
#else
  #define VK_CORE_ASSERT_DEVICE
#endif

namespace vkCore
{
  namespace global
  {
    inline vk::Instance instance             = nullptr;
    inline vk::PhysicalDevice physicalDevice = nullptr;
    inline vk::Device device                 = nullptr;
    inline vk::SurfaceKHR surface            = nullptr;
    inline uint32_t dataCopies               = 2U;
    inline vk::PhysicalDeviceLimits physicalDeviceLimits;
  } // namespace global

  // --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

  inline auto findMemoryType( vk::PhysicalDevice physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties ) -> uint32_t
  {
    static vk::PhysicalDeviceMemoryProperties memoryProperties = physicalDevice.getMemoryProperties( );

    for ( uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i )
    {
      if ( ( ( typeFilter & ( 1 << i ) ) != 0U ) && ( memoryProperties.memoryTypes[i].propertyFlags & properties ) == properties )
      {
        return i;
      }
    }

    throw std::runtime_error( "vkCore: Failed to find suitable memory type." );

    return 0U;
  }

  template <typename T>
  auto getMemoryRequirements( const T& object )
  {
    vk::MemoryRequirements memoryRequirements;

    if constexpr ( std::is_same<T, vk::Buffer>::value )
    {
      memoryRequirements = global::device.getBufferMemoryRequirements( object );
    }
    else if constexpr ( std::is_same<T, vk::Image>::value )
    {
      memoryRequirements = global::device.getImageMemoryRequirements( object );
    }
    else if constexpr ( std::is_same<T, vk::UniqueBuffer>::value )
    {
      memoryRequirements = global::device.getBufferMemoryRequirements( object.get( ) );
    }
    else if constexpr ( std::is_same<T, vk::UniqueImage>::value )
    {
      memoryRequirements = global::device.getImageMemoryRequirements( object.get( ) );
    }
    else
    {
      throw std::runtime_error( "vkCore: Failed to retrieve memory requirements for the provided type." );
    }

    return memoryRequirements;
  }

  inline auto parseShader( std::string_view shaderPath, std::string_view glslcPath ) -> std::vector<char>
  {
    std::string delimiter = "/";
    std::string pathToFile;
    std::string fileName;

    size_t pos = shaderPath.find_last_of( delimiter );
    if ( pos != std::string::npos )
    {
      pathToFile = std::string( shaderPath.substr( 0, pos + 1 ) );
      fileName   = std::string( shaderPath.substr( pos + 1 ) );
    }
    else
    {
      throw std::runtime_error( "Failed to process shader path." );
    }

    std::string fileNameOut = fileName + ".spv";

    // Calls glslc to compile the glsl file into spir-v.
    std::stringstream command;
    command << glslcPath << " " << shaderPath << " -o " << pathToFile << fileNameOut << " --target-env=vulkan1.2";

    std::system( command.str( ).c_str( ) );

    // Read the file and retrieve the source.
    std::string pathToShaderSourceFile = pathToFile + fileNameOut;
    std::ifstream file( pathToShaderSourceFile, std::ios::ate | std::ios::binary );

    if ( !file.is_open( ) )
    {
      throw std::runtime_error( "Failed to open shader source file " + pathToShaderSourceFile );
    }

    size_t fileSize = static_cast<size_t>( file.tellg( ) );
    std::vector<char> buffer( fileSize );

    file.seekg( 0 );
    file.read( buffer.data( ), fileSize );

    file.close( );

    return buffer;
  }

  inline auto isPhysicalDeviceQueueComplete( vk::PhysicalDevice physicalDevice ) -> bool
  {
    auto queueFamilies           = physicalDevice.getQueueFamilyProperties( );
    auto queueFamilyIndicesCount = static_cast<uint32_t>( queueFamilies.size( ) );

    // Get all possible queue family indices with transfer support.
    std::vector<uint32_t> graphicsQueueFamilyIndices;
    std::vector<uint32_t> transferQueueFamilyIndices;
    std::vector<uint32_t> computeQueueFamilyIndices;

    for ( uint32_t index = 0; index < queueFamilyIndicesCount; ++index )
    {
      // Make sure the current queue family index contains at least one queue.
      if ( queueFamilies[index].queueCount == 0 )
      {
        continue;
      }

      if ( ( queueFamilies[index].queueFlags & vk::QueueFlagBits::eGraphics ) == vk::QueueFlagBits::eGraphics )
      {
        if ( physicalDevice.getSurfaceSupportKHR( index, global::surface ) != 0U )
        {
          graphicsQueueFamilyIndices.push_back( index );
        }
      }

      if ( ( queueFamilies[index].queueFlags & vk::QueueFlagBits::eTransfer ) == vk::QueueFlagBits::eTransfer )
      {
        transferQueueFamilyIndices.push_back( index );
      }

      if ( ( queueFamilies[index].queueFlags & vk::QueueFlagBits::eCompute ) == vk::QueueFlagBits::eCompute )
      {
        computeQueueFamilyIndices.push_back( index );
      }
    }

    if ( graphicsQueueFamilyIndices.empty( ) || computeQueueFamilyIndices.empty( ) || transferQueueFamilyIndices.empty( ) )
    {
      return false;
    }

    return true;
  }

  inline auto isPhysicalDeviceWithDedicatedTransferQueueFamily( vk::PhysicalDevice physicalDevice ) -> bool
  {
    auto queueFamilyProperties = physicalDevice.getQueueFamilyProperties( );

    for ( auto& queueFamilyPropertie : queueFamilyProperties )
    {
      if ( ( queueFamilyPropertie.queueFlags & vk::QueueFlagBits::eGraphics ) != vk::QueueFlagBits::eGraphics )
      {
        return true;
      }
    }

    return false;
  }

  inline auto evaluatePhysicalDevice( vk::PhysicalDevice physicalDevice ) -> std::pair<uint32_t, std::string>
  {
    uint32_t score = 0U;

    auto properties = physicalDevice.getProperties( );

    std::string deviceName = properties.deviceName;

    // Always prefer dedicated GPUs.
    if ( properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu )
    {
      score += 100U;
    }
    else
    {
      return { 0U, deviceName };
    }

    // Prefer newer Vulkan support.
#ifdef VK_API_VERSION_1_2
    if ( properties.apiVersion >= VK_API_VERSION_1_2 )
    {
      score += 10U;
    }
#endif

    // Check if the physical device has compute, transfer and graphics families.
    if ( isPhysicalDeviceQueueComplete( physicalDevice ) )
    {
      score += 100U;
    }
    else
    {
      return { 0U, deviceName };
    }

    // Check if there is a queue family for transfer operations that is not the graphics queue itself.
    if ( isPhysicalDeviceWithDedicatedTransferQueueFamily( physicalDevice ) )
    {
      score += 25;
    }

    return { score, deviceName };
  }

  inline auto initPhysicalDevice( ) -> vk::PhysicalDevice
  {
    vk::PhysicalDevice physicalDevice;

    auto physicalDevices = global::instance.enumeratePhysicalDevices( );

    std::vector<std::pair<unsigned int, std::string>> results;

    unsigned int score = 0;
    for ( const auto& it : physicalDevices )
    {
      auto temp = evaluatePhysicalDevice( it );
      results.push_back( temp );

      if ( temp.first > score )
      {
        physicalDevice = it;
        score          = temp.first;
      }
    }

    // Print information about all GPUs available on the machine.
    const std::string_view separator = "===================================================================";
    std::cout << "Physical device report: "
              << "\n"
              << separator << "\n"
              << "Device name"
              << "\t\t\t"
              << "Score" << std::endl
              << separator << "\n";

    for ( const auto& result : results )
    {
      std::cout << std::left << std::setw( 32 ) << std::setfill( ' ' ) << result.second << std::left << std::setw( 32 ) << std::setfill( ' ' ) << result.first << std::endl;
    }

    VK_CORE_ASSERT( physicalDevice, "No suitable physical device was found." );

    // Print information about the GPU that was selected.
    auto properties = physicalDevice.getProperties( );
    std::cout << "Selected GPU: " << properties.deviceName << std::endl;

    global::physicalDeviceLimits = properties.limits;
    global::physicalDevice       = physicalDevice;

    return physicalDevice;
  }

  // --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

  inline auto initFence( vk::FenceCreateFlags flags = vk::FenceCreateFlagBits::eSignaled ) -> vk::Fence
  {
    vk::FenceCreateInfo createInfo( flags );

    auto fence = global::device.createFence( createInfo );
    VK_CORE_ASSERT( fence, "Failed to create fence." );

    return fence;
  }

  inline auto initSemaphore( vk::SemaphoreCreateFlags flags = { } ) -> vk::Semaphore
  {
    vk::SemaphoreCreateInfo createInfo( flags );

    auto semaphore = global::device.createSemaphore( createInfo );
    VK_CORE_ASSERT( semaphore, "Failed to create semaphore." );

    return semaphore;
  }

  inline auto initCommandPool( uint32_t queueFamilyIndex, vk::CommandPoolCreateFlags flags = { } ) -> vk::CommandPool
  {
    vk::CommandPoolCreateInfo createInfo( flags, queueFamilyIndex );

    auto commandPool = global::device.createCommandPool( createInfo );
    VK_CORE_ASSERT( commandPool, "Failed to create command pool." );

    return commandPool;
  }

  inline auto initDescriptorPool( const std::vector<vk::DescriptorPoolSize>& poolSizes, uint32_t maxSets = 1, vk::DescriptorPoolCreateFlags flags = { } ) -> vk::DescriptorPool
  {
    vk::DescriptorPoolCreateInfo createInfo( flags,                                      // flags
                                             maxSets,                                    // maxSets
                                             static_cast<uint32_t>( poolSizes.size( ) ), // poolSizeCount
                                             poolSizes.data( ) );                        // pPoolSizes

    auto descriptorPool = global::device.createDescriptorPool( createInfo );
    VK_CORE_ASSERT( descriptorPool, "Failed to create unique descriptor pool." );

    return descriptorPool;
  }

  inline auto allocateDescriptorSets( const vk::DescriptorPool& pool, const vk::DescriptorSetLayout& layout ) -> std::vector<vk::DescriptorSet>
  {
    std::vector<vk::DescriptorSetLayout> layouts( static_cast<size_t>( global::dataCopies ), layout );

    vk::DescriptorSetAllocateInfo allocateInfo( pool,
                                                global::dataCopies,
                                                layouts.data( ) );

    auto sets = global::device.allocateDescriptorSets( allocateInfo );

    for ( auto set : sets )
    {
      VK_CORE_ASSERT( set, "Failed to create unique descriptor sets." );
    }

    return sets;
  }

  template <typename T>
  auto allocateMemory( const T& object, vk::MemoryPropertyFlags propertyFlags = { }, void* pNext = nullptr )
  {
    vk::MemoryRequirements memoryRequirements = getMemoryRequirements( object );

    vk::MemoryAllocateInfo allocateInfo( memoryRequirements.size,                                                                      // allocationSize
                                         findMemoryType( global::physicalDevice, memoryRequirements.memoryTypeBits, propertyFlags ) ); // memoryTypeIndex

    allocateInfo.pNext = pNext;

    auto memory = global::device.allocateMemory( allocateInfo );
    VK_CORE_ASSERT( memory, "Failed to allocate memory." );

    return memory;
  }

  inline auto initImageView( const vk::ImageViewCreateInfo& createInfo ) -> vk::ImageView
  {
    auto imageView = global::device.createImageView( createInfo );
    VK_CORE_ASSERT( imageView, "Failed to create image view." );

    return imageView;
  }

  inline auto initSampler( const vk::SamplerCreateInfo& createInfo ) -> vk::Sampler
  {
    auto sampler = global::device.createSampler( createInfo );
    VK_CORE_ASSERT( sampler, "Failed to create sampler." );

    return sampler;
  }

  inline auto initFramebuffer( const std::vector<vk::ImageView>& attachments, vk::RenderPass renderPass, const vk::Extent2D& extent ) -> vk::Framebuffer
  {
    vk::FramebufferCreateInfo createInfo( { },                                          // flags
                                          renderPass,                                   // renderPass
                                          static_cast<uint32_t>( attachments.size( ) ), // attachmentCount
                                          attachments.data( ),                          // pAttachments
                                          extent.width,                                 // width
                                          extent.height,                                // height
                                          1U );                                         // layers

    auto framebuffer = global::device.createFramebuffer( createInfo );
    VK_CORE_ASSERT( framebuffer, "Failed to create framebuffer." );

    return framebuffer;
  }

  inline auto initQueryPool( uint32_t count, vk::QueryType type ) -> vk::QueryPool
  {
    vk::QueryPoolCreateInfo createInfo( { },   // flags
                                        type,  // queryType
                                        count, // queryCount
                                        { } ); // pipelineStatistics

    auto queryPool = global::device.createQueryPool( createInfo );
    VK_CORE_ASSERT( queryPool, "Failed to create query pool." );

    return queryPool;
  }

  inline auto initShaderModule( std::string_view shaderPath, std::string_view glslcPath ) -> vk::ShaderModule
  {
    std::vector<char> source = parseShader( shaderPath, glslcPath );

    vk::ShaderModuleCreateInfo createInfo( { },                                                   // flags
                                           source.size( ),                                        // codeSize
                                           reinterpret_cast<const uint32_t*>( source.data( ) ) ); // pCode

    auto shaderModule = global::device.createShaderModule( createInfo );
    VK_CORE_ASSERT( shaderModule, "Failed to create shader module." );

    return shaderModule;
  }

  // --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

#ifdef VK_CORE_ENABLE_UNIQUE
  inline auto initFenceUnique( vk::FenceCreateFlags flags = vk::FenceCreateFlagBits::eSignaled ) -> vk::UniqueFence
  {
    vk::FenceCreateInfo createInfo( flags );

    auto fence = global::device.createFenceUnique( createInfo );
    VK_CORE_ASSERT( fence, "Failed to create unique fence." );

    return std::move( fence );
  }

  inline auto initSemaphoreUnique( vk::SemaphoreCreateFlags flags = { } ) -> vk::UniqueSemaphore
  {
    vk::SemaphoreCreateInfo createInfo( flags );

    auto semaphore = global::device.createSemaphoreUnique( createInfo );
    VK_CORE_ASSERT( semaphore, "Failed to create unique semaphore." );

    return std::move( semaphore );
  }

  inline auto initCommandPoolUnique( uint32_t queueFamilyIndex, vk::CommandPoolCreateFlags flags = { } ) -> vk::UniqueCommandPool
  {
    vk::CommandPoolCreateInfo createInfo( flags, queueFamilyIndex );

    auto commandPool = global::device.createCommandPoolUnique( createInfo );
    VK_CORE_ASSERT( commandPool, "Failed to create unique command pool." );

    return std::move( commandPool );
  }

  inline auto initDescriptorPoolUnique( const std::vector<vk::DescriptorPoolSize>& poolSizes, uint32_t maxSets = 1, vk::DescriptorPoolCreateFlags flags = { } ) -> vk::UniqueDescriptorPool
  {
    vk::DescriptorPoolCreateInfo createInfo( flags,                                      // flags
                                             maxSets,                                    // maxSets
                                             static_cast<uint32_t>( poolSizes.size( ) ), // poolSizeCount
                                             poolSizes.data( ) );                        // pPoolSizes

    auto descriptorPool = global::device.createDescriptorPoolUnique( createInfo );
    VK_CORE_ASSERT( descriptorPool, "Failed to create unique descriptor pool." );

    return std::move( descriptorPool );
  }

  inline auto allocateDescriptorSetsUnique( const vk::UniqueDescriptorPool& pool, const vk::UniqueDescriptorSetLayout& layout ) -> std::vector<vk::UniqueDescriptorSet>
  {
    std::vector<vk::DescriptorSetLayout> layouts( static_cast<size_t>( global::dataCopies ), layout.get( ) );

    vk::DescriptorSetAllocateInfo allocateInfo( pool.get( ),
                                                global::dataCopies,
                                                layouts.data( ) );

    auto sets = global::device.allocateDescriptorSetsUnique( allocateInfo );

    for ( const auto& set : sets )
    {
      VK_CORE_ASSERT( set, "Failed to create unique descriptor sets." );
    }

    return std::move( sets );
  }

  template <typename T>
  auto allocateMemoryUnique( const T& object, vk::MemoryPropertyFlags propertyFlags = { }, void* pNext = nullptr )
  {
    vk::MemoryRequirements memoryRequirements = getMemoryRequirements( object );

    vk::MemoryAllocateInfo allocateInfo( memoryRequirements.size,                                                                      // allocationSize
                                         findMemoryType( global::physicalDevice, memoryRequirements.memoryTypeBits, propertyFlags ) ); // memoryTypeIndex

    allocateInfo.pNext = pNext;

    auto memory = global::device.allocateMemoryUnique( allocateInfo );
    VK_CORE_ASSERT( memory, "Failed to allocate memory." );

    return std::move( memory );
  }

  inline auto initImageViewUnique( const vk::ImageViewCreateInfo& createInfo ) -> vk::UniqueImageView
  {
    auto imageView = global::device.createImageViewUnique( createInfo );
    VK_CORE_ASSERT( imageView, "Failed to create image view." );

    return std::move( imageView );
  }

  inline auto initSamplerUnique( const vk::SamplerCreateInfo& createInfo ) -> vk::UniqueSampler
  {
    auto sampler = global::device.createSamplerUnique( createInfo );
    VK_CORE_ASSERT( sampler, "Failed to create sampler." );

    return std::move( sampler );
  }

  inline auto initFramebufferUnique( const std::vector<vk::ImageView>& attachments, vk::RenderPass renderPass, const vk::Extent2D& extent ) -> vk::UniqueFramebuffer
  {
    vk::FramebufferCreateInfo createInfo( { },                                          // flags
                                          renderPass,                                   // renderPass
                                          static_cast<uint32_t>( attachments.size( ) ), // attachmentCount
                                          attachments.data( ),                          // pAttachments
                                          extent.width,                                 // width
                                          extent.height,                                // height
                                          1U );                                         // layers

    auto framebuffer = global::device.createFramebufferUnique( createInfo );
    VK_CORE_ASSERT( framebuffer, "Failed to create framebuffer." );

    return std::move( framebuffer );
  }

  inline auto initQueryPoolUnique( uint32_t count, vk::QueryType type ) -> vk::UniqueQueryPool
  {
    vk::QueryPoolCreateInfo createInfo( { },   // flags
                                        type,  // queryType
                                        count, // queryCount
                                        { } ); // pipelineStatistics

    auto queryPool = global::device.createQueryPoolUnique( createInfo );
    VK_CORE_ASSERT( queryPool, "Failed to create query pool." );

    return std::move( queryPool );
  }

  inline auto initShaderModuleUnique( std::string_view shaderPath, std::string_view glslcPath ) -> vk::UniqueShaderModule
  {
    std::vector<char> source = parseShader( shaderPath, glslcPath );

    vk::ShaderModuleCreateInfo createInfo( { },                                                   // flags
                                           source.size( ),                                        // codeSize
                                           reinterpret_cast<const uint32_t*>( source.data( ) ) ); // pCode

    auto shaderModule = global::device.createShaderModuleUnique( createInfo );
    VK_CORE_ASSERT( shaderModule, "Failed to create shader module." );

    return std::move( shaderModule );
  }

#endif
} // namespace vkCore
