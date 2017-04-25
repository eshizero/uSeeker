#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <avr/pgmspace.h>

#define Version 0.9
//0.1 primera version funcional
//0.2 se agrega las funciones vibrar y led
//0.3 se eliminan lineas q no se usan y se agrega fecha de compilacion y hora,se agrega proteccion a seleccionar 2 compa�ias a la vez
//se corrigen porblemas del gps q no actualizaba los datos de la posicion 
//0.4 se agrega un caracter de salida * para hacer mas rapido el codigo en la funcion leer_GPRS
//0.5 se cambia la forma en que funciona los led y el motor
//0.6 se corrigue que los leds funcionaban de manera erratica
//0.7 Se modifica la respuesta de los leds y se investiga el error de cambio entre la posicion mas lejana y ningun lugar de interes
//0.8 Se modifica el bug que ocurre cuando se solapan 2 lugares de interes y cambia de color pero no vibra (Funcion vibrar() linea 350)
//    Se acorta el tiempo a 5 segundos del loop()
//0.9 HTTPACTION ahora tiene un papel mucho mas decisivo , si no llega una  codigo 200 entonces falla, se agrego el error 2


//agregar
//cuando la bateria este al 10% de su carga debe avisar al usuario mediante led y vibracion y no debe funcionar el GPS y el GPRS
//cuando supere el 30% deb volver a funcionar todo correctamente, se debe dormir el GPS y el GSM
//verificador de sim

//pines usados por los modulos
#define GPRS_Tx 7
#define GPRS_Rx  6
#define GPS_Tx  5
#define GPS_Rx 4
#define GPRS_Reset 11
#define BAUD_GPRS 9600
#define BAUD_GPS 9600
#define BAUD_Serial 115200

#define MOTOR_PIN 11
#define LED_R_PIN 3
#define LED_G_PIN 9
#define LED_B_PIN 10
//eleccion de chip a usar
//solo debe estar un define activo a la vez, el resto debe ir comentado
//#define MOVISTAR 
#define WOM 
//#define SIMPLE 
//#define DEBUG_GPS //si se comenta esta linea el gps no muestra nada

#define ID 1
#if defined(MOVISTAR) && defined(SIMPLE)
#error MOVISTAR y SIMPLE seleccionados
#endif 

#if defined(WOM) && defined(SIMPLE)
#error WOM y SIMPLE seleccionados
#endif 

#if defined(MOVISTAR) && defined(WOM)
#error MOVISTAR y WOM seleccionados
#endif 


#ifdef MOVISTAR
//#define apn     ((const  char *)"wap.tmovil.cl")
//#define user     ((const  char *)"wap")
//#define pass     ((const  char *)"wap")
#define compania  "AT+SAPBR=3,1,\"APN\",\"internet\""
#define PWD		"AT+SAPBR=3,1,\"USER\",\"wap\""
#define USER	"AT+SAPBR=3,1,\"PWD\",\"wap\""
#endif

#ifdef WOM
//#define apn     ((const  char *)"internet")
//#define user     ((const  char *)"")
//#define pass     ((const  char *)"")
//AT+SAPBR=3,1,"APN","internet"
#define compania  "AT+SAPBR=3,1,\"APN\",\"internet\""
#endif
#ifdef SIMPLE
//#define apn     ((const  char *)"simple.internet")
//#define user     ((const  char *)"")
//#define pass     ((const  char *)"")
#define compania  "AT+SAPBR=3,1,\"APN\",\"simple.internet\""

#endif
TinyGPSPlus gps;
SoftwareSerial gprsSerial(GPRS_Rx, GPRS_Tx);
SoftwareSerial GPSss(GPS_Rx, GPS_Tx);



volatile unsigned long previousMillis = 0, previousMillis_led = 0;
float distancia, distancia_anterior;
int iniciar_altiro = 1;
volatile int valor_anterior_vibracion = 0;
volatile int valor_anterior_color = 0;
volatile int pps_led = 5;
volatile int color = 4;
volatile int Vibrar = 5;
volatile bool estado = 0;
volatile bool bateria_cargada = 0;
int esperando=0;



void setup()
{
	gprsSerial.begin(BAUD_GPRS);
	GPSss.begin(BAUD_GPS);
	Serial.begin(BAUD_Serial);
	delay(100);
	Serial.println(F("iniciando, esperar 10 seg"));
	Serial.print(F("version compilacion "));
	Serial.println(Version);
	Serial.print(F("compilada en: "));
	Serial.print(__DATE__);
	Serial.print(F(" "));
	Serial.println(__TIME__);

	pinMode(LED_BUILTIN, OUTPUT);
	pinMode(LED_R_PIN, OUTPUT);
	pinMode(LED_G_PIN, OUTPUT);
	pinMode(LED_B_PIN, OUTPUT);
	pinMode(MOTOR_PIN, OUTPUT);

	test_led_mot();
	while (!iniciar_modem())//primero prueba si existe comunicacion con el modem y configura su apn
	{
		Serial.println(F("inicio de modem fail"));
		delay(5000);
	}

	// prueba si los led y el motor estan funcionando correctamente, el usuario debe comprobarlo 
	//iniciar_modem();


}

void loop()
{
   
	if (millis() - previousMillis >= 100 || iniciar_altiro) {//cada 10 segundos se realiza la magia
		iniciar_altiro = 0;
    esperando=0;
		int posicion_valida = obtner_posicion();//primeo se obtiene la pos
		if (posicion_valida)
		{
			pps();
			posicion_valida = 0;

			switch (conectar_server())
			{
			case 0:
				Serial.println(F("GPRS fail se intentara nuevamente"));
				break;
			case 1:
				Serial.println(F("EXITO"));
				break;
			case 2:
				Serial.println(F("ERROR de HTTP"));
				Serial.println(F("verificar saldo"));
				Serial.println(F("verificar señal"));
				Serial.println(F("verificar servidor"));				
				break;
			default:

				break;
			}
			
		}else{

        Serial.println(F("GPS fail se intentara 10 seg mas adelante"));      
		}
   


		previousMillis = millis();
	}
	pps();
	
}

boolean obtner_posicion(void)
{
	GPSss.listen();
	unsigned long anterior = millis();
	do
	{
		pps();

		while (GPSss.available() > 0)
		{

			char c = GPSss.read();
			gps.encode(c);
#ifdef DEBUG_GPS
			Serial.write(c);
#endif
		}

		if (gps.location.isUpdated())
		{
			if (gps.location.isValid())
			{
				Serial.println(gps.location.lat(), 6);
				Serial.println(gps.location.lng(), 6);
				return 1;
			}
		}

	} while (((millis() - anterior) < 5000));
	return 0;
}

int8_t sendATcommand(const char* ATcommand, const char* expected_answer, unsigned int timeout)
{
	uint8_t x = 0, answer = 0;
	static uint8_t tamano_response = 100;
	char response[tamano_response];
	unsigned long previous;
	pps();
	gprsSerial.listen();
	memset(response, '\0', tamano_response);    // Initialice the string	
	delay(100);//verificar que pasa al eliminarlo
	while (gprsSerial.available() > 0) {
		gprsSerial.read();
	}   // Clean the input buffer

	if (ATcommand[0] != '\0')
	{
		gprsSerial.write(ATcommand);
		gprsSerial.write("\r\n");
		Serial.print(ATcommand);
		Serial.print(" ");
		// Send the AT command
	}
	//Serial.println(F(" sent!"));
	x = 0;
	previous = millis();

	do
	{

		if (gprsSerial.available() != 0)
		{

			response[x] = gprsSerial.read();
			//Serial.write(response[x]);
			x++;
			if (strstr(response, expected_answer) != NULL)    // check if the desired answer (OK) is in the response of the module
			{
				Serial.println(F(" Send ok"));
				delay(10);
				answer = 1;
				return answer;
			}

		}
		pps();
	} while ((answer == 0) && ((millis() - previous) < timeout));    // Waits for the asnwer with time out

	if (millis() - previous > timeout)
	{
		Serial.println(F(" T out"));
	}
	else
	{
		//Serial.println(F("GPRS retorno ERROR"));
	}

	return answer;
}
int8_t sendATcommand(const char* ATcommand, const char* expected_answer, unsigned int timeout,bool detalle)
{
	uint8_t x = 0, answer = 0;
	static uint8_t tamano_response = 100;
	char response[tamano_response];
	unsigned long previous;
	pps();
	gprsSerial.listen();
	memset(response, '\0', tamano_response);    // Initialice the string	
	delay(100);//verificar que pasa al eliminarlo
	while (gprsSerial.available() > 0) {
		gprsSerial.read();
	}   // Clean the input buffer

	if (ATcommand[0] != '\0')
	{
		gprsSerial.write(ATcommand);
		gprsSerial.write("\r\n");
		Serial.print(ATcommand);
		Serial.print(" ");
		// Send the AT command
	}
	//Serial.println(F(" sent!"));
	x = 0;
	previous = millis();

	do
	{

		if (gprsSerial.available() != 0)
		{

			response[x] = gprsSerial.read();
			Serial.write(response[x]);
			x++;
			if (strstr(response, expected_answer) != NULL)    // check if the desired answer (OK) is in the response of the module
			{
				Serial.println(F(" Send ok"));
				delay(10);
				answer = 1;
				return answer;
			}

		}
		pps();
	} while ((answer == 0) && ((millis() - previous) < timeout));    // Waits for the asnwer with time out

	if (millis() - previous > timeout)
	{
		Serial.println(F(" T out"));
	}
	else
	{
		//Serial.println(F("GPRS retorno ERROR"));
	}

	return answer;
}
boolean iniciar_modem(void) {
	const char string_0[] PROGMEM = "AT";
	//const char string_1[] PROGMEM = "AT+CPIN?";
	const char string_2[] PROGMEM = "AT+CREG?";
	const char string_3[] PROGMEM = "AT+CGATT?";
	const char string_4[] PROGMEM = "AT+SAPBR=3,1,\"Contype\",\"GPRS\"";
	const char string_5[] PROGMEM = compania;
	const char string_6[] PROGMEM = "AT+SAPBR=1,1";
	const char string_12[] PROGMEM = "OK";

#if defined(MOVISTAR)  
  const char string_13[] PROGMEM = USER;
  const char string_14[] PROGMEM = PWD;
#endif
  
	Serial.println(F("iniciando modem"));
	sendATcommand(string_0, string_12, 500);//send AT  respuesta OK
	if (!sendATcommand(string_0, string_12, 500)) return 0; //send AT  respuesta OK  , si falla se se retorna 0

	//sendATcommand(string_1, string_12, 500);//verifica si la sim tiene pin , no se usa pq no tienen pin por ahora
	// if tiene pin deveria ingresar una clave que para todas las sim deve ser la misma
	sendATcommand(string_2, string_12, 500);//verifica si esta conectado a la red, por ahora solo se usa para ver si esta conectado a la red
	//if creg no hay conexion deberia resetear el modem
	sendATcommand(string_3, string_12, 500);

#if defined(WOM)
	if (!sendATcommand(string_4, string_12, 500)) return 0;
	//delay(3000);
	if (!sendATcommand(string_5, string_12, 650)) return 0;
#endif


#if defined(MOVISTAR) 
  if (!sendATcommand(string_4, string_12, 500)) return 0;
  if (!sendATcommand(string_5, string_12, 500)) return 0;
  //delay(3000);
  if (!sendATcommand(string_13, string_12, 650)) return 0;
  if (!sendATcommand(string_14, string_12, 650)) return 0;
  #endif

	delay(5000);
	sendATcommand(string_6, string_12, 2000,1);//se conecta a intenet
	delay(2000);
	return 1; //la conexion fue existosa 

	//AT+CIICR GPRS Connection bring up ...
	//AT+CGATT=1
}

boolean conectar_server() {
	//esta funcion se ejectua siempre pero no todos los comandos
	//se ebe crear un if que ejecute los comandos de configuracion solo cuando sea necesario
	const char string_0[] PROGMEM = "AT";

	const char string_6[] PROGMEM = "AT+SAPBR=1,1";

	const char string_7[] PROGMEM = "AT+SAPBR=2,1";
	const char string_8[] PROGMEM = "AT+HTTPINIT";
	const char string_9[] PROGMEM = "AT+HTTPACTION=0";
	const char string_10[] PROGMEM = "AT+HTTPREAD";
	const char string_11[] PROGMEM = "AT+HTTPTERM";
	const char string_12[] PROGMEM = "OK";


	const char string_13[] PROGMEM = ",\"http://www.mak3d.cl/GPS/get_nearby.php?id_user=";
	const char string_14[] PROGMEM = "&latitude=";
	const char string_15[] PROGMEM = "&longitude=";

	const char string_16[] PROGMEM = "AT+HTTPPARA=\"URL\"";
	const char string_17[] PROGMEM = "&requerir=1";
	const char string_19[] PROGMEM = "AT+SAPBR=1,0";
	const char string_20[] PROGMEM = "+HTTPACTION: 0,200";


	if (sendATcommand(string_0, string_12, 100))//se verifica nuevamente que el modem esta  ok
	{
		//if (!sendATcommand(string_6, string_12, 500)) return 0;

		sendATcommand(string_6, string_12, 1000,1);//se conecta a intenet
		sendATcommand(string_7, string_12, 1000,1);//ip
		String	strMSG = (String)string_13 + ID + (String)string_14 + String(gps.location.lat(), 6) + (String)string_15 + String(gps.location.lng(), 6);//concatena
		strMSG = string_16 + strMSG + "\"";//fin concatena
		//url = "AT+HTTPPARA=\"URL\",\"www.mak3d.cl/GPS/get_nearby.php?id_user=1&latitude=-33.408067&longitude=-70.559741\""; //ejemplo de url concatenada
		// initialize http service	
		if (!sendATcommand(string_8, string_12, 6000)) {//HHTTPINIT si falla se entonces se cierra el HTTP por seguridad //presenta fallas evaluar otra opcion
			sendATcommand(string_11, string_12, 500);//HTTPTERM
			return 0;
		}
		//HTTPINIT
		delay(500);//debera ir?//este delay hace que todo este ok o no funcione el GPRS pero no se puede
		//tener una titilacion estable con ese delay

		Serial.print(F("URL Variable= "));
		Serial.println(strMSG);
		gprsSerial.println(strMSG);//se envia la  url concatenada

		// set http action type 0 = GET, 1 = POST, 2 = HEAD
		if (!sendATcommand(string_9, string_20, 5000)) { 
			sendATcommand(string_11, string_12, 500);//HTTPTERM
			return 2;//HTTPACTION  retorna error 2
		}
		//delay(1000);
		//gprsSerial.println("AT+HTTPREAD");//HTTPREAD se mando a leer_GPRS solo para probar
		leer_GPRS();
		sendATcommand(string_11, string_12, 500);//HTTPTERM
		//	sendATcommand(string_6, string_19, 1000);//se desconecta a intenet
		return 1;//exito
	}
	else
	{
		return 0;//falla
	}
}

void vibrar(int cantidad) {
	//secuencia
	//1 pulso cada 2 seg entre 100 y 75 mt
	//2pps entre 75 y 20 mt
	//4pps menor a 20

	//si cambio el valor de vibracion se debe vibrar; pero tambien deberia cambiar
	//cuando exista un cambio de color y q ademas sea el mismo nivel de vibracion
	//if (valor_anterior_vibracion != cantidad)
		if (valor_anterior_vibracion != cantidad || color != valor_anterior_color)//agregado por yamil
	{
		valor_anterior_vibracion = cantidad;
		valor_anterior_color = color;    ////agregado por yamil
		Serial.print("vibrar");
		Serial.println(cantidad);
		digitalWrite(LED_R_PIN, 0);
		digitalWrite(LED_G_PIN, 0);
		digitalWrite(LED_B_PIN, 0);
		switch (cantidad)
		{
		case 1:

			digitalWrite(MOTOR_PIN, HIGH);
			led(color, HIGH);
     pps();
			delay(500);
			digitalWrite(MOTOR_PIN, LOW);
			led(color, LOW);

			break;
		case 3:
			for (int i = 1; i <= 2; i++)
			{
				digitalWrite(MOTOR_PIN, HIGH);
				led(color, HIGH);
       pps();
				delay(500);
				digitalWrite(MOTOR_PIN, LOW);
				led(color, LOW);
			}
			break;
		case 5:
			for (int i = 1; i <= 4; i++)
			{
				digitalWrite(MOTOR_PIN, HIGH);
				led(color, HIGH);
       pps();
				delay(500);
				digitalWrite(MOTOR_PIN, LOW);
				led(color, LOW);
			}

			break;
		default:
			break;
		}
	}

}

void pps() {
	//secuencia
	//1 pulso cada 2 seg entre 100 y 75 mt
	//2pps entre 75 y 20 mt
	//4pps menor a 20
	//Serial.println("a");//con esto queda la kgaa en el serial! solo usar para probar
	if (pps_led == 0)//0pps
	{
		//estado = 0;
		led(color, 0);
	}
	if (pps_led == 1)//0.5pps
	{

		if (millis() - previousMillis_led >= 2000)
		{
			estado = 1 - estado;
			led(color, estado);
			previousMillis_led = millis();
			
		}

	}
	if (pps_led == 3)//2pps
	{
		if (millis() - previousMillis_led >= 500) {
			estado = 1 - estado;
			led(color, estado);
			previousMillis_led = millis();
		}

	}
	if (pps_led == 5)//4pps
	{
		led(color, 1);//mantiene el color de forma permanente
	}
}

void led(int color, bool estado) {

	digitalWrite(LED_R_PIN, 0);
	digitalWrite(LED_G_PIN, 0);
	digitalWrite(LED_B_PIN, 0);

	switch (color)
	{
	case 1:
		digitalWrite(LED_R_PIN, estado);
		//Serial.print(F("R"));
		//Serial.println(estado);

		break;
	case 2:
		digitalWrite(LED_G_PIN, estado);
		//Serial.println(F("LED verde"));
		break;
	case 3:
		digitalWrite(LED_B_PIN, estado);
		//Serial.println(F("LED azul"));
		break;
	case 4:
		digitalWrite(LED_R_PIN, 1);
		digitalWrite(LED_G_PIN, 1);
		digitalWrite(LED_B_PIN, 0);
		//Serial.println(F("LED color X"));
	default:
		break;
	}
}

void leer_GPRS() {
	char bufferr[50];
	char *buff;
	buff = bufferr;
	boolean salir = 0;
	unsigned long timeout = millis();
	int i = 0;
	gprsSerial.println("AT+HTTPREAD");//HTTPREAD  para mantener el orden no deberia estar aqui
	while (millis() - timeout < 10000 && salir == 0)
		//  while (millis() - timeout < 7000)
	{	 
		while (gprsSerial.available())//lee los datos entregados por el server
		{
			char c = gprsSerial.read();
			buff[i++] = c;
			Serial.write(buff[i - 1]);
			if (c == '*') {
				//break;	
				 //  Serial.println("Salir");

				salir = 1;
				break;
			}
			timeout = millis();
		}
		pps();
	}
	if (strstr(buff, "v=")) {//buscar los datos entregados por el server, existe v=?
		Serial.println();
		buff = strchr(buff, '=') + 1;//busca el caracter =
		//Serial.println(buff);
		Vibrar = atoi(buff);//transforma el numero
		Serial.print(F("V="));
		Serial.println(Vibrar);
		
		pps_led = Vibrar;
		buff = strchr(buff, '=') + 1;
		color = atoi(buff);
		Serial.print(F("c="));
		Serial.println(color);
    pps();
    vibrar(Vibrar);//vibra el motor
		pps();
	}
}

void test_led_mot(void) {

	digitalWrite(LED_R_PIN, HIGH);
	delay(500);
	digitalWrite(LED_R_PIN, LOW);
	delay(500);
	digitalWrite(LED_G_PIN, HIGH);
	delay(500);
	digitalWrite(LED_G_PIN, LOW);
	delay(500);
	digitalWrite(LED_B_PIN, HIGH);
	delay(500);
	digitalWrite(LED_B_PIN, LOW);
	delay(500);
	digitalWrite(MOTOR_PIN, HIGH);
	delay(1000);
	digitalWrite(MOTOR_PIN, LOW);

}

int vbatt(void) {
	//AT+CBC para ver el voltaje de batt
	const char string_12[] PROGMEM = "OK";
	const char string_25[] PROGMEM = "AT+CBC";
	const char string_26[] PROGMEM = "AT+CFUN=0";
	const char string_27[] PROGMEM = "AT+CFUN=1";
	//sendATcommand(string_25, string_12, 1000);//

	char bufferr[50];
	char *buff;
	buff = bufferr;
	int carga;
	int respondio=0;

		unsigned long timeout = millis();
	int i = 0;
	gprsSerial.listen();
	gprsSerial.println(string_25);//AT+CBC
	do
	{
		if (gprsSerial.available() != 0)
		{

			while (gprsSerial.available())//lee los datos entregados por el server
			{
				char c = gprsSerial.read();
				buff[i++] = c;
				Serial.write(buff[i - 1]);
				timeout = millis();
				if (strstr(buff, string_12) != NULL)    // check if the desired answer (OK) is in the response of the module
				{
					respondio = 1;
					break;
				}

			}
		}
		}while ((respondio == 0) && millis() - timeout < 2000);

			if (strstr(buff, "+CBC:")) {//buscar los datos entregados por el server, existe v=?
				Serial.println();
				buff = strchr(buff, ':') + 1;//busca el caracter =
				carga = atoi(buff);//transforma el numero
				Serial.print(F("carga="));
				Serial.println(carga);
			}

		if (carga < 20)
		{
			sendATcommand(string_26, string_12, 10000);//funcionalidad minima de modem
			bateria_cargada = 0;
		}
		if (carga > 30 && (bateria_cargada == 0))
		{
			bateria_cargada = 1;
			sendATcommand(string_27, string_12, 10000);//funcionalidad normal de modem
		}

	}
