
void setup() {
  for (int pin=6; pin<=13; pin++)
  {
    digitalWrite(pin, 0);
    pinMode(pin, OUTPUT);
  }
}

void loop() {
  for (int pin=6; pin<=13; pin++)
  {
    digitalWrite(pin, 1);
    delay(120);
    digitalWrite(pin, 0);
  }  
}
