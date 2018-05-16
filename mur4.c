/*****************************************************************************/
/*                                                                           */
/*                           mur3.c                                          */
/*                                                                           */
/*  Compilar i executar:                                                     */
/*     El programa invoca les funcions definides a "winsuport.c", les        */
/*     quals proporcionen una interficie senzilla per crear una finestra     */
/*     de text on es poden escriure caracters en posicions especifiques de   */
/*     la pantalla (basada en CURSES); per tant, el programa necessita ser   */
/*     compilat amb la llibreria 'curses':                                   */
/*                                                                           */
/*      $ gcc -c winsuport2.c -o winsuport2.o                                */
/*      $ gcc ­Wall mur3.c winsuport2.o ­o mur3 ­lcurses ­lpthread				 */
/*		$ gcc ­Wall pilota3.c winsuport2.o ­o pilota3 ­lcurses -lcurses -lpthread */
/*                                                                           */
/*  Al tenir una orientació vertical cal tenir un terminal amb prou files.   */
/*  Per exemple:                                                             */
/*               xterm -geometry 80x50                                       */
/*               gnome-terminal --geometry 80x50                             */
/*                                                                           */
/*****************************************************************************/

#include <stdio.h>		/* incloure definicions de funcions estandard */
#include <stdint.h>		/* intptr_t for 64bits machines */
#include <stdlib.h>
#include <string.h>
#include <pthread.h>		/* incloure threads */
#include <unistd.h>	/* fork */
#include <time.h>
#include "winsuport2.h"		/* incloure definicions de funcions propies */
#include "memoria.h"
#include "semafor.h"
#include "missatge.h"
#include <sys/wait.h>

/* definicio de constants */
#define SIZE_ARRAY 32
#define MAX_THREADS	10
#define MAXBALLS (MAX_THREADS-1)
#define MIN_FIL	10		/* dimensions del camp. Depenen del terminal ! */
#define MAX_FIL	50
#define MIN_COL	10
#define MAX_COL	80
#define MIDA_PALETA (MIN_COL-4)	/* podria ser un paràmetre més */
#define BLKSIZE	3
#define BLKGAP	2
#define BLKCHAR 'B'
#define WLLCHAR '#'
#define FRNTCHAR 'A'
#define LONGMISS 65
			/* variables globals */

char *descripcio[] = {
	"\n",
	"Aquest programa implementa una versio basica del joc Arkanoid o Breakout:\n",
	"generar un camp de joc rectangular amb una porteria, una paleta que s\'ha\n",
	"de moure amb el teclat per a cobrir la porteria, i una pilota que rebota\n",
	"contra les parets del camp, a la paleta i els blocs. El programa acaba si\n",
	"la pilota surt per la porteria o no queden mes blocs. Tambe es pot acabar\n",
	"amb la tecla RETURN.\n",
	"\n",
	"  Arguments del programa:\n",
	"\n",
	"       $ ./mur1 fitxer_config [retard]\n",
	"\n",
	"     El primer argument ha de ser el nom d\'un fitxer de text amb la\n",
	"     configuracio de la partida, on la primera fila inclou informacio\n",
	"     del camp de joc (valors enters), i la segona fila indica posicio\n",
	"     i velocitat de la pilota (valors reals):\n",
	"          num_files  num_columnes  mida_porteria\n",
	"          pos_fila   pos_columna   vel_fila  vel_columna\n",
	"\n",
	"     on els valors minims i maxims admesos son els seguents:\n",
	"          MIN_FIL <= num_files     <= MAX_FIL\n",
	"          MIN_COL <= num_columnes  <= MAX_COL\n",
	"          0 < mida_porteria < num_files-2\n",
	"        1.0 <= pos_fila     <= num_files-3\n",
	"        1.0 <= pos_columna  <= num_columnes\n",
	"       -1.0 <= vel_fila     <= 1.0\n",
	"       -1.0 <= vel_columna  <= 1.0\n",
	"\n",
	"     Alternativament, es pot donar el valor 0 a num_files i num_columnes\n",
	"     per especificar que es vol que el tauler ocupi tota la pantalla. Si\n",
	"     tambe fixem mida_porteria a 0, el programa ajustara la mida d\'aquesta\n",
	"     a 3/4 de l\'altura del camp de joc.\n",
	"\n",
	"     A mes, es pot afegir un segon argument opcional per indicar el\n",
	"     retard de moviment del joc en mil.lisegons; el valor minim es 10,\n",
	"     el valor maxim es 1000, i el valor per defecte d'aquest parametre\n",
	"     es 100 (1 decima de segon).\n",
	"\n",
	"  Codis de retorn:\n",
	"     El programa retorna algun dels seguents codis:\n",
	"	0  ==>  funcionament normal\n",
	"	1  ==>  numero d'arguments incorrecte\n",
	"	2  ==>  no s\'ha pogut obrir el fitxer de configuracio\n",
	"	3  ==>  algun parametre del fitxer de configuracio es erroni\n",
	"	4  ==>  no s\'ha pogut crear el camp de joc (no pot iniciar CURSES)\n",
	"\n",
	"   Per a que pugui funcionar aquest programa cal tenir instal.lada la\n",
	"   llibreria de CURSES (qualsevol versio).\n",
	"\n",
	"*"
};				/* final de la descripcio */


pthread_t tid[MAX_THREADS];	/* taula d'identificadors de threads */
pid_t tpid[MAX_THREADS]; /* taula d'identificadors  de processos */

/* per inicialitzar el mutex es pot posar:
int pthread_mutex_init(pthread_mutex_t * mutex, const pthread_mutexattr_t * attr)
o bé es pot fer la següent assignació, que inicialitza el mutex amb els valors per defecte*/
//pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

intptr_t id = 0;
int *num_pilotes, n_p;
int n_fil, n_col;		/* numero de files i columnes del taulell */
int m_por;			/* mida de la porteria (en caracters) */
int *f_pal, *c_pal, f_p, c_p;		/* posicio del primer caracter de la paleta */
int f_pil, c_pil;		/* posicio de la pilota, en valor enter */
float pos_f, pos_c;		/* posicio de la pilota, en valor real */
float vel_f, vel_c;		/* velocitat de la pilota, en valor real */

int *nblocs, n_b;
int *dirPaleta, dir_p;
int retard;			/* valor del retard de moviment, en mil.lisegons */
int *fi1, fi_1, fi2;			/* valor de condicions finals */
char strin[LONGMISS];		/* variable per a generar missatges de text */
int id_mem_tauler, *addr_tauler;
int id_sem, id_bustia, id_sem_pilotes;
int *temp, temporitzador, start_t;
clock_t *start_time;


/* funcio per carregar i interpretar el fitxer de configuracio de la partida */
/* el parametre ha de ser un punter a fitxer de text, posicionat al principi */
/* la funcio tanca el fitxer, i retorna diferent de zero si hi ha problemes  */
int carrega_configuracio(FILE * fit)
{
	int ret = 0;
	int i = 1;
	fscanf(fit, "%d %d %d\n", &n_fil, &n_col, &m_por);				/* camp de joc */
	while(fscanf(fit, "%f %f %f %f\n", &pos_f, &pos_c, &vel_f, &vel_c) == 4)			/* pilota */
	{
		if ((n_fil != 0) || (n_col != 0)) 						/* si no dimensions maximes */
		{
			if ((n_fil < MIN_FIL) || (n_fil > MAX_FIL) || (n_col < MIN_COL) || (n_col > MAX_COL))
				ret = 1;
			else if (m_por > n_col - 3)
				ret = 2;
			else if ((pos_f < 1) || (pos_f >= n_fil - 3) || (pos_c < 1) || (pos_c > n_col - 1))				/* tres files especials: línia d'estat, porteria i paleta */
				ret = 3;
		}
		if ((vel_f < -1.0) || (vel_f > 1.0) || (vel_c < -1.0) || (vel_c > 1.0))
			ret = 4;

		if (ret != 0) 								/* si ha detectat algun error */
		{
			fprintf(stderr, "Error en fitxer de configuracio:\n");
			switch (ret)
			{
			case 1:
				fprintf(stderr, "\tdimensions del camp de joc incorrectes:\n");
				fprintf(stderr, "\tn_fil= %d \tn_col= %d\n", n_fil, n_col);
				break;
			case 2:
				fprintf(stderr, "\tmida de la porteria incorrecta:\n");
				fprintf(stderr, "\tm_por= %d\n", m_por);
				break;
			case 3:
				fprintf(stderr, "\tposicio de la pilota incorrecta:\n");
				fprintf(stderr, "\tpos_f= %.2f \tpos_c= %.2f\n", pos_f, pos_c);
				break;
			case 4:
				fprintf(stderr, "\tvelocitat de la pilota incorrecta:\n");
				fprintf(stderr, "\tvel_f= %.2f \tvel_c= %.2f\n", vel_f, vel_c);
				break;
			}
			return(ret);
		}
		i++;

	}
	fclose(fit);
	
	*num_pilotes = i-1;

	return(ret);
}


/* funcio per inicialitar les variables i visualitzar l'estat inicial del joc */
/* retorna diferent de zero si hi ha algun problema */
int inicialitza_joc(void)
{
	int i, retwin;
	int i_port, f_port;					/* inici i final de porteria */
	int c, nb, offset;
	
	retwin = win_ini(&n_fil, &n_col, '+', INVERS);			/* intenta crear taulell */

	if (retwin < 0)
	{								/* si no pot crear l'entorn de joc amb les curses */
		fprintf(stderr, "Error en la creacio del taulell de joc:\t");
		switch (retwin)
		{
			case -1:
				fprintf(stderr, "camp de joc ja creat!\n");
				break;
			case -2:
				fprintf(stderr,
					"no s'ha pogut inicialitzar l'entorn de curses!\n");
				break;
			case -3:
				fprintf(stderr,
					"les mides del camp demanades son massa grans!\n");
				break;
			case -4:
				fprintf(stderr, "no s'ha pogut crear la finestra!\n");
				break;
		}
		return (retwin);
	}

	id_mem_tauler = ini_mem(retwin);
	addr_tauler = map_mem(id_mem_tauler);
	win_set(addr_tauler, n_fil, n_col);

	if (m_por > n_col - 2)
		m_por = n_col - 2;					/* limita valor de la porteria */
	if (m_por == 0)
		m_por = 3 * (n_col - 2) / 4;				/* valor porteria per defecte */

	i_port = n_col / 2 - m_por / 2 - 1;				/* crea el forat de la porteria */
	f_port = i_port + m_por - 1;
	for (i = i_port; i <= f_port; i++)
		win_escricar(n_fil - 2, i, ' ', NO_INV);

	n_fil = n_fil - 1;						/* descompta la fila de missatges */
	*f_pal = n_fil - 2;						/* posicio inicial de la paleta per defecte */
	*c_pal = (n_col-MIDA_PALETA) / 2;				/* a baix i centrada */
	for (i = 0; i < MIDA_PALETA; i++)				/* dibuixar paleta inicial */
		win_escricar(*f_pal, *c_pal + i, '0', INVERS);

	/* generar la pilota */
	for (i = 1; i <= *num_pilotes; i++) /* for per si volem iniciaŕ els valors de més d'una pilota en params.txt */
	{
		if (pos_f > n_fil - 1)
			pos_f = n_fil - 1;					/* limita posicio inicial de la pilota */
		if (pos_c > n_col - 1)
			pos_c = n_col - 1;
		f_pil = pos_f;
		c_pil = pos_c;							/* dibuixar la pilota inicialment */
		win_escricar(f_pil, c_pil, 48+i, INVERS);
	}

	/* generar els blocs */
	nb = 0;
	*nblocs = n_col / (BLKSIZE + BLKGAP) - 1;
	offset = (n_col - (*nblocs) * (BLKSIZE + BLKGAP) + BLKGAP) / 2;	/* offset de columna inicial */
	for (i = 0; i < *nblocs; i++)
	{
		for (c = 0; c < BLKSIZE; c++)
		{
			win_escricar(3, offset + c, FRNTCHAR, INVERS);
			nb++;
			win_escricar(4, offset + c, BLKCHAR, NO_INV);
			nb++;
			win_escricar(5, offset + c, FRNTCHAR, INVERS);
			nb++;
		}
		offset += BLKSIZE + BLKGAP;
	}
	*nblocs = nb / BLKSIZE;
	/* generar les defenses */
	nb = n_col / (BLKSIZE + 2 * BLKGAP) - 2;
	offset = (n_col - nb * (BLKSIZE + 2 * BLKGAP) + BLKGAP) / 2;	/* offset de columna inicial */
	for (i = 0; i < nb; i++)
	{
		for (c = 0; c < BLKSIZE + BLKGAP; c++)
			win_escricar(6, offset + c, WLLCHAR, NO_INV);
		offset += BLKSIZE + 2 * BLKGAP;
	}

	sprintf(strin, "Tecles: \'%c\'-> Esquerra, \'%c\'-> Dreta, RETURN-> sortir\n", TEC_AMUNT, TEC_AVALL);
	win_escristr(strin);
	return (0);
}

/* funcio que escriu un missatge a la línia d'estat i tanca les curses */
void mostra_final(char *miss)
{
	int lmarge;
	char marge[LONGMISS];

	/* centrar el misssatge */
	lmarge=(n_col+strlen(miss))/2;
	sprintf(marge,"%%%ds",lmarge);

	sprintf(strin, marge, miss);
	win_escristr(strin);
	win_update();
	/* espera tecla per a que es pugui veure el missatge */
	getchar();
}

/* funcio per moure la paleta segons la tecla premuda */
/* retorna un boolea indicant si l'usuari vol acabar */
void * mou_paleta(void * nul)
{
	int tecla, result;
	do{												/* Bucle paleta */
		result = 0;
		while(sem_value(id_sem)>1);
		waitS(id_sem);
		//pthread_mutex_lock(&mutex);
		tecla = win_gettec();
		signalS(id_sem);
		//pthread_mutex_unlock(&mutex);
		

		if (tecla != 0) {
			if ((tecla == TEC_DRETA) && ((*c_pal + MIDA_PALETA) < n_col - 1))
			{
					while(sem_value(id_sem)>1);
					waitS(id_sem);
					//pthread_mutex_lock(&mutex);
					win_escricar( *f_pal, *c_pal, ' ', NO_INV);			/* esborra primer bloc */
					(*c_pal)++;							/* actualitza posicio */
					win_escricar(*f_pal,*c_pal + MIDA_PALETA - 1, '0', INVERS);	/*esc. ultim bloc */
					signalS(id_sem);
					//pthread_mutex_unlock(&mutex);
			}
			if ((tecla == TEC_ESQUER) && (*c_pal > 1)) {
					while(sem_value(id_sem)>1);
					waitS(id_sem);
					//pthread_mutex_lock(&mutex);
					win_escricar(*f_pal, *c_pal + MIDA_PALETA - 1, ' ', NO_INV);	/*esborra ultim bloc */
					(*c_pal)--;							/* actualitza posicio */

					win_escricar(*f_pal, *c_pal, '0', INVERS);			/* escriure primer bloc */
					signalS(id_sem);
					//pthread_mutex_unlock(&mutex);
			}
			if (tecla == TEC_RETURN)
				result = 1;							/* final per pulsacio RETURN */
			while(sem_value(id_sem)>1);
			waitS(id_sem);
			//pthread_mutex_lock(&mutex);
			*dirPaleta = tecla;							/* per a afectar al moviment de les pilotes*/
			signalS(id_sem);
			//pthread_mutex_unlock(&mutex);
		}
		while(sem_value(id_sem)>1);
		waitS(id_sem);
		//pthread_mutex_lock(&mutex);
		*fi1 = result;
		fi2 = ((*nblocs==0) || (*num_pilotes==0));
		signalS(id_sem);
		//pthread_mutex_unlock(&mutex);
		win_retard(5);
	} while(!(*fi1) && !fi2);

	return ((void *) 0);
}

/* programa principal */
int main(int n_args, char *ll_args[])
{
	/* inicialitzacio variable compartida num_pilotes*/
n_p = ini_mem(sizeof(int));/* crear zona mem. compartida */
num_pilotes = map_mem(n_p);/* obtenir adreça mem. compartida */
*num_pilotes = 1;/* inicialitza variable compartida */

/* inicialitzacio variable compartida nblocs*/
n_b = ini_mem(sizeof(int));/* crear zona mem. compartida */
nblocs = map_mem(n_b);/* obtenir adreça mem. compartida */
*nblocs = 0;/* inicialitza variable compartida */

/* inicialitzacio variable compartida c_pal*/
c_p = ini_mem(sizeof(int));/* crear zona mem. compartida */
c_pal = map_mem(c_p);/* obtenir adreça mem. compartida */

/* inicialitzacio variable compartida f_pal*/
f_p = ini_mem(sizeof(int));/* crear zona mem. compartida */
f_pal = map_mem(f_p);/* obtenir adreça mem. compartida */

/* inicialitzacio variable compartida dirPaleta*/
dir_p = ini_mem(sizeof(int));/* crear zona mem. compartida */
dirPaleta = map_mem(dir_p);/* obtenir adreça mem. compartida */
*dirPaleta = 0;/* inicialitza variable compartida */

/* inicialitzacio variable compartida dirPaleta*/
fi_1 = ini_mem(sizeof(int));/* crear zona mem. compartida */
fi1 = map_mem(fi_1);/* obtenir adreça mem. compartida */
*fi1 = 0;/* inicialitza variable compartida */

temporitzador =ini_mem(sizeof(int));
temp = map_mem(temporitzador);
*temp = 0;

start_t = ini_mem(sizeof(int));
start_time = map_mem(start_t);
*start_time=0;
int i;
FILE *fit_conf;
clock_t end_time=0;
int segons_temp=0;

id_sem = ini_sem(1);		/* creacio semafor */
	if ((n_args != 2) && (n_args != 3)) /* si numero d'arguments incorrecte */
	{
		i = 0;
		do
			fprintf(stderr, "%s", descripcio[i++]);	/* imprimeix descripcio */
		while (descripcio[i][0] != '*');	/* mentre no arribi al final */
		exit(1);
	}

	fit_conf = fopen(ll_args[1], "rt");	/* intenta obrir el fitxer */

	if (!fit_conf)
	{
		fprintf(stderr, "Error: no s'ha pogut obrir el fitxer \'%s\'\n",
			ll_args[1]);
		exit(2);
	}

	if (carrega_configuracio(fit_conf) != 0)	/* llegir dades del fitxer  */
		exit(3);	/* aborta si hi ha algun problema en el fitxer */

	if (n_args == 3) /* si s'ha especificat parametre de retard */
	{
		retard = atoi(ll_args[2]);	/* convertir-lo a enter */
		if (retard < 10)
			retard = 10;	/* ver(sizeof members[0])) ; identificar limits */

		if (retard > 1000)
			retard = 1000;
	}
	else
		retard = 100;	/* altrament, fixar retard per defecte */

	printf("Joc del Mur: prem RETURN per continuar:\n");
	getchar();



	if (inicialitza_joc() != 0)	/* intenta crear el taulell de joc */
		exit(4);	/* aborta si hi ha algun problema amb taulell */

	char id_str[SIZE_ARRAY], fil_str[SIZE_ARRAY], col_str[SIZE_ARRAY];
	char id_mem_tauler_str[SIZE_ARRAY], vel_f_str[SIZE_ARRAY], vel_c_str[SIZE_ARRAY];
	char f_pil_str[SIZE_ARRAY], c_pil_str[SIZE_ARRAY];
	char pos_f_str[SIZE_ARRAY], pos_c_str[SIZE_ARRAY];
	char nblocs_str[SIZE_ARRAY], npils_str[SIZE_ARRAY], retard_str[SIZE_ARRAY];
	char c_pal_str[SIZE_ARRAY], f_pal_str[SIZE_ARRAY], dirPaleta_str[SIZE_ARRAY], fi1_str[SIZE_ARRAY];
	char id_sem_str[SIZE_ARRAY], id_bustia_str[SIZE_ARRAY], temp_str[SIZE_ARRAY], start_str[SIZE_ARRAY];
	
		
	//int asd = sem_value(id_sem);
	//fprintf(stderr, "%d\n", asd);
	id_bustia = ini_mis();		/* creacio bustia IPC */
	
	
	pthread_create(&tid[0],NULL, &mou_paleta, (void *) NULL);
	//pthread_create(&tid[id],NULL, &mou_pilota , (intptr_t *) id);
	tpid[0] = fork();

	if (tpid[0] == (pid_t)0)   /* Es tracta del proces fill */
  	{
  		sprintf (id_str, "%d", 0);
      	sprintf (id_mem_tauler_str, "%d", id_mem_tauler);
      	sprintf (fil_str, "%d", n_fil);
      	sprintf (col_str, "%d", n_col);
      	sprintf (vel_f_str, "%f", vel_f);
      	sprintf (vel_c_str, "%f", vel_c);
      	sprintf (pos_f_str, "%f", pos_f);
      	sprintf (pos_c_str, "%f", pos_c);
      	sprintf (f_pil_str, "%d", f_pil);
      	sprintf (c_pil_str, "%d", c_pil);
      	sprintf (nblocs_str, "%d", n_b);
      	sprintf (npils_str, "%d", n_p);
      	sprintf (retard_str, "%d", retard);
      	sprintf (c_pal_str, "%d", c_p);
      	sprintf (f_pal_str, "%d", f_p);
      	sprintf (dirPaleta_str, "%d", dir_p);
      	sprintf (fi1_str, "%d", fi_1);
		sprintf (id_sem_str, "%d", id_sem);
		sprintf (id_bustia_str, "%d", id_bustia);
		sprintf (temp_str, "%d", temporitzador);
		sprintf (start_str, "%d", start_t);

		execlp("./pilota4", "pilota4", id_str, id_mem_tauler_str, fil_str,
			col_str, vel_f_str, vel_c_str, pos_f_str, pos_c_str, f_pil_str,
			c_pil_str, nblocs_str, npils_str, retard_str, c_pal_str, f_pal_str, dirPaleta_str, fi1_str, id_sem_str, id_bustia_str, temp_str, start_str, (char *)0);
      	fprintf(stderr, "Error: No puc executar el proces fill \'pilota3\' \n");
      	exit(1);  /* Retornem error */
	}
  	else if (tpid[0] <  0 )     /* ERROR*/
  	{
    	fprintf(stderr, "Hi ha hagut un error en la creacio del proces");
  	}
	
	clock_t inici_temps = clock();		/* variable tipo tiempo para tiempo inicial */
	clock_t t_actual = clock();		/* variable tipo tiempo para tiempo actual */
	int segons = 0, minuts = 0;		/* variable segundos y minutos inicializados a 0 */
	char tiempo[LONGMISS];			/* variable 'String' para tiempo del tamaño maximo que se puede imprimir por pantalla */

	do
	{
		t_actual = clock();
		segons = ((((float) t_actual - (float) inici_temps)/CLOCKS_PER_SEC)*100)-60 * minuts;

		if (segons >= 60)
			minuts ++;

		//pthread_mutex_lock(&mutex);
		while(sem_value(id_sem)>1);
		waitS(id_sem);

		if (*temp == 1)
		{
			end_time = clock();
			segons_temp = 5-((((float) t_actual - (float) inici_temps -((float) *start_time)) / CLOCKS_PER_SEC)*100) ;
			//end_time = 5 * CLOCKS_PER_SEC + (*start_time);
			memset(tiempo, 0, sizeof tiempo);
			sprintf(tiempo, "Tiempo: %02d:%02d \t\t Temp: %02d", minuts, segons, segons_temp);
			win_escristr(tiempo);
		}
	
		else
		{
			memset(tiempo, 0, sizeof tiempo);
			sprintf(tiempo, "Tiempo: %02d:%02d", minuts, segons);
			win_escristr(tiempo);
		}
		if((5-((((float) t_actual - (float) inici_temps -((float) *start_time)) / CLOCKS_PER_SEC)*100))<=0)
    		{
			*start_time = 0;
			*temp = 0;
    		}
		//pthread_mutex_unlock(&mutex);
		win_update();
		signalS(id_sem);
		win_retard(100);		/* retard del joc */

	} while (!(*fi1) && !((*nblocs) == 0 || (*num_pilotes) == 0));

	int stat;
	win_update();
	pthread_join(tid[0], 0);
	waitpid(tpid[0], &stat, 0);
	

	if ( *nblocs == 0)
	{
		sprintf(tiempo, "YOU WIN! Tiempo: %02d:%02d", minuts, segons);
		mostra_final(tiempo);
	}
	else
	{
		sprintf(tiempo, "GAME OVER, Tiempo: %02d:%02d", minuts, segons);
		mostra_final(tiempo);
	}
	
	elim_mem(id_mem_tauler);
	elim_mem(n_p);
	elim_mem(n_b);
	elim_mem(c_p);
	elim_mem(f_p);
	elim_mem(dir_p);
	elim_mem(temporitzador);
	elim_mem(start_t);
	elim_mem(fi_1);
	elim_sem(id_sem);
	elim_mis(id_bustia);
	win_fi();		/* tanca les curses */

	return (0);		/* retorna sense errors d'execucio */
}
