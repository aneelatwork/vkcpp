#include <vkcpp/elements.hpp>
#include <iostream>

std::ostream& operator << ( std::ostream& strm, vkcpp::version version )
{
    strm << version.major() << '.' << version.minor() << '.' << version.patch();
    return strm;
}

int main( [[maybe_unused]] int argc, [[maybe_unused]] char* argv[] )
{
    try
    {
        auto layers = vkcpp::layer::enumerate();
        for( auto const& il : layers ) { std::cout << "Layer: " << il.name() << ';' << il.desc() << ';' << il.spec_version() << ',' << il.impl_version() <<  std::endl; }

        auto extensions = vkcpp::extension::enumerate( nullptr );
        for( auto const& ie : extensions ) { std::cout << "Extension: " << ie.name() << ',' << ie.spec_version() << std::endl; }

        auto instance = vkcpp::instance::builder()
                            .add_extension( vkcpp::extension::debug_report )
                            .add_layer( vkcpp::layer::vendor_standard_layer )
                            .build( "vkcpp-test", vkcpp::version( 0, 0, 1 ), "vkcpp-engine", vkcpp::version( 0, 0, 1 ) );

        auto devlist = vkcpp::physical_device::enumerate( instance );
        for( auto const& id : devlist )
        {
            auto dev_attr = vkcpp::physical_device::property( id );
            std::cout << "Device: " << dev_attr.name() << ';' << dev_attr.api_version() << ';' << dev_attr.driver_version() << ';'
                      << dev_attr.deviceType << ';' << dev_attr.deviceID << ';' << dev_attr.vendorID << std::endl;

            auto devext_list = vkcpp::device_extension::enumerate( id, nullptr );
            for( auto const& ie : devext_list )  { std::cout << "    Extension: " << ie.name() << ',' << ie.spec_version() << std::endl; }
        
        }

        return 0;
    }
    catch( vkcpp::exception& ex )
    {
        std::cout << "Vulkan Exception: " << std::hex << ( unsigned )ex.object << ',' << ( unsigned )ex.result << ':' << ex.what() << std::endl;
    }
    catch( std::exception& ex )
    {
        std::cout << "Standard Exception: " << ex.what() << std::endl;
    }
    return 1;
}

