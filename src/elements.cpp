#include <utility>
#include <vkcpp/elements.hpp>
#include <cassert>

namespace
{
VKAPI_ATTR VkBool32 VKAPI_CALL report_callback( VkDebugReportFlagsEXT flag, VkDebugReportObjectTypeEXT object_type, uint64_t object, size_t location,
                                                int32_t message_code, const char* player_prefix, const char* pmessage, void* puser_data )
{
    auto* report = static_cast< vkcpp::dbg::report* >( puser_data );
    return ( *report )( static_cast< vkcpp::dbg::flag >( flag ), static_cast< vkcpp::dbg::object >( object_type ), object, location, message_code,
                        std::string_view( player_prefix ), std::string_view( pmessage ) )
             ? VK_TRUE
             : VK_FALSE;
}

VkDebugReportCallbackCreateInfoEXT debug_creation_info( uint32_t const i_level_flags, vkcpp::dbg::report* pdebug_report )
{
    VkDebugReportCallbackCreateInfoEXT result{};
    result.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    result.flags = i_level_flags;
    result.pfnCallback = &report_callback;
    result.pUserData = static_cast< void* >( pdebug_report );
    result.pNext = nullptr;
    return result;
}

} // namespace

namespace vkcpp
{
std::vector< layer > layer::enumerate()
{
    uint32_t count = 0;
    auto status = vkEnumerateInstanceLayerProperties( &count, nullptr );
    if( VK_SUCCESS == status )
    {
        if( 0 < count )
        {
            std::vector< layer > layer_list( count );
            status = vkEnumerateInstanceLayerProperties( &count, layer_list.data() );
            if( VK_SUCCESS == status )
            {
                return layer_list;
            }
        }
    }
    throw exception( status, dbg::object::INSTANCE, "enumeration of layer" );
}

std::vector< extension > extension::enumerate( layer::id_type const layer_id )
{
    uint32_t count = 0;
    auto status = vkEnumerateInstanceExtensionProperties( layer_id, &count, nullptr );
    if( VK_SUCCESS == status )
    {
        if( 0 < count )
        {
            std::vector< extension > extension_list( count );
            status = vkEnumerateInstanceExtensionProperties( layer_id, &count, extension_list.data() );
            if( VK_SUCCESS == status )
            {
                return extension_list;
            }
        }
    }
    throw exception( status, dbg::object::INSTANCE, "extension enumeration" );
}

instance::instance( std::string const& app_name, version app_version, std::string const& engine_name, version engine_version,
                    std::vector< layer::id_type > const& layers, std::vector< extension::id_type > const& extensions )
    : base_type()
{
    VkApplicationInfo app_info{ .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                                .pNext = nullptr,
                                .pApplicationName = app_name.c_str(),
                                .applicationVersion = ( uint32_t )app_version,
                                .pEngineName = engine_name.c_str(),
                                .engineVersion = ( uint32_t )engine_version,
                                .apiVersion = ( uint32_t )vkcpp::version( 1, 2, 0 ) };

    VkInstanceCreateInfo create_info{ .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                                      .pNext = nullptr,
                                      .flags = 0,
                                      .pApplicationInfo = &app_info,
                                      .enabledLayerCount = static_cast< uint32_t >( layers.size() ),
                                      .ppEnabledLayerNames = 0 < create_info.enabledLayerCount ? layers.data() : nullptr,
                                      .enabledExtensionCount = static_cast< uint32_t >( extensions.size() ),
                                      .ppEnabledExtensionNames = 0 < create_info.enabledExtensionCount ? extensions.data() : nullptr };

    auto status = vkCreateInstance( &create_info, nullptr, pnative() );
    if( VK_SUCCESS == status )
    {
        return;
    }
    throw exception( status, dbg::object::INSTANCE, "creation" );
}

namespace private_
{
void __stdcall destroy_debug_report( VkInstance instance, VkDebugReportCallbackEXT report, [[maybe_unused]] VkAllocationCallbacks const* callbacks )
{
    auto deleter = reinterpret_cast< PFN_vkDestroyDebugReportCallbackEXT >( vkGetInstanceProcAddr( instance, "vkDestroyDebugReportCallbackEXT" ) );
    if( nullptr != deleter )
    {
        deleter( instance, report, nullptr );
    }
}
} // namespace private_

namespace dbg
{
report::report( instance const instance, callback_type cb, flags const flags, void* const puser )
    : base_type( 1, instance.native() )
    , mc_report_( std::move( cb ) )
    , puser_data_( puser )
{
    auto create = reinterpret_cast< PFN_vkCreateDebugReportCallbackEXT >( vkGetInstanceProcAddr( instance.native(), "vkCreateDebugReportCallbackEXT" ) );
    if( nullptr != create )
    {
        auto create_info = debug_creation_info( flags(), this );
        auto status = create( instance.native(), &create_info, nullptr, pnative() );
        if( VK_SUCCESS == status )
        {
            return;
        }
        throw exception( status, object::DEBUG_REPORT_EXT, "creation" );
    }
}

bool report::operator()( flags const flags, object const object, int const message_code, std::string const& message )
{
    auto native_report = reinterpret_cast< PFN_vkDebugReportMessageEXT >( vkGetInstanceProcAddr( source_native(), "vkDebugReportMessageEXT" ) );
    if( VK_NULL_HANDLE != native_report )
    {
        native_report( source_native(), flags(), static_cast< VkDebugReportObjectTypeEXT >( object ), 0LL, 0L, message_code, nullptr, message.c_str() );
        return true;
    }
    return false;
}

bool report::operator()( flag const flag, object const object, unsigned long long const object_id, int const location, int const message_code,
                         std::string_view layer_prefix, std::string_view message )
{
    return mc_report_( flag, object, object_id, location, message_code, layer_prefix, message, puser_data_ );
}

} // namespace dbg

std::vector< physical_device > physical_device::enumerate( vkcpp::instance const& instance )
{
    uint32_t count = 0;
    vkEnumeratePhysicalDevices( instance.native(), &count, nullptr );
    if( 0 < count )
    {
        std::vector< physical_device > device_list( count );
        auto const status = vkEnumeratePhysicalDevices( instance.native(), &count, reinterpret_cast< VkPhysicalDevice* >( device_list.data() ) );
        if( VK_SUCCESS == status )
        {
            return device_list;
        }
    }
    return std::vector< physical_device >();
}

unsigned physical_device::memory_property::find_memory_type_index( unsigned const memory_type_index_bits, flags const memory_type_flags ) const noexcept
{
    for( uint32_t imt = 0; imt < memoryTypeCount; ++imt )
    {
        if( ( memory_type_index_bits & ( 1U << imt ) ) && ( memoryTypes[ imt ].propertyFlags & memory_type_flags() ) == memory_type_flags() )
        {
            return imt;
        }
    }
    return std::numeric_limits< unsigned >::max();
}

std::vector< extension > device_extension::enumerate( physical_device const device, layer::id_type const layer_id )
{
    uint32_t count = 0;
    auto status = vkEnumerateDeviceExtensionProperties( device.native(), layer_id, &count, nullptr );
    if( VK_SUCCESS == status )
    {
        if( 0 < count )
        {
            std::vector< extension > extension_list( count );
            vkEnumerateDeviceExtensionProperties( device.native(), layer_id, &count, extension_list.data() );
            return extension_list;
        }
    }
    throw exception( status, dbg::object::PHYSICAL_DEVICE, "extension enumeration" );
}

device::queue::queue( device const device, device::queue::family::id_type const family_index, id_type const index )
{
    vkGetDeviceQueue( device.native(), family_index, index, &native_ );
}

void device::queue::wait_idle() const
{
    auto status = vkQueueWaitIdle( native_ );
    if( VK_SUCCESS != status )
    {
        throw exception( status, dbg::object::QUEUE, "waiting for idle" );
    }
}

std::vector< device::queue::family > device::queue::family::enumerate( physical_device const physical_device )
{
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties( physical_device.native(), &count, nullptr );
    if( 0 < count )
    {
        std::vector< device::queue::family > queue_list( count );
        vkGetPhysicalDeviceQueueFamilyProperties( physical_device.native(), &count, queue_list.data() );
        return queue_list;
    }
    return std::vector< device::queue::family >();
}

void device::wait_idle() const
{
    auto status = vkDeviceWaitIdle( native() );
    if( VK_SUCCESS != status )
    {
        throw exception( status, dbg::object::DEVICE, "waiting for idle" );
    }
}

device::builder& device::builder::reserve_queue_family( queue::family::id_type const family_index, std::vector< queue::priority_type > queue_priority ) &
{
    queue_priorities_.push_back( std::move( queue_priority ) );
    reserved_queues_.push_back( VkDeviceQueueCreateInfo{ .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                                         .pNext = nullptr,
                                                         .flags = 0,
                                                         .queueFamilyIndex = family_index,
                                                         .queueCount = static_cast< uint32_t >( queue_priorities_.back().size() ),
                                                         .pQueuePriorities = queue_priorities_.back().data() } );
    return *this;
}

device device::builder::build( physical_device const physical_device, physical_device::feature const& feature, std::vector< layer::id_type > const& layers,
                               std::vector< device_extension::id_type > const& extensions )
{
    VkDeviceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.pNext = nullptr;
    create_info.flags = 0;

    create_info.enabledLayerCount = static_cast< uint32_t >( layers.size() );
    create_info.ppEnabledLayerNames = ( 0 < create_info.enabledLayerCount ? layers.data() : nullptr );
    create_info.enabledExtensionCount = static_cast< uint32_t >( extensions.size() );
    create_info.ppEnabledExtensionNames = ( 0 < create_info.enabledExtensionCount ? extensions.data() : nullptr );

    create_info.queueCreateInfoCount = static_cast< uint32_t >( reserved_queues_.size() );
    create_info.pQueueCreateInfos = reserved_queues_.data();
    create_info.pEnabledFeatures = &feature;

    VkResult status = vkCreateDevice( physical_device.native(), &create_info, nullptr, pnative() );
    if( VK_SUCCESS == status )
    {
        return device( std::move( *this ) );
    }
    throw exception( status, dbg::object::DEVICE, "creation" );
}

template< derived_handle_kind handle_kind >
semaphore< handle_kind >::semaphore( device const device, size_t const size )
    : base_type( size, device.native() )
{
    if( device )
    {
        VkSemaphoreCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        info.flags = 0;
        info.pNext = nullptr;
        for( size_t is = 0; is < size; ++is )
        {
            auto status = vkCreateSemaphore( device.native(), &info, nullptr, base_type::pnative( is ) );
            if( VK_SUCCESS != status )
            {
                throw exception( status, dbg::object::SEMAPHORE, "creation" );
            }
        }
    }
}

template< derived_handle_kind handle_kind >
fence< handle_kind >::fence( device const device, size_t const size, create_flags const flags )
    : base_type( size, device.native() )
{
    if( device )
    {
        VkFenceCreateInfo info{};

        info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        info.pNext = nullptr;
        info.flags = flags;
        for( size_t iif = 0; iif < size; ++iif )
        {
            auto status = vkCreateFence( device, &info, nullptr, base_type::pnative( iif ) );
            if( VK_SUCCESS != status )
            {
                throw vkcpp::exception( status, vkcpp::dbg::object::FENCE, "creation" );
            }
        }
    }
}

template< derived_handle_kind handle_kind >
void fence< handle_kind >::wait( unsigned long long const timeout )
{
    assert( *this );
    auto status = vkWaitForFences( this->source_native_, base_type::size(), base_type::pnative( 0 ), VK_TRUE, timeout );
    if( VK_SUCCESS == status )
    {
        return;
    }
    throw exception( status, dbg::object::FENCE, "waiting" );
}

template< derived_handle_kind handle_kind >
void fence< handle_kind >::reset_signal()
{
    assert( *this );
    auto status = vkResetFences( this->source_native_, base_type::size(), base_type::pnative( 0 ) );
    if( VK_SUCCESS == status )
    {
        return;
    }
    throw exception( status, dbg::object::FENCE, "reset" );
}

} // namespace vkcpp

