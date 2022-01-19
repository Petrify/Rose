#include "VulkanEngine.hpp"
#include <chrono>
#include <thread>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

int main() {
    initLogging();

    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    auto window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);

    auto instanceExt = VkGlfwOutput::getRequiredInstanceExtensions();

    VulkanEngine engine(instanceExt);
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
        windowOutput.draw();
        glfwPollEvents();
    }

    engine.stop();

    windowOutput.destroy();

    engine.destroy();

    return EXIT_SUCCESS;
}