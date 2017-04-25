
//const char *string_11[1] PROGMEM = { "AT+HTTPTERM" };
#define MOTOR_PIN 11
void setup()
{
	Serial.begin(9600);	
	pinMode(MOTOR_PIN, OUTPUT);

}
void loop() {



	



	digitalWrite(MOTOR_PIN, HIGH);
	delay(3000);
	digitalWrite(MOTOR_PIN, LOW);
	delay(3000);

}

