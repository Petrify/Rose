/**
 * @file app.hpp
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2022-01-18
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#pragma once

#include <RoseLogging.hpp>
#include <GLFW/glfw3.h>
#include <VulkanEngine.hpp>
#include <memory>

const uint32_t WIDTH = 1620;
const uint32_t HEIGHT = 900;

class App {
public:
    void run();
    App();
    ~App();

private:

    GLFWwindow* window;
    std::unique_ptr<VkGlfwOutput> windowOutput;
    std::unique_ptr<VulkanEngine> renderEngine;
    Logger logger;

    void mainLoop();
};