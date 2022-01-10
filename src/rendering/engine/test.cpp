#include "vk_engine.hpp"
#include "output/vk_output.hpp"
#include <chrono>
#include <thread>


int main() {
    initLogging();

    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    auto window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);

    VulkanEngine engine(true);
    VkGlfwOutput windowOutput(window, engine);
    
    try
    {
        engine.init();
    }
    catch (const std::exception &e)
    {
        auto logger = getLogger("Rose");
        logger->critical(e.what());
        return EXIT_FAILURE;
    }


    engine.start();

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }

    engine.stop();

    engine.shutdown();

    return EXIT_SUCCESS;
}