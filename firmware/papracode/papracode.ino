/*
  papracode
    - Reads battery voltage and lights up LEDs to mimic M12 battery fuel gauge
    - Reads user pot to control PWM of fan
  to do
    - look at averaging battery voltage
    - set a MIN PWM value for fan output, vary with battery level
  current code flash: 2072/4096 bytes (50%) RAM: 30/256 (11%) bytes without debug enabled
               flash: 3757/4096 bytes (91%) RAM: 93/256 (36%) with debug enabled  
 */

//be sure to go to Arduino >> tools >> Support SerialEvent "No(saves space)" or "yes"
//serial debug takes up about 1300 bytes of flash, out of 4096 available, not including print statements
//#define DEBUG_SERIAL   //uncomment line to enable serial debug

// Pin connections per PAPR V0.1 PCB
// PA0 - UPDI/RESET
// PA1 - ADC - BATTERY
// PA2 - ADC = POTENTIOMETER
// PA3 - PWM - FAN
// PA4 - FAN TACH INPUT
// PA5 - LED1
// PA6 - LED2
// PA7 - LED3
// PB0 - UART RX
// PB1 - UART TX
// PB2 - NC
// PB3 - LED4

// Battery Voltage to Fuel Gauge
// > 4.12V/Cell = 100% - 78% Battery  = 4 LEDs         => 960 > adc > 920
// > 3.95V/Cell =  77% - 55% Battery  = 3 LEDs         => 920 > adc > 862
// > 3.70V/Cell =  54% - 33% Battery  = 2 LEDs         => 862 > adc > 825
// > 3.54V/Cell =  32% - 10% Battery  = 1 LEDs         => 825 > adc > 757
// > 3.25V/Cell =  10% - 0%  Battery  = LEDS FLASHING  => 757 > adc > 652
// > 2.80V/Cell =  0%        Battery  = Shut Down

//Limits the min pot value     Voltage   Min PWM Percentage
const int minPWMfull =  69;  //11.85V      69     >77%
const int minPWM75p  =  72;  //11.10V      72     >54%
const int minPWM50p  =  77;  //10.62V      77     >33%
const int minPWM25p  =  88;  //9.75V       88     >10%
const int minPWM10p  = 120;  //8.40V      110     >0%
//These values were determined emperically by adjusting the input voltage and dialing the PWM down until motor stall

//State Machine states
const int batteryCheck = 7;
const int batteryFull  = 6;
const int battery75p   = 5;
const int battery50p   = 4;
const int battery25p   = 3;
const int battery10p   = 2;
const int batteryDead  = 1;

int batteryState = batteryCheck;

int led1 = PIN_PA5;
int led2 = PIN_PA6;
int led3 = PIN_PA7;
int led4 = PIN_PB0;
int analogBatt = PIN_A1; //10 bit resolution on ADC 
int analogPot = PIN_A2; 
int PWMPin = PIN_PA3;    //8 bit resolution with Arduino AnalogWrite
int offTime = 5; //ms
int onTime = 95;   //ms
int loopDelay = 25; //ms
int blinkCounter = 0;
const uint32_t numBatterySamples = 10;
uint32_t battery = 1023;
const uint32_t maxPot = 1023;
uint32_t speedPot = 0;
uint32_t minPWM = minPWM10p;
uint32_t maxPWM = 255;
uint32_t rawADC = 0;

const int LEDFlashLoop  = 25; //decrease for 10% battery LED to blink faster

// the setup routine runs once when you press reset:
void setup() {
  // initialize the digital pin as an output.
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(led3, OUTPUT);
  pinMode(led4, OUTPUT);
  pinMode(PWMPin, OUTPUT);

  digitalWrite(led1, HIGH);
  digitalWrite(led2, HIGH);
  digitalWrite(led3, HIGH);
  digitalWrite(led4, HIGH);
  digitalWrite(PWMPin, maxPWM); //Turn on fan 100%
#ifdef DEBUG_SERIAL
  Serial.begin(115200);
  Serial.println("Starting up");
#endif
  
  for (int i = 0; i <= 4; i++) { //startup knightrider 
    digitalWrite(led1, LOW);  delay(onTime);
    digitalWrite(led1, HIGH); 
    digitalWrite(led2, LOW);  delay(onTime);
    digitalWrite(led2, HIGH); 
    digitalWrite(led3, LOW);  delay(onTime);
    digitalWrite(led3, HIGH); 
    digitalWrite(led4, LOW);  delay(onTime); delay(onTime);
    digitalWrite(led4, HIGH); 
    digitalWrite(led3, LOW);  delay(onTime);
    digitalWrite(led3, HIGH); 
    digitalWrite(led2, LOW);  delay(onTime);
    digitalWrite(led2, HIGH); 
    digitalWrite(led1, LOW);  delay(onTime);
    digitalWrite(led1, HIGH); delay(offTime);
  }
}

// the loop routine runs over and over again forever:
void loop() {
  delay(25);
  //Clamp and rescale potentiometer input from a 10bit value to 8 bit
  if (maxPWM > 0) {
    rawADC = analogRead(analogPot);
    speedPot = ( ( ( maxPWM - minPWM ) * rawADC ) / maxPot ) + minPWM;
  } else {
    speedPot = maxPWM;
  }  
  analogWrite(PWMPin, speedPot);
  battery = battery = ( ( battery * ( numBatterySamples - 1 ) ) + analogRead(analogBatt) ) / numBatterySamples;
  switch (battery) {
    case 925 ... 1023: // Full = 78% - 100%
      if (batteryState > batteryFull) {
        batteryState = batteryFull;
        digitalWrite(led1, LOW);  //All LEDs ON
        digitalWrite(led2, LOW);
        digitalWrite(led3, LOW);
        digitalWrite(led4, LOW);
        minPWM = minPWMfull;
      }
      break;
    case 867 ... 924: // 75% = 55% - 77%
      if (batteryState > battery75p) {
        batteryState = battery75p;
        digitalWrite(led1, LOW);  //3 LEDs ON
        digitalWrite(led2, LOW);
        digitalWrite(led3, LOW);
        digitalWrite(led4, HIGH);
        minPWM = minPWM75p;
      }
      break;
    case 830 ... 866: // 50% = 33% - 54%
      if (batteryState > battery50p) {
        batteryState = battery50p;
        digitalWrite(led1, LOW);  //2 LEDs ON
        digitalWrite(led2, LOW);
        digitalWrite(led3, HIGH);
        digitalWrite(led4, HIGH);
        minPWM = minPWM50p;
      }
      break;
    case 762 ... 829: // 25% = 10% - 32%
      if (batteryState > battery25p) {
        batteryState = battery25p;
        digitalWrite(led1, LOW);  //1 LED on
        digitalWrite(led2, HIGH);
        digitalWrite(led3, HIGH);
        digitalWrite(led4, HIGH);
        minPWM = minPWM25p;
      }
      break;
    case 657 ... 761: // 10% - Need to blink LED
      if (batteryState > battery10p) {
        batteryState = battery10p;
        digitalWrite(led2, HIGH); //1 LED blinking
        digitalWrite(led3, HIGH);
        digitalWrite(led4, HIGH);
        digitalWrite(led1, LOW);
        minPWM = minPWM10p;
      }
      break;
    case 0 ... 652: // Shutdown
      if (batteryState > batteryDead) {
        batteryState = batteryDead;
        digitalWrite(led1, HIGH); //All LEDs off
        digitalWrite(led2, HIGH);
        digitalWrite(led3, HIGH);
        digitalWrite(led4, HIGH);
        maxPWM = 0; // turn off fan - overrides the measuremnt from above
      }
      break;
  }
  // For battery state of 0-10%, need to blink LED1 to indicate almost dead battery
  if (batteryState == battery10p){
    if (blinkCounter++ > LEDFlashLoop) {
      digitalWrite(led1, !digitalRead(led1));
      blinkCounter = 0;
    }
  }  
#ifdef DEBUG_SERIAL
  Serial.print(" Battery = ");      Serial.print(battery);
  Serial.print(" rawADC =");        Serial.print(rawADC);
  Serial.print(" pot =");           Serial.print(speedPot);
  Serial.print(" Battery State ="); Serial.println(batteryState);
  digitalWrite(PIN_PB1,!digitalRead(PIN_PB1)); //Toggle unused pin for debugging with scope 
#endif
}
