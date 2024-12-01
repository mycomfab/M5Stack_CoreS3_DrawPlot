#include <M5CoreS3.h>

#define PIN_UART_TX       2
#define PIN_UART_RX       1
#define DATA_SIZE         32768
#define DATA_PLOT_END     0xFA
#define DATA_PLOT_START   0xFB
#define DATA_PLOT_ACK     0xFC
#define DATA_PLOT_PENUP   0xFD
#define DATA_PLOT_PENDOWN 0xFE
#define BUTTON_UNDO       0
#define BUTTON_CLEAR      1
#define BUTTON_PLOT       2
#define BUTTON_PLOT_WAIT  3
#define BUTTON_ON         0
#define BUTTON_OFF        1
#define BUTTON_WIDTH      70
#define BUTTON_HEIGHT     60
#define BUTTON_POS_X      250
#define BUTTON_POS_Y      10
#define BUTTON_SPACE_Y    20
#define DRAW_WIDTH        240
#define DRAW_HEIGHT       240
#define DRAW_POS_X        0
#define DRAW_POS_Y        0

int32_t prev_x, prev_y;
bool    touch_on, button_on;

uint8_t  data[DATA_SIZE];
uint32_t data_ptr;

String button_text[] = { "Undo", "Clear", "Plot", "Wait" };

void plot_init(void) {
    Serial1.begin(9600, SERIAL_8N1, PIN_UART_RX, PIN_UART_TX);
    Serial1.flush();
}

bool plot_wait(void) {
    bool    flg_end, ret_val;
    uint8_t dat;
    
    flg_end = false;
    while ( !flg_end ) {
        CoreS3.update();
        auto t = CoreS3.Touch.getTouchPointRaw();
        auto td = CoreS3.Touch.getDetail();
        if ( td.state == m5::touch_state_t::touch_begin 
            || td.state == m5::touch_state_t::hold_begin
            || td.state == m5::touch_state_t::flick_begin
            || td.state == m5::touch_state_t::drag_begin ) {
            if ( t.x >= BUTTON_POS_X ) {
                if ( t.y > BUTTON_POS_Y+(BUTTON_HEIGHT*2+BUTTON_SPACE_Y*2)+2 && t.y < BUTTON_POS_Y+(BUTTON_HEIGHT*3+BUTTON_SPACE_Y*2)+2) {
                    ret_val = false;
                    flg_end = true;
                }
            }
        }
        if ( Serial1.available() ) {
            dat = Serial1.read();
            if ( dat == DATA_PLOT_START ) {
                ret_val = true;
                flg_end = true;
            }
        }
    }
    return ret_val;
}

void plot_process(void) {
    uint32_t ptr;
    uint16_t  x, y, prev_x, prev_y, dat, pen_flag;
    
    ptr = 0;
    pen_flag = 0;
    while ( ptr < data_ptr ) {
        x = data[ptr++];
        Serial1.write(x);
        if ( x != DATA_PLOT_PENUP ) {
            y = data[ptr++];
            Serial1.write(y);
        } else {
            pen_flag = 0;
        }
        while(!Serial1.available());
        dat = Serial1.read();
        if ( dat != DATA_PLOT_ACK ) {
            return;
        }
        if ( x < DRAW_WIDTH ) {
            if ( pen_flag == 1 ) {
                CoreS3.Display.drawLine(x, y, prev_x, prev_y, RED);
            } else {
                pen_flag = 1;
                Serial1.write(DATA_PLOT_PENDOWN);
                while(!Serial1.available());
            }
            prev_x = x;
            prev_y = y;
        }
    }
    ptr = 0;
    pen_flag = 0;
    while ( ptr < data_ptr ) {
        x = data[ptr++];
        if ( x != DATA_PLOT_PENUP ) {
            y = data[ptr++];
        } else {
            pen_flag = 0;
        }
        if ( x < DRAW_WIDTH ) {
            if ( pen_flag == 1 ) {
                CoreS3.Display.drawLine(x, y, prev_x, prev_y, BLACK);
            } else {
                pen_flag = 1;
            }
            prev_x = x;
            prev_y = y;
        }
    }
    Serial1.write(DATA_PLOT_END);
    while(!Serial1.available());
}

void button_disp(uint8_t no, uint8_t sw) {
    uint16_t x, y, color;
    
    if ( sw == BUTTON_ON ) {
        color = WHITE;
    } else {
        color = BLACK;
    }
    x = BUTTON_POS_X;
    if ( no == 3 ) {
        y = BUTTON_POS_Y + (BUTTON_HEIGHT+BUTTON_SPACE_Y) * (no-1);
    } else {
        y = BUTTON_POS_Y + (BUTTON_HEIGHT+BUTTON_SPACE_Y) * no;
    }
    CoreS3.Display.drawRect(x, y, BUTTON_WIDTH, BUTTON_HEIGHT, WHITE);
    CoreS3.Display.fillRect(x+1, y+1, BUTTON_WIDTH-2, BUTTON_HEIGHT-2, color);
    if ( color == BLACK ) {
        color = WHITE;
    } else {
        color = BLACK;
    }
    CoreS3.Display.setTextColor(color);
    CoreS3.Display.drawString(button_text[no], x+BUTTON_WIDTH/2, y+BUTTON_HEIGHT/2);
}

void data_disp()
{
    CoreS3.Display.setTextColor(WHITE);
    CoreS3.Display.setFont(&fonts::Font0);
    CoreS3.Display.fillRect(BUTTON_POS_X, 230, BUTTON_WIDTH, 10, BLACK);
    CoreS3.Display.setCursor(280, 237);
    CoreS3.Display.print(data_ptr);
    CoreS3.Display.setFont(&fonts::Font4);
}

void screen_init()
{
    CoreS3.Display.fillScreen(BLACK);
    CoreS3.Display.fillRect(DRAW_POS_X, DRAW_POS_Y, DRAW_WIDTH, DRAW_HEIGHT, WHITE);
    button_disp(BUTTON_UNDO,  BUTTON_OFF);
    button_disp(BUTTON_CLEAR, BUTTON_OFF);
    button_disp(BUTTON_PLOT,  BUTTON_OFF);
}

void setup() {
    auto cfg = M5.config();
    CoreS3.begin(cfg);
    
    CoreS3.Display.setTextDatum(middle_center);
    CoreS3.Display.setFont(&fonts::Font4);
    CoreS3.Display.setTextSize(1);
    
    screen_init();
    prev_x = -1;
    prev_y = -1;
    touch_on  = false;
    button_on = false;
    data_ptr = 0;
    data_disp();
    
    plot_init();
}

void loop() {
    uint16_t x, y, d;
    
    CoreS3.update();
    auto t = CoreS3.Touch.getTouchPointRaw();
    auto td = CoreS3.Touch.getDetail();
    
    if ( td.isPressed() && touch_on == false ) {
        if( t.x >= DRAW_POS_X && t.x < DRAW_POS_X+DRAW_WIDTH && t.y >= DRAW_POS_Y && t.y < DRAW_POS_Y+DRAW_HEIGHT ) {
            touch_on = true;
            prev_x = t.x;
            prev_y = t.y;
            if ( data_ptr < DATA_SIZE-3 ) {
                data[data_ptr++] = DATA_PLOT_PENUP;
                data[data_ptr++] = t.x;
                data[data_ptr++] = t.y;
                data_disp();
            }
        }
    }

    if ( touch_on == true ) {
        if( t.x >= DRAW_POS_X && t.x < DRAW_POS_X+DRAW_WIDTH && t.y >= DRAW_POS_Y && t.y < DRAW_POS_Y+DRAW_HEIGHT ) {
            if ( t.x != prev_x || t.y != prev_y ) {
                CoreS3.Display.drawLine(t.x, t.y, prev_x, prev_y, BLACK);
                prev_x = t.x;
                prev_y = t.y;
                if ( data_ptr < DATA_SIZE-2 ) {
                    data[data_ptr++] = t.x;
                    data[data_ptr++] = t.y;
                    data_disp();
                }
            }
        }
    }
    
    if ( button_on == false &&
        ( td.state == m5::touch_state_t::touch_begin 
        || td.state == m5::touch_state_t::hold_begin
        || td.state == m5::touch_state_t::flick_begin
        || td.state == m5::touch_state_t::drag_begin ) ) {
        if ( t.x >= BUTTON_POS_X ) {
            if ( t.y > BUTTON_POS_Y+2 && t.y < BUTTON_POS_Y+BUTTON_HEIGHT-2) {
                button_on = true;
                button_disp(BUTTON_UNDO, BUTTON_ON);
                if ( data_ptr > 0 ) {
                    do {
                        y = data[--data_ptr];
                        x = data[--data_ptr];
                        d = data[data_ptr-1];
                        CoreS3.Display.drawLine(x, y, prev_x, prev_y, WHITE);
                        prev_x = x;
                        prev_y = y;
                    } while ( d != DATA_PLOT_PENUP );
                    data_ptr--;
                    data_disp();
                }
            } else if ( t.y > BUTTON_POS_Y+(BUTTON_HEIGHT+BUTTON_SPACE_Y)+2 && t.y < BUTTON_POS_Y+(BUTTON_HEIGHT*2+BUTTON_SPACE_Y)+2) {
                button_on = true;
                button_disp(BUTTON_CLEAR, BUTTON_ON);
                CoreS3.Display.fillRect(DRAW_POS_X, DRAW_POS_Y, DRAW_WIDTH, DRAW_HEIGHT, WHITE);
                data_ptr = 0;
                data_disp();
            } else if ( t.y > BUTTON_POS_Y+(BUTTON_HEIGHT*2+BUTTON_SPACE_Y*2)+2 && t.y < BUTTON_POS_Y+(BUTTON_HEIGHT*3+BUTTON_SPACE_Y*2)+2) {
                button_disp(BUTTON_PLOT_WAIT, BUTTON_ON);
                if ( plot_wait() ) {
                    button_disp(BUTTON_PLOT, BUTTON_ON);
                    plot_process();
                }
                button_disp(BUTTON_PLOT, BUTTON_OFF);
            }
        }
    }
        
    if ( td.state == m5::touch_state_t::touch_end 
       || td.state == m5::touch_state_t::hold_end
       || td.state == m5::touch_state_t::flick_end
       || td.state == m5::touch_state_t::drag_end ) {
        if ( touch_on == true ) {
            touch_on = false;
            if ( t.x != prev_x || t.y != prev_y ) {
                CoreS3.Display.drawLine(t.x, t.y, prev_x, prev_y, BLACK);
                prev_x = t.x;
                prev_y = t.y;
                if ( data_ptr < DATA_SIZE-2 ) {
                    data[data_ptr++] = t.x;
                    data[data_ptr++] = t.y;
                    data_disp();
                }
            }
        }
        if ( button_on == true ) {
            button_on = false;
            button_disp(BUTTON_UNDO,  BUTTON_OFF);
            button_disp(BUTTON_CLEAR, BUTTON_OFF);
            button_disp(BUTTON_PLOT,  BUTTON_OFF);
        }
    }
}
