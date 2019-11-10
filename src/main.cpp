#include <Arduino.h>
// Muokattu 8.11.2019 melkein toimii. Ajastin ei pysahdy nollaan,ruutu vilkkuu ja menee mustaksi tulosta lisatessä/vahentaessa.
// kello heittaa
#include <PxMatrix.h>
// pins for led display
#define P_LAT 22
#define P_A 19
#define P_B 23
#define P_C 18
#define P_D 5
#define P_E 15
#define P_OE 2
hw_timer_t * timer = NULL;
hw_timer_t * timer2 = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

#define upPin 25
#define downPin 26
#define resetPin 27


#define matrix_width 64 
#define matrix_height 32


// This defines the 'on' time of the display is us. The larger this number,
// the brighter the display. If too large the ESP will crash
uint8_t display_draw_time=20; //10-50 is usually fine
PxMATRIX display(64,32,P_LAT, P_OE,P_A,P_B,P_C,P_D);

int resultCounter = 0;
int countDownSeconds = 0;
int timerOptionA = 600;
int timerOptionB = 1800;
int timerOptionC = 3600;

int sessionTimer = 0;

int stateMachine = 0;

void IRAM_ATTR display_updater()
{
  // Increment the counter and set the time of ISR
  portENTER_CRITICAL_ISR(&timerMux);
  display.display(display_draw_time);
  portEXIT_CRITICAL_ISR(&timerMux);
  //Serial.print("displayUpd");
}

void display_update_enable(bool is_enable)
{
  if (is_enable)
  {
    timer = timerBegin(0, 80, true);
    timerAttachInterrupt(timer, &display_updater, true);
    timerAlarmWrite(timer, 2000, true);
    timerAlarmEnable(timer);
  }
  else
  {
    timerDetachInterrupt(timer);
    timerAlarmDisable(timer);
  }
}



 // Defines the speed of the SPI bus (reducing this may help if you experience noisy images)
 //#define PxMATRIX_SPI_FREQEUNCY 20000000
 
void clearResultScreen()
{
  display.fillRect(0, 0, display.width(), 16, display.color565(0, 0, 0));
}
void clearTimerScreen()
{
  display.fillRect(0, 17, display.width(), 32, display.color565(0,0,0));
}
void clearScreen()
{
  display.fillRect(0,0, display.width(), display.height(), display.color565(0,0,0));
}


void printTime(){
  if (countDownSeconds > 0){
      display.setCursor(0,17);
      display.setTextSize(2);
      display.setTextColor(display.color565(0,0,200));
      display.print(String(countDownSeconds / 60));
      display.print(':');
      if (countDownSeconds % 60 < 10){
        display.print('0');
      }

      display.print(String(countDownSeconds % 60));
  }
  else {
      display.setCursor(0,17);
      display.setTextSize(2);
      display.setTextColor(display.color565(200,0,0));
      display.print("STOP");
      countDownSeconds = 0;
  }
 }



void addResult() {
    static unsigned long last_interrupt_time = 0;
    unsigned long interrupt_time = millis();
    if (interrupt_time - last_interrupt_time > 500){
      if(stateMachine == 2){
        resultCounter++;
        clearResultScreen();
        display.setTextColor(display.color565(200,0,0));
        display.setCursor(0, 0);
        display.setTextSize(2);
        display.print(String(resultCounter));
      }
      else{
        countDownSeconds = timerOptionA;
        sessionTimer = timerOptionA;
        stateMachine = 1;
       clearScreen();
      }
    }
    last_interrupt_time = interrupt_time;
}
void removeResult(){
    static unsigned long last_interrupt_time = 0;
    unsigned long interrupt_time = millis();
    if (interrupt_time - last_interrupt_time > 500){
      if(stateMachine == 2){
        resultCounter--;
        clearResultScreen();
        display.setTextColor(display.color565(200,0,0));
        display.setCursor(0, 0);
        display.setTextSize(2);
        display.print(String(resultCounter));
      }
      else{
        countDownSeconds = timerOptionB;
        sessionTimer = timerOptionB;
        stateMachine = 1;
        clearScreen();
      }
    }
    last_interrupt_time = interrupt_time;
}
void resetResults(){
    static unsigned long last_interrupt_time = 0;
    unsigned long interrupt_time = millis();
    if (interrupt_time - last_interrupt_time > 500){
      if ((stateMachine == 1) || (stateMachine == 2)){
        resultCounter = 0;
        countDownSeconds = sessionTimer;
       clearScreen();
        printTime();
        display.setTextColor(display.color565(0,0,200));
        display.setCursor(0, 0);
        display.setTextSize(2);
        display.print(String(resultCounter));
        if(stateMachine == 2){
          stateMachine = 1;
        }
        else{
          stateMachine = 2;
           timerAlarmEnable(timer2);
        }
      }
      else{
        countDownSeconds = timerOptionC;
        sessionTimer = timerOptionC;
        stateMachine = 1;
        
       clearScreen();
      }
    }
    last_interrupt_time = interrupt_time;
}

void countdownTimerFunc(){
    //noInterrupts();
    clearTimerScreen();
    printTime();
    countDownSeconds--;
    //interrupts();
    Serial.print("i am here");
}

void setup() {

  disableCore0WDT();
  disableCore1WDT();


  Serial.begin(115200);
  display.begin(16);
  display_update_enable(true);
  display.setFastUpdate(true);
  // Set the number of panels that make up the display area width (default is 1)
  //display.setPanelsWidth(1);
  // Set the brightness of the panels (default is 255)
  //display.setBrightness(50);
  //display.setFastUpdate(true);  
  timer2 = timerBegin(1, 80, true);
  timerAttachInterrupt(timer2, &countdownTimerFunc, true);
  timerAlarmWrite(timer2, 1000000, true);


  Serial.print(stateMachine);

  pinMode(upPin, INPUT_PULLUP);
  pinMode(downPin, INPUT_PULLUP);
  pinMode(resetPin, INPUT_PULLUP  );

  attachInterrupt(digitalPinToInterrupt(upPin), addResult, FALLING);
  attachInterrupt(digitalPinToInterrupt(downPin), removeResult, FALLING);
  attachInterrupt(digitalPinToInterrupt(resetPin), resetResults, FALLING);


    // fill the screen with 'black'
  display.fillScreen(display.color565(0, 0, 0));

  display.setCursor(0,0);
  display.setTextColor(display.color565(255,0,50));
  display.setTextSize(1);
  display.print("Set timer");

  display.setCursor(0,17);
  display.setTextColor(display.color565(0,0,255));
  display.print("A)");

  display.setCursor(21,17);
  display.setTextColor(display.color565(0,0,255));
  display.print("B)");

  display.setCursor(42,17);
  display.setTextColor(display.color565(0,0,255));
  display.print("C)");
}

void loop() {
  if(stateMachine == 2){
    Serial.print(stateMachine);
    


  }
  else if(stateMachine == 1){
    Serial.print(stateMachine);
    timerAlarmDisable(timer2);
    clearScreen();
    display.setCursor(0,0);
    display.setTextColor(display.color565(0,255,0));
    display.setTextSize(1);
    display.print("Press:");
    display.setCursor(0,17);
    display.print("Reset");
    while (stateMachine == 1)
    {
      delay(1000);
      Serial.print(stateMachine);
    }


  }
  //Serial.print(stateMachine);
  delay(100);
}