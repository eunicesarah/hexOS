#include "keyboard.h"
#include "../lib-header/portio.h"
#include "../lib-header/framebuffer.h"
#include "../lib-header/stdmem.h"


const char keyboard_scancode_1_to_ascii_map[256] = {
      0, 0x1B, '1', '2', '3', '4', '5', '6',  '7', '8', '9',  '0',  '-', '=', '\b', '\t',
    'q',  'w', 'e', 'r', 't', 'y', 'u', 'i',  'o', 'p', '[',  ']', '\n',   0,  'a',  's',
    'd',  'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   0, '\\',  'z', 'x',  'c',  'v',
    'b',  'n', 'm', ',', '.', '/',   0, '*',    0, ' ',   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0, '-',    0,    0,   0,  '+',    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,

      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
};

static struct KeyboardDriverState keyboard_state;

void keyboard_state_activate(){
  keyboard_state.keyboard_input_on = TRUE;
}

void keyboard_state_deactivate() {
  keyboard_state.keyboard_input_on = FALSE;
}

void get_keyboard_buffer(char *buf) {
  memcpy(buf, keyboard_state.keyboard_buffer, KEYBOARD_BUFFER_SIZE);
}

bool is_keyboard_blocking() {
  return keyboard_state.keyboard_input_on;
}

//ori 

void keyboard_isr(void) {
    if (!keyboard_state.keyboard_input_on) {
        keyboard_state.buffer_index = 0;
    } 
    else {
        uint8_t scancode = in(KEYBOARD_DATA_PORT);
        char mapped_char = keyboard_scancode_1_to_ascii_map[scancode];
        uint8_t cursor_x, cursor_y;
        static bool last_key_pressed = FALSE;
        static bool make_code = FALSE;
        
        if (!is_keyboard_blocking()) {
            keyboard_state.buffer_index = 0;
        }
        else {
            if (mapped_char != 0) {
                last_key_pressed = mapped_char;
                make_code = TRUE;
            }
            else if (make_code) {
                switch (last_key_pressed) {
                    case '\n':
                        keyboard_state.keyboard_buffer[keyboard_state.buffer_index] = 0;
                        framebuffer_get_cursor(&cursor_x, &cursor_y);
                        framebuffer_set_cursor(cursor_x + 1, 0);
                        break;
                    case '\b':
                        framebuffer_get_cursor(&cursor_x, &cursor_y);
                        if (keyboard_state.buffer_index > 0) {
                            keyboard_state.buffer_index--;
                            if (cursor_y == 0) {
                                if (cursor_x > 0) {
                                    cursor_y = 79;
                                    framebuffer_set_cursor(cursor_x - 1, cursor_y);
                                    framebuffer_write(cursor_x - 1, cursor_y, ' ', 0x0F, 0X00);
                                } 
                            } else {
                                framebuffer_set_cursor(cursor_x, cursor_y - 1);
                                framebuffer_write(cursor_x, cursor_y - 1, ' ', 0x0F, 0X00);
                            }
                        }
                        break;          
                    default:
                        keyboard_state.keyboard_buffer[keyboard_state.buffer_index++] = last_key_pressed;
                        framebuffer_get_cursor(&cursor_x, &cursor_y);
                        if (cursor_y == 79) {
                            framebuffer_write(cursor_x, cursor_y, last_key_pressed, 0x0F, 0x00);
                            framebuffer_set_cursor(cursor_x + 1, 0);
                            cursor_x++;
                            cursor_y = 0;
                        } else {
                            framebuffer_write(cursor_x, cursor_y, last_key_pressed, 0x0F, 0x00);
                            framebuffer_set_cursor(cursor_x, cursor_y + 1);
                            cursor_y++;
                        }
                        break;
                }
                make_code = FALSE;
            }
            switch (scancode) {
                case EXT_SCANCODE_RIGHT:
                    framebuffer_get_cursor(&cursor_x, &cursor_y);
                    if (cursor_y == 79) {
                        framebuffer_set_cursor(cursor_x + 1, 0);
                    } else {
                        if (cursor_y == keyboard_state.buffer_index) {
                            framebuffer_set_cursor(cursor_x, cursor_y);
                        } else {
                            framebuffer_set_cursor(cursor_x, cursor_y + 1);
                        }
                    }
                    break;
                case EXT_SCANCODE_LEFT:
                    framebuffer_get_cursor(&cursor_x, &cursor_y);
                    if (cursor_y == 0) {
                        if (cursor_x > 0) {
                            framebuffer_set_cursor(cursor_x - 1, 79);
                        }
                    } else {
                        framebuffer_set_cursor(cursor_x, cursor_y - 1);
                    }
                    break;
                case EXT_SCANCODE_UP:
                    framebuffer_get_cursor(&cursor_x, &cursor_y);
                    if (cursor_x > 0) {
                        framebuffer_set_cursor(cursor_x - 1, cursor_y);
                    }
                    break;
                case EXT_SCANCODE_DOWN:
                    framebuffer_get_cursor(&cursor_x, &cursor_y);
                    if (cursor_x < 24) {
                        framebuffer_set_cursor(cursor_x + 1, cursor_y);
                    }
                    break;
                default:
                    break;
            }
        }
    }
    pic_ack(IRQ_KEYBOARD);
}

// J

// void keyboard_isr(void) {
//     if (!keyboard_state.keyboard_input_on) {
//         keyboard_state.buffer_index = 0;
//     } 
//     else {
//         uint8_t scancode = in(KEYBOARD_DATA_PORT);
//         char mapped_char = keyboard_scancode_1_to_ascii_map[scancode];
//         uint8_t cursor_x, cursor_y;
//         // static bool last_key_pressed = FALSE;
//         // static uint8_t last_scancode = 0;
//         if (mapped_char != 0) {
//             if (mapped_char == '\n') {
//                 keyboard_state.keyboard_buffer[keyboard_state.buffer_index] = 0;
//                 framebuffer_get_cursor(&cursor_x, &cursor_y);
//                 framebuffer_set_cursor(cursor_x + 1, 0);
//             }
//             else if (mapped_char == '\b') {
//                 framebuffer_get_cursor(&cursor_x, &cursor_y);
//                 if (keyboard_state.buffer_index > 0) {
//                     keyboard_state.buffer_index--;
//                     if (cursor_y == 0) {
//                         if (cursor_x > 0) {
//                             cursor_y = 79;
//                             framebuffer_set_cursor(cursor_x - 1, cursor_y);
//                             framebuffer_write(cursor_x - 1, cursor_y, ' ', 0x0F, 0X00);
//                         } 
//                     } else {
//                         framebuffer_set_cursor(cursor_x, cursor_y - 1);
//                         framebuffer_write(cursor_x, cursor_y-1, ' ', 0x0F, 0X00);
//                     }
//                 }
//             }        
//             else {
//                 keyboard_state.keyboard_buffer[keyboard_state.buffer_index++] = mapped_char;
//                 framebuffer_get_cursor(&cursor_x, &cursor_y);
//                 if (cursor_y == 79) {
//                     framebuffer_write(cursor_x, cursor_y, mapped_char, 0x0F, 0x00);
//                     framebuffer_set_cursor(cursor_x+1, 0);
//                     cursor_x++;
//                     cursor_y = 0;
//                 } else {
//                     framebuffer_write(cursor_x, cursor_y, mapped_char, 0x0F, 0x00);
//                     framebuffer_set_cursor(cursor_x, cursor_y+1);
//                     cursor_y++;
//                 }
//             }
//         }
//         if (scancode == EXTENDED_SCANCODE_BYTE) {
//             keyboard_state.read_extended_mode = TRUE;
//         } 
//         else if (keyboard_state.read_extended_mode) {
//             if (scancode == EXT_SCANCODE_RIGHT) {
//                 framebuffer_get_cursor(&cursor_x, &cursor_y);
//                 if (cursor_y == 79) {
//                     framebuffer_set_cursor(cursor_x+1, 0);
//                 } else {
//                     framebuffer_set_cursor(cursor_x, cursor_y+1);
//                 }
//             }
//             else if (scancode == EXT_SCANCODE_LEFT) {
//                 framebuffer_get_cursor(&cursor_x, &cursor_y);
//                 if (cursor_y == 0) {
//                     if (cursor_x > 0) {
//                         framebuffer_set_cursor(cursor_x - 1, 79);
//                     }
//                 } else {
//                     framebuffer_set_cursor(cursor_x, cursor_y-1);
//                 }
//             }
//             else if (scancode == EXT_SCANCODE_UP) {
//                 framebuffer_get_cursor(&cursor_x, &cursor_y);
//                 if (cursor_x > 0) {
//                     framebuffer_set_cursor(cursor_x - 1, cursor_y);
//                 }             
//             }
//             else if (scancode == EXT_SCANCODE_DOWN) {
//                 framebuffer_get_cursor(&cursor_x, &cursor_y);
//                 if (cursor_x < 24) {
//                     framebuffer_set_cursor(cursor_x + 1, cursor_y);
//                 }
//             }
//         }
//     }
//     pic_ack(IRQ_KEYBOARD);
// }

// void keyboard_isr(void)
// {

//     uint8_t cursor_x, cursor_y;
//     uint8_t scancode = in(KEYBOARD_DATA_PORT);
//     char mapped_char = keyboard_scancode_1_to_ascii_map[scancode];
//     static char last_mapped_char = 0;
//     static bool make_code = FALSE;

//     if (!is_keyboard_blocking())
//     {
//         keyboard_state.buffer_index = 0;
//     }
//     else
//     {
//         // Register mapped character when pressed
//         if (mapped_char != 0)
//         {
//             make_code = TRUE;
//             last_mapped_char = mapped_char;
//         }
//         // Put mapped character in buffer after release
//         else if (make_code)
//         {
//             framebuffer_get_cursor(&cursor_x, &cursor_y);

//             // Backspace
//             if (last_mapped_char == '\b')
//             {
//                 if (keyboard_state.buffer_index > 0)
//                 {
//                     if (cursor_y != 0)
//                     {
//                         keyboard_state.keyboard_buffer[keyboard_state.buffer_index] = 0;
//                         keyboard_state.buffer_index--;
//                         framebuffer_set_cursor(cursor_x, cursor_y - 1);
//                         framebuffer_write(cursor_x, cursor_y - 1, ' ', 0x0F, 0x00);
//                     }
//                     if (cursor_y == 0) 
//                     {
//                         if (cursor_x > 0)
//                         {
//                             keyboard_state.keyboard_buffer[keyboard_state.buffer_index] = 0;
//                             keyboard_state.buffer_index--;
//                             framebuffer_set_cursor(cursor_x-1, 79);
//                             framebuffer_write(cursor_x-1, 79, ' ', 0x0F, 0x00);

//                         } 
//                     }
//                 }
//             }

//             // Enter
//             else if (last_mapped_char == '\n')
//             {
//                 keyboard_state.keyboard_buffer[keyboard_state.buffer_index++] = last_mapped_char;
//                 framebuffer_set_cursor(cursor_x + 1, 0);
//                 keyboard_state_deactivate();
//             }

//             // Character
//             else if (last_mapped_char != 0)
//             {
//                 keyboard_state.keyboard_buffer[keyboard_state.buffer_index++] = last_mapped_char;
//                 framebuffer_write(cursor_x, cursor_y, last_mapped_char, 0x0F, 0x00);
//                 framebuffer_set_cursor(cursor_x, cursor_y + 1);
//                 last_mapped_char = 0;
//             }
            
//             // Arrow
//             else if (scancode == EXT_SCANCODE_UP)
//             {
//                 framebuffer_get_cursor(&cursor_x, &cursor_y);
//                 if (cursor_x > 0) {
//                     framebuffer_set_cursor(cursor_x-1, cursor_y);
//                 }
//             }
//             else if (scancode == EXT_SCANCODE_DOWN)
//             {
//                 framebuffer_get_cursor(&cursor_x, &cursor_y);
//                 if (cursor_x < 24) {
//                     framebuffer_set_cursor(cursor_x+1, cursor_y);
//                 }
//             }
//             else if (scancode == EXT_SCANCODE_LEFT)
//             {
//                 framebuffer_get_cursor(&cursor_x, &cursor_y);
//                 if (cursor_y > 0) {
//                     framebuffer_set_cursor(cursor_x, cursor_y-1);
//                 } else {
//                     if(cursor_x > 0) {
//                         framebuffer_set_cursor(cursor_x-1, 79);
//                     }
//                 }
//             }
//             else if (scancode == EXT_SCANCODE_RIGHT)
//             {
//                 framebuffer_get_cursor(&cursor_x, &cursor_y);
//                 if (cursor_y < 79) {
//                     framebuffer_set_cursor(cursor_x, cursor_y+1);
//                 }
//             }
//             make_code = FALSE;
//         }
//     }

//     pic_ack(IRQ_KEYBOARD);
// }