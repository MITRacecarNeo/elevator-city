#include <Servo.h>

/* Pins */
const int entrance_beam = 0;
const int exit_beam = 1;
const int ready_beam = 2;
const int bottom_beam_1 = 12;
const int bottom_beam_2 = 13;
const int motor_pot = A0;
const int led_red = 3;
const int led_green = 5;
const int led_blue = 6;

// Motor that we are using as a servo, and its params
Servo motor;
const int min_width = 500;
const int max_width = 2500;
const int motor_pwm = 11;

// Current state of the elevator
enum elevator_state {bottom_waiting, ready_to_ascend, ascending, top_waiting, descending, descending_paused};
elevator_state state = descending;

// Speed of the motor, in [-1, 1]
int motor_speed;
const int ascend_speed = 1;
const int descend_speed = -1;

// Color of the RGB indicator, each color in [0, 255]
int* led_color;
const int red[] = {255, 0, 0};
const int yellow[] = {255, 255, 0};
const int green[] = {0, 255, 0};

void setup() {
  // Set up communication with pc
  Serial.begin(9600);

  // Set up pins
  pinMode(entrance_beam, INPUT);
  pinMode(exit_beam, INPUT);
  pinMode(ready_beam, INPUT);
  pinMode(bottom_beam_1, INPUT);
  pinMode(bottom_beam_2, INPUT);
  pinMode(motor_pot, INPUT);
  pinMode(led_red, OUTPUT);
  pinMode(led_green, OUTPUT);
  pinMode(led_blue, OUTPUT);
  motor.attach(motor_pwm, min_width, max_width);
}

void loop() { 
  if (state == bottom_waiting) {
    bottom_waiting_update();
  } else if (state == ready_to_ascend) {
    ready_to_ascend_update();
  } else if (state == ascending) {
    ascending_update();
  } else if (state == top_waiting) {
    top_waiting_update();
  } else {
    descending_update();
  }
  motor.write(speed_to_servo_angle(motor_speed));
  set_led_color(led_color);
}

// Update function called if current state is bottom_waiting
void bottom_waiting_update() {

  // When did we start waiting at the bottom?
  static unsigned long start_time = 0;
  // How many ms to wait for at the bottom
  const unsigned long waiting_amount = 3000; 
  // Initialize timer if necessary, and ascend if timer expired
  unsigned long cur_time = millis();

  if (start_time == 0) {
    start_time = cur_time;
  } else if (cur_time - start_time >= waiting_amount) {
    start_time = 0;
    state = ascending;
    return;
  }

  // If RACECAR get into position, indicate ready to ascend
  if (digitalRead(entrance_beam) == LOW) {
    if (digitalRead(ready_beam) == HIGH) {
      state = ready_to_ascend;
      return;
    }
  // If RACECAR approaches, reset timer
  } else {
    start_time = 0;
  }

  // Wait at the bottom for either the timer to expire, or RACECAR to get into position
  led_color = red;
  motor_speed = 0;
  return;
}

// Update function called if current state is ready_to_ascend
void ready_to_ascend_update() {
    // When did racecar get ready?
    static unsigned long start_time = 0;
    // How many ms to wait
    const unsigned long waiting_amount = 1000;
    // Initialize timer if necessary, and ascend if timer expired
    unsigned long cur_time = millis();

    if (start_time == 0) {
      start_time = cur_time;
    } else if (cur_time - start_time >= waiting_amount) {
      start_time = 0;
      state = ascending;
      return;
    }

    // Else if racecar moves out of the beam at any point, we go back to waiting
    if (digitalRead(ready_beam) == LOW) {
      state = bottom_waiting;
    }
  
    led_color = yellow;
    motor_speed = 0;
}

// Update function called if current state is ascending
void ascending_update() {
  // Stop when we have reached the top
  int pot_value = analogRead(motor_pot);
  if (pot_value /* TODO: figure out when to stop */) {
    state = top_waiting;
  }

  led_color = yellow;
  motor_speed = ascend_speed /* TODO: figure out motor speed*/;
}

// Update function called if current state is top_waiting
void top_waiting_update() {

  // When did we start waiting at the top?
  static unsigned long start_time = 0;
  // How many ms to wait for at the top
  const unsigned long waiting_amount = 3000; 
  // Initialize timer if necessary, and ascend if timer expired
  unsigned long cur_time = millis();

  if (start_time == 0) {
    start_time = cur_time;
  } else if (cur_time - start_time >= waiting_amount) {
    start_time = 0;
    state = descending;
    return;
  }

  // Do not descend if car is still in the way
  if (digitalRead(ready_beam) == HIGH || digitalRead(exit_beam) == HIGH) {
    start_time = 0;
  }

  // Wait at the bottom for either the timer to expire, or RACECAR to get into position
  led_color = green;
  motor_speed = 0;
  return;
}

// Update function called if current state is descending
void descending_update() {
  // Stop when we have reached the bottom
  int pot_value = analogRead(motor_pot);
  if (pot_value /* TODO: figure out when to stop */) {
    state = bottom_waiting;
  }

  // Pause if we are about to crush a fragile and expensive LiDAR
  if (digitalRead(bottom_beam_1) == HIGH || digitalRead(bottom_beam_2) == HIGH) {
    motor_speed = 0;
  } else {
      motor_speed = descend_speed /* TODO: figure out motor speed*/;
  }
  led_color = yellow;
}

/*
* Convert motor speed value in the range [-1, 1] into equivalent servo angle 
* in the range [0, 180] to be written by the Servo library.
* Clamps speed value if it is outside [-1, 1]  
*/
int speed_to_servo_angle(int speed) {
  speed = constrain(speed, -1, 1);
  return map(speed, -1, 1, 0, 180);
}

/*
* Change the color of the indicator LED using the given (R, G, B) array
* Each color value is expected to be in [0, 255]
*/
void set_led_color(int* color) {
  // This is a common anode LED, so the signal needs to be inverted
  analogWrite(led_red, 255 - color[0]);
  analogWrite(led_green, 255 - color[1]);
  analogWrite(led_blue, 255 - color[2]);
}
