#pragma once

#include <vk_mem_alloc.h>

struct AllocatedBuffer {
    VkBuffer buffer;
    VmaAllocation allocation;
};

class Initializable {
    public:
    virtual void init() = 0;
};

class Destroyable {
    public:
    virtual void destroy() = 0;
};