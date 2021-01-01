#pragma once

#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <optional>
#include <vulkan/vulkan.hpp>

// Requires compiler with support for C++17.

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

#define VK_CORE_THROW( ... ) details::log( true, "vkCore: ", __VA_ARGS__ );

#define VK_CORE_ENABLE_UNIQUE

#define VK_CORE_ASSERT_COMPONENTS

#ifdef VK_CORE_ASSERT_COMPONENTS
  #define VK_CORE_ASSERT_DEVICE VK_CORE_ASSERT( global::device, "Invalid device." )
#else
  #define VK_CORE_ASSERT_DEVICE
#endif

#define VK_CORE_LOGGING

#ifdef VK_CORE_LOGGING
  #define VK_CORE_LOG( ... ) details::log( false, "vkCore: ", __VA_ARGS__ )
#else
  #define VK_CORE_LOG( ... )
#endif

namespace vkCore
{
  namespace details
  {
    template <typename... Args>
    inline void log( bool error, Args&&... args )
    {
      std::stringstream temp;
      ( temp << ... << args );

      std::cout << temp.str( ) << std::endl;

      if ( error )
      {
        throw std::runtime_error( temp.str( ) );
      }
    }
  } // namespace details

  namespace global
  {
    inline vk::Instance instance             = nullptr;
    inline vk::PhysicalDevice physicalDevice = nullptr;
    inline vk::Device device                 = nullptr;
    inline vk::SurfaceKHR surface            = nullptr;
    inline uint32_t dataCopies               = 2U;
    inline uint32_t graphicsFamilyIndex      = 0U;
    inline uint32_t transferFamilyIndex      = 0U;
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

    VK_CORE_THROW( "vkCore: Failed to find suitable memory type." );

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
      VK_CORE_THROW( "vkCore: Failed to retrieve memory requirements for the provided type." );
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
      VK_CORE_THROW( "Failed to process shader path." );
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
      VK_CORE_THROW( "Failed to open shader source file ", pathToShaderSourceFile );
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

#ifdef VK_CORE_LOGGING
    // Print information about all GPUs available on the machine.
    const std::string_view separator = "===================================================================";

    std::cout << "vkCore: Physical device report: "
              << "\n\n"
              << separator << "\n "
              << " Device name "
              << "\t\t\t"
              << "Score" << std::endl
              << separator << "\n ";

    for ( const auto& result : results )
    {
      std::cout << std::left << std::setw( 32 ) << std::setfill( ' ' ) << result.second << std::left << std::setw( 32 ) << std::setfill( ' ' ) << result.first << std::endl;
    }

    std::cout << std::endl;
#endif

    VK_CORE_ASSERT( physicalDevice, "No suitable physical device was found." );

    // Print information about the GPU that was selected.
    auto properties = physicalDevice.getProperties( );
    VK_CORE_LOG( "Selected GPU: ", properties.deviceName );

    global::physicalDeviceLimits = properties.limits;
    global::physicalDevice       = physicalDevice;

    return physicalDevice;
  }

  inline void checkInstanceLayersSupport( const std::vector<const char*>& layers )
  {
    auto properties = vk::enumerateInstanceLayerProperties( );

    for ( const char* name : layers )
    {
      bool found = false;
      for ( const auto& property : properties )
      {
        if ( strcmp( property.layerName, name ) == 0 )
        {
          found = true;
          break;
        }
      }

      if ( !found )
      {
        VK_CORE_THROW( "Validation layer ", name, " is not available on this device." );
      }

      VK_CORE_LOG( "Added layer: ", name, "." );
    }
  }

  inline uint32_t assessVulkanVersion( uint32_t minVersion )
  {
    uint32_t apiVersion = vk::enumerateInstanceVersion( );

#if defined( VK_API_VERSION_1_0 ) && !defined( VK_API_VERSION_1_1 ) && !defined( VK_API_VERSION_1_2 )
    VK_CORE_LOG( "Found Vulkan SDK API version 1.0." );
#endif

#if defined( VK_API_VERSION_1_1 ) && !defined( VK_API_VERSION_1_2 )
    VK_CORE_LOG( "Found Vulkan SDK API version 1.1." );
#endif

#if defined( VK_API_VERSION_1_2 )
    VK_CORE_LOG( "Found Vulkan SDK API version 1.2." );
#endif

    if ( minVersion > apiVersion )
    {
      VK_CORE_THROW( "Local Vulkan SDK API version it outdated." );
    }

    return apiVersion;
  }

  inline void checkInstanceExtensionsSupport( const std::vector<const char*>& extensions )
  {
    auto properties = vk::enumerateInstanceExtensionProperties( );

    for ( const char* name : extensions )
    {
      bool found = false;
      for ( const auto& property : properties )
      {
        if ( strcmp( property.extensionName, name ) == 0 )
        {
          found = true;
          break;
        }
      }

      if ( !found )
      {
        VK_CORE_THROW( "Instance extensions ", std::string( name ), " is not available on this device." )
      }

      VK_CORE_LOG( "Added instance extension: ", name, "." );
    }
  }

  inline void checkDeviceExtensionSupport( const std::vector<const char*>& extensions )
  {
    // Stores the name of the extension and a bool that tells if they were found or not.
    std::map<const char*, bool> requiredExtensions;

    for ( const auto& extension : extensions )
    {
      requiredExtensions.emplace( extension, false );
    }

    std::vector<vk::ExtensionProperties> physicalDeviceExtensions = global::physicalDevice.enumerateDeviceExtensionProperties( );

    // Iterates over all enumerated physical device extensions to see if they are available.
    for ( const auto& physicalDeviceExtension : physicalDeviceExtensions )
    {
      for ( auto& requiredphysicalDeviceExtension : requiredExtensions )
      {
        if ( strcmp( physicalDeviceExtension.extensionName, requiredphysicalDeviceExtension.first ) == 0 )
        {
          requiredphysicalDeviceExtension.second = true;
        }
      }
    }

    // Give feedback on the previous operations.
    for ( const auto& requiredphysicalDeviceExtension : requiredExtensions )
    {
      if ( !requiredphysicalDeviceExtension.second )
      {
        VK_CORE_THROW( "Missing physical device extension: ", requiredphysicalDeviceExtension.first, ". Perhaps you have not installed the NVIDIA Vulkan Beta drivers?" );
      }
      else
      {
        VK_CORE_LOG( "Added device extension: ", requiredphysicalDeviceExtension.first );
      }
    }
  }

  inline void initQueueFamilyIndices( )
  {
    std::optional<uint32_t> graphicsFamilyIndex;
    std::optional<uint32_t> transferFamilyIndex;

    auto queueFamilyProperties = global::physicalDevice.getQueueFamilyProperties( );
    std::vector<uint32_t> queueFamilies( queueFamilyProperties.size( ) );

    bool dedicatedTransferQueueFamily = isPhysicalDeviceWithDedicatedTransferQueueFamily( global::physicalDevice );

    for ( uint32_t index = 0; index < static_cast<uint32_t>( queueFamilies.size( ) ); ++index )
    {
      if ( queueFamilyProperties[index].queueFlags & vk::QueueFlagBits::eGraphics && !graphicsFamilyIndex.has_value( ) )
      {
        if ( global::physicalDevice.getSurfaceSupportKHR( index, global::surface ) )
        {
          graphicsFamilyIndex = index;
        }
      }

      if ( dedicatedTransferQueueFamily )
      {
        if ( !( queueFamilyProperties[index].queueFlags & vk::QueueFlagBits::eGraphics ) && !transferFamilyIndex.has_value( ) )
        {
          transferFamilyIndex = index;
        }
      }
      else
      {
        if ( queueFamilyProperties[index].queueFlags & vk::QueueFlagBits::eTransfer && !transferFamilyIndex.has_value( ) )
        {
          transferFamilyIndex = index;
        }
      }
    }

    if ( !graphicsFamilyIndex.has_value( ) || !transferFamilyIndex.has_value( ) )
    {
      VK_CORE_THROW( "Failed to retrieve queue family indices." );
    }

    global::graphicsFamilyIndex = graphicsFamilyIndex.value( );
    global::transferFamilyIndex = transferFamilyIndex.value( );
  }

  inline std::vector<vk::DeviceQueueCreateInfo> getDeviceQueueCreateInfos( )
  {
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;

    const float queuePriority = 1.0F;

    std::vector<uint32_t> queueFamilyIndices = { global::graphicsFamilyIndex, global::transferFamilyIndex };

    uint32_t index = 0;
    for ( const auto& queueFamilyIndex : queueFamilyIndices )
    {
      vk::DeviceQueueCreateInfo queueCreateInfo( { },              // flags
                                                 queueFamilyIndex, // queueFamilyIndex
                                                 1,                // queueCount
                                                 &queuePriority ); // pQueuePriorties

      queueCreateInfos.push_back( queueCreateInfo );

      ++index;
    }

    return queueCreateInfos;
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

  inline auto initInstance( const std::vector<const char*>& layers, std::vector<const char*>& extensions, uint32_t minVersion = VK_API_VERSION_1_0 ) -> vk::Instance
  {
    vk::DynamicLoader dl;
    auto vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>( "vkGetInstanceProcAddr" );
    VULKAN_HPP_DEFAULT_DISPATCHER.init( vkGetInstanceProcAddr );

    // Check if all extensions and layers needed are available.
    checkInstanceLayersSupport( layers );
    checkInstanceExtensionsSupport( extensions );

    // Start creating the instance.
    vk::ApplicationInfo appInfo;
    appInfo.apiVersion = assessVulkanVersion( minVersion );

    vk::InstanceCreateInfo createInfo( { },                                         // flags
                                       &appInfo,                                    // pApplicationInfo
                                       static_cast<uint32_t>( layers.size( ) ),     // enabledLayerCount
                                       layers.data( ),                              // ppEnabledLayerNames
                                       static_cast<uint32_t>( extensions.size( ) ), // enabledExtensionCount
                                       extensions.data( ) );                        // ppEnabledExtensionNames

    auto instance    = createInstance( createInfo );
    global::instance = instance;
    VK_CORE_ASSERT( instance, "Failed to create instance." );

    VULKAN_HPP_DEFAULT_DISPATCHER.init( instance );

    return instance;
  }

  inline auto initDevice( std::vector<const char*>& extensions, const std::optional<vk::PhysicalDeviceFeatures>& features, const std::optional<vk::PhysicalDeviceFeatures2>& features2 = { } ) -> vk::Device
  {
    checkDeviceExtensionSupport( extensions );

    auto queueCreateInfos = getDeviceQueueCreateInfos( );

    vk::DeviceCreateInfo createInfo( { },                                                                                           // flags
                                     static_cast<uint32_t>( queueCreateInfos.size( ) ),                                             // queueCreateInfoCount
                                     queueCreateInfos.data( ),                                                                      // pQueueCreateInfos
                                     0,                                                                                             // enabledLayerCount
                                     nullptr,                                                                                       // ppEnabledLayerNames
                                     static_cast<uint32_t>( extensions.size( ) ),                                                   // enabledExtensionCount
                                     extensions.data( ),                                                                            // ppEnabledExtensionNames
                                     features2.has_value( ) ? nullptr : ( features.has_value( ) ? &features.value( ) : nullptr ) ); // pEnabledFeatures - must be NULL because the VkDeviceCreateInfo::pNext chain includes VkPhysicalDeviceFeatures2.

    createInfo.pNext = features2.has_value( ) ? &features2.value( ) : nullptr;

    auto device    = global::physicalDevice.createDevice( createInfo );
    global::device = device;
    VK_CORE_ASSERT( device, "Failed to create logical device." );

    VULKAN_HPP_DEFAULT_DISPATCHER.init( device );

    return device;
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

  inline auto initInstanceUnique( const std::vector<const char*>& layers, std::vector<const char*>& extensions, uint32_t minVersion = VK_API_VERSION_1_0 ) -> vk::UniqueInstance
  {
    vk::DynamicLoader dl;
    auto vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>( "vkGetInstanceProcAddr" );
    VULKAN_HPP_DEFAULT_DISPATCHER.init( vkGetInstanceProcAddr );

    // Check if all extensions and layers needed are available.
    checkInstanceLayersSupport( layers );
    checkInstanceExtensionsSupport( extensions );

    // Start creating the instance.
    vk::ApplicationInfo appInfo;
    appInfo.apiVersion = assessVulkanVersion( minVersion );

    vk::InstanceCreateInfo createInfo( { },                                         // flags
                                       &appInfo,                                    // pApplicationInfo
                                       static_cast<uint32_t>( layers.size( ) ),     // enabledLayerCount
                                       layers.data( ),                              // ppEnabledLayerNames
                                       static_cast<uint32_t>( extensions.size( ) ), // enabledExtensionCount
                                       extensions.data( ) );                        // ppEnabledExtensionNames

    auto instance    = createInstanceUnique( createInfo );
    global::instance = instance.get( );
    VK_CORE_ASSERT( instance, "Failed to create instance." );

    VULKAN_HPP_DEFAULT_DISPATCHER.init( instance.get( ) );

    return std::move( instance );
  }

  inline auto initDeviceUnique( std::vector<const char*>& extensions, const std::optional<vk::PhysicalDeviceFeatures>& features, const std::optional<vk::PhysicalDeviceFeatures2>& features2 = { } ) -> vk::UniqueDevice
  {
    checkDeviceExtensionSupport( extensions );

    auto queueCreateInfos = getDeviceQueueCreateInfos( );

    vk::DeviceCreateInfo createInfo( { },                                                                                           // flags
                                     static_cast<uint32_t>( queueCreateInfos.size( ) ),                                             // queueCreateInfoCount
                                     queueCreateInfos.data( ),                                                                      // pQueueCreateInfos
                                     0,                                                                                             // enabledLayerCount
                                     nullptr,                                                                                       // ppEnabledLayerNames
                                     static_cast<uint32_t>( extensions.size( ) ),                                                   // enabledExtensionCount
                                     extensions.data( ),                                                                            // ppEnabledExtensionNames
                                     features2.has_value( ) ? nullptr : ( features.has_value( ) ? &features.value( ) : nullptr ) ); // pEnabledFeatures - must be NULL because the VkDeviceCreateInfo::pNext chain includes VkPhysicalDeviceFeatures2.

    createInfo.pNext = features2.has_value( ) ? &features2.value( ) : nullptr;

    auto device    = global::physicalDevice.createDeviceUnique( createInfo );
    global::device = device.get( );
    VK_CORE_ASSERT( device, "Failed to create logical device." );

    VULKAN_HPP_DEFAULT_DISPATCHER.init( device.get( ) );

    return std::move( device );
  }

#endif
} // namespace vkCore
