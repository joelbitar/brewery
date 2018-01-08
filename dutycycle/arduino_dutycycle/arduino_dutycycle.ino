const int PIN_READ_POT_PERCENTAGE = 0;
const int PIN_READ_SHIFT_RELAYS = 10;
const int PIN_READ_PID_SELECTOR = 1;
const int MAX_WATTAGE = 9900;

const int MINIMUM_PERCENTAGE = 1;
const int SHIFT_RELEYS_INTERVAL = 1;
const int MIN_FLICKER_LENGTH = 100;
const boolean START_ACTIVE_STATE = true;

// States of the relays
boolean relays[3] = {LOW, LOW, LOW};
// Is set in shiftRelays function
int relay_order[3] = {0, 1, 2};

int calculate_wattage_output;
int val;
int percentage;
int window_length = 500;
int onTime_distribution[3] = {0, 0, 0};
// millisecond, max is window_length;
int frame;
float duty_cycle_factor;

// What pid is currently selected 0 = no pid, 1 = 1, 2 = 2;
int selected_pid;

int previous_frame;
int loop_counter = 0;

// the setup function runs once when you press reset or power the board
void setup() {
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  
  Serial.begin(9600);
}

void shiftRelays(){
  // If shift-relays is set to False, do not shift around relays.
  if(!digitalRead(PIN_READ_SHIFT_RELAYS)){
    for(int i = 0; i < 3; i++)
    {
      relay_order[i] = i;
    }
    
    return;
  }
  
  int first_relay_number = relay_order[0];
  
  for(int i = 0; i < 2; i++)
  {
    relay_order[i] = relay_order[i + 1];
  }
  relay_order[2] = first_relay_number;
}

int getPercentage(){
  int input = 0;
  int pot_value = analogRead(PIN_READ_POT_PERCENTAGE);
  int p;
  
  if(pot_value > 0){
    input = ceil(pot_value);
  }
  
  p = map(
    input,
    0,
    1023,
    MINIMUM_PERCENTAGE,
    100
  );
  
  return p;
  
  if(p >= 31 && p <= 34){
    return 35;
  }
  
  return p;
}

float getFactor(){
  return float(getPercentage()) / 100;
}


int getFrame(){
  return millis() % window_length;
}

void setOnTimeDistribution(){
  float factor = getFactor();
  int onTime;
  
  for(int i = 0; i < 3; i++)
  {
     if(factor < 0.34){
       onTime = float(window_length) * (factor * 3.3);
       
       // Do not engage with the last phase.
       if(i == 2){
         onTime = 0;
       }
     }else{
       onTime = float(window_length) * factor;
     }
     
     if(onTime < window_length && onTime > window_length - MIN_FLICKER_LENGTH){
       onTime = window_length - MIN_FLICKER_LENGTH;
     }
     
     if(onTime > 0 && onTime < MIN_FLICKER_LENGTH){
       onTime = MIN_FLICKER_LENGTH;
     }
     
     if(onTime > window_length){
       onTime = window_length;
     }
     
     Serial.println(String(i) + " - " + String(onTime));
     
     onTime_distribution[i] = onTime;
  }
}

void setRelays(){
  boolean current_relay_state;
  boolean wanted_relay_state;
  
  for(int i = 0; i < 3; i++)
  {
    wanted_relay_state = frame < onTime_distribution[i] ? HIGH : LOW;
    current_relay_state = relays[i];
    
    if(wanted_relay_state != current_relay_state){
      // Set relay state
      relays[i] = wanted_relay_state;
      
      // Toggle output
      digitalWrite(relay_order[i] + 2, wanted_relay_state);
    }
    /*
    if(frame < onTime_distribution[i]){
      digitalWrite(relay_order[i] + 2, HIGH);
    }else{
      digitalWrite(relay_order[i] + 2, LOW);
    }
    */
  }
}

void setCalculatedWattageOutput(){
  calculate_wattage_output = int(ceil(float(MAX_WATTAGE) * duty_cycle_factor));
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
    
    Serial.println(String(onTime_distribution[0]) + "ms");
    Serial.println(String(onTime_distribution[1]) + "ms");
    Serial.println(String(onTime_distribution[2]) + "ms");
    setOnTimeDistribution();
    shiftRelays();
    int percentage = getPercentage();
    Serial.println(String(calculate_wattage_output) + "w");
    Serial.print(percentage);
    Serial.print(" - ");
    Serial.print(frame);
    Serial.print(" - ");
    Serial.print(loop_counter);
    Serial.println("");
    Serial.print("PID selector reading: ");
    Serial.print(analogRead(PIN_READ_PID_SELECTOR));
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
