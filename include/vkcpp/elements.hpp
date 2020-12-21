#ifndef _VKCPP_ELEMENTS_INCLUDED_
#define _VKCPP_ELEMENTS_INCLUDED_

#include <vulkan/vulkan.h>

#include <vector>
#include <stdexcept>
#include <functional>
#include <string_view>
#include <cassert>

namespace vkcpp
{
enum class derived_handle_kind : bool
{
    unique,
    vector
};

namespace private_
{
template< typename vk_handle >
class unique_handle
{
public:
    using native_type = vk_handle;

    unique_handle() noexcept
        : native_( VK_NULL_HANDLE )
    {}
    unique_handle( unique_handle& ) = delete;
    unique_handle& operator=( unique_handle& ) = delete;
    unique_handle( unique_handle&& handle ) noexcept
        : native_( handle.native_ )
    {
        handle.native_ = VK_NULL_HANDLE;
    }

    explicit operator bool() const noexcept { return ( VK_NULL_HANDLE != native_ ); }

    [[nodiscard]] native_type native() const noexcept { return native_; }
    [[nodiscard]] native_type* pnative() noexcept { return &native_; }

protected:
    explicit unique_handle( native_type const native ) noexcept
        : native_( native )
    {}

    unique_handle& operator=( unique_handle&& ) noexcept = default;

    ~unique_handle() noexcept = default;

private:
    native_type native_;
};

template< typename vk_handle >
class weak_handle : public unique_handle< vk_handle >
{
public:
    using base_type = unique_handle< vk_handle >;
    using native_type = typename base_type::native_type;

    weak_handle()
        : base_type()
    {}
    weak_handle( weak_handle&& handle ) noexcept
        : base_type( std::move( handle ) )
    {}

    weak_handle( weak_handle& ) = delete;
    weak_handle& operator=( weak_handle& ) = delete;

    weak_handle& operator=( weak_handle&& handle ) noexcept
    {
        reset( std::move( handle ) );
        return *this;
    }

    void reset( weak_handle&& handle ) noexcept
    {
        assert( !( *this ) );
        reset_impl( std::move( handle ) );
    }
    void reset( native_type const native = VK_NULL_HANDLE ) noexcept
    {
        assert( !( *this ) );
        this->native_ = native;
    }

    [[nodiscard]] native_type replace( weak_handle&& handle ) noexcept
    {
        assert( *this );
        auto const result = this->native();
        reset_impl( std::move( handle ) );
        return result;
    }
    [[nodiscard]] native_type replace( native_type const native = VK_NULL_HANDLE ) noexcept
    {
        assert( *this );
        auto const result = this->native();
        *( this->pnative() ) = native;
        return result;
    }

    native_type release() noexcept
    {
        auto result = this->native();
        *( this->pnative() ) = VK_NULL_HANDLE;
        return result;
    }
    ~weak_handle() noexcept { assert( !( *this ) ); }

private:
    void reset_impl( weak_handle&& handle ) noexcept
    {
        *(this->pnative()) = handle.native();
        *(handle.pnative()) = VK_NULL_HANDLE;
    }
};

template< typename vk_handle >
using vk_source_deleter = void( __stdcall* )( vk_handle, VkAllocationCallbacks const* );

template< typename vk_handle, vk_source_deleter< vk_handle > native_deleter >
class source_handle : public unique_handle< vk_handle >
{
public:
    using ancestor_type = source_handle;
    using deleter_type = vk_source_deleter< vk_handle >;
    using base_type = unique_handle< vk_handle >;
    using native_type = typename base_type::native_type;

    source_handle( source_handle& ) = delete;
    source_handle& operator=( source_handle& ) = delete;

    source_handle( source_handle&& handle ) noexcept
        : base_type( std::move( handle ) )
    {}

    source_handle& operator=( source_handle&& handle ) noexcept
    {
        reset( std::move( handle ) );
        return *this;
    }
    ~source_handle() noexcept { free(); }

    void reset( source_handle&& handle ) noexcept
    {
        if( this->native_ != handle.native_ )
        {
            free();
            this->native_ = handle.native_;
            handle.native_ = VK_NULL_HANDLE;
        }
    }

    void reset() noexcept
    {
        free();
        this->native_ = VK_NULL_HANDLE;
    }

protected:
    source_handle() = default;

    explicit source_handle( native_type const native ) noexcept
        : base_type( native )
    {}

    void free()
    {
        if( *this )
        {
            native_deleter( this->native(), nullptr );
        }
    }
};

template< typename vk_source_handle, typename vk_derived_handle >
using vk_derived_deleter = void( __stdcall* )( vk_source_handle, vk_derived_handle, VkAllocationCallbacks const* );

template< typename vk_source_handle, typename vk_derived_handle, vk_derived_deleter< vk_source_handle, vk_derived_handle > native_deleter,
          derived_handle_kind handle_count = derived_handle_kind::unique >
class derived_handle_base
{
public:
    using source_native_type = vk_source_handle;
    using native_type = vk_derived_handle;

    derived_handle_base( derived_handle_base const& ) = delete;
    derived_handle_base& operator=( derived_handle_base& ) = delete;

protected:
    explicit derived_handle_base( [[maybe_unused]] size_t const size = 1 )
        : wnative_()
    {}

    derived_handle_base( derived_handle_base&& handle ) noexcept = default;
    derived_handle_base& operator=( derived_handle_base&& handle ) noexcept = default;
    ~derived_handle_base() = default;

    explicit operator bool() const { return ( bool )wnative_; }

    bool free( source_native_type const source_native )
    {
        if( *this )
        {
            native_deleter( source_native, wnative_.replace(), nullptr );
            return true;
        }
        return false;
    }

    void reset( source_native_type const source_native ) noexcept { free( source_native ); }

    void reset( source_native_type const source_native, derived_handle_base&& handle )
    {
        if( wnative_.native() != handle.wnative_.native() )
        {
            if( *this )
            {
                native_deleter( source_native, wnative_.replace( std::move( handle ) ), nullptr );
            }
            else
            {
                wnative_.reset( std::move( handle ) );
            }
        }
    }

    native_type* pnative( size_t const index = 0 )
    {
        assert( 0 == index );
        return wnative_.pnative();
    }

    native_type native( size_t const index = 0 )
    {
        assert( 0 == index );
        return wnative_.native();
    }

    [[nodiscard]] size_t size() const { return 1; }

private:
    weak_handle< native_type > wnative_;
};

template< typename vk_source_handle, typename vk_derived_handle, vk_derived_deleter< vk_source_handle, vk_derived_handle > native_deleter >
class derived_handle_base< vk_source_handle, vk_derived_handle, native_deleter, derived_handle_kind::vector >
{
public:
    using source_native_type = vk_source_handle;
    using native_type = vk_derived_handle;

    derived_handle_base( derived_handle_base const& ) = delete;
    derived_handle_base& operator=( derived_handle_base& ) = delete;

protected:
    explicit derived_handle_base( size_t const size )
        : wnative_vector_( size )
    {
        assert( 1 < size );
    }

    derived_handle_base( derived_handle_base&& handle ) noexcept
        : wnative_vector_( std::move( handle.wnative_vector_ ) )
    {}

    derived_handle_base& operator=( derived_handle_base&& handle ) noexcept = default;
    ~derived_handle_base() = default;

    explicit operator bool() const { return !wnative_vector_.empty() && wnative_vector_[ 0 ]; }

    bool free( source_native_type const source_native ) noexcept
    {
        if( *this )
        {
            free_impl( source_native );
            return true;
        }
        return false;
    }

    void reset( source_native_type const source_native ) noexcept
    {
        if( free( source_native ) )
        {
            wnative_vector_.clear();
        }
    }

    void reset( source_native_type const source_native, derived_handle_base&& handle ) noexcept
    {
        if( this != &handle )
        {
            free( source_native );
            std::move( handle.wnative_vector_ );
        }
    }

    native_type* pnative( size_t const index = 0 ) noexcept
    {
        assert( index < wnative_vector_.size() );
        return wnative_vector_[ index ].pnative();
    }

    native_type native( size_t const index = 0 ) noexcept
    {
        assert( index < wnative_vector_.size() );
        return wnative_vector_[ index ].native();
    }

    [[nodiscard]] size_t size() const noexcept { return wnative_vector_.size(); }

private:
    std::vector< weak_handle< native_type > > wnative_vector_;

    void free_impl( source_native_type const source_native ) noexcept
    {
        for( auto& in: wnative_vector_ )
        {
            native_deleter( source_native, in.replace(), nullptr );
        }
    }
};

template< typename vk_source_handle, typename vk_derived_handle, vk_derived_deleter< vk_source_handle, vk_derived_handle > native_deleter,
          derived_handle_kind handle_count = derived_handle_kind::unique >
class derived_handle : public derived_handle_base< vk_source_handle, vk_derived_handle, native_deleter, handle_count >
{
public:
    using base_type = derived_handle_base< vk_source_handle, vk_derived_handle, native_deleter, handle_count >;
    using source_native_type = typename base_type::source_native_type;
    using native_type = typename base_type::native_type;

    derived_handle( derived_handle&& handle ) noexcept
        : base_type( std::move( handle ) )
        , source_native_( handle.source_native_ )
    {
        handle.source_native_ = VK_NULL_HANDLE;
    }
    derived_handle( derived_handle& ) = delete;
    derived_handle& operator=( derived_handle& ) = delete;

    derived_handle& operator=( derived_handle&& handle ) noexcept { reset( source_native_, std::move( handle ) ); }

    ~derived_handle() { base_type::free( source_native_ ); }

    explicit operator bool() const { return source_native_ != VK_NULL_HANDLE && base_type::operator bool(); }

    void reset() noexcept
    {
        base_type::reset( source_native_ );
        source_native_ = VK_NULL_HANDLE;
    }

    void reset( derived_handle&& handle ) noexcept
    {
        if( this != &handle )
        {
            source_native_ = handle.source_native_;
            base_type::reset( std::move( handle ) );
            handle.source_native_ = VK_NULL_HANDLE;
        }
    }

    [[nodiscard]] native_type native( size_t const index = 0 ) const noexcept { return base_type::native( index ); }
    [[nodiscard]] source_native_type source_native() const noexcept { return source_native_; }

protected:
    explicit derived_handle( size_t const size, source_native_type const source_native = VK_NULL_HANDLE )
        : base_type( size )
        , source_native_( source_native )
    {}

private:
    source_native_type source_native_;
};

void __stdcall destroy_debug_report( VkInstance, VkDebugReportCallbackEXT, VkAllocationCallbacks const* );

} // namespace private_

class version
{
private:
    uint32_t packed_{ 0 };
    static constexpr unsigned short const major_bit_offset = 22;
    static constexpr unsigned short const minor_bit_offset = 12;
    static constexpr unsigned short const patch_bit_offset = 0;

    static constexpr uint32_t const minor_bit_mask = ( 1U << ( unsigned )( major_bit_offset - minor_bit_offset ) ) - 1U;
    static constexpr uint32_t const patch_bit_mask = ( 1U << minor_bit_offset ) - 1U;

public:
    version() = default;

    version( unsigned short const major, unsigned short const minor, unsigned short const patch )
        : packed_( ( unsigned )( major << major_bit_offset ) | ( unsigned )( minor << minor_bit_offset ) | patch )
    {}

    explicit version( uint32_t packed )
        : packed_( packed )
    {}

    [[nodiscard]] unsigned short major() const { return ( packed_ >> major_bit_offset ); }

    [[nodiscard]] unsigned short minor() const { return ( ( packed_ >> minor_bit_offset ) & minor_bit_mask ); }

    [[nodiscard]] unsigned short patch() const { return ( packed_ & patch_bit_mask ); }

    explicit operator uint32_t() const { return packed_; }
};

template< typename enum_type, typename std::enable_if< std::is_enum< enum_type >::value, int >::type = 0 >
class enum_flags
{
private:
    using value_type = std::underlying_type_t< enum_type >;
    value_type flags_;

public:
    enum_flags()
        : flags_( 0 )
    {}
    explicit enum_flags( enum_type const flag )
        : flags_( static_cast< value_type >( flag ) )
    {}
    explicit enum_flags( value_type const flags )
        : flags_( flags )
    {}

    value_type operator()() const { return flags_; }
    enum_flags operator~() { return enum_flags( ~flags_ ); }
};

template< typename enum_type, typename std::enable_if< std::is_enum< enum_type >::value, int >::type = 0 >
enum_flags< enum_type > operator|( enum_flags< enum_type > const lhs, enum_flags< enum_type > const rhs )
{
    return enum_flags< enum_type >( lhs() | rhs() );
}

template< typename enum_type, typename std::enable_if< std::is_enum< enum_type >::value, int >::type = 0 >
enum_flags< enum_type > operator|( enum_flags< enum_type > const lhs, enum_type const rhs )
{
    return lhs | enum_flags< enum_type >( rhs );
}

template< typename enum_type, typename std::enable_if< std::is_enum< enum_type >::value, int >::type = 0 >
enum_flags< enum_type > operator|=( enum_flags< enum_type >& lhs, enum_flags< enum_type > const rhs )
{
    lhs = lhs | rhs;
    return lhs;
}

template< typename enum_type, typename std::enable_if< std::is_enum< enum_type >::value, int >::type = 0 >
enum_flags< enum_type > operator|=( enum_flags< enum_type >& lhs, enum_type const rhs )
{
    lhs = lhs | rhs;
    return lhs;
}

template< typename enum_type, typename std::enable_if< std::is_enum< enum_type >::value, int >::type = 0 >
enum_flags< enum_type > operator&( enum_flags< enum_type > const lhs, enum_flags< enum_type > const rhs )
{
    return enum_flags< enum_type >( lhs() & rhs() );
}

template< typename enum_type, typename std::enable_if< std::is_enum< enum_type >::value, int >::type = 0 >
enum_flags< enum_type > operator&( enum_flags< enum_type > const lhs, enum_type const rhs )
{
    return lhs & enum_flags< enum_type >( rhs );
}

template< typename enum_type, typename std::enable_if< std::is_enum< enum_type >::value, int >::type = 0 >
enum_flags< enum_type > operator&=( enum_flags< enum_type >& lhs, enum_flags< enum_type > const rhs )
{
    lhs = lhs & rhs;
    return lhs;
}

template< typename enum_type, typename std::enable_if< std::is_enum< enum_type >::value, int >::type = 0 >
enum_flags< enum_type > operator&=( enum_flags< enum_type >& lhs, enum_type const rhs )
{
    lhs = lhs & rhs;
    return lhs;
}

enum class result
{
    SUCCESS = VK_SUCCESS,
    NOT_READY = VK_NOT_READY,
    TIMEOUT = VK_TIMEOUT,
    EVENT_SET = VK_EVENT_SET,
    EVENT_RESET = VK_EVENT_RESET,
    INCOMPLETE = VK_INCOMPLETE,
    ERROR_OUT_OF_HOST_MEMORY = VK_ERROR_OUT_OF_HOST_MEMORY,
    ERROR_OUT_OF_DEVICE_MEMORY = VK_ERROR_OUT_OF_DEVICE_MEMORY,
    ERROR_INITIALIZATION_FAILED = VK_ERROR_INITIALIZATION_FAILED,
    ERROR_DEVICE_LOST = VK_ERROR_DEVICE_LOST,
    ERROR_MEMORY_MAP_FAILED = VK_ERROR_MEMORY_MAP_FAILED,
    ERROR_LAYER_NOT_PRESENT = VK_ERROR_LAYER_NOT_PRESENT,
    ERROR_EXTENSION_NOT_PRESENT = VK_ERROR_EXTENSION_NOT_PRESENT,
    ERROR_FEATURE_NOT_PRESENT = VK_ERROR_FEATURE_NOT_PRESENT,
    ERROR_INCOMPATIBLE_DRIVER = VK_ERROR_INCOMPATIBLE_DRIVER,
    ERROR_TOO_MANY_OBJECTS = VK_ERROR_TOO_MANY_OBJECTS,
    ERROR_FORMAT_NOT_SUPPORTED = VK_ERROR_FORMAT_NOT_SUPPORTED,
    ERROR_SURFACE_LOST_KHR = VK_ERROR_SURFACE_LOST_KHR,
    ERROR_NATIVE_WINDOW_IN_USE_KHR = VK_ERROR_NATIVE_WINDOW_IN_USE_KHR,
    SUBOPTIMAL_KHR = VK_SUBOPTIMAL_KHR,
    ERROR_OUT_OF_DATE_KHR = VK_ERROR_OUT_OF_DATE_KHR,
    ERROR_INCOMPATIBLE_DISPLAY_KHR = VK_ERROR_INCOMPATIBLE_DISPLAY_KHR,
    ERROR_VALIDATION_FAILED_EXT = VK_ERROR_VALIDATION_FAILED_EXT,
    ERROR_INVALID_SHADER_NV = VK_ERROR_INVALID_SHADER_NV
};

namespace dbg
{
enum class flag
{
    INFO = VK_DEBUG_REPORT_INFORMATION_BIT_EXT,
    WARN = VK_DEBUG_REPORT_WARNING_BIT_EXT,
    PERF = VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT,
    ERR = VK_DEBUG_REPORT_ERROR_BIT_EXT,
    DEBUG = VK_DEBUG_REPORT_DEBUG_BIT_EXT
};

using flags = enum_flags< flag >;

enum class object
{
    UNKNOWN = VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT,
    INSTANCE = VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT,
    PHYSICAL_DEVICE = VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT,
    DEVICE = VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
    QUEUE = VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT,
    SEMAPHORE = VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT,
    COMMAND_BUFFER = VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
    FENCE = VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT,
    DEVICE_MEMORY = VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT,
    BUFFER = VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT,
    IMAGE = VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
    EVENT = VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT,
    QUERY_POOL = VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT,
    BUFFER_VIEW = VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT,
    IMAGE_VIEW = VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT,
    SHADER_MODULE = VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT,
    PIPELINE_CACHE = VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT,
    PIPELINE_LAYOUT = VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT,
    RENDER_PASS = VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT,
    PIPELINE = VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT,
    DESCRIPTOR_SET_LAYOUT = VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT,
    SAMPLER = VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT,
    DESCRIPTOR_POOL = VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT,
    DESCRIPTOR_SET = VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT,
    FRAMEBUFFER = VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT,
    COMMAND_POOL = VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT,
    SURFACE_KHR = VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT,
    SWAPCHAIN_KHR = VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT,
    VALIDATION_CACHE_EXT_EXT = VK_DEBUG_REPORT_OBJECT_TYPE_VALIDATION_CACHE_EXT_EXT,
    SAMPLER_YCBCR_CONVERSION_EXT = VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION_EXT,
    DESCRIPTOR_UPDATE_TEMPLATE_EXT = VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_EXT,
    ACCELERATION_STRUCTURE_KHR_EXT = VK_DEBUG_REPORT_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR_EXT,
    DEBUG_REPORT_EXT = VK_DEBUG_REPORT_OBJECT_TYPE_DEBUG_REPORT_EXT,
    VALIDATION_CACHE_EXT = VK_DEBUG_REPORT_OBJECT_TYPE_VALIDATION_CACHE_EXT,
    DESCRIPTOR_UPDATE_TEMPLATE_KHR_EXT = VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_KHR_EXT,
    SAMPLER_YCBCR_CONVERSION_KHR_EXT = VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION_KHR_EXT,
    ACCELERATION_STRUCTURE_NV_EXT = VK_DEBUG_REPORT_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV_EXT,
};

} // namespace dbg

struct exception : public std::runtime_error
{
    result result;
    dbg::object object;

    exception( enum result const result, dbg::object const object, char const* const error_str )
        : std::runtime_error( error_str )
        , result( result )
        , object( object )
    {}

    exception( int const status, dbg::object const object, char const* const error_str )
        : std::runtime_error( error_str )
        , result( static_cast< vkcpp::result >( status ) )
        , object( object )
    {}
};

using offset2d = VkOffset2D;
using offset3d = VkOffset3D;
using extent2d = VkExtent2D;
using extent3d = VkExtent3D;
using rect2d = VkRect2D;
using view_port = VkViewport;

struct layer : public VkLayerProperties
{
public:
    using id_type = char const*;

    static constexpr id_type const vendor_standard_layer = "VK_LAYER_KHRONOS_validation";
    static constexpr id_type const vendor_api_dump = "VK_LAYER_LUNARG_api_dump";

    static std::vector< layer > enumerate();

    [[nodiscard]] std::string_view name() const { return std::string_view( static_cast< char const* >( layerName ) ); }
    [[nodiscard]] std::string_view desc() const { return std::string_view( static_cast< char const* >( description ) ); }

    [[nodiscard]] version spec_version() const { return version( specVersion ); }
    [[nodiscard]] version impl_version() const { return version( implementationVersion ); }
};

struct extension : public VkExtensionProperties
{
    using id_type = char const*;

    static constexpr id_type const debug_report = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
    static constexpr id_type const khr_surface = VK_KHR_SURFACE_EXTENSION_NAME;

    static std::vector< extension > enumerate( layer::id_type layer_id );

    [[nodiscard]] std::string_view name() const { return std::string_view( static_cast< char const* >( extensionName ) ); }
    [[nodiscard]] version spec_version() const { return version( specVersion ); }
};

class instance : public private_::source_handle< VkInstance, vkDestroyInstance >
{
public:
    using base_type = private_::source_handle< VkInstance, vkDestroyInstance >;

    using base_type::base_type;
    explicit instance( base_type&& base )
        : base_type( std::move( base ) )
    {}

    class builder : public base_type
    {
    public:
        builder() = default;

        builder& add_layer( layer::id_type const i_id )
        {
            layer_list_.push_back( i_id );
            return *this;
        }

        template< typename iterator_type >
        builder& add_layer( iterator_type i_begin, iterator_type const i_end )
        {
            for( ; i_begin != i_end; i_begin++ )
            {
                layer_list_.push_back( *i_begin );
            }
            return *this;
        }

        builder& add_extension( extension::id_type const& i_id )
        {
            extension_list_.push_back( i_id );
            return *this;
        }

        template< typename iterator_type >
        builder& add_extension( iterator_type i_begin, iterator_type const i_end )
        {
            for( ; i_begin != i_end; i_begin++ )
            {
                extension_list_.push_back( *i_begin );
            }
            return *this;
        }

        instance build( std::string const& app_name, version app_version, std::string const& engine_name, version engine_version );

    private:
        std::vector< layer::id_type > layer_list_;
        std::vector< extension::id_type > extension_list_;
    };
};

namespace dbg
{
using callback_type = std::function< bool( flag flag, object const, unsigned long long const object_id, int const location, int const message_code,
                                           std::string_view layer_prefix, std::string_view message, void* puserdata ) >;

struct report : public private_::derived_handle< VkInstance, VkDebugReportCallbackEXT, private_::destroy_debug_report, derived_handle_kind::unique >
{
private:
    callback_type mc_report_;
    void* puser_data_;

public:
    using base_type = private_::derived_handle< VkInstance, VkDebugReportCallbackEXT, private_::destroy_debug_report, derived_handle_kind::unique >;

    report( vkcpp::instance instance, callback_type cb, flags flags, void* puser );

    bool operator()( flags flags, object object, int message_code, std::string const& message );
    bool operator()( flag flag, object object, unsigned long long object_id, int location, int message_code, std::string_view layer_prefix,
                     std::string_view message );
};

} // namespace dbg

class physical_device
{
private:
    VkPhysicalDevice native_{ VK_NULL_HANDLE };

public:
    enum class kind
    {
        OTHER = VK_PHYSICAL_DEVICE_TYPE_OTHER,
        INTEGRATED_GPU = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU,
        DISCRETE_GPU = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU,
        VIRTUAL_GPU = VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU,
        CPU = VK_PHYSICAL_DEVICE_TYPE_CPU
    };
    using id_type = uint32_t;

    static std::vector< physical_device > enumerate( vkcpp::instance const& instance );

    physical_device() = default;

    [[nodiscard]] VkPhysicalDevice native() const noexcept { return native_; }

    struct property : public VkPhysicalDeviceProperties
    {
        explicit property( physical_device const i_physical_device )
            : VkPhysicalDeviceProperties{}
        {
            vkGetPhysicalDeviceProperties( i_physical_device.native(), this );
        }

        [[nodiscard]] version api_version() const noexcept { return version( apiVersion ); }
        [[nodiscard]] version driver_version() const noexcept { return version( driverVersion ); }
        [[nodiscard]] physical_device::kind kind() const noexcept { return static_cast< physical_device::kind >( deviceType ); }
        [[nodiscard]] std::string_view name() const noexcept { return std::string_view( static_cast< char const* >( deviceName ) ); }

    };

    class feature : public VkPhysicalDeviceFeatures
    {
    public:
        feature()
            : VkPhysicalDeviceFeatures{}
        {}
        explicit feature( physical_device const physical_device )
            : VkPhysicalDeviceFeatures{}
        {
            vkGetPhysicalDeviceFeatures( physical_device.native(), this );
        }
    };

    class memory_property : public VkPhysicalDeviceMemoryProperties
    {
    public:
        enum class flag
        {
            DEVICE_LOCAL = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            HOST_VISIBLE = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
            HOST_COHERENT = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            HOST_CACHED = VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
            LAZILY_ALLOCATED = VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT
        };
        using flags = enum_flags< flag >;

        explicit memory_property( physical_device const physical_device )
            : VkPhysicalDeviceMemoryProperties{}
        {
            vkGetPhysicalDeviceMemoryProperties( physical_device.native(), this );
        }

        [[nodiscard]] unsigned find_memory_type_index( unsigned memory_type_index_bits, flags memory_type_flags ) const noexcept;
    };
};

class device_extension : public extension
{
public:
    static constexpr id_type const swap_chain = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    static std::vector< extension > enumerate( physical_device device, layer::id_type layer_id );
};

class device : public private_::source_handle< VkDevice, vkDestroyDevice >
{
public:
    using base_type = private_::source_handle< VkDevice, vkDestroyDevice >;

    device() = default;

    explicit device( base_type&& i_handle )
        : base_type( std::move( i_handle ) )
    {}

    void wait_idle() const;

    class queue
    {
    public:
        enum class sharing
        {
            EXCLUSIVE = VK_SHARING_MODE_EXCLUSIVE,
            CONCURRENT = VK_SHARING_MODE_CONCURRENT,
        };

        struct family : public VkQueueFamilyProperties
        {
            enum ability_flag : uint32_t
            {
                SUPPORTS_GRAPHICS = VK_QUEUE_GRAPHICS_BIT,
                SUPPORTS_COMPUTATION = VK_QUEUE_COMPUTE_BIT,
                SUPPORTS_TRANSFER = VK_QUEUE_TRANSFER_BIT,
                SUPPORTS_SPARSE_BINDING = VK_QUEUE_SPARSE_BINDING_BIT
            };

            using ability_flags = enum_flags< ability_flag >;
            using id_type = uint32_t;

            static constexpr id_type const IGNORE_FAMILY = VK_QUEUE_FAMILY_IGNORED;

            static std::vector< family > enumerate( physical_device physical_device );

            [[nodiscard]] bool does( ability_flags const abilities ) const { return 0 != ( queueFlags & abilities() ); }
        };

        enum kind
        {
            GRAPHICS,
            COMPUTATION,
            TRANSFER,
            SPARSE_BINDING,
            PRESENTATION,
            MAX_KIND
        };

        using id_type = uint32_t;
        using priority_type = float;

        queue() = default;

        queue( device device, family::id_type family_index, id_type index );

        [[nodiscard]] VkQueue native() const { return native_; }

        void wait_idle() const;

    private:
        VkQueue native_{ VK_NULL_HANDLE };
    };

    class builder : public base_type
    {
    private:
        std::vector< layer::id_type > layer_list_;
        std::vector< extension::id_type > extension_list_;
        std::vector< VkDeviceQueueCreateInfo > reserved_queues_;
        std::vector< std::vector< queue::priority_type > > queue_priorities_;

    public:
        builder() = default;

        builder& add_layer( layer::id_type const id )
        {
            layer_list_.push_back( id );
            return *this;
        }

        template< typename iterator_type >
        builder& add_layer( iterator_type begin, iterator_type const end )
        {
            for( ; begin != end; begin++ )
            {
                layer_list_.push_back( *begin );
            }
            return *this;
        }

        builder& add_extension( extension::id_type const id )
        {
            extension_list_.push_back( id );
            return *this;
        }

        template< typename iterator_type >
        builder& add_extension( iterator_type begin, iterator_type const end )
        {
            for( ; begin != end; begin++ )
            {
                extension_list_.push_back( *begin );
            }
            return *this;
        }

        builder& reserve_queue_family( queue::family::id_type family_index, std::vector< queue::priority_type > queue_priority );

        device build( physical_device physical_device, physical_device::feature const& feature );
    };
};

template< derived_handle_kind handle_kind = derived_handle_kind::unique >
class semaphore : public private_::derived_handle< VkDevice, VkSemaphore, vkDestroySemaphore, handle_kind >
{
public:
    using base_type = private_::derived_handle< VkDevice, VkSemaphore, vkDestroySemaphore, handle_kind >;
    explicit semaphore( device device = vkcpp::device(), size_t size = 1 );
};

template< derived_handle_kind handle_kind = derived_handle_kind::unique >
class fence : public private_::derived_handle< VkDevice, VkFence, vkDestroyFence, handle_kind >
{
public:
    using base_type = private_::derived_handle< VkDevice, VkFence, vkDestroyFence, handle_kind >;

    enum class create_flag
    {
        CREATE_SIGNALED = VK_FENCE_CREATE_SIGNALED_BIT
    };
    using create_flags = enum_flags< create_flag >;

    explicit fence( device device = vkcpp::device(), size_t size = 1, create_flags flags = create_flags() );

    void wait( unsigned long long timeout );
    void reset_signal();

protected:
    using base_type::base_type;
};

} // namespace vkcpp

#endif // _VKCPP_ELEMENTS_INCLUDED_

