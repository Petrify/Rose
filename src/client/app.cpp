/**
 * @file app.cpp
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2022-01-18
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "app.hpp"
#include <chrono>
#include <thread>

App::App() {
    initLogging();

    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Rose", nullptr, nullptr);

    auto instanceExt = VkGlfwOutput::getRequiredInstanceExtensions();

    renderEngine = std::make_unique<VulkanEngine>(instanceExt);
    windowOutput = std::make_unique<VkGlfwOutput>(window, *renderEngine);
    
    try
    {
        renderEngine->init();
    }
    catch (const std::exception &e)
    {
        auto logger = getLogger("Rose");
        logger->critical(e.what());
        return;
    }
}

App::~App() {
    windowOutput->destroy();
    renderEngine->destroy();
}

void App::run() {
    renderEngine->start();
    
    using namespace std::chrono_literals;

    auto lastFrame = std::chrono::system_clock::now();
    std::chrono::duration<double> frameTime = 1.0s / 65.0;
    float speed = 0.3f;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // std::this_thread::sleep_until(lastFrame + frameTime);
        // lastFrame = std::chrono::system_clock::now();
        
        if(glfwGetKey(window, GLFW_KEY_W)) {
            windowOutput->view.cameraPos += glm::vec3(0, 0, speed);
        } else if(glfwGetKey(window, GLFW_KEY_S)) {
            windowOutput->view.cameraPos += glm::vec3(0, 0, -speed);
        } else if(glfwGetKey(window, GLFW_KEY_A)) {
            windowOutput->view.cameraPos += glm::vec3(speed, 0, 0);
        } else if(glfwGetKey(window, GLFW_KEY_D)) {
            windowOutput->view.cameraPos += glm::vec3(-speed, 0, 0);
        } else if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT)) {
            windowOutput->view.cameraPos += glm::vec3(0, speed, 0);
        } else if(glfwGetKey(window, GLFW_KEY_SPACE)) {
            windowOutput->view.cameraPos += glm::vec3(0, -speed, 0);
        }
        
        windowOutput->beginImguiFrame();
        ImGui::ShowDemoWindow();

        ImGui::Begin("Rose!");
        ImGui::SliderAngle("FOV", &windowOutput->view.fov.x, 0, 180.0F);
        ImGui::SliderFloat("Speed", &speed, 0, 4.f);
        ImGui::SliderAngle("Rotation", &windowOutput->view.fov.y, -180.0F, 180.0F);
        ImGui::End(); 
        
        windowOutput->endImguiFrame();
        windowOutput->draw();
    }

    renderEngine->stop();
}

