#include <llri/llri.hpp>
#include <llri-vk/utils.hpp>
#include <graphics/vulkan/volk.h>

namespace legion::graphics::llri
{
    namespace internal
    {
        constexpr adapter_type mapPhysicalDeviceType(const VkPhysicalDeviceType& type)
        {
            switch (type)
            {
            case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                return adapter_type::Other;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                return adapter_type::Integrated;
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                return adapter_type::Discrete;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                return adapter_type::Virtual;
            default:
                break;
            }

            return adapter_type::Other;
        }
    }

    result Adapter::impl_queryInfo(adapter_info* info) const
    {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(static_cast<VkPhysicalDevice>(m_ptr), &properties);

        adapter_info result;
        result.vendorId = properties.vendorID;
        result.adapterId = properties.deviceID;
        result.adapterName = properties.deviceName;
        result.adapterType = internal::mapPhysicalDeviceType(properties.deviceType);
        *info = result;
        return result::Success;
    }

    result Adapter::impl_queryFeatures(adapter_features* features) const
    {
        VkPhysicalDeviceFeatures physicalFeatures;
        vkGetPhysicalDeviceFeatures(static_cast<VkPhysicalDevice>(m_ptr), &physicalFeatures);

        adapter_features result;

        //Set all the information in a structured way here

        *features = result;
        return result::Success;
    }

    result Adapter::impl_queryExtensionSupport(const adapter_extension_type& type, bool* supported) const
    {
        *supported = false;

        switch (type)
        {
        default:
            break;
        }

        return result::Success;
    }
}