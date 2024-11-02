//
// Created by filda on 05.10.2024.
//

#ifndef QUEST_NUMBER_INPUT_HPP
#define QUEST_NUMBER_INPUT_HPP

#include "pico/stdlib.h"

struct number_input {
    uint start;
    uint length;
    int read_value() const;
};


#endif //QUEST_NUMBER_INPUT_HPP
