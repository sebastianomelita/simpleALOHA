# simpleALOHA
Implementazione del protocollo ALOHA con rilevazione delle collisioni basata sulla perdita o sulla non correttezza degli ack inviati dal ricevitore. Si adopera su un bus interfacciato con un transceiver RS485. 

Si può usare per realizzare un sistema multimaster con stazioni che trasmettono indipendentemente l'una dall'altra senza la supervisione di un dispositivo centrale (master).

Il transceiver provato è un MAX485 con piedino di controllo della direzione. Dovrebbe funzionare anche con un transceiver col controllo automatico della direzione (piedino con una impostazione qualsiasi).

Sono possibili sia trasmissioni unicast che multicast trattate indipendentemente l'una dall'altra a livello di datagramma semplicemente impostando l'indirizzo individuale al valore di broadcast (255)

Supporta la libreria SoftwareSerial ma, in questo caso, bisogna impostare la velocità a 0 mella funzione init() e impostare la effettiva velocità della seriale emulata con il comando begin() dell'oggetto SoftwareSerial (nel setup() dello sketch).
 
 Trama: 
 
        |---DA---|---SA---|---GROUP---|---SI---|---BYTE_CNT---|---PAYLOAD---|---CRC---|---EOF---|
 
        |---1B---|---1B---|----1B-----|---1B---|------1B------|---VARIABLE--|---2B----|---1B----|
 
 
 - DA: destination address - 1byte (1-254, 255 indirizzo di broadcast)
 
 - SA: source addresss - 1byte da 1 a 254
 
 - GROUP: group addresss - 1byte da 1 a 254 (per inviare a tutti membri del gruppo DA deve essere 0xFF o 255)
 
 - SI: service identifier (ACK, MSG) - 1byte
 
 - BYTE_CNT: numero byte complessivi (+payload -CRC) - 1byte
 
 - CRC: Cyclic Redundancy Check - 2byte (calcolati su tutto il messaggio)
 
  - EOF: End Of Frame delimitatore di fine trama che dovrebbe limitare gli errori su trame molto vicine. Il valore NON DEVE essere presente nel campo dati (funzione di bit stuffing non implementata!)
 
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

Trama unicast:

- ha un indirizzo di gruppo (ad es. txobj.u8group = 2) e un indirizzo individuale unicast (ad es. txobj.u8da = 3)

- Manda un messaggio non più lungo di 250 bit, può essere un numero o una sequenza di caratteri (ad es. un JSON)

- All'invio riceve automaticamente la conferma dal destinatario, se non la riceve, ritrasmette fino a 5 volte, poi rinuncia.

- Se l'ack di un invio non arriva in tempo, allo scadere di un timeout, la ritrasmissione avviene dopo un tempo casuale (backoff) all'interno di una finestra di trasmissione che si allarga, ad ogni nuovo tentativo, in maniera esponenziale. Il tempo casuale serve a minimizzare la probabilità di collisione con le altre stazioni, la finestra variabile a tenere conto delle varie situazioni di traffico.

Trama multicast:

- ha un indirizzo di gruppo (ad es. txobj.u8group = 2) e un indirizzo individuale broadcast (ad es. txobj.u8da = 255)

- Manda un messaggio non più lungo di 250 bit, può essere un numero o una sequenza di caratteri (ad es. un JSON)

- è caratterizzata dall'indirizzo di destinazione 0xFF che significa broadcast all'interno del proprio gruppo

- Alla sua ricezione, il destinatario invia automaticamente una conferma negativa (NACK) se questo ha ricevuto una trama corrotta, alla ricezione del NACK il mittente reinvia la trama in unicast al membro del gruppo che ha ricevuto la trama corrotta.

Tempistiche e impostazioni:

- MAXATTEMPTS  	numero di tentativi di ritrasmissione in mancanza di un ack dal ricevitore

- WNDW    		dimensione iniziale della finestra di ritrasmissione, aumenta con la progressione WNDW*(2^NTENTATIVI)

- TXTIMEOUT 		timeoute di ritrasmissione. Tempo massimo di attesa di un ack, passato il quale, avviene la ritrasmissione

- Slot time tempo di propagazione tra le stazioni più distanti del sistema

- DEBUG  			1 attiva modo debug

Sostanzialmente il presente software è un rimaneggiamento del codice citato di seguito di cui si ringrazia l'autore:
 * @file 	ModbusRtu.h
 * @version     1.21
 * @date        2016.02.21
 * @author 	Samuel Marco i Armengol
 * @contact     sammarcoarmengol@gmail.com
 * @contribution Helium6072




