#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h> 
#include <string.h>

#define VELICINA_OBALE 10000

pthread_mutex_t m;
pthread_cond_t redObala;
pthread_cond_t redCamac;

int misionar_br = 0;
int mjesto_desno = 0;
int mjesto_lijevo = 0;
int kanibal_br = 0;
int strana_camca = 0;
char* putnici[7] ={"Ante", "Luka", "Andjelko", "Mate", "Stipe", "Hrvoje", "Sime"};
int ind = 0;
char* lijeva_obala[VELICINA_OBALE] = {NULL};
char* desna_obala[VELICINA_OBALE] = {NULL};



void printaj_stanje(void)
{
    char str1[100] = "";
    for (int i=0; i < ind; i++)
    {
        strcat(str1, putnici[i]);
        strcat(str1, " ");
    }
    str1[strlen(str1)-1] = '\0';

    char str2[100] = "";
    for (int i = 0; i < VELICINA_OBALE; i++)
    {
        if (lijeva_obala[i] != NULL)
        {
            strcat(str2, lijeva_obala[i]);
            strcat(str2, " ");
        }    
    }
    str2[strlen(str2)-1] = '\0';

    char str3[100] = "";
    for (int i = 0; i < VELICINA_OBALE; i++)
    {
        if (desna_obala[i] != NULL) 
        {
            strcat(str3, desna_obala[i]);
            strcat(str3, " ");
        }
    }
    str3[strlen(str3)-1] = '\0';
    char str4;
    if (strana_camca == 0 || strana_camca == -2) str4 = 'D';
    else if (strana_camca == 1 || strana_camca == -1) str4 = 'L';
    
    printf("C[%c]={%s} LO={%s} DO={%s}\n", str4, str1, str2, str3);
    printf("\n");
}


int dodji_na_obalu(char* tip, int i, int obala)
{
    int moje_mjesto;
    char tmp_str1[20];
    sprintf(tmp_str1, "%d", i);
    char tmp_str2[20];
    strcpy(tmp_str2, tip);
    strcat(tmp_str2, tmp_str1);
    if (obala == 0)
    {
        moje_mjesto = mjesto_desno;
        desna_obala[mjesto_desno] = malloc(strlen(tmp_str2) + 1);
        strcpy(desna_obala[mjesto_desno], tmp_str2);
        mjesto_desno++;
        //printf("%s\n", desna_obala[i]);
        printf("%s%d: dosao na desnu obalu\n", tip, i);
        printaj_stanje();

    }
    else if (obala == 1)
    {
        moje_mjesto = mjesto_lijevo;
        lijeva_obala[mjesto_lijevo] = malloc(strlen(tmp_str2) + 1);
        strcpy(lijeva_obala[mjesto_lijevo], tmp_str2);
        mjesto_lijevo++;
        printf("%s%d: dosao na lijevu obalu\n", tip, i);
        printaj_stanje();
    }

    return moje_mjesto;
}

void udji_u_camac(char* tip, int i, int obala, int moje_mjesto)
{
    if (strcmp(tip, "M") == 0) misionar_br++;
    else if (strcmp(tip, "K") == 0) kanibal_br++;
    char string2[20];
    sprintf(string2, "%d", i);
    char string1[20];
    strcpy(string1, tip);
    strcat(string1, string2);
    putnici[ind] = malloc(strlen(string1) + 1);
    strcpy(putnici[ind], string1);
    ind++;

    printf("%s%d: usao u camac\n", tip, i);
    if (obala == 1) lijeva_obala[moje_mjesto] = NULL;
    else if (obala == 0) desna_obala[moje_mjesto] = NULL;
    printaj_stanje();
}


void *misionar(void *x)
{
    int i = *((int*)x);
    srand(time(NULL));
    int obala = rand() % 2;
    pthread_mutex_lock(&m);
    
    int moje_mjesto = dodji_na_obalu("M", i, obala);

    int br_ljudi = misionar_br + kanibal_br;
    while(obala != strana_camca || misionar_br + 1 < kanibal_br || misionar_br + kanibal_br + 1 > 7)
    {
        pthread_cond_wait(&redObala, &m);
    }

    udji_u_camac("M", i, obala, moje_mjesto);

    if (misionar_br + kanibal_br >= 3) pthread_cond_broadcast(&redCamac);
    pthread_mutex_unlock(&m);

    pthread_cond_broadcast(&redObala);
    
    return NULL; 
}

void *kanibal(void *x)
{
    int i = *((int*)x);
    srand(time(NULL));
    int obala = rand() % 2;
    pthread_mutex_lock(&m);

    int moje_mjesto = dodji_na_obalu("K", i, obala);

    int br_ljudi = misionar_br + kanibal_br;
    while(obala != strana_camca || (misionar_br < (kanibal_br + 1) && misionar_br != 0) || misionar_br + kanibal_br + 1 > 7) 
    {
        pthread_cond_wait(&redObala, &m);
    }    
    
    udji_u_camac("K", i, obala, moje_mjesto);

    if (misionar_br + kanibal_br >= 3) pthread_cond_broadcast(&redCamac);
    pthread_mutex_unlock(&m);

    pthread_cond_broadcast(&redObala);

    return NULL; 
}

void *camac(void *x)
{
    // 1 = lijevo, 0 = desno
    while(1)
    {
        pthread_cond_broadcast(&redObala);
    
        int di_sam_bio;
        
        pthread_mutex_lock(&m);
        if (kanibal_br + misionar_br == 0)
        {
             printf("C: prazan na %s obali\n", (strana_camca == 0) ? "desnoj" : "lijevoj");
             printaj_stanje();
        }
           
        while (kanibal_br + misionar_br < 3)
        {
            pthread_cond_wait(&redCamac, &m);
        }
        di_sam_bio = strana_camca;
        
        //printf("Polazim\n");
        printf("C: tri putnika ukrcana, polazim za jednu sekundu\n");
        printaj_stanje();
        pthread_mutex_unlock(&m);

        sleep(1);

        pthread_mutex_lock(&m);
        strana_camca -= 2;
        printf("C: prevozim s %s obalu: ", (di_sam_bio == 0) ? "desne na lijevu" : "lijeve na desnu");
        for(int i = 0; i < ind; i++)
        {
            printf("%s ", putnici[i]);
        }  
        printf("\n");
        printf("\n");
        pthread_mutex_unlock(&m);

        sleep(2);

        pthread_mutex_lock(&m);
        printf("C: preveo s %s obalu: ", (di_sam_bio == 0) ? "desne na lijevu" : "lijeve na desnu");
        for(int i = 0; i < ind; i++)
        {
            printf("%s ", putnici[i]);
            free(putnici[i]);
        }  
        printf("\n");

        ind = 0;
        kanibal_br = 0;
        misionar_br = 0;
        strana_camca = (di_sam_bio + 1) % 2;
        pthread_mutex_unlock(&m);
    }

    return NULL;
}

pthread_t thrd_id[100];
void *generator(void *x)
{
    int parametri[101];
    for (int j = 0; j <= 100; j++)
    {
    parametri[j] = j+1;
    } 
    

    for(int i = 0, j = 0; i < 100; i = i + 1)
    {
        sleep(1);
        
        pthread_create(&thrd_id[i], NULL, kanibal, &parametri[2*i]); 
        
        sleep(1);
        
        pthread_create(&thrd_id[i+1], NULL, kanibal, &parametri[2*i+1]);
        pthread_create(&thrd_id[i+2], NULL, misionar, &parametri[i]); 
    }
    return NULL; 
}

int main(void)
{
    pthread_mutex_init(&m, NULL);
    pthread_cond_init(&redObala, NULL);
    pthread_cond_init(&redCamac, NULL);

    pthread_t camac_id;
    if (pthread_create(&camac_id, NULL, camac, NULL) != 0) 
    {
        printf("Camac se nije stvorio!\n");
        exit(0);
    }

    pthread_t generator_id[1];
    if (pthread_create(&generator_id[0], NULL, generator, NULL) != 0) 
    {
        printf("Generator se nije stvorio!\n");
        exit(0);
    }

    

    printf("Legenda: M-misionar, K-kanibal, C-čamac,\n");
    printf("                     LO-lijeva obala, DO-desna obala\n");
    printf("                     L-lijevo, D-desno\n");
    printf("\n");

    pthread_join(generator_id[0], NULL); 
    pthread_join(camac_id, NULL);
    for (int i = 0; i < 100; i++)
    {
        pthread_join(thrd_id[i], NULL);
    }
    return 0;
}
