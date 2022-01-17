/**
 * @file world.hpp
 * @author your name (you@domain.com)
 * @brief Rules determine the 
 * @version 0.1
 * @date 2022-01-16
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once
#include <vector>               
#include <memory>
#include <string>

#include "rule.hpp"
#include "actor.hpp"

class WorldComponent {
    std::string type;
};

class World {
public:
    virtual void step();
    std::vector<std::shared_ptr<WorldComponent>> components;
    std::vector<std::shared_ptr<Rule>> rules;
    std::vector<std::shared_ptr<Actor>> actors;
};