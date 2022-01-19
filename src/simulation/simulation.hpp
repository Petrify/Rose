/**
 * @file simulation.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2022-01-17
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "world.hpp"

class Simulation {

    Simulation(float speed = 1.0f) : speed(speed) {};

    World world;
    float speed;

    void step();

    void start();
    void stop();
};