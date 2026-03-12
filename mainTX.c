#define F_CPU 4000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdbool.h>
#include <avr/cpufunc.h>
#include <string.h>
// Här skapar vi vår bärvåg på 38kHz, för att detta ska stämma måste varje cykel vara 26µs lång. 
// T = 1 / 38000 = 26us 
void ir_burst(uint16_t cycles) {
    for (uint16_t i = 0; i < cycles; i++) {
        PORTD.OUTTGL = PIN0_bm; //Detta inverterar PIN0 nuvarande läge, LOW->HIGh eller vice versa
        _delay_us(13);
        PORTD.OUTTGL = PIN0_bm;
        _delay_us(13);
    }
}

// Denna funktion forcerar vår LED till LOW och så skapar en paus
void ir_space_us(uint16_t us) {
    PORTD.OUTCLR = PIN0_bm;
    for (uint16_t i = 0; i < us; i++) {
        _delay_us(1);
    }
}

//samma som ovan fast ms
void ir_space_ms(uint16_t ms) {
    PORTD.OUTCLR = PIN0_bm;
    for (uint16_t i = 0; i < ms; i++) {
        _delay_ms(1);
    }
}

//NEC, itererar från MSB till LSB.
void nec_send_byte(uint8_t b) {
    for (int8_t bit = 7; bit >= 0; bit--) {
        ir_burst(21);              /* 562µs burst (21 cycles * 26µs) */
        if (b & (1 << bit)) {
            ir_space_us(1687);     /* '1': 1687µs space */
        } else {
            ir_space_us(562);      /* '0': 562µs space */
        }
    }
}

// Skicka en buffer av bytes med den förlängda leader-signalen. + en stoppmarkör. 
void nec_transmit(uint8_t *data, uint8_t len) {
    /* Ledsignalen: 9ms burst + 4.5ms paus */
    ir_burst(346);
    ir_space_ms(4);
    ir_space_us(500);

    for (uint8_t i = 0; i < len; i++) {
        nec_send_byte(data[i]);
    }

    /* Stop bit */
    ir_burst(21);
    PORTD.OUTCLR = PIN0_bm;
}

void serial_init() {
    PORTF.DIRSET = PIN0_bm;
    USART2.BAUD = (uint16_t)((float)(4.0 * F_CPU / 9600) + 0.5);
    USART2.CTRLB |= USART_TXEN_bm;
}


int main(void) {
    _PROTECTED_WRITE(CLKCTRL.OSCHFCTRLA, CLKCTRL_FREQSEL_4M_gc);
    serial_init();

    //Aktivera pins
    PORTD.DIRCLR = PIN3_bm;
    PORTD.PIN3CTRL = PORT_PULLUPEN_bm;

    PORTD.DIRSET = PIN0_bm;
    PORTD.OUTCLR = PIN0_bm;

    while (1) {
        static const char text[] = "Hej klasskamrater";
        uint8_t key = 0xAA; //krypteringsnyckeln, standard XOR för test. 
        uint8_t text_len = sizeof(text) - 1;

        /* Här skickar vi en hel, oavbruten, NEC "frame"  —
           */
        ir_burst(346);
        ir_space_ms(4);
        ir_space_us(500);
        for (uint8_t i = 0; i < text_len; i++) {
            nec_send_byte(text[i] ^ key);
        }
        ir_burst(21);
        PORTD.OUTCLR = PIN0_bm;

        /* (AI genererad) UART debug som skapas EFTER att allt har skickats  */
        {
        
            char line[] = "00000000\n";
            for (uint8_t i = 0; i < text_len; i++) {
                uint8_t enc = text[i] ^ key;
                line[0] = (enc & 0x80) ? '1' : '0';
                line[1] = (enc & 0x40) ? '1' : '0';
                line[2] = (enc & 0x20) ? '1' : '0';
                line[3] = (enc & 0x10) ? '1' : '0';
                line[4] = (enc & 0x08) ? '1' : '0';
                line[5] = (enc & 0x04) ? '1' : '0';
                line[6] = (enc & 0x02) ? '1' : '0';
                line[7] = (enc & 0x01) ? '1' : '0';
                for (int j = 0; line[j] != '\0'; j++) {
                    while (!(USART2.STATUS & USART_DREIF_bm));
                    USART2.TXDATAL = line[j];
                }
            }
        }

        /* paus mellan varje meddelande */
        for (uint16_t i = 0; i < 5000; i++) _delay_ms(1);
    }

}