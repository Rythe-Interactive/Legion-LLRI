/**
 * @file testsystem.cpp
 * @copyright 2021-2021 Leon Brands. All rights served.
 * @license: https://github.com/Legion-Engine/Legion-LLRI/blob/main/LICENSE
 */

/**
 * Sandbox is a testing area for LLRI development.
 * The code written in sandbox should be up to spec but may not contain the best practices or cleanest examples.
 *
 * See the samples for recommended usage and more detailed comments.
 */

#include "testsystem.hpp"

//#define LLRI_DISABLE_VALIDATION //uncommenting this disables API validation (see docs)
//#define LLRI_DISABLE_IMPLEMENTATION_MESSAGE_POLLING //uncommenting this disables implementation message polling
#include <llri/llri.hpp>

using namespace legion;

#define THROW_IF_FAILED(operation) { \
    auto r = operation; \
    if (r != llri::result::Success) \
    { \
        log::error("LLRI Operation failed because: {}, operation: {}", llri::to_string(r), #operation); \
        throw std::runtime_error("LLRI Operation failed"); \
    } \
} \

void callback(llri::validation_callback_severity severity, llri::validation_callback_source source, const char* message, void* userData)
{
    log::severity sev = log::severity_info;
    switch (severity)
    {
        case llri::validation_callback_severity::Verbose:
            sev = log::severity_trace;
            break;
        case llri::validation_callback_severity::Info:
            //Even though this semantically maps to info, we'd recommend running this on the trace severity to avoid the excessive info logs that some APIs output
            sev = log::severity_trace;
            break;
        case llri::validation_callback_severity::Warning:
            sev = log::severity_warn;
            break;
        case llri::validation_callback_severity::Error:
            sev = log::severity_error;
            break;
        case llri::validation_callback_severity::Corruption:
            sev = log::severity_error;
    }

    println(sev, "LLRI [{}]: {}", to_string(source), message);
}

void TestSystem::setup()
{
    filter(log::severity_debug);

    createInstance();
    selectAdapter();
    createDevice();

    //Cleanup created resources
    m_instance->destroyDevice(m_device);
    destroyInstance(m_instance);
}

void TestSystem::update(time::span deltaTime)
{
    (void)deltaTime;
}

void TestSystem::createInstance()
{
    //Select Instance Extensions
    std::vector<llri::instance_extension> instanceExtensions;
    if (queryInstanceExtensionSupport(llri::instance_extension_type::DriverValidation))
        instanceExtensions.emplace_back(llri::instance_extension_type::DriverValidation, llri::driver_validation_ext { true });
    if (queryInstanceExtensionSupport(llri::instance_extension_type::GPUValidation))
        instanceExtensions.emplace_back(llri::instance_extension_type::GPUValidation, llri::gpu_validation_ext { true });

    m_adapterNodesSupported = queryInstanceExtensionSupport(llri::instance_extension_type::AdapterNodes);
    if (m_adapterNodesSupported)
        instanceExtensions.emplace_back(llri::instance_extension_type::AdapterNodes, llri::adapter_nodes_ext { true });

    const llri::instance_desc instanceDesc{
        (uint32_t)instanceExtensions.size(), instanceExtensions.data(),
        "sandbox",
        llri::validation_callback_desc { &callback, nullptr }
    };

    //Create instance
    THROW_IF_FAILED(llri::createInstance(instanceDesc, &m_instance));
}

void TestSystem::selectAdapter()
{
    //Iterate over adapters
    std::vector<llri::Adapter*> adapters;
    THROW_IF_FAILED(m_instance->enumerateAdapters(&adapters));

    for (llri::Adapter* adapter : adapters)
    {
        //Log adapter info
        llri::adapter_info info;
        THROW_IF_FAILED(adapter->queryInfo(&info));

        log::info("Found adapter {}", info.adapterName);
        log::info("\tVendor ID: {}", info.vendorId);
        log::info("\tAdapter ID: {}", info.adapterId);
        log::info("\tAdapter Type: {}", to_string(info.adapterType));

        if (m_adapterNodesSupported)
        {
            uint8_t nodeCount;
            THROW_IF_FAILED(adapter->queryNodeCountEXT(&nodeCount));
            log::info("\tAdapter Nodes: {}", nodeCount);
        }

        uint8_t maxGraphicsQueueCount, maxComputeQueueCount, maxTransferQueueCount;
        THROW_IF_FAILED(adapter->queryQueueCount(llri::queue_type::Graphics, &maxGraphicsQueueCount));
        THROW_IF_FAILED(adapter->queryQueueCount(llri::queue_type::Compute, &maxComputeQueueCount));
        THROW_IF_FAILED(adapter->queryQueueCount(llri::queue_type::Transfer, &maxTransferQueueCount));

        log::info("\t Max number of queues: ");
        log::info("\t\t Graphics: {}", maxGraphicsQueueCount);
        log::info("\t\t Compute: {}", maxComputeQueueCount);
        log::info("\t\t Transfer: {}", maxTransferQueueCount);

        //Discrete adapters tend to be more powerful and have more resources so we can decide to pick them
        if (info.adapterType == llri::adapter_type::Discrete)
        {
            log::info("Adapter selected");
            m_adapter = adapter;
        }
    }

    if (m_adapter == nullptr)
        throw std::runtime_error("Failed to find a suitable LLRI adapter");
}

void TestSystem::createDevice()
{
    llri::adapter_features selectedFeatures {};

    std::vector<llri::adapter_extension> adapterExtensions;

    std::array<llri::queue_desc, 3> adapterQueues {
        llri::queue_desc { llri::queue_type::Graphics, llri::queue_priority::High }, //We can give one or more queues a higher priority
        llri::queue_desc { llri::queue_type::Compute, llri::queue_priority::Normal },
        llri::queue_desc { llri::queue_type::Transfer, llri::queue_priority::Normal }
    };

    //Create device
    const llri::device_desc deviceDesc{
        m_adapter, selectedFeatures,
        (uint32_t)adapterExtensions.size(), adapterExtensions.data(),
        (uint32_t)adapterQueues.size(), adapterQueues.data()
    };

    THROW_IF_FAILED(m_instance->createDevice(deviceDesc, &m_device));

    THROW_IF_FAILED(m_device->queryQueue(llri::queue_type::Graphics, 0, &m_graphicsQueue));
    THROW_IF_FAILED(m_device->queryQueue(llri::queue_type::Compute, 0, &m_computeQueue));
    THROW_IF_FAILED(m_device->queryQueue(llri::queue_type::Transfer, 0, &m_transferQueue));
}
