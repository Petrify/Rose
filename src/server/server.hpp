/**
 * @file server.cpp
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2022-01-17
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include <string>
#include "simulation/simulation.hpp"

class RoseServer {
public:
    RoseServer();
    
    void loadScene(std::string);
    void saveScene(std::string);
    
    Simulation simulation;
};