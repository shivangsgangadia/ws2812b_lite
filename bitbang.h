/**
 * Extremely lightweight system to control WS2812B Leds.
 * Works with AtMega / Uno boards at both 16MHz and 8MHz
 * Original assembly code and explaination from : https://www.instructables.com/id/Bitbanging-step-by-step-Arduino-control-of-WS2811-/
 * Modified for general purpose use and 8MHz version modifications by
 * @author Shivang Gangadia (shivangsgangadia@gmail.com)
 * */

// Change pins and ports here. Look up pin diagram for whatever AtMega IC your board uses
#define DIGITAL_PIN (7)         // Digital pin number
#define PORT (PORTD)            // Digital pin's port
#define PORT_PIN (PORTD7)       // Digital pin's bit position
#define NUM_BITS (8) // Constant value: bits per byte


/**
 *  This single class does everything.
 * */
class LedController
{
  private:
    uint32_t totalLeds = 12;
    uint32_t t_f;

  
  public:
    LedController(uint32_t leds)
    {
      this->totalLeds = leds;
    }

    void setTotalLeds(uint32_t leds)
    {
      this->totalLeds = leds;
    }

    /**
     * Initializes output mode of digital pin that will be controlling the LEDs and sets it to low initially
     * */
    void ledInit()
    {
      pinMode(DIGITAL_PIN, OUTPUT);
      digitalWrite(DIGITAL_PIN, LOW);
    }


  /**
   * Actually woks the bitBang magic using AVR assembly for tight timings. Internally handles code changes for 8 or 16MHz at compile time.
   * @param colors array of color specification in GRB format. Only pass one set of GRB.
   * */
   void bitBang(uint8_t *colors)
   {
         volatile uint8_t
           *p = colors,
           val = *p++,
           high = PORT | _BV(PORT_PIN), // Bitmask for sending HIGH to pin
           low = PORT & ~_BV(PORT_PIN), // Bitmask for sending LOW to pin
           tmp = low,                   // Swap variable to adjust duty cycle
           nbits = NUM_BITS;            // Bit counter for inner loop

         uint8_t counter = 3;

#if F_CPU == 8000000UL
            asm volatile(
             // Instruction        CLK     Description                 Phase
             "nextbiti:\n\t"      // -    label                       (T =  0)
             "sbrc %4, 7\n\t"    // 1-2  if MSB set                  (T =  ?)
             "mov  %6, %3\n\t"   // 0-1   tmp'll set signal high     (T =  4)
             "dec  %5\n\t"       // 1    decrease bitcount           (T =  5)
             "sbi  %0, %1\n\t"   // 2    signal HIGH                 (T =  2)
             "st   %a2, %6\n\t"  // 2    set PORT to tmp             (T =  8)
             "mov  %6, %7\n\t"   // 1    reset tmp to low (default)  (T =  9)
             "breq nextbytei\n\t" // 1-2  if bitcount ==0 -> nextbyte (T =  ?)
             "cbi   %0, %1\n\t"  // 2    signal LOW                  (T = 12)
             "rol  %4\n\t"       // 1    shift MSB leftwards         (T = 11)
             "rjmp .+0\n\t"      // 2    nop nop                     (T = 17)
             "nop\n\t"           // 1    nop                         (T = 18)
             "rjmp nextbiti\n\t"  // 2    bitcount !=0 -> nextbit     (T = 20)
             "nextbytei:\n\t"     // -    label                       -
             "ldi  %5, 8\n\t"    // 1    reset bitcount              (T = 11)
             "ld   %4, %a8+\n\t"  // 2    val = *p++                  (T = 13)
             "cbi   %0, %1\n\t"  // 2    signal LOW                  (T = 15)
             "rjmp .+0\n\t"      // 2    nop nop                     (T = 17)
             "nop\n\t"           // 1    nop                         (T = 18)
             "dec %9\n\t"        // 1    decrease bytecount          (T = 19)
             "brne nextbiti\n\t"  // 2    if bytecount !=0 -> nextbit (T = 20)
             ::
                 // Input operands         Operand Id (w/ constraint)
             "I"(_SFR_IO_ADDR(PORT)), // %0
             "I"(PORT_PIN),           // %1
             "e"(&PORT),              // %a2
             "r"(high),               // %3
             "r"(val),                // %4
             "r"(nbits),              // %5
             "r"(tmp),                // %6
             "r"(low),                // %7
             "e"(p),          // %a8
             "w"(counter)      // %9
         );
#elif F_CPU == 16000000UL
          asm volatile(
              // Instruction        CLK     Description                 Phase
              "nextbiti:\n\t"      // -    label                       (T =  0)
              "sbi  %0, %1\n\t"   // 2    signal HIGH                 (T =  2)
              "sbrc %4, 7\n\t"    // 1-2  if MSB set                  (T =  ?)
              "mov  %6, %3\n\t"   // 0-1   tmp'll set signal high     (T =  4)
              "dec  %5\n\t"       // 1    decrease bitcount           (T =  5)
              "nop\n\t"           // 1    nop (idle 1 clock cycle)    (T =  6)
              "st   %a2, %6\n\t"  // 2    set PORT to tmp             (T =  8)
              "mov  %6, %7\n\t"   // 1    reset tmp to low (default)  (T =  9)
              "breq nextbytei\n\t" // 1-2  if bitcount ==0 -> nextbyte (T =  ?)
              "rol  %4\n\t"       // 1    shift MSB leftwards         (T = 11)
              "rjmp .+0\n\t"      // 2    nop nop                     (T = 13)
              "cbi   %0, %1\n\t"  // 2    signal LOW                  (T = 15)
              "rjmp .+0\n\t"      // 2    nop nop                     (T = 17)
              "nop\n\t"           // 1    nop                         (T = 18)
              "rjmp nextbiti\n\t"  // 2    bitcount !=0 -> nextbit     (T = 20)
              "nextbytei:\n\t"     // -    label                       -
              "ldi  %5, 8\n\t"    // 1    reset bitcount              (T = 11)
              "ld   %4, %a8+\n\t"  // 2    val = *p++                  (T = 13)
              "cbi   %0, %1\n\t"  // 2    signal LOW                  (T = 15)
              "rjmp .+0\n\t"      // 2    nop nop                     (T = 17)
              "nop\n\t"           // 1    nop                         (T = 18)
              "dec %9\n\t"        // 1    decrease bytecount          (T = 19)
              "brne nextbiti\n\t"  // 2    if bytecount !=0 -> nextbit (T = 20)
              ::
                  // Input operands         Operand Id (w/ constraint)
              "I"(_SFR_IO_ADDR(PORT)), // %0
              "I"(PORT_PIN),           // %1
              "e"(&PORT),              // %a2
              "r"(high),               // %3
              "r"(val),                // %4
              "r"(nbits),              // %5
              "r"(tmp),                // %6
              "r"(low),                // %7
              "e"(p),          // %a8
              "w"(counter)      // %9
          );
#endif
   }

    /**
     * Lights up leds in sequence : Black - Color - Black
     * You need to specify only count of initial black and color, the rest are filled up according to total number of Leds
     * @param countInitBlack number of Leds to not light up initially
     * @param countColor Number of Leds to glow with color
     * @param r Red specification of color (0 - 255)
     * @param g Green specification of color (0 - 255)
     * @param b Blue specification of color (0 - 255)
    */
    void render(uint8_t countInitBlack, uint8_t countColor, uint8_t r, uint8_t g, uint8_t b)
    {
      uint8_t countLastBlack = this->totalLeds - (countInitBlack + countColor);

      uint8_t *black_arr = new uint8_t(3);
      black_arr[0] = 0;
      black_arr[1] = 0;
      black_arr[2] = 0;

      uint8_t *color_arr = new uint8_t(3);
      color_arr[0] = (g);
      color_arr[1] = (r);
      color_arr[2] = (b);
      


      cli(); // Disable interrupts so that timing is as precise as possible


      for (uint8_t i = 0; i < countInitBlack; i++)
      {
        bitBang(black_arr);
      }

      for (uint8_t i = 0; i < countColor; i++)
      {
        bitBang(color_arr);
      }

      for (uint8_t i = 0; i < countLastBlack; i++)
      {
        bitBang(black_arr);
      }
    

      sei();          // Enable interrupts
      t_f = micros(); // t_f will be used to measure the 50us
                      // latching period in the next call of the
                      // function.
      while ((micros() - t_f) < 50L); // wait for 50us (data latch)
    }

};
