#include <SoftwareSerial.h>
//RX is digital pin 2 (connect to TX of other device)
//TX is digital pin 3 (connect to RX of other device)
#include "simpleALOHA.h"
#define TBASE			20
#define nstep			1000

SoftwareSerial mySerial(2, 3); // RX, TX
modbus_t txobj, rxobj;
//toggle per più pulsanti
unsigned long prec=0;
byte led=9; 
byte btn=4;
byte txpin=10;
uint8_t val = 0;
unsigned long step = 0;
uint8_t statet=0; 
byte precval=0; 

bool toggleh(byte valb){
	//edge == HIGH --> fronte di salita
	//edge == LOW  --> fronte di discesa
	//n: numero di pulsanti
	bool changed=false;
	if ((valb == HIGH) && (precval == LOW)){ //campiona solo le transizioni da basso a alto 
		statet = !statet;
		changed=true;
	}   
	precval = valb;            // valore di val campionato al loop precedente 
	return changed;
}


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
  txobj.u8da = 1; 
  txobj.msglen = 1;
  //txobj.data = "Salve sono Nano da disp 1";
  //txobj.msglen = strlen((char*)txobj.data )+1;
}

void loop() // run over and over
{
	poll(&rxobj,&val);
	
	if(millis()-prec > TBASE){
		prec = millis();
		if(toggleh(digitalRead(btn))){
			txobj.data = &statet;
			txobj.msglen = 1;
			sendMsg(&txobj);
		}	
	}
}

void rcvEventCallback(modbus_t* rcvd){
	digitalWrite(led, val);
	Serial.print("VAL-N:");
	Serial.println((unsigned)val);
	Serial.print("RCV_LED-N:");
	Serial.print((unsigned) getInCnt());
	Serial.print("- RCV_ERR-N:");
	Serial.print((unsigned) getErrCnt());
	Serial.print("- BER:");
	Serial.print((unsigned) getErrInRatio());
	Serial.print(" - OUT_ACKED:");
	Serial.print((unsigned) getInAckOutMsgRatio());
	Serial.print(" - REOUTED:");
	Serial.println((unsigned) getReOutMsgOutMsgRatio());
}

