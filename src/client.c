#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <ctype.h>

/* TODO
 * client-server application to transfer files from client to server
 *
 * ./client SERVER_IP PORT_NUMBER FILE
 */

void controllaNParametri(int argc, char *nomeProgramma);
void preparaIndirizzo(struct sockaddr_in *indirizzo, char *stringaIP, uint16_t numeroPorta);
int apriFile(char *filePath, int *dimensione);
char *estraiNomeFile(char *);
uint16_t getVeroPortNumber(char *);
int isInteger(char *);

const int DEBUG = 0;

int main(int argc, char *argv[]) {

  int errore;
  int inputFileDescriptor;
  int dimensioneFile;
  int dimensioneFileNetworkOrder;
  int dimensioneNomeFile;
  int dimensioneNomeFileNetworkOrder;
  int socketDescriptor;
  char *nomeFile;
  struct sockaddr_in address;
  const int BUFDIM = 100;
  char buffer[BUFDIM];
  int i;
  int byteLetti;


  controllaNParametri(argc, argv[0]);
  preparaIndirizzo(&address, argv[1], getVeroPortNumber(argv[2]));
  inputFileDescriptor = apriFile(argv[3], &dimensioneFile);
  nomeFile = estraiNomeFile(argv[3]);
  dimensioneNomeFile = strlen(nomeFile) + 1;
  dimensioneFileNetworkOrder = htonl(dimensioneFile);
  dimensioneNomeFileNetworkOrder = htonl(dimensioneNomeFile);

  if (DEBUG) {
    printf("nomeFile:%s dimensione:%d\n", nomeFile, dimensioneNomeFile);
  }

  socketDescriptor = socket(PF_INET, SOCK_STREAM, 0);
  errore = connect(socketDescriptor, (struct sockaddr *)&address, sizeof(address));
  if (errore == -1) {
    perror("Errore di connect su socket");
    close(inputFileDescriptor);
    close(socketDescriptor);
    exit(1);
  }

  // scrivi dimensione del nome del file in network order
  errore = write(socketDescriptor, &dimensioneNomeFileNetworkOrder, sizeof(int));
  if (errore == -1) {
    perror("Errore di write dimensioneNomeFile");
    close(inputFileDescriptor);
    close(socketDescriptor);
    exit(1);
  }
  //scrivi il nome del file, completo di \0
  i = 0;
  errore = write(socketDescriptor, nomeFile, dimensioneNomeFile);
  if (errore == -1) {
    perror("Errore di write nomeFile");
    close(inputFileDescriptor);
    close(socketDescriptor);
    exit(1);
  }
  // scrivi dimensione del file in network order
  errore = write(socketDescriptor, &dimensioneFileNetworkOrder, sizeof(int));
  if (errore == -1) {
    perror("Errore di write dimensioneFile");
    close(inputFileDescriptor);
    close(socketDescriptor);
    exit(1);
  }
  // scrivi il messaggio
  i = 0;
  while (i < dimensioneFile) {
    byteLetti = read(inputFileDescriptor, &buffer, BUFDIM);
    if (byteLetti == -1) {
      perror("Errore di read del file input");
      close(inputFileDescriptor);
      close(socketDescriptor);
      // TODO come segnalo al server che sto terminando prematuramente?
      exit(1);
    }
    errore = write(socketDescriptor, &buffer, byteLetti);
    if (errore == -1) {
      perror("Errore di write del file su socket");
      close(inputFileDescriptor);
      close(socketDescriptor);
      exit(1);
    }
    i += byteLetti;
  }

  close(inputFileDescriptor);
  close(socketDescriptor);
  exit(0);
}

void controllaNParametri(int argc, char *nomeProgramma) {
  if (argc != 4) {
    fprintf(stderr, "%s:usage:%s IP N_PORTA FILE\n", nomeProgramma, nomeProgramma);
    exit(1);
  }
}

void preparaIndirizzo(struct sockaddr_in *indirizzo, char *stringaIP, uint16_t numeroPorta) {
  int errore;
  memset(indirizzo, 0, sizeof(struct sockaddr_in));
  indirizzo->sin_family = AF_INET;
  indirizzo->sin_port = htons(numeroPorta);
  errore = inet_pton(AF_INET, stringaIP, &(indirizzo->sin_addr));
  if (errore == 0) {
    fprintf(stderr, "Formato dell'indirizzo non corretto\n");
    exit(1);
  }
}

int apriFile(char *filePath, int *dimensione) {
  int fd = open(filePath, O_RDONLY);
  if (fd == -1) {
    perror("Errore di open");
    exit(1);
  }
  if (dimensione != NULL) {
    struct stat buf;
    fstat(fd, &buf);
    *dimensione = (int)buf.st_size;
  }
  return fd;
}

char *estraiNomeFile(char *path) {
  char *start = strrchr(path, '/');
  if (start != NULL) {
    return start + 1;
  }
  return path;
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
