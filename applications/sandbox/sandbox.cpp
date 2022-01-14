/**
 * @file sandbox.cpp
 * @copyright 2021-2021 Leon Brands. All rights served.
 * @license: https://github.com/Legion-Engine/Legion-LLRI/blob/main/LICENSE
 */

#include <cstdio>
#include <cassert>
#include <llri/llri.hpp>

/**
 * Sandbox is a testing area for LLRI development.
 * The code written in sandbox should be up to spec but may not contain the best practices or cleanest examples.
 *
 * See the samples for recommended usage and more detailed comments.
 */

#define THROW_IF_FAILED(operation) { \
    auto r = operation; \
    if (r != llri::result::Success) \
    { \
        printf("LLRI Operation { %s } returned: { %s }", #operation, llri::to_string(r).c_str()); \
        throw std::runtime_error("LLRI Operation failed"); \
    } \
} \

void callback(llri::message_severity severity, llri::message_source source, const char* message, void* userData)
{
    switch (severity)
    {
        case llri::message_severity::Verbose:
            return;
        case llri::message_severity::Info:
            // Even though this semantically maps to info, we'd recommend running this on the trace severity to avoid the excessive info logs that some APIs output
            return;
        case llri::message_severity::Warning:
            printf("Warning: ");
            break;
        case llri::message_severity::Error:
            printf("Error: ");
            break;
        case llri::message_severity::Corruption:
            printf("Corruption error: ");
    }

    printf("LLRI [%s]: %s\n", to_string(source).c_str(), message);
}

llri::Instance* m_instance = nullptr;
llri::Adapter* m_adapter = nullptr;
llri::Device* m_device = nullptr;

llri::Queue* m_graphicsQueue = nullptr;

llri::CommandGroup* m_commandGroup = nullptr;
llri::CommandList* m_commandList = nullptr;

llri::Fence* m_fence = nullptr;
llri::Semaphore* m_semaphore = nullptr;

llri::Resource* m_buffer = nullptr;
llri::Resource* m_texture = nullptr;

void createInstance();
void selectAdapter();
void createDevice();
void createCommandLists();
void createSynchronization();
void createResources();

int main()
{
    printf("LLRI linked Implementation: %s", llri::to_string(llri::getImplementation()).c_str());

    llri::setMessageCallback(&callback);

    createInstance();
    selectAdapter();
    createDevice();
    createCommandLists();
    createSynchronization();
    createResources();
    
    while (true)
    {
        // wait for our frame to be ready
        m_device->waitFence(m_fence, LLRI_TIMEOUT_MAX);

        // record command list
        THROW_IF_FAILED(m_commandGroup->reset());
        const llri::command_list_begin_desc beginDesc {};
        THROW_IF_FAILED(m_commandList->record(beginDesc, [](llri::CommandList* cmd)
        {
        }, m_commandList));

        // submit
        const llri::submit_desc submitDesc { 0, 1, &m_commandList, 0, nullptr, 0, nullptr, m_fence };
        THROW_IF_FAILED(m_graphicsQueue->submit(submitDesc));
    }
    
    m_device->destroyResource(m_buffer);
    m_device->destroyResource(m_texture);

    m_device->destroySemaphore(m_semaphore);
    m_device->destroyFence(m_fence);
    m_device->destroyCommandGroup(m_commandGroup);

    m_instance->destroyDevice(m_device);
    destroyInstance(m_instance);
}

void createInstance()
{
    // Select Instance Extensions
    std::vector<llri::instance_extension> instanceExtensions;
    if (queryInstanceExtensionSupport(llri::instance_extension::DriverValidation))
        instanceExtensions.emplace_back(llri::instance_extension::DriverValidation);
    if (queryInstanceExtensionSupport(llri::instance_extension::GPUValidation))
        instanceExtensions.emplace_back(llri::instance_extension::GPUValidation);

    const llri::instance_desc instanceDesc{ static_cast<uint32_t>(instanceExtensions.size()), instanceExtensions.data(), "sandbox" };

    // Create instance
    THROW_IF_FAILED(llri::createInstance(instanceDesc, &m_instance));
}

void selectAdapter()
{
    // Iterate over adapters
    std::vector<llri::Adapter*> adapters;
    THROW_IF_FAILED(m_instance->enumerateAdapters(&adapters));
    assert(!adapters.empty());

    std::unordered_map<int, llri::Adapter*> sortedAdapters;
    for (llri::Adapter* adapter : adapters)
    {
        // Log adapter info
        llri::adapter_info info = adapter->queryInfo();

        printf("Found adapter %s", info.adapterName.c_str());
        printf("\tVendor ID: %u", info.vendorId);
        printf("\tAdapter ID: %u", info.adapterId);
        printf("\tAdapter Type: %s", to_string(info.adapterType).c_str());

        uint8_t nodeCount = adapter->queryNodeCount();
        printf("\tAdapter Nodes: %u", nodeCount);

        uint8_t maxGraphicsQueueCount = adapter->queryQueueCount(llri::queue_type::Graphics);
        uint8_t maxComputeQueueCount = adapter->queryQueueCount(llri::queue_type::Compute);
        uint8_t maxTransferQueueCount = adapter->queryQueueCount(llri::queue_type::Transfer);

        printf("\t Max number of queues: ");
        printf("\t\t Graphics: %u", maxGraphicsQueueCount);
        printf("\t\t Compute: %u", maxComputeQueueCount);
        printf("\t\t Transfer: %u", maxTransferQueueCount);

        uint32_t score = 0;

        // Discrete adapters tend to be more powerful and have more resources so we can decide to pick them
        if (info.adapterType == llri::adapter_type::Discrete)
            score += 1000;

        sortedAdapters[score] = adapter;
    }

    m_adapter = sortedAdapters.begin()->second;
}

void createDevice()
{
    llri::adapter_features selectedFeatures {};

    std::vector<llri::adapter_extension> adapterExtensions;

    std::array<llri::queue_desc, 1> adapterQueues {
        llri::queue_desc { llri::queue_type::Graphics, llri::queue_priority::High } // We can give one or more queues a higher priority
    };

    // Create device
    const llri::device_desc deviceDesc{
        m_adapter, selectedFeatures,
        static_cast<uint32_t>(adapterExtensions.size()), adapterExtensions.data(),
        static_cast<uint32_t>(adapterQueues.size()), adapterQueues.data()
    };

    THROW_IF_FAILED(m_instance->createDevice(deviceDesc, &m_device));

    m_graphicsQueue = m_device->getQueue(llri::queue_type::Graphics, 0);
}

void createCommandLists()
{
    THROW_IF_FAILED(m_device->createCommandGroup(llri::queue_type::Graphics, &m_commandGroup));

    const llri::command_list_alloc_desc listDesc { 0, llri::command_list_usage::Direct };
    THROW_IF_FAILED(m_commandGroup->allocate(listDesc, &m_commandList));
}

void createSynchronization()
{
    THROW_IF_FAILED(m_device->createFence(llri::fence_flag_bits::Signaled, &m_fence));
    THROW_IF_FAILED(m_device->createSemaphore(&m_semaphore));
}

void createResources()
{
    llri::resource_desc bufferDesc = llri::resource_desc::buffer(llri::resource_usage_flag_bits::ShaderWrite, llri::memory_type::Local, llri::resource_state::ShaderReadWrite, 64);

    THROW_IF_FAILED(m_device->createResource(bufferDesc, &m_buffer));

    llri::resource_desc textureDesc;
    textureDesc.createNodeMask = 0;
    textureDesc.visibleNodeMask = 0;
    textureDesc.type = llri::resource_type::Texture2D;
    textureDesc.usage = llri::resource_usage_flag_bits::TransferDst | llri::resource_usage_flag_bits::Sampled;
    textureDesc.memoryType = llri::memory_type::Local;
    textureDesc.initialState = llri::resource_state::TransferDst;
    textureDesc.width = 1028;
    textureDesc.height = 1028;
    textureDesc.depthOrArrayLayers = 1;
    textureDesc.mipLevels = 1;
    textureDesc.sampleCount = llri::sample_count::Count1;
    textureDesc.textureFormat = llri::format::RGBA8sRGB;

    THROW_IF_FAILED(m_device->createResource(textureDesc, &m_texture));
}
