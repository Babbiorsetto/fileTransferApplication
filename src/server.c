#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <ctype.h>

/* TODO
 * client-server application to transfer files from client to server
 *
 * ./server PORT_NUMBER DESTINATION_DIRECTORY
 */

void cleanup(int);
void gestisciConnessione(int, char*);
void controllaNParametri(int argc, char *nomeProgramma);
void controllaDirectory(char *nomeDirectory);
void preparaIndirizzo(struct sockaddr_in *indirizzo, uint16_t numeroPorta);
char *getVeroNomeDirectory(char *);
uint16_t getVeroPortNumber(char *);
int isInteger(char *);

const int DEBUG = 0;

int main(int argc, char *argv[]) {
  int socketDescriptor;
  int pid;
  struct sockaddr_in address;
  int fd;
  int errore;
  char *nomeDirectory;
  uint16_t numeroPorta;

  signal(SIGINT, cleanup);

  controllaNParametri(argc, argv[0]);
  controllaDirectory(argv[2]);
  nomeDirectory = getVeroNomeDirectory(argv[2]);
  if (DEBUG) {
    printf("veroNomeDirectory:%s\n", nomeDirectory);
  }
  numeroPorta = getVeroPortNumber(argv[1]);
  preparaIndirizzo(&address, numeroPorta);
  if (DEBUG) {
    printf("address sin_family:%d, sin_port:%d, sin_addr:%d\n", address.sin_family, address.sin_port, address.sin_addr.s_addr);
  }

  socketDescriptor = socket(PF_INET, SOCK_STREAM, 0);
  if (socketDescriptor == -1) {
    perror("Errore di socket");
    exit(1);
  }
  fprintf(stderr, "Socket aperto con successo\n");
  errore = bind(socketDescriptor, (struct sockaddr *)&address, sizeof(address));
  if (errore < 0) {
    perror("Errore di bind");
    exit(1);
  }
  fprintf(stderr, "Bind sulla porta %d effettuato con successo\n",ntohs(address.sin_port));
  errore = listen(socketDescriptor, 5);
  if (errore < 0) {
    perror("Errore di listen");
    exit(1);
  }
  fprintf(stderr, "In ascolto...\n");
  while((fd = accept(socketDescriptor, NULL, NULL)) > -1) {
    fprintf(stderr, "Nuova connessione in entrata\n");

    pid = fork();
    if (pid == 0) {
      gestisciConnessione(fd, nomeDirectory);
    }
    close(fd);
  }
  close(socketDescriptor);
  exit(0);
}

void gestisciConnessione(int socketDescriptor, char *nomeDirectory) {
  const int BUFDIM = 100;
  char buffer[BUFDIM];
  char nomeFile[100];
  char nomeFileCompleto[200];
  int outputFileDescriptor;
  int dimensioneFile;
  int dimensioneFileNetworkOrder;
  int dimensioneNomeFile;
  int dimensioneNomeFileNetworkOrder;
  int byteLetti;
  int i;

  // leggi la dimensione del nome del file
  int errore = read(socketDescriptor, &dimensioneNomeFileNetworkOrder, sizeof(int));
  if (errore == -1) {
    perror("Errore di read di dimensioneNomeFile");
    close(socketDescriptor);
    exit(1);
  }
  dimensioneNomeFile = ntohl(dimensioneNomeFileNetworkOrder);
  if (DEBUG) {
    printf("dimensioneNomeFile:%d\n", dimensioneNomeFile);
  }
  // leggi il nome del file. Dovrebbe essere NULL terminated, dovrebbe.
  strcpy(nomeFileCompleto, nomeDirectory);
  i = 0;
  while (i < dimensioneNomeFile) {
    byteLetti = read(socketDescriptor, &buffer, dimensioneNomeFile);
    if (byteLetti == -1) {
      perror("Errore di read nomeFile");
      close(socketDescriptor);
      exit(1);
    }
    memcpy(nomeFile + i, buffer, byteLetti);
    i += byteLetti;
  }
  strcat(nomeFileCompleto, nomeFile);
  if (DEBUG) {
    printf("nomeFileCompleto:%s\n", nomeFileCompleto);
  }
  // apri il file di output
  outputFileDescriptor = open(nomeFileCompleto, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
  if (outputFileDescriptor == -1) {
    perror("Errore di open outputFile");
    close(socketDescriptor);
    exit(1);
  }
  // leggi dimensione file
  errore = read(socketDescriptor, &dimensioneFileNetworkOrder, sizeof(int));
  if (errore == -1) {
    perror("Errore di read dimensioneFile");
    close(socketDescriptor);
    close(outputFileDescriptor);
    exit(1);
  }
  dimensioneFile = ntohl(dimensioneFileNetworkOrder);
  if (DEBUG) {
    printf("dimensioneFile:%d\n", dimensioneFile);
  }
  // leggi il file da socket e scrivi in output
  i = 0;
  while (i < dimensioneFile) {
    byteLetti = read(socketDescriptor, &buffer, BUFDIM);
    if (byteLetti == -1) {
      perror("Errore di read dal socket");
      close(socketDescriptor);
      close(outputFileDescriptor);
      exit(1);
    }
    errore = write(outputFileDescriptor, &buffer, byteLetti);
    if (errore == -1) {
      perror("Errore di write su file di output");
      close(socketDescriptor);
      close(outputFileDescriptor);
      exit(1);
    }
    i += byteLetti;
  }

  close(outputFileDescriptor);
  close(socketDescriptor);
  printf("File ricevuto: %s dimensione:%dB\n", nomeFile, dimensioneFile);
  exit(0);
}

void cleanup(int signal) {
  exit(0);
}

void controllaNParametri(int argc, char *nomeProgramma) {
  if (argc != 3) {
    fprintf(stderr, "%s:usage:%s N_PORTA DIRECTORY\n", nomeProgramma, nomeProgramma);
    exit(1);
  }
}

void controllaDirectory(char *nomeDirectory) {
  struct stat check;
  stat(nomeDirectory, &check);
  if (!S_ISDIR(check.st_mode)) {
    fprintf(stderr, "L'argomento passato non e' una directory\n");
    exit(1);
  }
  if (access(nomeDirectory, W_OK | X_OK) == -1) {
    fprintf(stderr, "Impossibile scrivere nella directory destinazione\n");
    exit(1);
  }
}

void preparaIndirizzo(struct sockaddr_in *indirizzo, uint16_t numeroPorta) {
  indirizzo->sin_family = AF_INET;
  indirizzo->sin_port = htons(numeroPorta);
  indirizzo->sin_addr.s_addr = INADDR_ANY;
}

char *getVeroNomeDirectory(char *nomeDaControllare) {
  char *lastSlash = strrchr(nomeDaControllare, '/');
  int lunghezza = strlen(nomeDaControllare);

  // se non ci sono slash la directory e' relativa e con un solo passaggio (. oppure miaDir)
  // se l'ultimo carattere non e' lo slash trovato come ultimo slash, bisogna aggiungerlo
  // in ogni caso bisogna solo aggiungere uno slash
  if ( (lastSlash == NULL) || ( (nomeDaControllare + lunghezza - 1) != lastSlash) ) {
    // nuova stringa per mettere vecchia, slash in piu' e \0 non contato da strlen
    char *nomeDirectory = malloc(sizeof(char) * lunghezza + 2);
    strcpy(nomeDirectory, nomeDaControllare);
    nomeDirectory[lunghezza] = '/';
    nomeDirectory[lunghezza+1] = '\0';
    return nomeDirectory;
  }
  return nomeDaControllare;
}

uint16_t getVeroPortNumber(char *argomento) {
  if (!isInteger(argomento)) {
    fprintf(stderr, "Il numero di porta non e' un intero valido\n");
    exit(1);
  }
  int numero = atoi(argomento);
  if (numero < 1 || numero > 65535) {
    fprintf(stderr, "Il numero di porta non e' valido (1-65535)\n");
    exit(1);
  }
  return (uint16_t)numero;
}

int isInteger(char * string) {
  int x = 0;
  int len = strlen(string);

  while(x < len) {
    if(!isdigit(string[x]) && x > 0) {
      return 0;
    } else if (!isdigit(string[x]) && x == 0){
      if (string[0] != '-') {
        return 0;
      }
    }
    x++;
  }

  return 1;
}
