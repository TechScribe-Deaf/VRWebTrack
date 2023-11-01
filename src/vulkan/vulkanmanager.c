#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include <vulkan/vulkan_core.h>

void InitializeVulkanInstance(ComputeApplication this)
{
    const char* layers[] = { "VK_LAYER_KHRONOS_validation" };
    const char* extensions[] = {"VK_EXT_debug_report", "VK_EXT_debug_utils"};
    VkApplicationInfo applicationInfo = (VkApplicationInfo){
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Example1",
        .applicationVersion = 0,
        .pEngineName = "VulkaNN",
        .engineVersion = 0,
        .apiVersion = VK_API_VERSION_1_3
    };
    VkInstanceCreateInfo createInfo = (VkInstanceCreateInfo){
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .flags = 0,
        .pApplicationInfo = &applicationInfo,
        .enabledLayerCount = 1,
        .ppEnabledLayerNames = layers,
        .enabledExtensionCount = 2,
        .ppEnabledExtensionNames = extensions
    };
    VK_CHECK_RESULT(vkCreateInstance(&createInfo, NULL, &this->instance));
}

void SelectPhysicalDevice(ComputeApplication this)
{
    uint32_t deviceCount;
    VK_CHECK_RESULT(vkEnumeratePhysicalDevices(this->instance, &deviceCount, NULL));
    if (deviceCount == 0)
    {
        printf("could not find a device with vulkan support\n");
        return;
    }

    VkPhysicalDevice devices[deviceCount];
    VK_CHECK_RESULT(vkEnumeratePhysicalDevices(this->instance, &deviceCount, devices));
    this->physicalDevice = devices[0];
}

uint32_t getComputeQueueFamilyIndex(ComputeApplication this)
{
    uint32_t queueFamilyCount;

    vkGetPhysicalDeviceQueueFamilyProperties(this->physicalDevice, &queueFamilyCount, NULL);

    VkQueueFamilyProperties queueFamilies[queueFamilyCount];
    vkGetPhysicalDeviceQueueFamilyProperties(this->physicalDevice, &queueFamilyCount, queueFamilies);

    uint32_t i = 0;
    for (; i < queueFamilyCount; ++i)
    {
        VkQueueFamilyProperties props = queueFamilies[i];

        if (props.queueCount > 0 && (props.queueFlags & VK_QUEUE_COMPUTE_BIT))
        {
            break;
        }
    }

    if (i == queueFamilyCount)
    {
        printf("could not find a queue family that supports operations\n");
        exit(1);
    }

    return i;
}

void InitializeVulkanDevice(ComputeApplication this)
{
    this->queueFamilyIndex = getComputeQueueFamilyIndex(this);
    float queuePriorities = 1.0;
    VkDeviceQueueCreateInfo queueCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = this->queueFamilyIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriorities
    };
    VkPhysicalDeviceFeatures deviceFeatures = {0};
    VkDeviceCreateInfo deviceCreateInfo = (VkDeviceCreateInfo){
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL,
        .pQueueCreateInfos = &queueCreateInfo,
        .queueCreateInfoCount = 1,
        .pEnabledFeatures = &deviceFeatures
    };

    VK_CHECK_RESULT(vkCreateDevice(this->physicalDevice, &deviceCreateInfo, NULL, &this->device));
    vkGetDeviceQueue(this->device, this->queueFamilyIndex, 0, &this->queue);
}

uint32_t RetrieveMemoryType(ComputeApplication this, uint32_t memoryTypeBits, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memoryProperties;

    vkGetPhysicalDeviceMemoryProperties(this->physicalDevice, &memoryProperties);

    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
    {
        if ((memoryTypeBits & (1 << i)) &&
            ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties))
            return i;
    }
    printf("Memory type not found!\n");
    return -1;
}

void CleanUpVulkan(ComputeApplication this)
{
    vkDestroyDevice(this->device, NULL);
    vkDestroyInstance(this->instance, NULL);
}

ComputeApplication initializeComputeApplication()
{
    ComputeApplication this = (ComputeApplication)calloc(sizeof(struct ComputeApplication), 1);
    InitializeVulkanInstance(this);
    SelectPhysicalDevice(this);
    InitializeVulkanDevice(this);
    return this;
}

VkDescriptorPool CreatePoolForDescriptors(ComputeApplication this, size_t numOfDescriptorSets, size_t numBufferInfos, Buffer bufferInfos[numBufferInfos])
{
    if (numBufferInfos <= 0 || bufferInfos == NULL || this == NULL || numOfDescriptorSets <= 0)
        return NULL;
    uint32_t storageBufferCount = 0, uniformBufferCount = 0, dynamicStorageBufferCount = 0, dynamicUniformBufferCount = 0;
    
    for (size_t i = 0; i < numBufferInfos; ++i)
    {
        if (bufferInfos[i]->typeOfBuffer == ReadOnlyBufferType)
            ++uniformBufferCount;
        if (bufferInfos[i]->typeOfBuffer == ReadOnlyDynamicBufferType)
            ++dynamicUniformBufferCount;
        if (bufferInfos[i]->typeOfBuffer == ReadAndWriteBufferType)
            ++storageBufferCount;
        if (bufferInfos[i]->typeOfBuffer == ReadAndWriteDynamicBufferType)
            ++dynamicStorageBufferCount;
    }
    
    struct VkDescriptorPoolSize descriptorPoolSizes[4];
    uint32_t index = 0;
    if (uniformBufferCount > 0)
    {
        descriptorPoolSizes[index].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorPoolSizes[index].descriptorCount = uniformBufferCount;
        ++index;
    }
    if (dynamicUniformBufferCount > 0)
    {
        descriptorPoolSizes[index].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        descriptorPoolSizes[index].descriptorCount = dynamicUniformBufferCount;
        ++index;
    }
    if (storageBufferCount > 0)
    {
        descriptorPoolSizes[index].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorPoolSizes[index].descriptorCount = storageBufferCount;
        ++index;
    }
    if (dynamicStorageBufferCount > 0)
    {
        descriptorPoolSizes[index].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
        descriptorPoolSizes[index].descriptorCount = dynamicStorageBufferCount;
        ++index;
    }
    VkDescriptorPoolCreateInfo poolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets = numOfDescriptorSets,
        .pNext = NULL,
        .poolSizeCount = index,
        .pPoolSizes = descriptorPoolSizes
    };
    VkDescriptorPool pool = {0};
    VK_CHECK_RESULT(vkCreateDescriptorPool(this->device, &poolCreateInfo, NULL, &pool));
    return pool;
}

DescriptorSetForBuffers CreateDescriptorsForBuffers(ComputeApplication this, VkDescriptorPool pool, size_t numBufferInfos, Buffer bufferInfos[numBufferInfos])
{
    if (numBufferInfos <= 0 || bufferInfos == NULL || this == NULL)
        return NULL;
    VkDescriptorSetLayoutBinding bindings[numBufferInfos];
    for (size_t i = 0; i < numBufferInfos; ++i)
    {
        if (bufferInfos[i]->typeOfBuffer == ReadOnlyBufferType)
            bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        else if (bufferInfos[i]->typeOfBuffer == ReadOnlyDynamicBufferType)
            bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        else if (bufferInfos[i]->typeOfBuffer == ReadAndWriteBufferType)
            bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        else if (bufferInfos[i]->typeOfBuffer == ReadAndWriteDynamicBufferType)
            bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        else
            return NULL; // Invalid buffer
        bindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        bindings[i].binding = bufferInfos[i]->binding;
        bindings[i].descriptorCount = 1;
        bindings[i].pImmutableSamplers = NULL;
    }
    DescriptorSetForBuffers output = (DescriptorSetForBuffers)calloc(sizeof(struct DescriptorSetForBuffers), 1);
    output->numBuffersAndDescriptorSets = numBufferInfos;
    output->buffers = (Buffer*)calloc(sizeof(Buffer), numBufferInfos);
    memcpy(output->buffers, bufferInfos, sizeof(Buffer)*numBufferInfos);
    
    VkDescriptorSetLayoutCreateInfo descriptorSetLayout = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pBindings = bindings,
        .flags = 0,
        .pNext = NULL,
        .bindingCount = numBufferInfos
    };
    
    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(this->device, &descriptorSetLayout, NULL, &output->layout));
    
    VkDescriptorSetAllocateInfo allocateInfo = {};
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorPool = pool;
    allocateInfo.descriptorSetCount = 1;
    allocateInfo.pSetLayouts = &output->layout;
    vkAllocateDescriptorSets(this->device, &allocateInfo, &output->descriptorSet);
    
    for (size_t i = 0; i < numBufferInfos; ++i)
    {
        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = bufferInfos[i]->buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = bufferInfos[i]->size;
        VkWriteDescriptorSet descriptorWrite = {};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = output->descriptorSet;
        descriptorWrite.dstBinding = bindings[i].binding;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = bindings[i].descriptorType;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;
        descriptorWrite.pImageInfo = NULL;
        descriptorWrite.pTexelBufferView = NULL;
        vkUpdateDescriptorSets(this->device, 1, &descriptorWrite, 0, NULL);
    }
    return output;
}

Buffer CreateBuffer(ComputeApplication this, const char* name, enum ComputeBufferType typeOfBuffer, size_t size, uint64_t binding)
{
    if (size <= 0 || this == NULL)
        return NULL;
    enum VkBufferUsageFlagBits usageFlag = {0};
    if (typeOfBuffer == ReadOnlyBufferType)
        usageFlag = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    else if (typeOfBuffer == ReadOnlyDynamicBufferType)
        usageFlag = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    else if (typeOfBuffer == ReadAndWriteBufferType)
        usageFlag = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    else if (typeOfBuffer == ReadAndWriteDynamicBufferType)
        usageFlag = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    else
        return NULL; // Invalid buffer
    Buffer buffer = (Buffer)calloc(sizeof(struct Buffer), 1);
    buffer->name = name;
    buffer->typeOfBuffer = typeOfBuffer;
    buffer->size = size;
    buffer->binding = binding;
    
    uint32_t queueIdx = getComputeQueueFamilyIndex(this);
    VkBufferCreateInfo bufferCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .usage = usageFlag,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .pQueueFamilyIndices = &queueIdx,
        .queueFamilyIndexCount = 1,
        .size = size
    };
    VK_CHECK_RESULT(vkCreateBuffer(this->device, &bufferCreateInfo, NULL, &buffer->buffer));
    
    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(this->device, buffer->buffer, &memoryRequirements);
    VkMemoryAllocateInfo allocateInfo = (VkMemoryAllocateInfo){
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memoryRequirements.size,
        .pNext = NULL,
        .memoryTypeIndex = RetrieveMemoryType(this, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
    };

    VK_CHECK_RESULT(vkAllocateMemory(this->device, &allocateInfo, NULL, &buffer->memory));
    VK_CHECK_RESULT(vkBindBufferMemory(this->device, buffer->buffer, buffer->memory, 0));
    
    return buffer;
}

VkShaderModule LoadShader(ComputeApplication this, void* shaderCode, size_t sharderCodeSize)
{
    if (shaderCode == NULL || sharderCodeSize <= 0)
        return NULL;
    VkShaderModuleCreateInfo shaderInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pCode = (const uint32_t*) shaderCode,
        .codeSize = sharderCodeSize,
        .flags = 0,
        .pNext = NULL
    };
    VkShaderModule shader = {0};
    VK_CHECK_RESULT(vkCreateShaderModule(this->device, &shaderInfo, NULL, &shader));
    return shader;
}

ComputePipeline CreatePipeline(ComputeApplication this, DescriptorSetForBuffers descSetForBuffs, VkShaderModule shaderModule, const char* mainShaderFunction)
{
    if (mainShaderFunction == NULL)
        mainShaderFunction = "main";
    VkPipelineLayoutCreateInfo pipelineCreate = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .pSetLayouts = &descSetForBuffs->layout,
        .setLayoutCount = 1,
        .flags = 0,
        .pPushConstantRanges = NULL,
        .pushConstantRangeCount = 0,
        
    };
    ComputePipeline pipeline = (ComputePipeline)calloc(sizeof(struct ComputePipeline), 1);
    pipeline->descriptorSetLayout = descSetForBuffs->layout;
    pipeline->shaderModule = shaderModule;
    VK_CHECK_RESULT(vkCreatePipelineLayout(this->device, &pipelineCreate, NULL, &pipeline->pipelineLayout));
    VkPipelineShaderStageCreateInfo shaderStage = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = shaderModule,
        .pName = mainShaderFunction,
        .flags = 0,
        .pNext = NULL,
        .pSpecializationInfo = NULL
    };
    VkComputePipelineCreateInfo computeCreate = {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage = shaderStage,
        .layout = pipeline->pipelineLayout
    };
    VK_CHECK_RESULT(vkCreateComputePipelines(this->device, VK_NULL_HANDLE, 1, &computeCreate, NULL, &pipeline->computePipeline));
    return pipeline;
}

CommandBuffer CreateCommandBuffer(ComputeApplication this)
{
    CommandBuffer cmdbuf = (CommandBuffer)calloc(sizeof(struct CommandBuffer), 1);
    VkCommandPoolCreateInfo commandPoolCreateInfo = (VkCommandPoolCreateInfo){
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = 0,
        .queueFamilyIndex = this->queueFamilyIndex
    };
    VK_CHECK_RESULT(vkCreateCommandPool(this->device, &commandPoolCreateInfo, NULL, &cmdbuf->pool));
    
    VkCommandBufferAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = cmdbuf->pool,
        .pNext = NULL,
        .commandBufferCount = 1,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY
    };
    VK_CHECK_RESULT(vkAllocateCommandBuffers(this->device, &allocInfo, &cmdbuf->cmdbuffer));
    return cmdbuf;
}

void BeginCommand(CommandBuffer cmdbuf)
{
    VkCommandBufferBeginInfo beginInfo = (VkCommandBufferBeginInfo){
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = 0,
        .pNext = NULL,
        .pInheritanceInfo = NULL
    };
    VK_CHECK_RESULT(vkBeginCommandBuffer(cmdbuf->cmdbuffer, &beginInfo));
}

void EndCommand(CommandBuffer cmdbuf)
{
    VK_CHECK_RESULT(vkEndCommandBuffer(cmdbuf->cmdbuffer));
}

void AddDispatchComputeShaderToCommandBufferQueue(CommandBuffer cmdbuf, ComputePipeline pipeline, uint64_t workgroupSize, DescriptorSetForBuffers descSetForBuffs)
{
    vkCmdBindPipeline(cmdbuf->cmdbuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->computePipeline);
    vkCmdBindDescriptorSets(cmdbuf->cmdbuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->pipelineLayout, 0, 1, &descSetForBuffs->descriptorSet, 0, NULL);
    vkCmdDispatch(cmdbuf->cmdbuffer, workgroupSize, 1, 1);
}

void CopyDataToBuffer(ComputeApplication this, Buffer dst, size_t dataLen, void* src)
{
    void *mappedMemory = NULL;
    VK_CHECK_RESULT(vkMapMemory(this->device, dst->memory, 0, VK_WHOLE_SIZE, 0, &mappedMemory));
    memcpy(mappedMemory, src, dataLen);
    vkUnmapMemory(this->device, dst->memory);
}

void CopyBufferToData(ComputeApplication this, Buffer src, size_t dataLen, void* dst)
{
    void *mappedMemory = NULL;
    VK_CHECK_RESULT(vkMapMemory(this->device, src->memory, 0, VK_WHOLE_SIZE, 0, &mappedMemory));
    memcpy(dst, mappedMemory, dataLen);
    vkUnmapMemory(this->device, src->memory);
}

void ExecuteCommandBufferSync(ComputeApplication this, CommandBuffer cmdbuf)
{
    VkFence fence;
    VkFenceCreateInfo fenceCreateInfo = (VkFenceCreateInfo){
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = 0,
        .pNext = NULL
    };
    VkSubmitInfo cmdSubmitInfo = (VkSubmitInfo){
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmdbuf->cmdbuffer
    };
    
    VK_CHECK_RESULT(vkCreateFence(this->device, &fenceCreateInfo, NULL, &fence));
    VK_CHECK_RESULT(vkQueueSubmit(this->queue, 1, &cmdSubmitInfo, fence));
    VK_CHECK_RESULT(vkWaitForFences(this->device, 1, &fence, VK_TRUE, 100000000000));
    vkDestroyFence(this->device, fence, NULL);
}
