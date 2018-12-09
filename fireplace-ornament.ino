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

// uint8_t Physical_Lookup[12] PROGMEM = {
//  0x3, 0x0, 0x7, 0x4, 0x6, 0x1, 0xB, 0x8, 0x0A, 0x5, 0x9, 0x2};

// Lookup from logical LED numbering to electrical
//    2  4  6  8  10  12
//    1  3  5  7  9   1
const uint8_t Logical_Lookup[12] = {
  0x3, 0x0, 0x7, 0x4, 0x6, 0x1, 0xB, 0x8, 0x0A, 0x5, 0x9, 0x2};
  
void DisplaySetup () {
  // ATtiny25/45/85
  // Set up Timer/Counter1 to multiplex the display - 8MHz
  TCCR1 = 1<<CTC1 | 6<<CS10;      // CTC mode; prescaler 32
  OCR1C = 24;                     // Divide by 25 (CLK=8000000Hz/32/25 = 10KHz)
  TIMSK = TIMSK | 1<<OCIE1A;      // Enable overflow interrupt
  // ~50Hz complete refresh rate
} 

void setup() {
  DisplaySetup();
  // Setup mode switch
  PORTB |= (1 << PB4); // input
  DDRB &= ~(1 << PB4);
}

void DisplayNextLed() {
  static uint16_t cycle = 0; // uint16_t is more efficient
  // Turn off the cathode from the last cycle
  DDRB = DDRB & ~(1<<((cycle % Num_Leds)/3));
  // Get the current cycle (0-191)
  // Each LED is updated 16 times every 192 cycles
  cycle = (cycle + 1) % 0xC0;
  // Get the current LED number (0-11) 
  uint8_t led = cycle % Num_Leds;
  // Get LED update number (0-15)
  uint8_t count = cycle>>4;
  // Read the LED value from the buffer
  uint8_t value = Buffer[led];

  uint8_t anode = (count < value) << led % 3;
  uint8_t cathode = led/3;
  anode = anode + (anode & 0x07<<cathode);
  
  DDRB = (DDRB & 0xF0) | anode;
  PORTB = (PORTB & 0xF0) | anode;
  DDRB = DDRB | 1<<cathode;
}

ISR(TIM0_COMPA_vect) {
  DisplayNextLed();
}

uint8_t Mode = 0;
uint8_t Num_Modes = 12;
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
    case 1:
    case 2:
    case 3:
      UpdateTriangleWave(50*(Mode+1));
      break;
    case 4:
    case 5:
    case 6:
    case 7:
      UpdateSquareWave(50*((Mode+1)%4));
      break;
    case 8:
      UpdateSequence(100);
    case 9:
      UpdatePairs(100);
    case 10:
      UpdateSnake(100);
    default:
      break;
  }  
  Step++;
  // Prevent overflow
  Step = Step % 32768;
}

void UpdateTriangleWave(uint8_t wait)
{ 
  // Parameterised fade in/out
  // Adjust a for different amplitude
  // Adjust p for different half-period
  // Adjust s for different start time
  static uint8_t a = On;
  static uint8_t p = 20;
  static uint8_t s = 0;
  for(uint8_t i;i<Num_Leds;i++)
  {
    //Buffer[i] = abs(((s + Step/p) % (2*a)) - a);
    Buffer[i] = a*(abs(((s + Step) % (2*p)) - p))/p;
  
  }
  delay(wait);
}

void UpdateSquareWave(uint8_t wait)
{
  // Parameterised flash on/off
  // Adjust a for different amplitude
  // Adjust d for different duty cycle
  // Adjust p for different period
  // Adjust s for different start time
  static uint8_t a = On;
  static uint8_t d = 8;
  static uint8_t p = 1;
  static uint8_t s = 0;
  for(uint8_t i;i<Num_Leds;i++)
  {
    if (((s + Step/p) % a) < d)
    {
      Buffer[i] = 0x0;
    }
    else
    {
      Buffer[i] = a;
    }
  }
  delay(wait);
}

void UpdateSequence(uint8_t wait)
{
  // Light LEDs in order turning off previous
  // Add extra delay every second LED
  uint8_t led = Step%12;
  Buffer[Logical_Lookup[led]] = On;
  if (led == 0)
  {
    Buffer[Logical_Lookup[11]] = 0x0;
  }
  else
  {
    Buffer[Logical_Lookup[led]] = 0x0;
  }

  if (led%2==1)
  {
    wait=wait<<1;
  }
  delay(wait);
}

void UpdatePairs(uint8_t wait)
{
  // Flash LEDs in pairs
  uint8_t first_led = Step%12;
  uint8_t second_led = (Step+3)%12;
  for(uint8_t i;i<Num_Leds;i++)
  {
    if(i==first_led || i==second_led)
    {
      Buffer[Logical_Lookup[i]] = On;
    }
    else
    {
      Buffer[Logical_Lookup[i]] = 0x0;
    }
  }
  delay(wait);
}

void UpdateSnake(uint8_t wait)
{
  // Z-Snake
  // Set current LED to full brightness, and each LED to the left
  // half the brightness of the previous
  uint8_t led = Step%12;
  uint8_t j = 0;
  for(uint8_t i=led;i>=0;i--)
  {
      Buffer[Logical_Lookup[i]] = On >> j;
      ++j;
  }
  for(uint8_t i=Num_Leds-1;i>led;i--)
  {
      Buffer[Logical_Lookup[i]] = On >> j;
      ++j;
  }
  delay(wait);
}
