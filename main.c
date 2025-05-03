#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>

#define NB_BUS_X 5
#define NB_BUS_Y 4
#define NB_TRAJETS 10

sem_t mutex;
int nb_dans_tunnel = 0;
int direction = -1; // 0 = X->Y, 1 = Y->X, -1 = libre
int en_attente[2] = {0, 0};
int tour = 0; // pour l'équité

typedef struct {
    int id;
    int ville; // 0 = X, 1 = Y
} Bus;

// Nouvelle fonction modifiée — claire et structurée
void entrer_tunnel(int sens) {
    int autre = 1 - sens;
    int peut_entrer = 0;

    en_attente[sens]++; // Le bus se met en file d'attente dans sa direction

    while (!peut_entrer && tour == sens) {
        sem_wait(&mutex); // Entrée en section critique protégée

        // Cas 1 : le tunnel est vide ET (personne de l’autre sens OU c’est son tour)
        // Cas 2 : le tunnel est occupé par la même direction
        if ((nb_dans_tunnel == 0 && (en_attente[autre] == 0 || tour == sens)) ||
            (nb_dans_tunnel > 0 && direction == sens)) {

            // Il quitte la file d’attente
            en_attente[sens]--;

            // On augmente le nombre de bus dans le tunnel
            nb_dans_tunnel++;

            // On fixe la direction courante du tunnel
            direction = sens;

            // On marque qu’il peut entrer
            peut_entrer = 1;
        }

        sem_post(&mutex); // Sortie de section critique

        if (!peut_entrer)
            usleep(10000); // Attendre avant de réessayer si ce n’est pas encore son tour
    }
}

void sortir_tunnel() {
    sem_wait(&mutex);
    nb_dans_tunnel--;
    if (nb_dans_tunnel == 0) {
        direction = -1;
        tour = 1 - tour; // Changer de tour pour respecter l’équité
    }
    sem_post(&mutex);
}

void* trajet_bus(void* arg) {
    Bus* b = (Bus*) arg;
    char* nomVille[2] = {"X", "Y"};
    for (int i = 1; i <= NB_TRAJETS; i++) {
        // Aller
        entrer_tunnel(b->ville);
        printf("Bus %d de Ville %s : %s -> %s (Trajet %d)\n",
               b->id, nomVille[b->ville], nomVille[b->ville], nomVille[1 - b->ville], i);
        usleep((rand() % 501 + 1000) * 1000);
        sortir_tunnel();

        // Retour
        entrer_tunnel(1 - b->ville);
        printf("Bus %d de Ville %s : %s -> %s (Trajet %d)\n",
               b->id, nomVille[b->ville], nomVille[1 - b->ville], nomVille[b->ville], i);
        usleep((rand() % 501 + 1000) * 1000);
        sortir_tunnel();
    }
    pthread_exit(NULL);
}

int main() {
    srand(time(NULL));
    pthread_t threads[NB_BUS_X + NB_BUS_Y];
    Bus bus[NB_BUS_X + NB_BUS_Y];

    sem_init(&mutex, 0, 1);

    for (int i = 0; i < NB_BUS_X; i++) {
        bus[i].id = i + 1;
        bus[i].ville = 0;
        pthread_create(&threads[i], NULL, trajet_bus, &bus[i]);
    }

    for (int i = 0; i < NB_BUS_Y; i++) {
        bus[NB_BUS_X + i].id = i + 1;
        bus[NB_BUS_X + i].ville = 1;
        pthread_create(&threads[NB_BUS_X + i], NULL, trajet_bus, &bus[NB_BUS_X + i]);
    }

    for (int i = 0; i < NB_BUS_X + NB_BUS_Y; i++) {
        pthread_join(threads[i], NULL);
    }

    sem_destroy(&mutex);

    return 0;
}