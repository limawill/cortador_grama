#define VERSION     "\n\nAndroTest V2.0 - @kas2014\ndemo for V5.X App (6 button version)"

#include "SoftwareSerial.h"
#include <AFMotor.h>
#include <Ultrasonic.h>
#include <SimpleTimer.h>

#define    STX          0x02
#define    ETX          0x03
#define    SLOW         750                            // Datafields refresh rate (ms)
#define    FAST         250                            // Datafields refresh rate (ms)

//Pinos do Ultrasom
#define PINO_TRIGGER A5
#define PINO_ECHO    A4
#define BUZZER       A3

SoftwareSerial mySerial(A1,A0);                        // BlueTooth module: pin#2=TX pin#3=RX
SimpleTimer timer;

byte cmd[8] = {0, 0, 0, 0, 0, 0, 0, 0};                // bytes received
byte buttonStatus = 0;                                 // first Byte sent to Android device
long previousMillis = 0;                               // will store last time Buttons status was updated
long sendInterval = SLOW;                              // interval between Buttons status transmission (milliseconds)
String displayStatus = "xxxx";                         // message to Android device
int timer_id;

// Motores
AF_DCMotor Motor1(1);
AF_DCMotor Motor2(3);
AF_DCMotor Motor3(4); //Lamina do Cortador

//Inicializa o sensor ultrasonico
Ultrasonic ultrasonic(PINO_TRIGGER, PINO_ECHO);

void toca_alarme() 
{
//  Serial.print("Uptime (s): ");
//  Serial.println(millis() / 1000);  
  tone(BUZZER,100,300);
}

void setup()  {
  Serial.begin(9600);
  mySerial.begin(9600);                                // 57600 = max value for softserial
  Serial.println(VERSION);
  while(mySerial.available())  mySerial.read();         // empty RX buffer
}

void loop() 
{
  timer.run();  // necessário para o timer
  
  if(mySerial.available())  {                           // data received from smartphone
    delay(2);
    cmd[0] =  mySerial.read();  
    if(cmd[0] == STX)  {
      int i=1;      
      while(mySerial.available())  {
        delay(1);
        cmd[i] = mySerial.read();
        if(cmd[i]>127 || i>7)                 break;     // Communication error
        if((cmd[i]==ETX) && (i==2 || i==7))   break;     // Button or Joystick data
        i++;
      }
      if     (i==2)          getButtonState(cmd[1]);    // 3 Bytes  ex: < STX "C" ETX >
      else if(i==7)          getJoystickState(cmd);     // 6 Bytes  ex: < STX "200" "180" ETX >
    }
  } 
  sendBlueToothData(); 
}

void sendBlueToothData()  {
  static long previousMillis = 0;                             
  long currentMillis = millis();
  if(currentMillis - previousMillis > sendInterval) {   // send data back to smartphone
    previousMillis = currentMillis; 

// Data frame transmitted back from Arduino to Android device:
// < 0X02   Buttons state   0X01   DataField#1   0x04   DataField#2   0x05   DataField#3    0x03 >  
// < 0X02      "01011"      0X01     "120.00"    0x04     "-4500"     0x05  "Motor enabled" 0x03 >    // example

    mySerial.print((char)STX);                                             // Start of Transmission
    mySerial.print(getButtonStatusString());  mySerial.print((char)0x1);   // buttons status feedback
    mySerial.print(Voltagem_bateria());            mySerial.print((char)0x4);   // datafield #1
    mySerial.print(GetdataFloat2());          mySerial.print((char)0x5);   // datafield #2
    mySerial.print(displayStatus);                                         // datafield #3
    mySerial.print((char)ETX);                                             // End of Transmission
  }  
}

String getButtonStatusString()  {
  String bStatus = "";
  for(int i=0; i<6; i++)  {
    if(buttonStatus & (B100000 >>i))      bStatus += "1";
    else                                  bStatus += "0";
  }
  return bStatus;
}

int Voltagem_bateria()  {              // Data dummy values sent to Android device for demo purpose
int Valor_sensor = analogRead (A2);
float voltagem = Valor_sensor * (5.0 / 1023.0);

return voltagem;

}

float GetdataFloat2()  {           // Data dummy values sent to Android device for demo purpose
  static float i=50;               // Replace with your own code
  i-=.5;
  if(i <-50)    i = 50;
  return i;  
}

void getJoystickState(byte data[8])    {
  int velocidade = 0;  // vai de 0 a 255
  int joyX = (data[1]-48)*100 + (data[2]-48)*10 + (data[3]-48);       // obtain the Int from the ASCII representation
  int joyY = (data[4]-48)*100 + (data[5]-48)*10 + (data[6]-48);
  joyX = joyX - 200;                                                  // Offset to avoid
  joyY = joyY - 200;                                                  // transmitting negative numbers

  if(joyX<-100 || joyX>100 || joyY<-100 || joyY>100)     return;      // commmunication error
  
// Your code here ...
    Serial.print("Joystick position:  ");
    Serial.print(joyX);  
    Serial.print(", ");  
    Serial.println(joyY);
   
    // Controle dos motores
    if (joyY > 0 && joyX <= 11 && joyX >= -11)  // direção do joystick para frente
    {
      float cmMsec;   // Variavel de Sensor Ultrasom 
      long microsec = ultrasonic.timing();                   //Le os dados do sensor, com o tempo de retorno do sinal
      cmMsec = ultrasonic.convert(microsec, Ultrasonic::CM); //Calcula a distancia em centimetros
      
      if(cmMsec > 10)
      {      
          Motor1.run(FORWARD);
          Motor2.run(FORWARD);
          velocidade = joyY * 255/100;
          // os 2 motores em frente
          Motor1.setSpeed(velocidade);  // esquerdo
          Motor2.setSpeed(velocidade);  // direito
      }
      else
      {
        // motores parados
        Motor1.run(RELEASE);
        Motor2.run(RELEASE);
        Motor3.run(RELEASE);

        timer_id = timer.setTimeout(10000, toca_alarme);      // toca buzzer após 10 segundos
      }
    }
    else if (joyY < 0 && joyX <= 11 && joyX >= -11) // direção do joystick para trás (ré)
    {
      Motor1.run(BACKWARD);
      Motor2.run(BACKWARD);
      velocidade = -joyY * 255/100;
      // os 2 motores em ré
      Motor1.setSpeed(velocidade);  // esquerdo  
      Motor2.setSpeed(velocidade);  // direito
      
      timer.deleteTimer(timer_id);  // para o timer
    }        
    else if ( joyX < -11 && joyY >= -11 && joyY <= 11 )
    {
      Motor1.run(FORWARD);
      Motor2.run(FORWARD);
      velocidade = -joyX * 255/100;
      // apenas o motor direito
      Motor1.setSpeed(0);  // esquerdo
      Motor2.setSpeed(velocidade);  // direito
      
      timer.deleteTimer(timer_id);  // para o timer
    }
    else if ( joyX > 11 && joyY >= -11 && joyY <= 11 )
    {
      Motor1.run(FORWARD);
      Motor2.run(FORWARD);
      velocidade = joyX * 255/100;
      // apenas o motor esquerdo
      Motor1.setSpeed(velocidade);  // esquerdo
      Motor2.setSpeed(0);  // direito
      
      timer.deleteTimer(timer_id);  // para o timer
    }
    else // if (joyX == 0)
    {
      // motores parados
      Motor1.run(RELEASE);
      Motor2.run(RELEASE);
    }
    
}

void getButtonState(int bStatus)  {
  switch (bStatus) {
// -----------------  BUTTON #1  -----------------------
    case 'A':
      buttonStatus |= B000001;        // ON
      Serial.println("\n** Button_1: ON **");
      // your code...
      Motor3.setSpeed(255); //Lamina do Cortador 
       Motor3.run(FORWARD);
      displayStatus = "Lamina <ON>";
      Serial.println(displayStatus);
      break;
    case 'B':
      buttonStatus &= B111110;        // OFF
      Serial.println("\n** Button_1: OFF **");
      // your code... 
      Motor3.run(RELEASE); //Lamina do Cortador      
      displayStatus = "Lamina <OFF>";
      Serial.println(displayStatus);
      break;

// -----------------  BUTTON #2  -----------------------
    case 'C':
      buttonStatus |= B000010;        // ON
      Serial.println("\n** Button_2: ON **");
      // your code...      
      displayStatus = "Button2 <ON>";
      Serial.println(displayStatus);
      break;
    case 'D':
      buttonStatus &= B111101;        // OFF
      Serial.println("\n** Button_2: OFF **");
      // your code...      
      displayStatus = "Button2 <OFF>";
      Serial.println(displayStatus);
      break;

// -----------------  BUTTON #3  -----------------------
    case 'E':
      buttonStatus |= B000100;        // ON
      Serial.println("\n** Button_3: ON **");
      // your code...      
      displayStatus = "Motor #1 enabled"; // Demo text message
      Serial.println(displayStatus);
      break;
    case 'F':
      buttonStatus &= B111011;      // OFF
      Serial.println("\n** Button_3: OFF **");
      // your code...      
      displayStatus = "Motor #1 stopped";
      Serial.println(displayStatus);
      break;

// -----------------  BUTTON #4  -----------------------
    case 'G':
      buttonStatus |= B001000;       // ON
      Serial.println("\n** Button_4: ON **");
      // your code...      
      displayStatus = "Datafield update <FAST>";
      Serial.println(displayStatus);
      sendInterval = FAST;
      break;
    case 'H':
      buttonStatus &= B110111;    // OFF
      Serial.println("\n** Button_4: OFF **");
      // your code...      
      displayStatus = "Datafield update <SLOW>";
      Serial.println(displayStatus);
      sendInterval = SLOW;
     break;

// -----------------  BUTTON #5  -----------------------
    case 'I':           // configured as momentary button
//      buttonStatus |= B010000;        // ON
      Serial.println("\n** Button_5: ++ pushed ++ **");
      // your code...      
      displayStatus = "Button5: <pushed>";
      break;
//   case 'J':
//     buttonStatus &= B101111;        // OFF
//     // your code...      
//     break;

// -----------------  BUTTON #6  -----------------------
    case 'K':
      buttonStatus |= B100000;        // ON
      Serial.println("\n** Button_6: ON **");
      // your code...      
       displayStatus = "Button6 <ON>"; // Demo text message
     break;
    case 'L':
      buttonStatus &= B011111;        // OFF
      Serial.println("\n** Button_6: OFF **");
      // your code...      
      displayStatus = "Button6 <OFF>";
      break;
  }
// ---------------------------------------------------------------
}

