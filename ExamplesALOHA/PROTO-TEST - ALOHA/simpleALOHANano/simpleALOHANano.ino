#include <SoftwareSerial.h>
//RX is digital pin 2 (connect to TX of other device)
//TX is digital pin 3 (connect to RX of other device)
#include "simpleALOHA.h"
#define TBASE			20
#define nstep			1000
SoftwareSerial mySerial(2, 3); // RX, TX
modbus_t txobj, rxobj;
//toggle per più pulsanti
uint8_t statet=0; 

unsigned long prec=0;
byte led=9; 
byte btn=2;
byte txpin=10;
uint8_t val;
unsigned long step = 0;


void setup()
{
  pinMode(txpin, OUTPUT);  
  pinMode(led, OUTPUT); 
  pinMode(btn, INPUT);
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  Serial.println("I am nano!");
  mySerial.begin(9600);
  init(&mySerial, txpin, 2, 1, 0); // port485, txpin, mysa, mygroup4, speed=9600
  //preparazione messaggio TX (parallelo)
  txobj.u8da = 1; //multicast sul gruppo
  txobj.msglen = 1;
  //txobj.data = "Salve sono Nano da disp 1";
  //txobj.msglen = strlen((char*)txobj.data )+1;
}

void loop() // run over and over
{
	poll(&rxobj,&val);
	
	if(millis()-prec > TBASE){
		prec = millis();
		step = (step + 1) % nstep;    // conteggio circolare arriva al massimo a nstep-1
		
		if(!(step%random(0, 10))){	
		//if(!(step%50)){
			statet = !statet;
			txobj.data = &statet;
			sendMsg(&txobj);
			//digitalWrite(led, !digitalRead(led));
		}	
	}
}

void rcvEventCallback(modbus_t* rcvd){
	Serial.println((int) rcvd->data);
	digitalWrite(led, rcvd->data);
	Serial.print("RCV_LED-N:");
	Serial.print(getInCnt());
	Serial.print("- BER:");
	Serial.print(getErrInRatio());
	Serial.print(" - OUT_ACKED:");
	Serial.print(getInAckOutMsgRatio());
	Serial.print(" - REOUTED:");
	Serial.println(getReOutMsgOutMsgRatio());
}
