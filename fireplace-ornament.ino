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

void DisplaySetup () {
  // ATtiny13A
  // Set up Timer/Counter1 to multiplex the display - 9.6MHz
//  TCCR0A |= 1<<WGM01;             // CTC mode
//  TCCR0B |= 1<<CS01 | 1<<CS00;    // prescaler 64 
//  OCR0A = 24;                     // Divide by 25 (CLK=9600000Hz/64/25 = 6KHz)
//  TIMSK0 |= 1<<OCIE0A;            // Enable overflow interrupt

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
  static uint16_t cycle = 0;
  // Turn off the cathode from the last cycle
  DDRB = DDRB & ~(1<<((cycle % 12)/3));
  // Get the current cycle (0-191)
  // Each LED is updated 16 times every 192 cycles
  cycle = (cycle + 1) & 0xC0;
  // Get the current LED number (0-11) 
  uint8_t led = cycle % 12;
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
uint8_t Num_Modes = 10;
uint16_t Step = 0;

void check_button_state()
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
  check_button_state();
  switch (Mode)
  {
    case 0:
    case 1:
    case 2:
    case 3:
      updateTriangleWave(50*(Mode+1));
      break;
    case 4:
    case 5:
    case 6:
    case 7:
      updateSquareWave(50*((Mode+1)%4));
      break;
    default:
      break;
  }

  delay(200);
  
  Step++;
  // Prevent overflow
  Step = Step % 32768;
}

void updateTriangleWave(uint8_t wait)
{ 
  // Parameterised fade in/out
  // Adjust p for different period (m*p)
  // Adjust s for different start time
  static uint8_t m = 15;
  static uint8_t p = 1;
  static uint8_t s = 0;
  for(uint8_t i;i<12;i++)
  {
    Buffer[i] = m - abs(((s + Step/p) % (2*m)) - m);
  }
  delay(wait);
}

void updateSquareWave(uint8_t wait)
{
  // Parameterised flash on/off
  // Adjust d for different duty cycle
  // Adjust p for different period
  // Adjust s for different start time
  static uint8_t m = 15;
  static uint8_t d = 8;
  static uint8_t p = 1;
  static uint8_t s = 0;
  for(uint8_t i;i<12;i++)
  {
    if (((s + Step/p) % m) < d)
    {
      Buffer[i] = 0x0;
    }
    else
    {
      Buffer[i] = m;
    }
  }
  delay(wait);
}

void updateChase(uint8_t wait)
{
  
}

const uint8_t Physical_Lookup[12] PROGMEM = {
  0x3, 0x0, 0x7, 0x4, 0x6, 0x1, 0xB, 0x8, 0x0A, 0x5, 0x9, 0x2};


//static void charlie(uint8_t var)
//{
//  uint8_t anode = pgm_read_byte(&Physical_Lookup[var]) % 3;
//  uint8_t cathode = var/3;
//  anode = anode + (anode & 0x07<<cathode);
//  
//  DDRB = (DDRB & 0xF0) | anode;
//  PORTB = (PORTB & 0xF0) | anode;
//  DDRB = DDRB | 1<<cathode;
//}
