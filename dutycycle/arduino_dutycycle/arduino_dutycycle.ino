const int PIN_READ_POT_PERCENTAGE = A0;
const int PIN_READ_SHIFT_RELAYS = 2;
const int PIN_READ_MASTER_SWITCH = A5;

const int SHIFT_RELEYS_INTERVAL = 1;
const int MIN_FLICKER_LENGTH = 20; // one cycle at 50Hz is 10ms,
const int MIN_ON_TIME = 350; // Set to at least min flicker time.
const boolean START_ACTIVE_STATE = true;

// Is set in shiftRelays function
const int RELAY_PINS[3] = {A1, A2, A4};
int relays[3] = {RELAY_PINS[0], RELAY_PINS[1], RELAY_PINS[2]};

int val;
int percentage;
int window_length = 500;
int onTime_distribution[3] = {0, 0, 0};
// millisecond, max is window_length;
int frame;

// What pid is currently selected 0 = no pid, 1 = 1, 2 = 2;
int selected_pid;

int previous_frame;
int loop_counter = 0;

// the setup function runs once when you press reset or power the board
void setup() {
  
  pinMode(relays[0], OUTPUT);
  pinMode(relays[1], OUTPUT);
  pinMode(relays[2], OUTPUT);
  pinMode(PIN_READ_MASTER_SWITCH, INPUT);
  pinMode(PIN_READ_SHIFT_RELAYS, INPUT);
  
  Serial.begin(9600);
}

void shiftRelays(){
  // If shift-relays is set to False, do not shift around relays.
  if(!digitalRead(PIN_READ_SHIFT_RELAYS)){
    for(int i = 0; i < 3; i++)
    {
      relays[i] = RELAY_PINS[i];
    }
    
    return;
  }
  
  int first_relay_pin = relays[0];
  
  for(int i = 0; i < 2; i++)
  {
    relays[i] = relays[i + 1];
  }
  relays[2] = first_relay_pin;
}

int getFrame(){
  return millis() % window_length;
}

void resetOnTimeDistribution(){
  for(int i = 0; i < 3; i++)
  {
    onTime_distribution[i] = 0;
  }
}

int getMaxOnTime(){
  return window_length * 3;
}

int getMinOnTime(){
  return MIN_ON_TIME;
}

int getTotalOnTimeToDistribute(){
  int input = 0;
  int pot_value = analogRead(PIN_READ_POT_PERCENTAGE);
  int p;
  
  if(pot_value > 0){
    input = ceil(pot_value);
  }

  Serial.println("POT: " + String(input));
  
  return map(
    input,
    0,
    1015,
    getMinOnTime(),
    getMaxOnTime()
  );
}

void setOnTimeDistribution(){
  int onTimeToDistribute = getTotalOnTimeToDistribute();
  int onTime;
  
  Serial.println("Total time to distribute: " + String(onTimeToDistribute));
  
  for(int i = 0; i < 3; i++)
  {
     onTime = onTimeToDistribute;

     // If onTime is greater than the length of the window, set the maximum amout.
     // This happend every time we are at above 33%
     if(onTime > window_length){
       onTime = window_length;
     }

     // If we are at a ontime that the gap at the end is Less than the flicker-length
     // We need to set the off-time to the flicker-length
     // Example:
     // Max flicker lenght is 100
     // Window length is 1000
     // On Time is 950,
     // Off time is then 50 (less than flicker lenght)
     // Then we round down and set onTime to 900.
     if(onTime < window_length && onTime > window_length - MIN_FLICKER_LENGTH){
       onTime = window_length - MIN_FLICKER_LENGTH;
     }

     // onTime can Not be less than flicker time.
     if(onTime < MIN_FLICKER_LENGTH){
       onTime = 0;
     }

     onTimeToDistribute = onTimeToDistribute - onTime;
     
     Serial.println(String(i) + " - " + String(onTime));
     
     onTime_distribution[i] = onTime;
  }
}

void setRelays(){
  // If the master-on-switch is Off, reset on time distribution, should only be set at frame 0 to avoid fast-switching of relays.
  if(!digitalRead(PIN_READ_MASTER_SWITCH) == false){
     resetOnTimeDistribution();
  }
  
  for(int i = 0; i < 3; i++)
  {
    digitalWrite(
      relays[i],
      frame < onTime_distribution[i] ? HIGH : LOW
    );
  }
}

int getDisplayOutput(){
  return map(
    getTotalOnTimeToDistribute(),
    getMinOnTime(),
    getMaxOnTime(),
    0,
    9
  );
}

// the loop function runs over and over again forever
void loop() {
  frame = getFrame();
  // Never start at a frame higher than 0
  if(previous_frame < 0 && frame > 0){
    return;
  }
  
  // Do not run on the same frame twice
  if(previous_frame == frame){
    return;
  }
  
  // Do all calculations and whatnot at the first frame.
  if(frame == 0){
    
    setOnTimeDistribution();
    shiftRelays();
    
    Serial.println(String(onTime_distribution[0]) + "ms");
    Serial.println(String(onTime_distribution[1]) + "ms");
    Serial.println(String(onTime_distribution[2]) + "ms");


    Serial.println("releys[0]: " + String(relays[0]));
    Serial.println("releys[1]: " + String(relays[1]));
    Serial.println("releys[2]: " + String(relays[2]));

    Serial.print("Display: ");
    Serial.println(getDisplayOutput());
    
    Serial.print(" - ");
    Serial.print(frame);
    Serial.print(" - ");
    Serial.print(loop_counter);
    Serial.println("");
    Serial.print("Master switch reading: ");
    Serial.print(!digitalRead(PIN_READ_MASTER_SWITCH));
    Serial.println();
    Serial.println("--------------------------------------");
    Serial.println();
  }
  
  // Increment loop_counter
  if(previous_frame > frame){
    loop_counter += 1;
  }
  
  setRelays();
  
  previous_frame = frame;
}
