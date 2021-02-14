//#include <SoftwareSerial.h>
#include "simpleALOHA.h"
#define TBASE			20
#define nstep			1000

modbus_t txobj, rxobj;
unsigned long prec=0;
byte led=9; 
byte btn=4;
byte txpin=10;
uint8_t val;
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
  Serial.println("I am Mega!");
  init(&Serial1, txpin, 1, 1, 9600); // port485, txpin, mysa, mygroup4, speed=9600
  //preparazione messaggio TX (parallelo)
  txobj.u8da = 2;
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
	Serial.println((int)rcvd->data);
	digitalWrite(led, val);
	Serial.print("RCV_LED: ");
}

/*
void loop() // run over and over
{
	poll(&rxobj,&val);
	
	if(millis()-prec > TBASE){
		prec = millis();
		step = (step + 1) % nstep;    // conteggio circolare arriva al massimo a nstep-1
		//if(togglen(digitalRead(btn), HIGH, 0)){
			//txobj.data = &statet[0];
			txobj.msglen = 1;
			//sendMsg(&txobj);
			if(!(step%random(0, 10))){	
				statet[0] = !statet[0];
				txobj.data = &statet[0];
				sendMsg(&txobj);
			}	
		//}	
	}
}

void rcvEventCallback(modbus_t* rcvd){
	Serial.println((int)val);
	digitalWrite(led, val);
	Serial.print("RCV_LED: ");
}
*/



