#define F_CPU 4000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>


// Initierar våra pins osv.  
void serial_init(void) {
    PORTF.DIRSET = PIN0_bm;
    USART2.BAUD = (uint16_t)((float)(4.0 * F_CPU / 9600) + 0.5);
    USART2.CTRLB |= USART_TXEN_bm;
}
//Mäter en period av "tystnad"
uint16_t measure_low(void) {
    uint16_t count = 0;
    while (!(PORTD.IN & PIN3_bm)) {
        _delay_us(50);
        count++;
        if (count >= 10000) return count;
    }
    return count;
}

//Mäter en period av signaler
uint16_t measure_high(void) {
    uint16_t count = 0;
    while (PORTD.IN & PIN3_bm) {
        _delay_us(50);
        count++;
        if (count >= 10000) return count;
    }
    return count;
}

int main(void) {
    serial_init();

    PORTD.DIRCLR = PIN3_bm;
    PORTD.PIN3CTRL = PORT_PULLUPEN_bm;

    { //Debugging UART meddelande, för att vara säker på att main körs trots eventuell avsaknad av signaler.
        char msg[] = "RX READY\n";
        for (int i = 0; msg[i] != '\0'; i++) {
            while (!(USART2.STATUS & USART_DREIF_bm));
            USART2.TXDATAL = msg[i];
        }
    }

    while (1) {
        //Här väntar vi på att vår PIN3 ska slås om till LOW

        if (PORTD.IN & PIN3_bm) continue;

        /* 
        Följande logik kom AI fram till, detta var det stora problemet som gjorde att min egen lösning
        inte funkade som den skulle:
            Drain HS0038B AGC glitches from the long leader burst.
         * Keep consuming LOW/HIGH pairs until we find a HIGH > 70 ticks.
         * Real leader space is ~90 ticks (4.5ms).
         * AGC glitch spaces are ~26-40 ticks — skip them. */
        //
        uint16_t leader_high;
        for (;;) {
            measure_low();                    /* consume LOW */
            leader_high = measure_high();     /* measure HIGH */
            if (leader_high >= 10000) break;  /* timeout — no frame */
            if (leader_high > 70) break;      /* found real leader space */
        }
        if (leader_high >= 10000 || leader_high <= 70) continue;

        /* Avkodningsbits */
        uint8_t rx_bytes[16];
        uint8_t byte_count = 0;
        uint8_t cur_byte = 0;
        uint8_t bit_cnt = 0;

        for (;;) {
            uint16_t burst = measure_low();
            if (burst >= 10000) break;

            uint16_t space = measure_high();
            if (space >= 10000) break;

            /* Om ledsignalen fortsätter längre så skippar vi den */
            if (space > 90) continue;

            /* Threshold: > 45 ticks = '1', else '0' */
            // 
            cur_byte <<= 1;
            if (space > 45) {
                cur_byte |= 1;
            }
            bit_cnt++;

            if (bit_cnt == 8) {
                if (byte_count < 16) {
                    rx_bytes[byte_count++] = cur_byte;
                }
                cur_byte = 0;
                bit_cnt = 0;
            }
        }

        if (byte_count == 0) continue;

        /* Här printar vi ut fulla resultatet! */
        {
            char hdr[] = "RX: ";
            for (int i = 0; hdr[i] != '\0'; i++) {
                while (!(USART2.STATUS & USART_DREIF_bm));
                USART2.TXDATAL = hdr[i];
            }
        }
        for (uint8_t i = 0; i < byte_count; i++) {
            while (!(USART2.STATUS & USART_DREIF_bm));
            USART2.TXDATAL = rx_bytes[i] ^ 0xAA;
        }
        while (!(USART2.STATUS & USART_DREIF_bm));
        USART2.TXDATAL = '\n';

        /* Här printar vi ut våra bytes till vår UART, samma sätt som i transmittern */
        for (uint8_t i = 0; i < byte_count; i++) {
            char line[] = "00000000\n";
            line[0] = (rx_bytes[i] & 0x80) ? '1' : '0';
            line[1] = (rx_bytes[i] & 0x40) ? '1' : '0';
            line[2] = (rx_bytes[i] & 0x20) ? '1' : '0';
            line[3] = (rx_bytes[i] & 0x10) ? '1' : '0';
            line[4] = (rx_bytes[i] & 0x08) ? '1' : '0';
            line[5] = (rx_bytes[i] & 0x04) ? '1' : '0';
            line[6] = (rx_bytes[i] & 0x02) ? '1' : '0';
            line[7] = (rx_bytes[i] & 0x01) ? '1' : '0';
            for (int j = 0; line[j] != '\0'; j++) {
                while (!(USART2.STATUS & USART_DREIF_bm));
                USART2.TXDATAL = line[j];
            }
        }

        /* 500ms paus */
        for (uint16_t i = 0; i < 500; i++) {
            _delay_ms(1);
        }
    }
}