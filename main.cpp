#include "pico/stdlib.h"
#include "pico/time.h"
#include <cstdio>

#include "number_input.hpp"
#include "lcd_control.hpp"

const uint button_pin = 15;
const uint to_left_pin = 11;
const uint to_right_pin = 12;
const uint accept_char_pin = 10;
const number_input number = {3, 6};
const uint red_led_pin = 28;
const uint green_led_pin = 22;
const uint notify_binary_input_led = 12;
const uint notify_text_input_led = 14;

lcd_display* display;


void setup_button(uint pin);
void setup_number_input() {
    for (uint i = number.start; i < number.start + number.length; ++i) {
        setup_button(i);
    }
}

void setup_button(uint pin) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
    gpio_pull_up(pin);
}

void setup_led(uint pin) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
}

void setup_pins() {
    setup_button(button_pin);
    setup_button(to_left_pin);
    setup_button(to_right_pin);
    setup_button(accept_char_pin);

    setup_number_input();
    setup_led(red_led_pin);
    setup_led(green_led_pin);
}

void blink_led(uint pin) {
    for (int i = 0; i < 9; i++) {
        gpio_put(pin, i % 2);
        sleep_ms(100);
    }
}

bool handle_button_press(int target_value) {
    if (number.read_value() != target_value) {
        blink_led(red_led_pin);
        return false;
    }
    blink_led(green_led_pin);
    return true;
}

void handle_text_question(const char* question, const char* answer) {
    gpio_put(notify_text_input_led, true);
    gpio_put(notify_binary_input_led, false);
    int max_text_len = 2;
    char* current_text = (char*) calloc(max_text_len, sizeof(char));
    int current_text_len = 0;

    char left = 'a';
    char right = 'z';

    display->write_text(question);
    display->set_cursor_position(3, 0);
    display->write_from_current_position("[m] ");
    display->blink_cursor(true);

    bool was_action = false;
    while (true) {
        uint32_t values = gpio_get_all();
        bool prev_action = was_action;
        char current_midpoint = (left + right) / 2;
        if (((values >> to_left_pin) & 1u) == 0) {
            if (!was_action) {
                right = current_midpoint;
            }
            was_action = true;
        } else if (((values >> to_right_pin) & 1u) == 0) {
            if (!was_action) {
                left = current_midpoint;
            }
            was_action = true;
        } else if (((values >> accept_char_pin) & 1u) == 0) {
            if (!was_action) {
                if (current_text_len >= max_text_len) {
                    max_text_len *= 2;
                    char* new_text = (char *) realloc(current_text, max_text_len);
                    if (new_text == nullptr) {
                        free(current_text);
                        display->write_text("neocekavana chyba alokace");
                        display->blink_cursor(false);
                        sleep_ms(5000);
                        return;
                    } else {
                        current_text = new_text;
                    }
                }
                current_text[current_text_len++] = current_midpoint;
                left = 'a';
                right = 'z';
            }
            was_action = true;
        } else if (((values >> button_pin) & 1u) == 0) {
            if (!was_action) {
                if (strcmp(current_text, answer) == 0) {
                    // ok
                    free(current_text);
                    blink_led(green_led_pin);
                    display->blink_cursor(false);
                    return;
                }
                blink_led(red_led_pin);
                left = 'a';
                right = 'z';
                display->write_text(question);
                display->set_cursor_position(3, 0);
                display->write_from_current_position("[m] ");
                memset(current_text, 0, strlen(answer) + 1);
                current_text_len = 0;
            }
            was_action = true;
        } else {
            was_action = false;
        }

        if (!prev_action && was_action) {
            display->set_cursor_position(3, 1);
            char midpoint[2];
            midpoint[0] = (left + right) / 2;
            midpoint[1] = 0;
            display->write_from_current_position(midpoint);
            display->set_cursor_position(3, 4);

            if (strlen(current_text) > 0) {
                display->write_from_current_position(current_text);
            }
            // debounce
            sleep_ms(300);
        }

        sleep_ms(100);
    }
}

void handle_question(const char* question, int answer) {
    gpio_put(notify_text_input_led, false);
    gpio_put(notify_binary_input_led, true);
    display->write_text(question);
    int button_value = 0;
    while (true) {
        int current_button_value = gpio_get(button_pin);
        if (button_value == 1 && current_button_value == 0) {
            bool correct_answer = handle_button_press(answer);
            sleep_ms(300); // debounce
            if (correct_answer) {
                return;
            }
        }
        button_value = current_button_value;

        sleep_ms(100);
    }
}

int main() {
    setup_pins();
    // wait a bit before initing the display
    sleep_ms(1000);
    lcd_display _d(26, 27, 1);
    display = &_d;

    display->write_line_centered(1, "A h o j !");
    sleep_ms(2000);

    handle_question("Kolik je (1<<5)|(3<<2)?", 44);
    handle_text_question("Co je opakem syra?", "sunka");
    handle_question("Kolik serii ma The Office?", 9);
    handle_text_question("Ucitel statistiky, zaroven se prodava v cukrarne...", "kolacek");
    handle_text_question("Am I a hero? I really cant say, but yes. rekl (prijmeni)", "scott");
    handle_text_question("O ktere divce z Honolulu zpiva tvuj oblibeny Banjo Band?", "lulu");

    display->clear();
    display->write_line_centered(0, "najdes me");
    display->write_line_centered(1, "ve zmrzline");
    display->write_line_centered(2, "v bufferu");
}
