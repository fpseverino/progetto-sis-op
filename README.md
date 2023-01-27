# progetto-sis-op
## Corso di Sistemi Operativi UNIKORE A.A. 2022/2023 - Progetto di fine semestre

### Simulazione di casa intelligente in C

#### Descrizione dettagliata
Il progetto, di cui alla relazione seguente, prevede la creazione di un server multithread e dei relativi client che simulano il funzionamento di dispositivi di domotica usando il protocollo di comunicazione TCP/IP.
Il progetto è composto da quattro file di codice sorgente in linguaggio C (`hub.c`, `device.c`, `accessory.c`, `libraries.c`) e da un file header (`libraries.h`).
Il server gestisce le richieste dei client che riceve sotto forma di packet, definiti da una struct apposita, assegnando ogni connessione a un thread di una thread pool.
I client previsti possono essere di due tipi: i “device”, intesi come l’interfaccia usata dall’utente per controllare la casa, e gli “accessory”, ovvero gli accessori di domotica in sé. I due tipi di client si distinguono per richieste inoltrate al server e per l’I/O su terminale (i _device_ richiedono input dall’utente e stampano informazioni, gli _accessory_ si limitano a stampare gli aggiornamenti ricevuti).
La comunicazione tra server e client avviene esclusivamente via socket, fatta eccezione per la condivisione del numero di porta e per la sincronizzazione del printing di _device_ e _accessory_ che condividono lo stesso terminale.
All’avvio del server (definito nel file `hub.c`) è necessario inserire, da riga di comando, il numero di porta che si intende utilizzare, il quale verrà poi condiviso a tutti i client tramite shared memory. Non viene definita a priori per permetterne la sostituzione in caso sia occupata senza modificare il codice sorgente.
Fatto ciò viene inizializzato l’array `home`, che contiene le informazioni relative agli accessori attualmente connessi all’hub sotto forma di struct `Accessory`, costituita da una stringa e un intero per identificare il nome e lo status dell’accessorio.
Vengono successivamente inizializzati l’handler del segnale `SIGINT`, la shared memory per il numero di porta, la coda di messaggi per inviare le richieste alla thread pool e i semafori POSIX per la sincronizzazione dei thread del server. I mutex e le variabili condizione vengono inizializzate staticamente. Se ogni operazione va a buon fine, il programma prosegue con la creazione dei thread della thread pool, che eseguono la funzione `threadFunction`, mettendosi in attesa di ricevere il socket relativo a un client dalla coda di messaggi.
Successivamente viene creato il socket del server, legato all’indirizzo e messo in ascolto. Viene creato un ulteriore thread che accetta le connessioni in arrivo dei client e invia sulla coda di messaggi i nuovi socket creati, in un ciclo infinito. Ciò viene svolto da un thread per permetterne la cancellazione all’arrivo del segnale `SIGINT`, inviato via tastiera e gestito da un handler, anche se il thread si trova bloccato sulla `accept`, così da uscire dal ciclo infinito e, dopo aver joinato il thread, procedere con la corretta deallocazione di tutte le risorse del server.
Quando uno dei thread della thread pool recupera un socket dalla coda di messaggi chiama la funzione `requestHandler`, che riceve dal client un packet di tipo `Packet`, formato da una struct `Accessory` e da un intero che identifica la richiesta. In base al valore di quest’ultimo vengono eseguite le seguenti operazioni:
- aggiunta di un accessorio (previo controllo sull’unicità del nome e sullo spazio libero),
- lettura dello status di un accessorio,
- lettura degli status di tutti gli accessori,
- aggiornamento dello status di un accessorio,
- eliminazione di tutti gli accessori,
- uscita.

All’avvio del programma _device_, si ottiene la shared memory contenente il numero di porta scelto all’avvio di _hub_, viene allocato il semaforo System V, che verrà utilizzato dal _device_ e dagli _accessory_ lanciati da esso per garantire la mutua esclusione al momento di stampare e leggere da terminale, e viene creato il socket con cui si connetterà al server. Una volta stabilita la connessione, il _device_ entra in un ciclo while in cui viene stampato il menù fino a che l’utente non inserisce il codice per uscire dal programma. In base all’opzione scelta dall’utente, viene mandata una richiesta al server corrispondente a quelle sopra citate. In particolare, quando l’utente chiede di aggiungere un accessorio, viene chiesto prima al server se il nome scelto per l’accessorio non è già in uso e se vi è spazio libero. Se tali condizioni vengono soddisfatte il processo _device_ crea un processo figlio che avvia tramite `exec` il programma “accessory” e gli passa come parametro da riga di comando il nome scelto dall’utente. L’_accessory_ una volta ottenuta la shared memory, il semaforo System V e il socket, invia al server la richiesta di aggiunta all’array `home`. Dopo di che entra in un ciclo infinito in cui si mette in attesa di ricevere aggiornamenti dal server sul suo status. Non appena riceve un aggiornamento, stampa il nuovo status e si rimette in attesa. Se riceve come nuovo status -127, esce dal ciclo while, dealloca le risorse e chiude l’esecuzione. Il server avrà già svuotato il suo posto nell’array `home`.
Il file `libraries.c` dichiara le funzioni relative ai semafori System V, una funzione per inizializzare gli indirizzi dei client e una funzione per gestire gli errori delle system call.
Nel file `libraries.h` vengono dichiarate tre struct:
- `Message` (relativa alla coda di messaggi), costituita da un `long` che identifica il tipo di messaggi e un intero che identifica un socket;
- `Accessory`, costituita da una stringa e un intero per identificare il nome e lo status di un accessorio;
- `Packet`, costituita da una variabile `Accessory` e da un intero che identifica il numero di richiesta. In particolare:
  - 1 per chiedere da parte del _device_ di aggiungere un accessorio,
  - 2 per leggere lo status di un accessorio,
  - 3 per leggerli tutti,
  - 4 per aggiornare lo status di un accessorio,
  - 5 per rimuovere tutti gli accessori (e interrompere tutti i processi _accessory_),
  - 6 per chiudere il _device_,
  - 7, la richiesta inviata dai processi _accessory_ per connettersi al server.

#### System call utilizzate
- processi (`fork`, `exit`):

Il processo _device_, dopo aver ricevuto l’ok dal server, crea con la `fork()` un processo figlio, che tramite `exec` avvia il processo _accessory_. La system call `exit()` viene utilizzata sia per interrompere l’esecuzione dei vari processi quando concludono tutte le operazioni con successo, sia nella funzione `check`, che in caso di errore, di una qualsiasi funzione o system call, lo stampa con `perror` e chiama la `exit` con status `EXIT_FAILURE`.
- `wait` e `waitpid`:

Il processo _device_ salva il numero dei processi figli (cioè degli accessori) da lui creati e, quando l’utente chiede la cancellazione di tutti gli accessori, aspetta con `wait()` la terminazione dei suoi processi figli, al fine di non lasciarli in stato di “zombie”, e ne stampa l’exit status, ottenuto sempre dalla `wait`.
- `exec`:

Il processo _device_, quando l’utente desidera aggiungere un accessorio, e dopo aver ottenuto l’ok del server, in un processo figlio chiama la `execl`, avviando il programma “accessory” e passandogli come parametro da riga di comando il nome scelto dall’utente per l’accessorio.
- segnali tra processi (`signal`, `sigaction`, `kill`, `pause`, `raise`, `alarm`):

Il processo _hub_, tramite la struct e la system call `sigaction`, intercetta il segnale `SIGINT` (`^C` da tastiera) e tramite l’handler `signalHandler` cancella il thread che esegue la `acceptConnection`, che non interromperebbe la sua esecuzione altrimenti. Questa cancellazione permette poi al thread di essere joinato e al processo _hub_ di procedere con la corretta deallocazione di tutte le risorse, cosa che non avverrebbe con la normale interruzione del processo.
- pipe e fifo:

non utilizzati.
- shared memory:

All’avvio di _hub_ l’utente passa da riga di comando il numero della porta su cui il server si metterà in ascolto, dato che è necessario cambiarla di frequente e risulta scomodo modificarla nel codice sorgente a ogni avvio. Per una questione di praticità, e dato che la connessione avviene in local host, il numero di porta viene condiviso a tutti i client tramite shared memory.
- coda di messaggi:

Nell’_hub_ viene implementata una coda di messaggi che funge da “queue” per la thread pool. Il socket ottenuto dalla `accept` viene inviato sulla coda, un thread lo riceve e gestisce la connessione del relativo client. Lo scambio di messaggi avviene garantendo la concorrenza fra il produttore (il thread che chiama la `accept`) e i consumatori (i thread della pool).
- gestione dei thread (`create`, `self`, `exit`, `cancel`, `join`, attributi):

L’_hub_ è strutturato come un server multithread, ogni richiesta viene gestita da un thread creato all’avvio del programma facente parte di una thread pool. Quest’ultima è stata implementata al fine di evitare rallentamenti in caso di un numero elevato di connessioni. Un altro thread (che esegue `acceptConnection`) gestisce l’accettazione delle connessioni al server in un ciclo infinito. Tale thread è cancellabile per poterlo interrompere (tramite handler del segnale `SIGINT`) quando l’utente desidera chiudere il server. Viene poi joinato in modo che il listato successivo (che prevede la deallocazione delle risorse del server) attenda la terminazione del ciclo infinito per essere eseguito. La `pthread_exit` è presente nelle funzioni dei thread per segnalare la terminazione dello stesso, sia in caso di successo sia di fallimento.
- semafori System-V per processi:

Ogni processo _device_ crea un semaforo System V inserendo nella key (`ftok`) il suo PID, ogni processo _accessory_ figlio del processo _device_ ottiene il semaforo inserendo nella key il PID del padre (`getppid`). I processi che condividono il semaforo lo utilizzeranno per alternarsi mutuamente nello stampare e nel leggere dal terminale.
- semafori POSIX con nome per thread:

I semafori POSIX vengono usati dai thread della thread pool dell’_hub_ che devono accedere alla coda di messaggi nel paradigma dei produttori-consumatori (un semaforo per segnalare quando la coda è vuota, uno per segnalare quando è “piena”, secondo un valore prefissato, più un mutex per l’accesso vero e proprio alla coda). Un altro semaforo POSIX (`deleteSem`) viene utilizzato da un thread che aspetta la terminazione dei thread che gestiscono gli accessori. Il thread in questione richiama la `sem_wait` tante volte quanti sono gli accessori connessi, mentre ogni thread che gestisce un accessorio chiama la `sem_post` a eliminazione conclusa.
- mutex per thread:

Nell’_hub_ sono implementati un mutex per accedere al semaforo `deleteSem`, un mutex per risolvere il problema dei produttori-consumatori della coda dei messaggi, e due mutex per risolvere il problema dei thread lettori-scrittori che accedono alle variabili globali (un mutex per la lettura, un mutex per la lettura/scrittura, più un intero `readCount`).
- variabili condizione per thread:

Viene implementata una variabile condizione nell’_hub_, legata al mutex lettura/scrittura di cui sopra. I thread che comunicano con i client “accessory”, una volta aggiunto l’accessorio all’array `home`, entrano in un ciclo infinito e si mettono subito in attesa della variabile condizione. Ogni thread che aggiorna lo status di un accessorio manda un segnale a tutti i thread in attesa con la `pthread_cond_broadcast`. Ogni thread che gestisce un accessorio controlla dunque se il suo status è cambiato, se è così manda l’aggiornamento via socket al processo _accessory_ e ricomincia il ciclo, in caso contrario torna subito in attesa.
- segnali tra thread:

non utilizzati.

#### Funzioni implementate
Funzioni in `hub.c`:
```c
void homeInit();
```
  - Inizializza l’array `home`, sovrascrivendo ogni variabile `Accessory` con valori di default; non prevede alcun input. Accede all’array `home` come scrittore, utilizzando gli opportuni mutex.
  
```c
bool checkName(char * newName);
```
  - Prende come input una stringa e verifica se la stringa è già presente in `home`; ritorna `true` se la stringa non è presente, `false` altrimenti. Accede all’array `home` come lettore, utilizzando gli opportuni mutex.
  
```c
void signalHandler(int sig);
```
  - L’handler del segnale `SIGINT`. Quando il segnale viene catturato cancella con `pthread_cancel` il thread che sta eseguendo `acceptConnection`.
  
```c
void * acceptConnection(void * arg);
```
  - La funzione del thread che accetta le connessioni in arrivo. In un ciclo infinito, ottiene il socket relativo al nuovo client con la `accept`, e lo manda sulla coda di messaggi, utilizzando i semafori POSIX comportandosi da produttore.
  
```c
void * threadFunction(void * arg);
```
  - La funzione eseguita dai thread della thread pool. In un ciclo infinito, riceve un socket dalla coda di messaggi, accedendovi con i semafori POSIX come consumatore, invoca la funzione `requestHandler` passandogli il socket e quando questa termina la sua esecuzione ricomincia il ciclo.
  
```c
void * requestHandler(int newSocketFD);
```
  - Viene chiamata dai thread della thread pool, riceve come argomento il socket relativo a un client, dal quale riceve un packet (di tipo `Packet`). In base al numero di richiesta esegue le varie operazioni, fino a quando l’utente non invia la richiesta per uscire, a quel punto l’esecuzione torna alla `threadFunction`. La funzione comunica con altri thread attraverso semafori e mutex, e riceve e invia dati al client. In particolare, quando un processo _accessory_ manda la sua richiesta (7), viene aggiunto l’accessorio all’array `home` (accedendovi come scrittore con il relativo mutex) e successivamente entra in un ciclo infinito in cui si mette in attesa della variabile condizione `updateCond`, che viene segnalata dal thread che aggiorna lo status del dispositivo. Dopo la segnalazione, se vi sono aggiornamenti vengono inviati ad _accessory_ e ricomincia il ciclo.
  
```c
void startReading();
```
  - Utilizza i mutex `readMutex` e `readWriteMutex`, insieme alla variabile `readCount`, per permettere al thread di iniziare a leggere le variabili globali.
  
```c
void endReading();
```
  - Utilizza i mutex `readMutex` e `readWriteMutex`, insieme alla variabile `readCount`, per far concludere la lettura del thread delle variabili globali.
  
```c
void addrInitServer(struct sockaddr_in *address, long IPaddr, int port);
```
  - Inizializza la struct puntata da `address` utilizzando l’indirizzo IP e la porta che gli vengono passati.

Funzioni in `device.c`:
```c
int mainMenu(int semID);
```
  - Prende come argomento l’ID del semaforo System V che gli garantisce di non essere interrotto nelle funzioni di stampa e lettura da terminale. Stampa il menù del _device_ e ritorna come intero il numero della richiesta scelta dall’utente.

Funzioni in `libraries.c`:
```c
void check(int result, char * message);
```
  - Prende come argomenti il valore di ritorno di una funzione e il messaggio da stampare in caso di errore. Se il valore di ritorno della funzione corrisponde a un errore, stampa con `perror` il messaggio e invoca la `exit` con status `EXIT_FAILURE`.

```c
int deallocateSem(int semID);
```
  - Rimuove il semaforo System V con ID `semID`, ottentuto dalla `semget`.

```c
int initSem(int semID, int initialValue);
```
  - Inizializza il semaforo System V con ID `semID` dandogli come valore iniziale `initialValue`.

```c
int waitSem(int semID);
```
  - Decrementa di uno il valore del semaforo System V con ID `semID`.

```c
int signalSem(int semID);
```
  - Aumenta di uno il valore del semaforo System V con ID `semID`.

```c
void addrInitClient(struct sockaddr_in *address, int port);
```
  - Inizializza la struct puntata da `address` utilizzando la porta che gli viene passata e l’indirizzo IP `127.0.0.1` (local host).
