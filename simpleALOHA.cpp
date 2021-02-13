/**
 * @license
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; version
 *  2.1 of the License.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 **/
//#include <SoftwareSerial.h>
#include <inttypes.h>
#include "Arduino.h"
#include "simpleCSMA.h"
uint8_t u8Buffer[MAX_BUFFER]; // buffer unico condiviso per TX e RX (da usare alternativamente)
uint8_t _txpin;
uint16_t u16InCnt, u16OutCnt, u16errCnt, u16inAckCnt, u16inMsgCnt, u16OutMsgCnt, u16reOutMsgCnt, u16noreOutMsgCnt;
uint8_t u8lastRec; // per rivelatore di fine trama (stop bit)
modbus_t ackobj, *appobj;
uint8_t mysa;
uint8_t mygroup;
uint32_t u32time, u32difsTime;
Stream *port;
//uint8_t state;
uint8_t u8state = WAITSTATE;
unsigned long precBack=0;
unsigned long precAck=0;
unsigned long backoffTime=0;
unsigned long timeoutTime=TXTIMEOUT;
uint8_t retry;
bool cca = true;
bool started = false;


uint8_t getMySA(){
	return mysa;
}

uint8_t getMyGroup(){
	return mygroup;
}

uint16_t getErrCnt(){
	return u16errCnt;
}

uint16_t getInCnt(){
	return u16InCnt;
}

uint16_t getOutCnt(){
	return u16OutCnt;
}

uint16_t getInAckCnt(){
	return u16inAckCnt;
}

uint16_t getOutAckCnt(){
	return u16inAckCnt;
}

float getErrInRatio(){
	return  ((float) u16errCnt / u16InCnt)*100;
}

float getInAckOutMsgRatio(){
	return ((float) u16inAckCnt / u16OutMsgCnt)*100;
}

float getReOutMsgOutMsgRatio(){
	return ((float) u16reOutMsgCnt / u16OutMsgCnt)*100;
}

void init(Stream *port485, uint8_t _txpin485, uint8_t mysa485, uint8_t mygroup485, uint32_t u32speed=9600){
	randomSeed(analogRead(0));
	port = port485;
	_txpin = _txpin485;
	mysa = mysa485;
	mygroup = mygroup485;
	u16InCnt = u16OutCnt = u16errCnt = u16inAckCnt = u16inMsgCnt = u16OutMsgCnt = u16reOutMsgCnt = u16noreOutMsgCnt = 0;
	static_cast<HardwareSerial*>(port)->begin(u32speed);
	ackobj.u8sof = SOFV;
	ackobj.u8sa = mysa;
	ackobj.u8group = mygroup;
	ackobj.u8si = ACK;
	ackobj.data = 0;
	ackobj.msglen = 0;
}

extern void rcvEventCallback(modbus_t* rcvd);

inline long getBackoff(){
	return random(0, WNDW*pow(2,retry));
}

inline void parallelToSerial(const modbus_t *tosend){
	//copia header
	u8Buffer[ DA ] = tosend->u8da;
	u8Buffer[ SA ] = tosend->u8sa;
	u8Buffer[ GROUP ] = tosend->u8group;
	u8Buffer[ SI ] = tosend->u8si;
	u8Buffer[ BYTE_CNT ] = tosend->msglen + PAYLOAD;
	//copia payload
	for(int i=0; i < tosend->msglen; i++){
		u8Buffer[i+PAYLOAD] = tosend->data[i];
	}
}
//--------------------------------------------------------------------------------------------------------------
// Converte il messaggio dal formato parallelo (struct) a quello seriale (array di char)
bool sendMsg(modbus_t *tosend){
	tosend->u8si = (uint8_t) MSG;
	appobj = tosend;
	tosend->u8sa = mysa;
	tosend->u8group = mygroup;
	tosend->multicast = (tosend->u8da == 0xFF);
	bool sent = false;
	//DEBUG_PRINTLN(((int)u8state);
	DEBUG_PRINT("Sent Msg DA: ");
	DEBUG_PRINTLN((uint8_t)tosend->u8da);
	DEBUG_PRINT("Sent Msg SA: ");
	DEBUG_PRINTLN((uint8_t)tosend->u8sa);
	if(u8state == WAITSTATE){
		DEBUG_PRINTLN("copiato:");
		if(cca){//accesso immediato
			sent = true;
			parallelToSerial(tosend);
			sendTxBuffer(u8Buffer[ BYTE_CNT ]); //trasmette sul canale
			if(tosend->multicast){
				u8state = WAITSTATE;
			}else{
				u8state = ACKSTATE;
				precAck = millis();	
				DEBUG_PRINTLN("DIFS_ACKSTATE:");
			}
			u16noreOutMsgCnt++;
			u16OutMsgCnt++;
		}else{//accesso differito
			u8state = TX_DEFERRED;
			//backoffTime = getBackoff();
			//precBack = millis();
			retry = 0;
			DEBUG_PRINT("TX_DEFERRED: ");
			//DEBUG_PRINT(backoffTime);
			//DEBUG_PRINT(", retry: ");
			//DEBUG_PRINTLN(retry);
		}
	}//else messaggio non si invia....
	return sent;
}

void resendMsg(const modbus_t *tosend){
	//copia header
	parallelToSerial(tosend);
	sendTxBuffer(u8Buffer[ BYTE_CNT ]); //trasmette sul canale
}

int8_t poll(modbus_t *rt, uint8_t *buf) // valuta risposte pendenti
{	
	// controlla se è in arrivo un messaggio
	uint8_t u8current;
	
    u8current = port->available(); // vede se è arrivato un "pezzo" iniziale del messaggio (frame chunk)

    if (u8current == 0){ //canale libero
		//se il canale è libero controlla la scadenza di un eventuale backoff di prima trasmissione
		if(u8state == DIFS_BACKOFF_STARTED){ 
			// controlla se è scaduto il backoff
			if(millis()-precBack > backoffTime){
				DEBUG_PRINTLN("DIFS_BACKOFF_scaduto: ");
				resendMsg(appobj); //trasmette sul canale
				u16OutMsgCnt++;
				u16noreOutMsgCnt++;
				if(appobj->multicast){
					u8state = WAITSTATE;
				}else{
					u8state = ACKSTATE;	
					precAck = millis();
				}
			}
		}
        //se il canale è libero controlla la scadenza di un eventuale backoff di ritrasmissione
		if(u8state == BACKOFF_STARTED){ 
			// controlla se è scaduto il backoff
			if(millis()-precBack > backoffTime){
				DEBUG_PRINTLN("BACKOFF_SCADUTO: ");
				resendMsg(appobj); //trasmette sul canale
				u16reOutMsgCnt++;
				u16OutMsgCnt++;
				if(appobj->multicast){
					u8state = WAITSTATE;
				}else{
					u8state = ACKSTATE;	
					precAck = millis();
				}
			}
		}
		//controlla se c'è una transizione da occupato a libero
		if(cca == false && !started){ //transizione da occupato a libero
			u32difsTime = millis(); // da adesso aspetta un DIFS
			started = true;
			if((u8state == BACKOFF_STARTED) || (u8state == DIFS_BACKOFF_STARTED)){
				precBack = millis(); 
			}
			DEBUG_PRINTLN("cca changed to true 1: ");
		}
		//controlla se è passato un DIFS
		if((unsigned long)(millis() -u32difsTime) > (unsigned long)DIFS){
			cca = true; // IDLE ed è passato un DIFS
			started = false;	
			// elenco operazioni da fare allo scadere del DIFS
			//1) fare partire il backoff di prima trasmissione
			if(u8state == TX_DEFERRED){//la catena di backoff INIZIA a cca asserito (DIFS scaduto)
					u8state = DIFS_BACKOFF_STARTED;
					backoffTime = getBackoff();
					precBack = millis();
					retry = 0;
					DEBUG_PRINT("DIFS_BACKOFF_STARTED: ");
					DEBUG_PRINT(backoffTime);
					DEBUG_PRINT(", retry: ");
					DEBUG_PRINTLN(retry);
			}
			//2) fare partire il backoff di ritrasmissione
			else if(u8state == ACKSTATE){ //canale libero dopo un DIFS
				if(millis()-precAck > timeoutTime){
					if(retry < MAXATTEMPTS){
						backoffTime = getBackoff();
						precBack = millis();
						retry++;
						DEBUG_PRINT("BACKOFF_STARTED: ");
						DEBUG_PRINT(backoffTime);
						DEBUG_PRINT(", retry: ");
						DEBUG_PRINTLN(retry);
						u8state = BACKOFF_STARTED;
					}else{
						DEBUG_PRINTLN("MAX_ATTEMPT");
						retry = 0;
						u8state = WAITSTATE;
						DEBUG_PRINTLN("WAITSTATE:");
					}
				}
			}
		}
		return 0;  // se non è arrivato nulla per ora basta, ricontrolla al prossimo giro
	}else{ // canale occupato
		//controlla transizione da libero a occupato
		if(cca){
			cca = false;
			if((u8state == BACKOFF_STARTED) || (u8state == DIFS_BACKOFF_STARTED)){
				backoffTime = backoffTime - millis();
			}	
			DEBUG_PRINTLN("cca changed to false: ");
		}
		//u8complete = port2->find(SOFV);
	}
	
    // controlla se c'è uno STOP_BIT dopo la fine, se non c'è allora la trama non è ancora completamente arrivata
    if (u8current != u8lastRec)
    {
        // aggiorna ogni volta che arriva un nuovo carattere!
		u8lastRec = u8current;
        u32time = millis();
		//DEBUG_PRINTLN(("STOP_BIT:");
        return 0;
    }
	
	// Se la distanza tra nuovo e vecchio carattere è minore di uno stop bit ancora la trama non è completa
    if ((unsigned long)(millis() -u32time) < (unsigned long)STOP_BIT) return 0;
	
	int8_t i8state = getRxBuffer();  // altrimenti recupera tutto il messaggio e mettilo sul buffer
	
    if ((i8state > 0) && (i8state < PAYLOAD + 1)) // se è incompleto scartalo
    {
		u16errCnt++;
        return i8state;
    }
	//DEBUG_PRINTLN(("msg completo");
	//DEBUG_PRINT("DA messaggio: ");
	//DEBUG_PRINTLN(((uint8_t)u8Buffer[ DA ]);
	//DEBUG_PRINT("SA mio: ");
	//DEBUG_PRINTLN(((uint8_t)mysa);
    if ((u8Buffer[ DA ] != mysa) && !((u8Buffer[ GROUP ] == mygroup)) && (u8Buffer[ DA ] == 255)){
		DEBUG_PRINTLN("msg non destinato a me");
		DEBUG_PRINT("DA: ");
		DEBUG_PRINTLN((uint8_t)u8Buffer[ DA ]);
		DEBUG_PRINT("SA mio: ");
		DEBUG_PRINTLN((uint8_t)mysa);
		return 0;  // altrimenti se il messaggio non è indirizzato a me...scarta
	}else{
		DEBUG_PRINTLN("msg destinato a me");
		DEBUG_PRINT("DA: ");
		DEBUG_PRINTLN((uint8_t)u8Buffer[ DA ]);
		DEBUG_PRINT("SA mio: ");
		DEBUG_PRINTLN((uint8_t)mysa);
		DEBUG_PRINT("SI: ");
		DEBUG_PRINTLN((uint8_t)u8Buffer[ SI ]);
	}
	
	// se il messaggio è destinato a me ma è corrotto
	if (i8state < 0){
		if(u8state == ACKSTATE){ 
			DEBUG_PRINTLN("ACK_CORROTTO: ");
			if(retry < MAXATTEMPTS){
				backoffTime = getBackoff();
				precBack = millis();
				retry++;
				DEBUG_PRINT("BACKOFF_STARTED: ");
				DEBUG_PRINT(backoffTime);
				DEBUG_PRINT(", retry: ");
				DEBUG_PRINTLN(retry);
				u8state = BACKOFF_STARTED;
			}else{
				DEBUG_PRINTLN("MAX_ATTEMPT");
				retry = 0;
				u8state = WAITSTATE;
				DEBUG_PRINTLN("WAITSTATE:");
			}
		}
		
		if (u8Buffer[ SI ] == MSG){
			if(u8Buffer[ DA ] == 0xFF){
				ackobj.multicast = true;
				ackobj.u8da = u8Buffer[ SA ];
				ackobj.u8si = NACK;
				sendMsg(&ackobj);
				DEBUG_PRINT("ERROR: ");
				DEBUG_PRINTLN((int) i8state);
				DEBUG_PRINT("SENDING NACK TO: ");
				DEBUG_PRINTLN((int) ackobj.u8da);
				return i8state; // altrimenti gli altri di seguito leggono il valore del buffer di trasmissione
			}
		}
	}
	//DEBUG_PRINTLN(("msg destinato a me");
	if (u8Buffer[ SI ] == MSG){
		//ackobj.u8sa = mysa;
		ackobj.u8da = u8Buffer[ SA ]; //problema!
		//ackobj.u8group = u8Buffer[ GROUP ];
		//ackobj.u8si = u8Buffer[ SI ];
		//ackobj.data = "";
		//ackobj.msglen = strlen((char*)ackobj.data )+1;
		rt->data = buf;
		rcvEvent(rt, i8state); // il messaggio è valido allora genera un evento di "avvenuta ricezione"	
		DEBUG_PRINTLN("MSG RECEIVED:");
		if(u8Buffer[ DA ] != 0xFF){
			ackobj.multicast = false;
			ackobj.u8si = ACK;
			resendMsg(&ackobj);
			DEBUG_PRINTLN(" ACKSENT:");
		}
		u16inMsgCnt++;
		rcvEventCallback(rt);   // l'evento ha il messaggio come parametro di out
		return i8state; // altrimenti gli altri di seguito leggono il valore del buffer di trasmissione
	}else if (u8Buffer[ SI ] == ACK){
		DEBUG_PRINTLN("ACK_RECEIVED:");
		if(u8state == ACKSTATE || u8state == BACKOFF_STARTED){
			u8state = WAITSTATE;	//next go to WAITSTATE
			DEBUG_PRINTLN("WAITSTATE:");
			retry = 0;
			u16inAckCnt++;
		}//else messaggio di ack si perde....
	}else if(u8Buffer[ SI ] == NACK){
		DEBUG_PRINTLN("NACK_RECEIVED:");
		sendMsg(appobj);// il nack eve arrivare velocemente, altrimenti il TX reinvia il successivo!
		return i8state; // altrimenti gli altri di seguito leggono il valore del buffer di trasmissione
	}else{
		DEBUG_PRINT("MESSAGGIO SCONOSCIUTO, SI:");
		DEBUG_PRINTLN((int) u8Buffer[ SI ]);
	}
    return i8state;
}
//----------------------------------------------------------------------------------------------------------
void sendTxBuffer(uint8_t u8BufferSize){
	//DEBUG_PRINT("size1: ");
	//DEBUG_PRINTLN((u8BufferSize);
    // append CRC to MSG
    uint16_t u16crc = calcCRC( u8BufferSize );
    u8Buffer[ u8BufferSize ] = u16crc >> 8;  //seleziona il byte più significativo
    u8BufferSize++;
    u8Buffer[ u8BufferSize ] = u16crc & 0x00ff; //seleziona il byte meno significativo
    u8BufferSize++;
	// add end delimiter
	u8Buffer[ u8BufferSize ] = SOFV;
    u8BufferSize++;

	if (_txpin > 1)
    {
        // set RS485 transceiver to transmit mode
        digitalWrite( _txpin, HIGH );
    }
	DEBUG_PRINT("size: ");
	DEBUG_PRINTLN(u8BufferSize);
	for(int i=0;i<u8BufferSize;i++){
		DEBUG_PRINT(u8Buffer[i]);DEBUG_PRINT("-");
	}
	DEBUG_PRINTLN();
    // transfer buffer to serial line
    port->write( u8Buffer, u8BufferSize );
    if (_txpin > 1)
    {
        // must wait transmission end before changing pin state
        // soft serial does not need it since it is blocking
        // ...but the implementation in SoftwareSerial does nothing
        // anyway, so no harm in calling it.
        port->flush();
        // return RS485 transceiver to receive mode
        digitalWrite( _txpin, LOW );
    }
    while(port->read() >= 0);
    // increase MSG counter
    u16OutCnt++;
}

int8_t getRxBuffer()
{
    boolean bBuffOverflow = false;
	// ripritina il transceiver RS485  in modo ricezione (default)
    if (_txpin > 1) digitalWrite( _txpin, LOW );
	//DEBUG_PRINT("received: ");
    uint8_t u8BufferSize = 0;
	DEBUG_PRINT("RCV_BUFFER: ");
    while ( port->available() ) // finchè ce ne sono, leggi tutti i caratteri disponibili
    {							// e mettili sul buffer di ricezione
		uint8_t curr = port->read();
		if(curr != SOFV){
			u8Buffer[ u8BufferSize ] = curr;
			DEBUG_PRINT("(");
			DEBUG_PRINT((char) u8Buffer[ u8BufferSize ]);
			DEBUG_PRINT(":");
			DEBUG_PRINT((uint8_t) u8Buffer[ u8BufferSize ]);
			DEBUG_PRINT("),");
			u8BufferSize ++;
			// segnala evento di buffer overflow (un attacco hacker?)
			if (u8BufferSize >= MAX_BUFFER){
				u16InCnt++;
				u16errCnt++;
				return ERR_BUFF_OVERFLOW;
			}
		}else{
			break;
		}
    }
	DEBUG_PRINTLN();
	// confonta il CRC ricevuto con quello calcolato in locale
    uint16_t u16MsgCRC =
        ((u8Buffer[u8BufferSize - 2] << 8)
         | u8Buffer[u8BufferSize - 1]); // combine the crc Low & High bytes
    if ( calcCRC( u8BufferSize-2 ) != u16MsgCRC ) //except last two byte
    {
        u16errCnt ++;
		u16InCnt++;
        return ERR_BAD_CRC;
    }

	DEBUG_PRINTLN("");
    u16InCnt++;
    return u8BufferSize;
}

void rcvEvent(modbus_t* rcvd, uint8_t msglen){
	// converti da formato seriale (array di char) in formato parallelo (struct)
	// header

	rcvd->u8da = u8Buffer[ SA ];
	rcvd->u8group = u8Buffer[ GROUP ];
	rcvd->u8si = u8Buffer[ SI ];
	rcvd->msglen = u8Buffer[ BYTE_CNT ];
	// payload
	for(int i=0; i < msglen-PAYLOAD; i++){
		rcvd->data[i] = u8Buffer[i+PAYLOAD];
	}
	// notifica l'evento di ricezione all'applicazione con una callback
}
//--------------------------------------------------------------------------------------------------------------
//lo calcola dal primo byte del messaggio (header compreso)
uint16_t calcCRC(uint8_t u8length)
{
    unsigned int temp, temp2, flag;
    temp = 0xFFFF;
    for (unsigned char i = 0; i < u8length; i++)
    {
        temp = temp ^ u8Buffer[i];
        for (unsigned char j = 1; j <= 8; j++)
        {
            flag = temp & 0x0001;
            temp >>=1;
            if (flag)
                temp ^= 0xA001;
        }
    }
    // Reverse byte order.
    temp2 = temp >> 8;
    temp = (temp << 8) | temp2;
    temp &= 0xFFFF;
    // the returned value is already swapped
    // crcLo byte is first & crcHi byte is last
    return temp;
}
