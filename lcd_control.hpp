//
// Created by filda on 28.10.2024.
//

#ifndef QUEST_LCD_CONTROL_HPP
#define QUEST_LCD_CONTROL_HPP

#include <pico/binary_info.h>
#include <hardware/i2c.h>
#include <pico/stdlib.h>
#include <cstring>
#include <algorithm>

/* Example code to drive a 16x2 LCD panel via a I2C bridge chip (e.g. PCF8574)

   NOTE: The panel must be capable of being driven at 3.3v NOT 5v. The Pico
   GPIO (and therefore I2C) cannot be used at 5v.

   You will need to use a level shifter on the I2C lines if you want to run the
   board at 5v.

   Connections on Raspberry Pi Pico board, other boards may vary.

   GPIO 4 (pin 6)-> SDA on LCD bridge board
   GPIO 5 (pin 7)-> SCL on LCD bridge board
   3.3v (pin 36) -> VCC on LCD bridge board
   GND (pin 38)  -> GND on LCD bridge board
*/
// commands
const int LCD_CLEARDISPLAY = 0x01;
const int LCD_RETURNHOME = 0x02;
const int LCD_ENTRYMODESET = 0x04;
const int LCD_DISPLAYCONTROL = 0x08;
const int LCD_CURSORSHIFT = 0x10;
const int LCD_FUNCTIONSET = 0x20;
const int LCD_SETCGRAMADDR = 0x40;
const int LCD_SETDDRAMADDR = 0x80;

// flags for display entry mode
const int LCD_ENTRYSHIFTINCREMENT = 0x01;
const int LCD_ENTRYLEFT = 0x02;

// flags for display and cursor control
const int LCD_BLINKON = 0x01;
const int LCD_CURSORON = 0x02;
const int LCD_DISPLAYON = 0x04;

// flags for display and cursor shift
const int LCD_MOVERIGHT = 0x04;
const int LCD_DISPLAYMOVE = 0x08;

// flag for backlight control
const int LCD_BACKLIGHT = 0x08;

const int LCD_ENABLE_BIT = 0x04;

// By default these LCD display drivers are on bus address 0x27
static int addr = 0x27;

// Modes for lcd_send_byte
#define LCD_CHARACTER  1
#define LCD_COMMAND    0

struct lcd_display {
private:
    i2c_inst* inst;

public:
    const uint8_t display_width = 20;
    const uint8_t n_lines = 4;

    lcd_display(int sda_pin, int scl_pin, int i2c_instance): inst(i2c_get_instance(i2c_instance)) {
        i2c_init(inst, 100 * 1000);
        gpio_set_function(sda_pin, GPIO_FUNC_I2C);
        gpio_set_function(scl_pin, GPIO_FUNC_I2C);
        gpio_pull_up(sda_pin);
        gpio_pull_up(scl_pin);

        lcd_send_byte(0x03, LCD_COMMAND);
        lcd_send_byte(0x03, LCD_COMMAND);
        lcd_send_byte(0x03, LCD_COMMAND);
        lcd_send_byte(0x02, LCD_COMMAND);

        lcd_send_byte(LCD_ENTRYMODESET | LCD_ENTRYLEFT, LCD_COMMAND);
        lcd_send_byte(LCD_DISPLAYCONTROL | LCD_DISPLAYON, LCD_COMMAND);
        clear();
    };


    /**
     * write text to display from current position of the cursor
     * undefined behavior when length is > display width
     * @param text
     */
    void write_from_current_position(const char* text) {
        lcd_string(text);
    }

    /**
     * write to center of line, does not clear display
     * @param line nth line
     * @param text data to write, undefined behavior when length is > display width
     */
    void write_line_centered(uint8_t line, const char* text) {
        size_t text_len = std::min(strlen(text), (size_t)display_width);
        set_cursor_position(line, ((int)display_width -(int)text_len) / 2);
        write_from_current_position(text);
    }

    void blink_cursor(bool blink) {
        uint8_t value = LCD_DISPLAYCONTROL | LCD_DISPLAYON;
        if (blink) {
            value |= (LCD_BLINKON | LCD_CURSORON);
        }
        lcd_send_byte(value, LCD_COMMAND);
    }
    /**
     * clear the display and write all text. text is wrapped on space automatically,
     * but must fit the display.
     * @param text text to write
     */
    void write_text(const char* text) {
        clear();
        set_cursor_position(0, 0);
        char* text_copy = (char*) calloc(sizeof(char), strlen(text) + 1);
        strcpy(text_copy, text);

        size_t current_line_size = 0;
        int current_line = 0;
        for(char* word = strtok(text_copy, " "); word != nullptr && current_line < n_lines; word = strtok(nullptr, " ")) {
            size_t word_len = strlen(word);
            while(word_len > display_width) {
                size_t available_len = display_width - current_line_size;
                char buf[available_len + 1];
                strncpy(buf, word, available_len);
                buf[available_len] = 0;
                word += available_len;
                word_len -= available_len;
                current_line_size = 0;
                write_from_current_position(buf);
                set_cursor_position(++current_line, 0);
                if (current_line >= n_lines) {
                    break;
                }
            }

            if (current_line_size + word_len > display_width) {
                set_cursor_position(++current_line, 0);
                if (current_line >= n_lines) {
                    break;
                }
                current_line_size = 0;
            }

            current_line_size += word_len;
            write_from_current_position(word);

            if (current_line_size != display_width) {
                ++current_line_size;
                lcd_char(' ');
            }
        }
    }

    void set_cursor_position(uint8_t line, uint8_t position) {
        int val;
        switch (line) {
            case 0:
                val = 0x80;
                break;
            case 1:
                val = 0xC0;
                break;
            case 2:
                val = 0x94;
                break;
            default:
                val = 0xD4;
                break;
        }
        lcd_send_byte(val + position, LCD_COMMAND);
    }

    void clear() {
        lcd_send_byte(LCD_CLEARDISPLAY, LCD_COMMAND);
    }

private:
    /* Quick helper function for single byte transfers */
    void i2c_write_byte(uint8_t val) {
        i2c_write_blocking(inst, addr, &val, 1, false);
    }

    void lcd_toggle_enable(uint8_t val) {
        // Toggle enable pin on LCD display
        // We cannot do this too quickly or things don't work
#define DELAY_US 600
        sleep_us(DELAY_US);
        i2c_write_byte(val | LCD_ENABLE_BIT);
        sleep_us(DELAY_US);
        i2c_write_byte(val & ~LCD_ENABLE_BIT);
        sleep_us(DELAY_US);
    }

    // The display is sent a byte as two separate nibble transfers
    void lcd_send_byte(uint8_t val, int mode) {
        uint8_t high = mode | (val & 0xF0) | LCD_BACKLIGHT;
        uint8_t low = mode | ((val << 4) & 0xF0) | LCD_BACKLIGHT;

        i2c_write_byte(high);
        lcd_toggle_enable(high);
        i2c_write_byte(low);
        lcd_toggle_enable(low);
    }

    void lcd_char(char val) {
        lcd_send_byte(val, LCD_CHARACTER);
    }

    void lcd_string(const char *s) {
        while (*s) {
            lcd_char(*s++);
        }
    }
};

#endif //QUEST_LCD_CONTROL_HPP
