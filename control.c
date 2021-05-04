#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "store.h"

key_t clientsKey;
key_t cartsKey;
key_t catalogKey;
char *names[NAMELENGTH] = {"Carlos/Fuentes/Hernandez", "Pedro/Flores/Gutierrez", "Patricia/Mercedez/Saucedo",
    "Jesus/Ricardo/Suarez/Perez", "Alfredo/Ricardo/Dominguez", "Jose/Alfredo/Martinez/Sanchez"},
    *pswds[PSWDLENGTH] = {"LobitoVeloz777", "31234327jjfj23", "contraseña", "12345678", "Alfredofeo", "JefFErzon666"};

void initClients(client *clients){ 
    FILE *file;
    const char *fileName = "Clients";
    if((file = fopen(fileName, "a")) == NULL) fprintf(stderr, "Error al crear el archivo");
    clients[0].id = 0;
    strcpy(clients[0].name, names[0]);
    strcpy(clients[0].pswd, pswds[0]);
    clients[1].id = 1;
    strcpy(clients[1].name, names[1]);
    strcpy(clients[1].pswd, pswds[1]);
    clients[2].id = 2;
    strcpy(clients[2].name, names[2]);
    strcpy(clients[2].pswd, pswds[2]);
    clients[3].id = 3;
    strcpy(clients[3].name, names[3]);
    strcpy(clients[3].pswd, pswds[3]);
    clients[4].id = 4;
    strcpy(clients[4].name, names[4]);
    strcpy(clients[4].pswd, pswds[4]); 
    clients[5].id = 5;
    strcpy(clients[5].name, names[5]);
    strcpy(clients[5].pswd, pswds[5]);
    unsigned short i = 0;
    for (i = 0; i < 6; i++){
        fprintf(file, "%hd", clients[i].id);
        fputs("\n", file);
        fputs(clients[i].name, file);
        fputs("\n", file);
		fputs(clients[i].pswd, file);
        if(i != 5) fputs("\n", file);
    }
	fclose(file);
}

void loadClients(){
    FILE *file;
    const char *fileName = "Clients";
	client clients[6];
	if((file = fopen(fileName, "r")) == NULL) fprintf(stderr, "Error al leer el archivo");
    unsigned short i;
	for(i = 0; !feof(file); i++){
        fscanf(file, "%hd", &clients[i].id);
		fscanf(file, "%s", clients[i].name);
        fscanf(file, "%s", clients[i].pswd);
	}
    fclose(file);
    if(i == 1) {
        initClients(clients);
    }
    clientsKey = ftok("ClientsKey", 'c');
    int shmid = shmget(clientsKey, sizeof(client) * 6, IPC_CREAT | 0600);
    client *shmClients = (client*)shmat(shmid, 0, 0);
    for(i = 0; i <= 5; i++) shmClients[i] = clients[i];
    puts("Clients loaded");
    pthread_exit(NULL);
}

void *loadCatalog() {
    puts("Loading Catalog");
    FILE *file;
    const char *fileName = "Catalog";
    productArray *catalog;
    catalog = createCatalog();
    if((file = fopen(fileName, "r")) == NULL) fprintf(stderr, "Error al leer el archivo");
    puts("Got Catalog File");
    unsigned short i, j, len;
	for(i = 0; !feof(file); i++){
        puts("Loading");
        fscanf(file, "%hd", &len);
        createProductArray(&catalog[i], len);
        printf("%hd\n", catalog[i].length);
        for(j = 0; j < catalog[i].length; j++){
            fscanf(file, "%hd", &catalog[i].array[j].id);
            fscanf(file, "%hd", &catalog[i].array[j].stock);
		    fscanf(file, "%s", catalog[i].array[j].name);
        }
	}
    fclose(file);
    puts("Got Catalog from File");
    catalogKey = ftok("CatalogKey", 'b');
    int shmid = shmget(catalogKey, sizeof(productArray) * i, IPC_CREAT | 0600);
    productArray *shmCatalog = (productArray*)shmat(shmid, 0, 0);
    for(j = 0; j <= i; j++) shmCatalog[j] = catalog[j];
    puts("Catalog loaded");
    pthread_exit(NULL);
}

void loadCarts() {
    puts("Loading Carts");
    FILE *file;
    const char *fileName = "Carts";
    cart *carts = (cart*)malloc(sizeof(cart));
    if((file = fopen(fileName, "r")) == NULL) fprintf(stderr, "Error al leer el archivo");
    unsigned short i, j, len;
    puts("Got Carts File");
	for(i = 0; !feof(file); i++){
        fscanf(file, "%hd", &carts[i].clientId);
        fscanf(file, "%hd", &len);
        createProductArray(&carts[i].products, len);
        for(j = 0; j < carts[i].products.length; j++){
            fscanf(file, "%hd", &carts[i].products.array[j].id);
            fscanf(file, "%hd", &carts[i].products.array[j].stock);
		    fscanf(file, "%s", carts[i].products.array[j].name);
        }
	}
    fclose(file);
    puts("Got Carts from File");
    cartsKey = ftok("CartsKey", 'a');
    int shmid = shmget(cartsKey, sizeof(cart) * i, IPC_CREAT | 0600);
    cart *shmCarts = (cart*)shmat(shmid, 0, 0);
    for(j = 0; j <= i; j++) shmCarts[j] = carts[j];
    puts("Carts loaded");
    pthread_exit(NULL);
}

void clientLogin() {
    unsigned short i;
    int pipe;
    const char *clientFIFOPath = "ClientFIFO";
    char *name, *pswd;
    bool login;
    mkfifo(clientFIFOPath, 0666);
    while(1) {
        pipe = open(clientFIFOPath, O_RDONLY);
        read(pipe, name, NAMELENGTH);
        read(pipe, pswd, PSWDLENGTH);
        printf("Client: %s, %s\n", name, pswd);
        close(pipe);
        for(i = 0; i < 6; i++) {
            login = !strcmp(name, names[i]) && !strcmp(pswd, pswds[i]);
            if(login) break;
        }
        pipe = open(clientFIFOPath, O_WRONLY);
        write(pipe, &login, 1);
        write(pipe, &cartsKey, sizeof(key_t));
        write(pipe, &catalogKey, sizeof(key_t));
        close(pipe);
    }
}

int main(){
    pthread_t clientsThread, catalogThread, cartsThread;
    pthread_create(&clientsThread, NULL, (void*)loadClients, NULL);
    pthread_join(clientsThread, NULL);
    pthread_create(&catalogThread, NULL, (void*)loadCatalog, NULL);
    pthread_join(catalogThread, NULL);
    pthread_create(&cartsThread, NULL, (void*)loadCarts, NULL);
    pthread_join(cartsThread, NULL);
    clientLogin();
    return 0;
}
