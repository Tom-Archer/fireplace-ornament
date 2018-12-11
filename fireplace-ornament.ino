/*******************************************************************
 *  Fireplace Ornament PCB
 *  
 *  12 Charlie-plexed LEDs running off an ATtiny25
 *  
 *  By Tom Archer
 *  Tindie: https://www.tindie.com/stores/tomm_archer/
 *  Twitter: @groundcontrolto
 *******************************************************************/
volatile uint8_t Buffer[12] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

const uint8_t Num_Leds = 12;
const uint8_t On = 0xF;

// Lookup from logical LED numbering to electrical
//    2  4  6  8  10  12
//    1  3  5  7  9   11
const uint8_t Logical_Lookup[12] = {
  0x3, 0x5, 0x0, 0xA, 0x2, 0x7, 0x9, 0x4, 0x06, 0x8, 0x1, 0xB};
  
void DisplaySetup () {
  // ATtiny25/45/85
  // Set up Timer/Counter1 to multiplex the display - 8MHz
  TCCR1 = 1<<CTC1 | 6<<CS10;      // CTC mode; prescaler 32
  OCR1C = 24;                     // Divide by 25 (CLK=8000000Hz/32/25 = 10KHz)
  TIMSK = TIMSK | 1<<OCIE1A;      // Enable overflow interrupt
} 

void setup() {
  DisplaySetup();
  // Setup mode switch
  PORTB |= (1 << PB4); // input
  DDRB &= ~(1 << PB4);
}

void DisplayNextLed() {
  static uint16_t led = 0;
  // Turn off all LEDs
  DDRB = DDRB & 0xF0;
  // Get the current LED (0-11)
  led = (led + 1) % Num_Leds;
  // Read the LED value from the buffer
  uint8_t value = Buffer[led];

  uint8_t anode = (value>0) << (led % 3);
  uint8_t cathode = led/3;
  anode = anode + (anode & 0x07<<cathode);
  
  DDRB = (DDRB & 0xF0) | anode;
  PORTB = (PORTB & 0xF0) | anode;
  DDRB = DDRB | 1<<cathode;
}

ISR(TIM1_COMPA_vect) {
  DisplayNextLed();
}

uint8_t Mode = 0;
uint8_t Num_Modes = 4;
uint16_t Step = 0;

void CheckButtonState()
{
  static bool Switch_On = false; 
  if (!(PINB & (1<<PB4)))
  {
    if (!Switch_On)
    {
      Switch_On = true;
      //Step = 0;
      Mode++;
      Mode = Mode % Num_Modes; // constrain
    }
  }
  else
  {
    Switch_On = false;
  }
}

void loop() {
  CheckButtonState();
  switch (Mode)
  {
    case 0:
      UpdateSnake(100,4);
      break;
    case 1:
      UpdateRandom(100);
      break;
    case 2:
      UpdateSequence(100);
      break;
    case 3:
      UpdatePairs(100);
      break;  
  }
  
  Step++;
  // Prevent overflow
  Step = Step % 32768;
}

void ClearLeds()
{
  for(int i=0; i<Num_Leds;i++)
  {
    Buffer[i] = 0x0;
  }
}

void UpdateRandom(uint8_t wait)
{
  // Light LEDs in electrical order
  ClearLeds();
  Buffer[Step%12] = On;
  delay(wait);
}

void UpdateSequence(uint8_t wait)
{
  // Light LEDs in logical order
  // Add extra delay every second LED
  ClearLeds();
  uint8_t led = Step%12;
  Buffer[Logical_Lookup[led]] = On;
  if (led%2==1)
  {
    wait=wait<<1;
  }
  delay(wait);
}

void UpdatePairs(uint8_t wait)
{
  // Flash LEDs in pairs
  ClearLeds();
  uint8_t first_led = Step%12;
  uint8_t second_led = (Step+3)%12;
  Buffer[Logical_Lookup[first_led]] = On;
  Buffer[Logical_Lookup[second_led]] = On;
  delay(wait);
}

void UpdateSnake(uint8_t wait, uint8_t chain)
{
  // Z-Snake - Chain a number of LEDs
  ClearLeds();
  uint8_t led = Step%12;
  for(uint8_t j=0;j<chain;j++)
  {
    Buffer[Logical_Lookup[(Step-j)%12]] = On;
    ++j;
  }
  delay(wait);
}
