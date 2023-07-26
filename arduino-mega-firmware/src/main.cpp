#include <Arduino.h>
#include <U8g2lib.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include "Adafruit_NeoPixel.h"
#include "FlowMeter.h"
#include <math.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include <HCSR04.h>

#define valve1 4
#define valve2 5
#define valve3 6
#define valve4 7
#define valve5 8
#define valve6 9
#define valve7 24
#define valve8 25
#define UVlightRelay 12
#define ozonoRelay 13
#define pumpRelay 14
#define auxiliarRelay 15

// Por interrupciones
#define caudalSensor1 20
#define caudalSensor2 21
#define caudalSensor3 3

#define coinCounter 2

////////////////////////
#define buton1 47
#define buton2 46
#define buton3 45
#define buton4 44
#define buton5 43

#define motor1A 39
#define motor1B 40
#define motor2A 41
#define motor2B 42

#define motor1SwUp 33
#define motor1SwDown 32
#define motor2SwUp A15
#define motor2SwDown A14


///////////Luz RGB
#define ambientalLight 38

/////////////////////////// analogo
#define presureSensor A13

//////////////////////////
// #define triggPerson 30
// #define echoPerson 31
HCSR04 personSensor(30, 31); // trigg, echo
HCSR04 bigContainer(28, 29);
HCSR04 smallContainer(26, 27);

////////////Pantalla //////////////////////////////////
U8G2_ST7920_128X64_F_SW_SPI u8g2(U8G2_R0, /* EN=*/37, /* RW=*/35, /* RS=*/36, /* reset=*/34);

////////////RFID//////////////////////////////////////
#define RST_PIN 49 // Reset
#define SS_PIN 48  // SDA
// SCK=52/MOSI=51/MISO=50
MFRC522 mfrc522(SS_PIN, RST_PIN);
byte actualUID[4] = {0, 0, 0, 0};
byte user1[4] = {0x1A, 0x97, 0xEA, 0x81}; // código del usuario 1
byte user2[4] = {0xA7, 0x8F, 0xA0, 0xB4}; // código del usuario 2 B5
///////////////////////////////////////////////////////
Adafruit_NeoPixel ambientalStrip = Adafruit_NeoPixel(90, ambientalLight, NEO_GRB + NEO_KHZ800);
/////////////////////////////////////////////////////////

FlowMeter *Meter1;
FlowMeter *Meter2;
FlowMeter *Meter3;

///////////////////////////////////////////////// audio
SoftwareSerial mySoftwareSerial(10, 11); // RX, TX
DFRobotDFPlayerMini myDFPlayer;
// myDFPlayer.play(1);  //Play the first mp3

String bigContainerString = "bigContainer";
String fiveLitersString = "fiveLiters";
String tenLitersString = "tenLiters";
String fifteenLitersString = "fifteenLiters";
String smallContainerString = "smallContainer";
String personDetectionString = "personDetection";


const int serviceValue1 = 10;
const int serviceValue2 = 25;
const int serviceValue3 = 50;

const int washTime = 4000;

// contadores de interrupciones
int coinsCounter = 0;
int butonPulse1 = 0;
int butonPulse2 = 0;

//litros a llenar
const int litersReecipientOne = 1;
const int litersReecipientTwo = 1;
const int litersReecipientTree = 1;

const float compenstation5Liters = 0.8125;
const float compenstation10Liters = 0.8125;
const float compenstation15Liters = 0.8125;


void setupPins();
uint32_t Wheel(byte WheelPos);
void rainbow(uint8_t wait);
void oneColorLight();
void setupDFplayer();
void Meter1ISR();
void Meter2ISR();
void Meter3ISR();
void butonCount1();
void butonCount2();
void coin();
bool anybuttonPressed();
void welcome();
void coinBalance();
void putRecipientInverse();
void putRecipient();
void filtersOff();
void filtersOn();
void endWashSmallContainer();
void endWashBigContainer();
void washing();
void filling();
void thanks();
void takeRecipient();
void invalidCard();
bool readUID();
void relunchRFID();
void fadeOutSound();
float readUltrasonicSensor(String sensor);
void personDetection();
bool compareArray(byte array1[], byte array2[]);
bool compareUsers();
void clearUID();
int selectService();
void bigDoorDown();
void bigDoorUp();
void smallDoorDown();
void smallDoorUp();
void washBigContainer();
void washSmallContainer();
void fillContainer(String containersize, String liters);

void  measureLiters(int volumeToFill, String recipientSize);
void fillLiters(int volumeToFill, String recipientSize);

void setup()
{
  setupPins();
  Serial.begin(115200); // Inicializa comunicación con el PC
  setupDFplayer();
  u8g2.begin();                      // Inicializa la pantalla
  SPI.begin();                       // Inicializa la comunicación SPI
  mfrc522.PCD_Init();                // Inicializa el módulo MFRC522
  delay(4);                          // Tiempo de inicialización del módulo MFR522
  mfrc522.PCD_DumpVersionToSerial(); // Muestra la versión de firmware del módulo MRF522
  ambientalStrip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  ambientalStrip.show();            // Turn OFF all pixels ASAP
  ambientalStrip.setBrightness(255); // Set BRIGHTNESS to about 1/5 (max = 255)
  welcome();
  
}

void loop()
{
  static bool selectServiceFlag = false;
  int serviceSelected = 0;

  relunchRFID();
  rainbow(1);

  personDetection();

  if(coinsCounter >= 5){
    coinBalance();
  }
  else{
    welcome();
  }

  

  selectServiceFlag = readUID();

  if (selectServiceFlag == true || coinsCounter >= serviceValue1)
  {
    oneColorLight();
    serviceSelected = selectService();
    coinBalance();
    Serial.println("coins = " + String(coinsCounter));

    Serial.println("servicio seleccionado = " + String(serviceSelected));

    if (coinsCounter >= serviceValue1 && serviceSelected == 5)
    { // 5litros
      Serial.println("seleccionado 5 litros");
      washSmallContainer();
      fillContainer(smallContainerString, fiveLitersString);
      coinsCounter = 0;
    }
    else if (coinsCounter >= serviceValue2 && serviceSelected == 10)
    { // 10 litros
      Serial.println("seleccionado 10 litros");
      washBigContainer();

      fillContainer(bigContainerString, tenLitersString);
      coinsCounter = 0;
    }
    else if (coinsCounter >= serviceValue3 && serviceSelected == 20)
    { // 15 litros
      Serial.println("seleccionado 15 litros");
      washBigContainer();

      fillContainer(bigContainerString, fifteenLitersString);
      coinsCounter = 0;
    }
  }

}

void setupPins()
{
  // Válvulas
  pinMode(valve1, OUTPUT);
  pinMode(valve2, OUTPUT);
  pinMode(valve3, OUTPUT);
  pinMode(valve4, OUTPUT);
  pinMode(valve5, OUTPUT);
  pinMode(valve6, OUTPUT);
  pinMode(valve7, OUTPUT);
  pinMode(valve8, OUTPUT);
  // Relés
  pinMode(UVlightRelay, OUTPUT);
  pinMode(ozonoRelay, OUTPUT);
  pinMode(pumpRelay, OUTPUT);
  pinMode(auxiliarRelay, OUTPUT);
  // Control de motores
  pinMode(motor1A, OUTPUT);
  pinMode(motor1B, OUTPUT);
  pinMode(motor2A, OUTPUT);
  pinMode(motor2B, OUTPUT);
  // Cinta led
  pinMode(ambientalLight, OUTPUT);

  pinMode(buton1, INPUT);
  pinMode(buton2, INPUT);
  pinMode(buton3, INPUT);
  pinMode(buton4, INPUT);
  pinMode(buton5, INPUT);
  // Pines por interrupción
  // pinMode(caudalSensor1, INPUT);
  // pinMode(caudalSensor2, INPUT);
  // pinMode(caudalSensor3, INPUT);
  pinMode(coinCounter, INPUT);
  // Finales de carrera
  pinMode(motor1SwUp, INPUT);
  pinMode(motor1SwDown, INPUT);
  pinMode(motor2SwUp, INPUT);
  pinMode(motor2SwDown, INPUT);
  // Sensor de presión
  pinMode(presureSensor, INPUT);
  // interrupciones
  Meter1 = new FlowMeter(digitalPinToInterrupt(caudalSensor1), UncalibratedSensor, Meter1ISR, RISING);
  Meter2 = new FlowMeter(digitalPinToInterrupt(caudalSensor2), UncalibratedSensor, Meter2ISR, RISING);
  Meter3 = new FlowMeter(digitalPinToInterrupt(caudalSensor3), UncalibratedSensor, Meter3ISR, RISING);

  attachInterrupt(digitalPinToInterrupt(coinCounter), coin, RISING);

  // Ultrasónicos
  //  // pinMode(triggPerson, OUTPUT);
  //  pinMode(triggBigContainer, OUTPUT);
  //  pinMode(triggSmallContainer, OUTPUT);
  //  // pinMode(echoPerson, INPUT);
  //  pinMode(echoBigContainer, INPUT);
  //  pinMode(echoSmallContainer, INPUT);
}
void Meter1ISR()
{
  
  Meter1->count();
}
void Meter2ISR()
{
  Meter2->count();
}
void Meter3ISR()
{
  Meter3->count();
}
void coin()
{
  coinsCounter++;
}
void butonCount1()
{
  butonPulse1++;
}
void butonCount2()
{
  butonPulse2++;
}
void welcome()
{


  static unsigned long previousTimeToChange = 0;
  int timeToChange = 1500;
  static int screenNumber = 0;

  Serial.println("welcome" + String(millis() - previousTimeToChange));


  if(millis() - previousTimeToChange >= timeToChange){
    Serial.println("screen" + String(screenNumber));

    switch (screenNumber){
      case 0:
        u8g2.firstPage();
          do
          {
            u8g2.setFont(u8g2_font_ncenB18_tr);
            u8g2.drawStr(25, 40, "iAquA");
          } while (u8g2.nextPage());
        break;

      case 1:
        u8g2.firstPage();
          do
          {
            u8g2.setFont(u8g2_font_ncenB14_te);
            u8g2.drawStr(40, 30, "Agua");
            u8g2.drawStr(15, 50, "Purificada");
          } while (u8g2.nextPage());
        break;
      }

    screenNumber++;

    if(screenNumber > 1){
      screenNumber = 0;    
  }
    previousTimeToChange = millis();
           

    }
}
void coinBalance()
{
	static int previousCoinCounter = 0;

	if (coinsCounter >= 1)
	{
    Serial.println("mayor a 5");
    Serial.println("previous coin = " + String(previousCoinCounter) + "coinsCounter = " + String(coinsCounter));
		if (previousCoinCounter != coinsCounter)
		{
			previousCoinCounter = coinsCounter;
			int coinsCount = coinsCounter * 100;
			String coins = "Saldo: $" + String(coinsCount);

			Serial.println(coins);

			int len = coins.length() + 1;
			char coinsArray[len] = {""};
			coins.toCharArray(coinsArray, len);

			u8g2.firstPage();
			do
			{
        Serial.println("balance");
				u8g2.setFont(u8g2_font_ncenB14_te);
				u8g2.drawStr(0, 20, "Bienvenido!");
				//u8g2.setFont(u8g2_font_6x10_tf);
				//u8g2.drawStr(0, 40, "5 Litros por  $1.000");
				u8g2.drawStr(0, 60, coinsArray);

				//u8g2.drawStr(0, 60, "25 Litros por $5.000");

			} while (u8g2.nextPage());
		}
	}
}
void putRecipientInverse()
{
	u8g2.firstPage();
	do
	{
		
    u8g2.setFont(u8g2_font_6x10_tf);
		u8g2.drawStr(0, 20, "Coloque el");
		u8g2.drawStr(0, 40, "recipiente ");
		u8g2.setFont(u8g2_font_ncenB14_te);
		u8g2.drawStr(0, 60, "boca abajo");

	} while (u8g2.nextPage());
}
void putRecipient()
{
	u8g2.firstPage();
	do
	{
		
    u8g2.setFont(u8g2_font_6x10_tf);
		u8g2.drawStr(0, 20, "Coloque el recipiente ");
		u8g2.drawStr(0, 40, "boca arriba");
    u8g2.drawStr(0, 60, "y presione un boton");

	} while (u8g2.nextPage());
}
void washing()
{
	u8g2.firstPage();
	do
	{
		
		u8g2.setFont(u8g2_font_ncenB14_te);
		u8g2.drawStr(0, 30, "Lavando...");

	} while (u8g2.nextPage());
}
void filling()
{
	u8g2.firstPage();
	do
	{
		
		u8g2.setFont(u8g2_font_ncenB14_te);
		u8g2.drawStr(0, 30, "Llenando...");

	} while (u8g2.nextPage());
}
void thanks()
{
	u8g2.firstPage();
	do
	{
		
		u8g2.setFont(u8g2_font_ncenB14_te);
		u8g2.drawStr(0, 30, "Gracias por");
    u8g2.drawStr(0, 60, "usarme!");

	} while (u8g2.nextPage());
  delay(5000);
}
void takeRecipient()
{
	u8g2.firstPage();
	do
	{
		
		u8g2.setFont(u8g2_font_ncenB14_te);
		u8g2.drawStr(0, 30, "Llenado");
    u8g2.drawStr(0, 60, "completado!");

	} while (u8g2.nextPage());
}
void invalidCard()
{
	u8g2.firstPage();
	do
	{
		
		u8g2.setFont(u8g2_font_ncenB14_te);
		u8g2.drawStr(0, 30, "Tarjeta");
    u8g2.drawStr(0, 60, "invalida!");

	} while (u8g2.nextPage());
}

void setupDFplayer()
{
  mySoftwareSerial.begin(9600);
  if (!myDFPlayer.begin(mySoftwareSerial))
  {
    Serial.println(F("Error inicializando modulo mp3:"));
    Serial.println(F("1.Porfavor revisa las conexiones!"));
    Serial.println(F("2.Porfavor inserta memoria microSD!"));
    // while (true)
    // {
    //   delay(0);
    // }
  }
  Serial.println(F("Inicialización correcta DFPlayer."));
  myDFPlayer.volume(30); // volúmen de 0 a 30
}
void fadeOutSound()
{
  // for (int i = 30; i >= 0; i--)
  // {
  //   myDFPlayer.volume(i);
  //   delay(1);
  // }
  // Serial.println("apagar!!!");
  // delay(100);
  // myDFPlayer.stop();
  // delay(150);
  // myDFPlayer.volume(30);
  delay(150);
}
float readUltrasonicSensor(String sensor)
{
  // long echoTime = 0;

  int distance = 0;
  int samples = 4;
  float distanceAverage = 0;
  float distanceArray[samples] = {0};

  for (int i = 0; i < samples; i++)
  {
    if (sensor == personDetectionString)
    {
      distanceArray[i] = personSensor.dist();
    }
    else if (sensor == bigContainerString)
    {
      distanceArray[i] = bigContainer.dist();
    }
    else if (sensor == smallContainerString)
    {
      distanceArray[i] = smallContainer.dist();
    }

    // Serial.print(String(distanceArray[i]) + " / ");
    distanceAverage = distanceAverage + distanceArray[i];
    delay(60);
  }
  distanceAverage = distanceAverage / samples;
  // Serial.println("promedio= " +String(distanceAverage));

  for (int i = 0; i < samples; i++)
  {
    if (distanceArray[i] > distanceAverage + 20)
    {
      // Serial.println("error en medida");
      return 0;
    }
  }
  return distanceAverage;
}
void personDetection()
{

  float personDistance = readUltrasonicSensor(personDetectionString);

  int detectionLongUpRange = 100; // distancia para música de detección
  int detectionShortUpRange = 50;
  int minimumDistance = 3;
  int playMusicState = 0;
  int timeToActiveAlert = 100;
  int timeToSayHiAgain = 10000;
  static unsigned long previousTimeToSayHi = 0;
  static bool sayHiFlag = false;

  // Serial.println(personDistance);
  playMusicState = myDFPlayer.readState();
  delay(150);
  // Serial.println("estado audio " + String(playMusicState));

  Serial.println("distancia persona = " + String(personDistance));

  if (personDistance < detectionLongUpRange && personDistance > minimumDistance)
  {
    delay(timeToActiveAlert);
    personDistance = readUltrasonicSensor(personDetectionString);

    if (personDistance <= detectionShortUpRange && personDistance > minimumDistance)
    {
      Serial.println("reproducir Saludo  " + String(personDistance));
      if (sayHiFlag == false)
      {
        fadeOutSound();
        myDFPlayer.play(2);
        // digitalWrite(valve2, HIGH);
        // delay(300);
        // digitalWrite(valve2, LOW);
        sayHiFlag = true;
        previousTimeToSayHi = millis();
      }
    }

    if (personDistance < detectionLongUpRange && personDistance > detectionShortUpRange)
    {
      Serial.println("reproducir música");
      if (playMusicState == 0)
      {
        myDFPlayer.play(1);
      }
      // digitalWrite(valve1, HIGH);
      // delay(300);
      // digitalWrite(valve1, LOW);
    }
  }
  // evita reproducir constantemente el saludo
  if (sayHiFlag == true)
  {
    if ((millis() - previousTimeToSayHi) >= timeToSayHiAgain)
    {
      sayHiFlag = false;
    }
  }
}
void relunchRFID()
{

  // Reinicializa el lector de tarjetas cada determinado tiempo (6 segundos), evita problemas por desconexión accidental
  static int relunchTime = 6 * 1000;
  static bool timerStart = 0;
  static unsigned long previousTimeToRelunchRFIF = 0;

  if ((millis() - previousTimeToRelunchRFIF) >= relunchTime)
  {
    mfrc522.PCD_Init();
    delay(4);
    previousTimeToRelunchRFIF = millis();
  }
}
bool readUID()
{
  bool correctTarget = false;

  if (mfrc522.PICC_IsNewCardPresent())
  {
    if (mfrc522.PICC_ReadCardSerial())
    {
      Serial.println(F("Card UID"));
      for (byte i = 0; i < mfrc522.uid.size; i++)
      {
        Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
        Serial.print(mfrc522.uid.uidByte[i], HEX);
        actualUID[i] = mfrc522.uid.uidByte[i];
      }
      Serial.println("");
    }

    correctTarget = compareUsers();
    if (correctTarget == true)
    {
      // Serial.println("reproducir selección servicio");
      // myDFPlayer.play(3);//seleccione el servicio
      coinsCounter = serviceValue3;
    }
    else if (correctTarget == false)
    {
      Serial.println("reproducir tarjeta invalida");
      myDFPlayer.play(4); // tarjeta invalida
      invalidCard();
      delay(2000);
    }
    clearUID();
  }
  return correctTarget;
}
void clearUID()
{
  for (int i = 0; i < sizeof(actualUID); i++)
  {
    actualUID[i] = 0;
  }
}
bool compareArray(byte array1[], byte array2[])
{
  if (array1[0] != array2[0])
  {
    return (false);
  }
  if (array1[1] != array2[1])
  {
    return (false);
  }
  if (array1[2] != array2[2])
  {
    return (false);
  }
  if (array1[3] != array2[3])
  {
    return (false);
  }

  return (true);
}
bool compareUsers()
{
  // Agregar tantos usuarios como tarjetas de referencia... Desarrollar más para trabajar con Keys de las tarjetas
  bool RfIdFlag = 0;

  if (compareArray(actualUID, user1) == true)
  {
    RfIdFlag = true;
  }
  if (compareArray(actualUID, user2) == true)
  {
    RfIdFlag = true;
  }
  return RfIdFlag;
}
int selectService()
{
  bool pulsedButonFlag = false;
  int pulsedButon = 0;

  Serial.println("reproducir selección servicio");
  myDFPlayer.play(3); // seleccione el servicio

  while (pulsedButonFlag == false)
  {
    coinBalance();

    if (digitalRead(buton1) == true)
    { // 5 litros
      delay(50);
      if (digitalRead(buton1) == true)
      {
        Serial.println("5 litros presionado");
        pulsedButon = 5;
        pulsedButonFlag = true;
      }
    }
    else if (digitalRead(buton2) == true)
    { // 10 litros
      delay(50);
      if (digitalRead(buton2) == true)
      {
        Serial.println("10 litros presionado");
        pulsedButon = 10;
        pulsedButonFlag = true;
      }
    }
    else if (digitalRead(buton3) == true)
    { // 15 litros
      delay(50);
      if (digitalRead(buton3) == true)
      {
        Serial.println("20 litros presionado");
        pulsedButon = 20;
        pulsedButonFlag = true;
      }
    }
  }
  return pulsedButon;
}
void bigDoorUp()
{
  bool switchUp = 0;
  Serial.println("esperando switch Up");

  digitalWrite(motor1A, HIGH);
  digitalWrite(motor1B, LOW);

  switchUp = digitalRead(motor1SwUp);
  while (switchUp != true)
  {
    switchUp = digitalRead(motor1SwUp);
    if (switchUp == true)
    {
      delay(50);
      switchUp = digitalRead(motor1SwUp);
    }
  }
  digitalWrite(motor1A, HIGH);
  digitalWrite(motor1B, HIGH);
  Serial.println("Switch Up Presionado");
}
void bigDoorDown()
{
  bool switchDown = 0;
  Serial.println("esperando switch Down");

  digitalWrite(motor1A, LOW);
  digitalWrite(motor1B, HIGH);

  switchDown = digitalRead(motor1SwDown);
  while (switchDown != true)
  {
    switchDown = digitalRead(motor1SwDown);
    if (switchDown == true)
    {
      delay(50);
      switchDown = digitalRead(motor1SwDown);
    }
  }
  digitalWrite(motor1A, HIGH);
  digitalWrite(motor1B, HIGH);
  Serial.println("Switch Down Presionado");
}

void smallDoorUp()
{
  bool switchUp = 0;
  Serial.println("esperando switch Up");

  digitalWrite(motor2A, HIGH);
  digitalWrite(motor2B, LOW);

  switchUp = digitalRead(motor2SwUp);
  while (switchUp != true)
  {
    switchUp = digitalRead(motor2SwUp);
    if (switchUp == true)
    {
      delay(50);
      switchUp = digitalRead(motor2SwUp);
    }
  }
  digitalWrite(motor2A, HIGH);
  digitalWrite(motor2B, HIGH);
  Serial.println("Switch Up Presionado");
}
void smallDoorDown()
{
  bool switchDown = 0;
  Serial.println("esperando switch Down");

  digitalWrite(motor2A, LOW);
  digitalWrite(motor2B, HIGH);

  switchDown = digitalRead(motor2SwDown);
  while (switchDown != true)
  {
    switchDown = digitalRead(motor2SwDown);
    if (switchDown == true)
    {
      delay(50);
      switchDown = digitalRead(motor2SwDown);
    }
  }
  digitalWrite(motor2A, HIGH);
  digitalWrite(motor2B, HIGH);
  Serial.println("Switch Down Presionado");
}

void closeDoorBySensor(String containersize)
{
  float bigContainerDistance = readUltrasonicSensor(bigContainerString);
  float smallContainerDistance = readUltrasonicSensor(bigContainerString);

  int detectionLongRange = 30;
  int detectionShortRange = 3;
  int delayToConfirmDistance = 1000;

  if (containersize == bigContainerString)
  {
    while (bigContainerDistance < detectionLongRange)
    {
      bigContainerDistance = readUltrasonicSensor(bigContainerString);
      if (bigContainerDistance >= detectionLongRange)
      {
        delay(delayToConfirmDistance);
        if (bigContainerDistance >= detectionLongRange)
        {
          bigDoorDown();
          myDFPlayer.play(8); // gracias por usar el servicio
        }
      }
    }
  }
   else if (containersize == smallContainerString)
  {
    while (smallContainerDistance < detectionLongRange)
    {
      smallContainerDistance = readUltrasonicSensor(smallContainerString);
      if (smallContainerDistance >= detectionLongRange)
      {
        delay(delayToConfirmDistance);
        if (smallContainerDistance >= detectionLongRange)
        {
          smallDoorDown();
          myDFPlayer.play(8); // gracias por usar el servicio
        }
      }
    }
  }
}
void washBigContainer()
{

  float bigContainerDistance = readUltrasonicSensor(bigContainerString);

  int detectionLongRange = 30;
  int detectionShortRange = 3;
  int delayToConfirmDistance = 1000;
  //int washTime = 4000;
  bool butonPressed = false;

  bigDoorUp();
  myDFPlayer.play(5); // poner recipiente para lavado
  putRecipientInverse();

  // while ((bigContainerDistance > detectionLongRange || bigContainerDistance < detectionShortRange))
  // {
  //   bigContainerDistance = readUltrasonicSensor(bigContainerString);

  //   if (bigContainerDistance <= detectionLongRange && bigContainerDistance >= detectionShortRange)
  //   {
  //     delay(delayToConfirmDistance);
  //     if (bigContainerDistance <= detectionLongRange && bigContainerDistance >= detectionShortRange)
  //     {
  //       bigDoorDown();
  //     }
  //   }
  // }
    while (butonPressed == false)
  {
    butonPressed = anybuttonPressed();
  }

  bigDoorDown();

  washing();
  digitalWrite(UVlightRelay, HIGH);
  digitalWrite(ozonoRelay, HIGH);
  digitalWrite(valve4, HIGH); // lavado recamara grande para
  delay(washTime);
  digitalWrite(UVlightRelay, LOW);
  digitalWrite(ozonoRelay, LOW);
  digitalWrite(valve4, LOW);

  bigDoorUp();
}

void washSmallContainer()
{
  
  // float smallContainerDistance = readUltrasonicSensor(smallContainerString);
  float smallContainerDistance = 20;

  int detectionLongRange = 10;
  int detectionShortRange = 3;
  int delayToConfirmDistance = 3000;
  //int washTime = 4000;
  bool butonPressed = false;

  smallDoorUp();
  // delay(3000);
  myDFPlayer.play(5); // poner recipiente para lavado
  putRecipientInverse();

  // while ((smallContainerDistance > detectionLongRange || smallContainerDistance < detectionShortRange))
  // {
  //   smallContainerDistance = readUltrasonicSensor(smallContainerString);

  //   if (smallContainerDistance <= detectionLongRange && smallContainerDistance >= detectionShortRange)
  //   {
  //     delay(delayToConfirmDistance);
  //     if (smallContainerDistance <= detectionLongRange && smallContainerDistance >= detectionShortRange)
  //     {
  //       smallDoorDown();
  //     }
  //   }
  // }
    while (butonPressed == false)
  {
    butonPressed = anybuttonPressed();
  }

  smallDoorDown();

  washing();
  digitalWrite(UVlightRelay, HIGH);
  digitalWrite(ozonoRelay, HIGH);
  digitalWrite(valve3, HIGH); // lavado recamara grande para
  delay(washTime);
  digitalWrite(UVlightRelay, LOW);
  digitalWrite(ozonoRelay, LOW);
  digitalWrite(valve3, LOW);

  smallDoorUp();
}
bool anybuttonPressed()
{
  bool fiveLitersButon = digitalRead(buton1);
  bool tenLitersButon = digitalRead(buton2);
  bool fifteenLitersButon = digitalRead(buton3);

  if (fiveLitersButon == true || tenLitersButon == true || fifteenLitersButon == true)
  {
    delay(50);
    fiveLitersButon = digitalRead(buton1);
    tenLitersButon = digitalRead(buton2);
    fifteenLitersButon = digitalRead(buton3);
    if (fiveLitersButon == true || tenLitersButon == true || fifteenLitersButon == true)
    {
      return true;
    }
  }
  return false;
}
void fillContainer(String containersize, String liters)
{
  bool butonPressed = 0;
  myDFPlayer.play(6);
  putRecipient();

  // float bigContainerDistance = readUltrasonicSensor(containersize);

  // int detectionLongRange = 30;
  // int detectionShortRange = 3;
  // int delayToConfirmDistance = 1000;


  // while ((bigContainerDistance > detectionLongRange || bigContainerDistance < detectionShortRange))
  // {
  //   bigContainerDistance = readUltrasonicSensor(containersize);

  //   if (bigContainerDistance <= detectionLongRange && bigContainerDistance >= detectionShortRange)
  //   {
  //     delay(delayToConfirmDistance);
  //     if (bigContainerDistance <= detectionLongRange && bigContainerDistance >= detectionShortRange)
  //     {
  //       delay(1);
  //     }
  //   }
  // }

  while (butonPressed == false)
  {
    butonPressed = anybuttonPressed();
  }

  if (containersize == bigContainerString)
  {
    bigDoorDown();
    if (liters == fifteenLitersString)
    {
      filtersOn();
      bigDoorDown();
      filling();
      fillLiters(litersReecipientTree, bigContainerString);
      //delay(3000);
      takeRecipient();
      bigDoorUp();
      myDFPlayer.play(7); // retire el recipiente
      // closeDoorBySensor(bigContainerString);
      delay(10000);
      bigDoorDown();
      //thanks();
      endWashBigContainer();
      filtersOff();
    }
    else if (liters == tenLitersString){
      filtersOn();
      bigDoorDown();
      filling();
      fillLiters(litersReecipientTwo, bigContainerString);
      takeRecipient();
      bigDoorUp();
      myDFPlayer.play(7); // retire el recipiente
      // closeDoorBySensor(bigContainerString);
      delay(10000);
      bigDoorDown();
      //thanks();
      endWashBigContainer();
      filtersOff();
    }
  }
  else if (containersize == smallContainerString){
      filtersOn();
      smallDoorDown();
      filling();
      fillLiters(litersReecipientOne, smallContainerString);
      takeRecipient();
      smallDoorUp();
      myDFPlayer.play(7);
      // closeDoorBySensor(smallContainerString);
      delay(10000);
      smallDoorDown();
      //thanks();
      endWashSmallContainer();
      filtersOff();
    }
}

void measureLiters(int volumeToFill, String recipientSize)
{
  double volume1 = Meter1->getTotalVolume();
  double volume2 = Meter2->getTotalVolume();

  double compenstation = 1;

  if(volumeToFill == litersReecipientOne){
    compenstation = compenstation5Liters;
  }
  else if(volumeToFill == litersReecipientTwo){
    compenstation = compenstation10Liters;
  }
  else if(volumeToFill == litersReecipientTree){
    compenstation = compenstation15Liters;
  } 

  if(recipientSize == bigContainerString){
    digitalWrite(valve1, HIGH);

    if (volume1 >= volumeToFill*compenstation)
    {
        Serial.println("LLENADO!!");
        digitalWrite(valve1, LOW);
    }
  }
  if(recipientSize == smallContainerString){
    digitalWrite(valve2, HIGH);
    
    if (volume2 >= volumeToFill*compenstation)
    {
        Serial.println("LLENADO!!");
        digitalWrite(valve2, LOW);
    }
  }
    
    
    
}

void fillLiters(int volumeToFill, String recipientSize)
{
  const int period = 1000;
  
  bool fillFlag = true;

  double compenstation = 1;

  if(volumeToFill == litersReecipientOne){
    compenstation = compenstation5Liters;
  }
  else if(volumeToFill == litersReecipientTwo){
    compenstation = compenstation10Liters;
  }
  else if(volumeToFill == litersReecipientTree){
    compenstation = compenstation15Liters;
  }  

  Meter1->setTotalVolume(0.00);
  Meter2->setTotalVolume(0.00);
  Meter3->setTotalVolume(0.00);

  while (fillFlag == true)
  {
    delay(period);
    Meter1->tick(period);
    Meter2->tick(period);
    Meter3->tick(period);
    measureLiters(volumeToFill, recipientSize);
    if (Serial.availableForWrite())
    {
      Serial.println("Currently S1 " + String(Meter1->getCurrentFlowrate()) + " l/min, " + String(Meter1->getTotalVolume()) + " l total.");
      Serial.println("Currently S2 " + String(Meter2->getCurrentFlowrate()) + " l/min, " + String(Meter2->getTotalVolume()) + " l total.");
      Serial.println("Currently S3 " + String(Meter3->getCurrentFlowrate()) + " l/min, " + String(Meter3->getTotalVolume()) + " l total.");
      Serial.println("/////////////////////////////");
    }

    if(recipientSize == bigContainerString){

      if (Meter1->getTotalVolume() >= (volumeToFill*compenstation))
      {
        fillFlag = false;
        Meter1->setTotalVolume(0.00);
        Meter2->setTotalVolume(0.00);
        Meter3->setTotalVolume(0.00);
      }      

    }
    else if(recipientSize == smallContainerString){
      if (Meter2->getTotalVolume() >= (volumeToFill*compenstation))
      {
        fillFlag = false;
        Meter1->setTotalVolume(0.00);
        Meter2->setTotalVolume(0.00);
        Meter3->setTotalVolume(0.00);
      }      
    }


  }
}

void endWashSmallContainer(){
  int washTime = 2000;
  digitalWrite(UVlightRelay, HIGH);
  digitalWrite(ozonoRelay, HIGH);
  digitalWrite(valve3, HIGH); // lavado recamara grande para
  delay(washTime);
  digitalWrite(UVlightRelay, LOW);
  digitalWrite(ozonoRelay, LOW);
  digitalWrite(valve3, LOW);
}
void endWashBigContainer(){
  int washTime = 2000;
  digitalWrite(UVlightRelay, HIGH);
  digitalWrite(ozonoRelay, HIGH);
  digitalWrite(valve4, HIGH); // lavado recamara grande para
  delay(washTime);
  digitalWrite(UVlightRelay, LOW);
  digitalWrite(ozonoRelay, LOW);
  digitalWrite(valve4, LOW);
}
uint32_t Wheel(byte WheelPos)
{
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85)
  {
    return ambientalStrip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170)
  {
    WheelPos -= 85;
    return ambientalStrip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return ambientalStrip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}
void rainbow(uint8_t wait)
{
  static unsigned long previousTimeToChange = 0;
  static int i, j = 0;

  if ((millis() - previousTimeToChange) >= wait)
  {
    j++;
    for (i = 0; i < ambientalStrip.numPixels(); i++)
    {
      ambientalStrip.setPixelColor(i, Wheel((i + j) & 255));
    }

    ambientalStrip.show();

    // if (Serial.availableForWrite())
    // {
    //   Serial.println(j);
    // }
    if (j >= 255)
    {
      j = 0;
    }
    previousTimeToChange = millis();
  }
}

void oneColorLight()
{

  for (int i = 0; i < ambientalStrip.numPixels(); i++)
  { 
    ambientalStrip.setPixelColor(i, 255, 0, 0);
  }
  ambientalStrip.show();
}

void filtersOn(){
  digitalWrite(UVlightRelay, HIGH);
  digitalWrite(ozonoRelay, HIGH);
}

void filtersOff(){
  digitalWrite(UVlightRelay, LOW);
  digitalWrite(ozonoRelay, LOW);
}