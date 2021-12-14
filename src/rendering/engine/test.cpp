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
    VkGlfwOutput windowOutput(window);

    VulkanEngine engine(true);

    engine.addOutput(&windowOutput);
    
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

    engine.removeOutput(&windowOutput);

    engine.shutdown();

    return EXIT_SUCCESS;
}