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
#ifndef __myprotoALOHA__
#define __myprotoALOHA__
#include <inttypes.h>
#include "Arduino.h"
#define MAX_BUFFER 		64
#define STOP_BIT  		5  
#define MSG  			1 	
#define ACK  			129 //(100000001)  il primo (MSB) bit è un ack bit
#define TBASE			20
#define MAXATTEMPTS  	5
#define WNDW    		20
// SlotTime = CCATime + RxTxTurnaroundTime + AirPropagationTime+ MACProcessingDelay 
// SIFS < DIFS < EIFS
#define DIFS 			5	//DIFS =  SIFS  + (2 * Slot time) 
#define SIFS 			1
#define TXTIMEOUT 		2000
#define DEBUG  			0

#if (DEBUG)
	#define DEBUG_PRINT(x)   	Serial.print (x)
	#define DEBUG_PRINTLN(x)  	Serial.println (x)
#else
	#define DEBUG_PRINT(x)
	#define DEBUG_PRINTLN(x) 
#endif		
//SoftwareSerial mySerial(3, 4); // RX, TX
enum PROTO_STATE
{
    WAITSTATE             	  	= 1,
    ACKSTATE                  	= 2,
	BACKOFF_STARTED				= 3,
	TRANSMIT_INTERRUPTED		= 4
};

enum ERR_LIST
{
    ERR_BUFF_OVERFLOW             = -1,
    ERR_BAD_CRC                   = -2,
    ERR_EXCEPTION                 = -3
};

enum MESSAGE
{
    DA                           = 0, //!< ID field
	SA,
    GROUP, 		//!< Function code position
    SI, 		//!< Service identifier
    BYTE_CNT,  	//!< byte counter
	PAYLOAD 	//!<  start of data
};

typedef struct
{
    uint8_t u8da;          /*!< Slave address between 1 and 247. 0 means broadcast */ 
	uint8_t u8sa;          /*!< Slave address between 1 and 247. 0 means broadcast */
    uint8_t u8group;         /*!< Function code: 1, 2, 3, 4, 5, 6, 15 or 16 */
    uint8_t u8si;    /*!< Address of the first register to access at slave/s */
    uint8_t *data;     /*!< Pointer to memory image in master */
	uint8_t msglen;
} modbus_t;

uint8_t getMySA();

uint8_t getMyGroup();

uint16_t getOutCnt();

uint16_t getErrCnt();

uint16_t getInCnt();

uint16_t getAckCnt();

float getErrInRatio();

float getInAckOutMsgRatio();

float getReOutMsgOutMsgRatio();

void init(Stream *, uint8_t , uint8_t , uint8_t , uint32_t);

extern void rcvEventCallback(modbus_t* );

inline long getBackoff();

inline void parallelToSerial(const modbus_t *);
//--------------------------------------------------------------------------------------------------------------
// Converte il messaggio dal formato parallelo (struct) a quello seriale (array di char)
bool sendMsg(modbus_t *);

void resendMsg(const modbus_t *);

int8_t poll(modbus_t *, uint8_t *);
//----------------------------------------------------------------------------------------------------------
void sendTxBuffer(uint8_t);

int8_t getRxBuffer();

void rcvEvent(modbus_t* , uint8_t );
//--------------------------------------------------------------------------------------------------------------
//lo calcola dal primo byte del messaggio (header compreso)
uint16_t calcCRC(uint8_t);
#endif //myprotoCSMA