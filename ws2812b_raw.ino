#include "bitbang.h"
#define NUM_LEDS 12

LedController controller(NUM_LEDS);

void setup() {
    controller.ledInit();
    controller.render( 2, 2, 40, 0, 40);
}

void loop() {
    
}
