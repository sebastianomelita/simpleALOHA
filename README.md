# simpleALOHA
Implementazione del protocollo ALOHA con rilevazione delle collisioni basata sulla perdita o sulla non correttezza degli ack inviati dal ricevitore. Si adopera su un bus interfacciato con un transceiver RS485. Il transceiver provato è un MAX485 con piedino di controllo della direzione. Dovrebbe funzionare anche con un transceiver col controllo automatico della direzione (piedino con una impostazione qualsiasi).
Sostanzialmente è un rimaneggiamento del codice citato di seguito:
 * @file 	ModbusRtu.h
 * @version     1.21
 * @date        2016.02.21
 * @author 	Samuel Marco i Armengol
 * @contact     sammarcoarmengol@gmail.com
 * @contribution Helium6072
 
 Trama: 
 
        |---DA---|---SA---|---GROUP---|---SI---|---BYTE_CNT---|---PAYLOAD---|---CRC---|
 
        |---1B---|---1B---|----1B-----|---1B---|------1B------|---VARIABLE--|---2B----|
 
 DA: destination address - 1byte (1-254, 255 indirizzo di broadcast)
 
 SA: source addresss - 1byte da 1 a 254
 
 Group: group addresss - 1byte da 1 a 254 (per inviare a tutti membri del gruppo DA deve essere 0xFF o 255)
 
 SI: service identifier (ACK, MSG) - 1byte
 
 BYTE_CNT: numero byte complessivi (+payload -CRC) - 1byte
 
 CRC: Cyclic Redundancy Check - 2byte (calcolati su tutto il messaggio)
 
 Il buffer di trasmissione memorizza un solo messaggio ed è a comune tra trasmissione e ricezione. 
 
 E' possibile rispondere ad un messaggio con un ack mentre si attende, a propria volta, un ACK.
 
Si può trasmettere un messaggio solo al completemento della trasmissione del precedente che si conclude alla ricezione di un ACK o allo scadere dei tentativi di ritrasmissione (di default 5). 

L'accettazione del messaggio per la trasmissione è segnalato da un true restituito dalla funzione di trasmissione sendMsg(). 

Se sendMsg() restisce false allora vuol dire che il protocollo è impegnato nella trasmissione precedente.

La rcvEventCallback() si attiva alla ricezione di un messaggio.

La poll(&rxobj,&val) deve essere chiamata ad ogni loop e ha due parametri modificabili: 

- rxobj è il datagramma in ricezione, e può essere sempre lo stesso

- val è una variabile o un'array di byte (BYTE, char, uint8_t, unsigned short) il cui riferimento è copiato nel campo dati del datagramma, può cambiare sia in riferimento che in valore. E' l'area di memoria su cui viene copiato il messaggio in arrivo.

Manda un messaggio non più lungo di 250 bit, può essere un numero o una sequenza di caratteri (ad es. un JSON)

All'invio riceve automaticamente la conferma dal destinatario, se non la riceve, ritrasmette fino a 5 volte, poi rinuncia.

Se l'ack di un invio non arriva in tempo, allo scadere di un timeout, la ritrasmissione avviene dopo un tempo casuale (backoff) all'interno di una finestra di trasmissione che si allarga ad ogni nuovo tentativo in maniera esponenziale.

Il tempo casuale serve a minimizzare la probabilità di collisione con le altre stazioni, la finestra variabile a tenere conto delle varie situazioni di traffico.



