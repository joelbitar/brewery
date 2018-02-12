const int PIN_READ_POT_PERCENTAGE = A0;
const int PIN_READ_SHIFT_RELAYS = 13;
const int PIN_READ_MASTER_SWITCH = A5;

const int SHIFT_RELEYS_INTERVAL = 1;
const int MIN_FLICKER_LENGTH = 20; // one cycle at 50Hz is 10ms,
const int MIN_ON_TIME = MIN_FLICKER_LENGTH + 400; // Set to at least min flicker time.
const boolean START_ACTIVE_STATE = true;
const int MAX_WATTAGE = 7200;

// Is set in shiftRelays function
const int RELAY_PINS[3] = {
  6,
  7,
  8
};

int relays[3] = {RELAY_PINS[0], RELAY_PINS[1], RELAY_PINS[2]};
int relay_indexes[3] = {0, 1, 2};

boolean relay_states[3] = {false, false, false};
String display_output[2] = {"", ""};

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
      relay_indexes[i] = i;
    }
    
    return;
  }
  
  int first_relay_index = relay_indexes[0];
  
  for(int i = 0; i < 2; i++)
  {
    relay_indexes[i] = relay_indexes[i + 1];
  }
  relay_indexes[2] = first_relay_index;
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
    1020,
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
  boolean wanted_state;
  
  // If the master-on-switch is Off, reset on time distribution, should only be set at frame 0 to avoid fast-switching of relays.
  if(!digitalRead(PIN_READ_MASTER_SWITCH) == false){
     resetOnTimeDistribution();
  }
  
  for(int i = 0; i < 3; i++)
  {
    wanted_state = frame < onTime_distribution[i] ? HIGH : LOW;

    if(wanted_state == relay_states[relay_indexes[i]]){
      continue;
    }

    relay_states[relay_indexes[i]] = wanted_state;
    
    digitalWrite(
      RELAY_PINS[relay_indexes[i]],
      wanted_state
    );
  }
}

void setDisplayOutput(){
  String line_one;
  String line_two;
  
  String percent_string;
  String wattage_string;
  int percent_on;
  int wattage;

  String relay_one;
  String relay_two;
  String relay_three;
  
  percent_on = map(
    getTotalOnTimeToDistribute(),
    0,
    getMaxOnTime(),
    0,
    100
  );
  
  wattage = map(
    percent_on,
    0,
    100,
    0,
    MAX_WATTAGE
  );

  percent_string = String(percent_on) + "%";
  wattage_string = String(wattage) + " watt";

  line_one = wattage_string;
  
  while((line_one.length() + percent_string.length()) < 16){
    line_one = line_one + " ";
  }

  line_one = line_one + percent_string;

  // Line two
  relay_one   = String(map(onTime_distribution[relay_indexes[0]], 0, window_length, 0, 100)) + "%";
  relay_two   = String(map(onTime_distribution[relay_indexes[1]], 0, window_length, 0, 100)) + "%";
  relay_three = String(map(onTime_distribution[relay_indexes[2]], 0, window_length, 0, 100)) + "%";

  if(relay_two.length() <= 2){
    relay_two = " " + relay_two;
  }
  
  while(relay_one.length() < 6){
    relay_one = relay_one + " ";
  }

  line_two = relay_one + relay_two;

  while((line_two.length() + relay_three.length()) < 16){
    line_two = line_two + " ";
  }

  line_two = line_two + relay_three;
  

  display_output[0] = line_one;
  display_output[1] = line_two;
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
    setDisplayOutput();
    
    Serial.println(String(onTime_distribution[0]) + "ms");
    Serial.println(String(onTime_distribution[1]) + "ms");
    Serial.println(String(onTime_distribution[2]) + "ms");


    Serial.println("releys[0]: " + String(relays[0]));
    Serial.println("releys[1]: " + String(relays[1]));
    Serial.println("releys[2]: " + String(relays[2]));

    Serial.println("Display: ");
    Serial.println(display_output[0]);
    Serial.println(display_output[1]);
    
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
