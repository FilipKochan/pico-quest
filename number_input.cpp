//
// Created by filda on 05.10.2024.
//

#include "number_input.hpp"


int number_input::read_value() const {
    int value = 0;

    for (uint i = start; i < start + length; ++i) {
        value <<= 1;
        value |= 1 - gpio_get(i);
    }

    return value;
}