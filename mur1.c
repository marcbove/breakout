/*****************************************************************************/
/*                                                                           */
/*                           mur2.c                                          */
/*                                                                           */
/*  Compilar i executar:                                                     */
/*     El programa invoca les funcions definides a "winsuport.c", les        */
/*     quals proporcionen una interficie senzilla per crear una finestra     */
/*     de text on es poden escriure caracters en posicions especifiques de   */
/*     la pantalla (basada en CURSES); per tant, el programa necessita ser   */
/*     compilat amb la llibreria 'curses':                                   */
/*                                                                           */
/*       $ gcc -c winsuport.c -o winsuport.o                                 */
/*       $ gcc mur2.c winsuport.o -o mur2 -lcurses -lpthread         	     */
/*                                                                           */
/*  Al tenir una orientació vertical cal tenir un terminal amb prou files.   */
/*  Per exemple:                                                             */
/*               xterm -geometry 80x50                                       */
/*               gnome-terminal --geometry 80x50                             */
/*                                                                           */
/*****************************************************************************/

//#include <stdint.h>		/* intptr_t for 64bits machines */

#include <stdio.h>		/* incloure definicions de funcions estandard */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "winsuport.h"		/* incloure definicions de funcions propies */
#include <pthread.h>		/* incloure threads */

/* definicio de constants */
#define MAX_THREADS	10
#define MAXBALLS	(MAX_THREADS-1)
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

pthread_t tid[MAX_THREADS];	/* tabla de identificadors de threads */

intptr_t id = 1;
int num_pilotes = 1;
int n_fil, n_col;		/* numero de files i columnes del taulell */
int m_por;			/* mida de la porteria (en caracters) */
int f_pal, c_pal;		/* posicio del primer caracter de la paleta */
int f_pil[MAXBALLS], c_pil[MAXBALLS];		/* posicio de la pilota, en valor enter */
float pos_f[MAXBALLS], pos_c[MAXBALLS];		/* posicio de la pilota, en valor real */
float vel_f[MAXBALLS], vel_c[MAXBALLS];		/* velocitat de la pilota, en valor real */
int nblocs = 0;
int dirPaleta = 0;
int retard;			/* valor del retard de moviment, en mil.lisegons */
int fi1, fi2;			/* valor de condicions finals */

char strin[LONGMISS];		/* variable per a generar missatges de text */

/* funcio per carregar i interpretar el fitxer de configuracio de la partida */
/* el parametre ha de ser un punter a fitxer de text, posicionat al principi */
/* la funcio tanca el fitxer, i retorna diferent de zero si hi ha problemes  */
int carrega_configuracio(FILE * fit)
{
	int ret = 0;
	int i = 1;
	fscanf(fit, "%d %d %d\n", &n_fil, &n_col, &m_por);				/* camp de joc */
	while(fscanf(fit, "%f %f %f %f\n", &pos_f[i], &pos_c[i], &vel_f[i], &vel_c[i]) == 4)			/* pilota */
	{
		if ((n_fil != 0) || (n_col != 0)) 						/* si no dimensions maximes */
		{
			if ((n_fil < MIN_FIL) || (n_fil > MAX_FIL) || (n_col < MIN_COL) || (n_col > MAX_COL))
				ret = 1;
			else if (m_por > n_col - 3)
				ret = 2;
			else if ((pos_f[i] < 1) || (pos_f[i] >= n_fil - 3) || (pos_c[i] < 1) || (pos_c[i] > n_col - 1))				/* tres files especials: línia d'estat, porteria i paleta */
				ret = 3;
		}
		if ((vel_f[i] < -1.0) || (vel_f[i] > 1.0) || (vel_c[i] < -1.0) || (vel_c[i] > 1.0))
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
				fprintf(stderr, "\tpos_f= %.2f \tpos_c= %.2f\n", pos_f[i], pos_c[i]);
				break;
			case 4:
				fprintf(stderr, "\tvelocitat de la pilota incorrecta:\n");
				fprintf(stderr, "\tvel_f= %.2f \tvel_c= %.2f\n", vel_f[i], vel_c[i]);
				break;
			}
			return(ret);
		}
		i++;

	}
	fclose(fit);
	num_pilotes = i-1;
	return(ret);
}

/* funcio per inicialitar les variables i visualitzar l'estat inicial del joc */
/* retorna diferent de zero si hi ha algun problema */
int inicialitza_joc(void)
{
	int i, retwin;
	int i_port, f_port;						/* inici i final de porteria */
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

	if (m_por > n_col - 2)
		m_por = n_col - 2;					/* limita valor de la porteria */
	if (m_por == 0)
		m_por = 3 * (n_col - 2) / 4;				/* valor porteria per defecte */

	i_port = n_col / 2 - m_por / 2 - 1;				/* crea el forat de la porteria */
	f_port = i_port + m_por - 1;
	for (i = i_port; i <= f_port; i++)
		win_escricar(n_fil - 2, i, ' ', NO_INV);

	n_fil = n_fil - 1;						/* descompta la fila de missatges */
	f_pal = n_fil - 2;						/* posicio inicial de la paleta per defecte */
	c_pal = (n_col-MIDA_PALETA) / 2;				/* a baix i centrada */
	for (i = 0; i < MIDA_PALETA; i++)				/* dibuixar paleta inicial */
		win_escricar(f_pal, c_pal + i, '0', INVERS);

	/* generar la pilota */
	for (i = 1; i <= num_pilotes; i++)
	{
		if (pos_f[i] > n_fil - 1)
			pos_f[i] = n_fil - 1;					/* limita posicio inicial de la pilota */
		if (pos_c[i] > n_col - 1)
			pos_c[i] = n_col - 1;
		f_pil[i] = pos_f[i];
		c_pil[i] = pos_c[i];							/* dibuixar la pilota inicialment */
		win_escricar(f_pil[i], c_pil[i], 48+i, INVERS);
	}

	/* generar els blocs */
	nb = 0;
	nblocs = n_col / (BLKSIZE + BLKGAP) - 1;
	offset = (n_col - nblocs * (BLKSIZE + BLKGAP) + BLKGAP) / 2;	/* offset de columna inicial */
	for (i = 0; i < nblocs; i++)
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
	nblocs = nb / BLKSIZE;
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

	sprintf(strin, marge,miss);
	win_escristr(strin);

	/* espera tecla per a que es pugui veure el missatge */
	getchar();
}

/* funcio per a calcular rudimentariament els efectes amb la pala */
/* no te en compta si el moviment de la paleta no és recent */
/* cal tenir en compta que després es calcula el rebot */
void control_impacte(void)
{
	int i;
	for(i = 1; i <= num_pilotes; i++)
	{
		if (dirPaleta == TEC_DRETA)
		{
			if (vel_c[i] <= 0.0)					/* pilota cap a l'esquerra */
				vel_c[i] = -vel_c[i] - 0.2;				/* xoc: canvi de sentit i reduir velocitat */
			else
			{							/* a favor: incrementar velocitat */
				if (vel_c[i] <= 0.8)
					vel_c[i] += 0.2;
			}
		}
		else
		{
			if (dirPaleta == TEC_ESQUER)
			{
				if (vel_c[i] >= 0.0)				/* pilota cap a la dreta */
					vel_c[i] = -vel_c[i] + 0.2;			/* xoc: canvi de sentit i reduir la velocitat */
				else
				{						/* a favor: incrementar velocitat */
					if (vel_c[i] >= -0.8)
						vel_c[i] -= 0.2;
				}
			}
			else
			{							/* XXX trucs no documentats */
				if (dirPaleta == TEC_AMUNT)
					vel_c[i] = 0.0;				/* vertical */
				else
				{
					if (dirPaleta == TEC_AVALL)
						if (vel_f[i] <= 1.0)
							vel_f[i] -= 0.2;		/* frenar */
				}
			}
		}
		dirPaleta=0;							/* reset perque ja hem aplicat l'efecte */
	}
}

float control_impacte2(int c_pil, float velc0)
{
	int distApal;
	float vel_c;

	distApal = c_pil - c_pal;
	if (distApal >= 2*MIDA_PALETA/3)				/* costat dreta */
		vel_c = 0.5;
	else if (distApal <= MIDA_PALETA/3)				/* costat esquerra */
		vel_c = -0.5;
	else if (distApal == MIDA_PALETA/2)				/* al centre */
		vel_c = 0.0;
	else 								/*: rebot normal */
		vel_c = velc0;
	return vel_c;
}

void * mou_pilota(void * index);

/*Si hi ha una col.lisió pilota-bloci esborra el bloc */
void comprovar_bloc(int f, int c)
{
	int col;
	
	char quin = win_quincar(f, c);
	

	if (quin == BLKCHAR || quin == FRNTCHAR)
	{
		col = c;
		while (win_quincar(f, col) != ' ')
		{
			
			win_escricar(f, col, ' ', NO_INV);
			
			col++;
		}
		col = c - 1;
		while (win_quincar(f, col) != ' ')
		{
			
			win_escricar(f, col, ' ', NO_INV);
			
			col--;
		}
		nblocs--;

		if (quin == BLKCHAR)
		{
			
			pos_f[id] = f;
			pos_c[id] = c;
			vel_f[id] = (float)rand()/(float)(RAND_MAX/2)-1;
			vel_c[id] = (float)rand()/(float)(RAND_MAX/2)-1;
			pthread_create(&tid[id],NULL, &mou_pilota , (intptr_t *) id);
			id++;
			num_pilotes++;
			
		}
	}
}

/* funcio per moure la pilota: El valor que es passa pel paràmetre index serà un enter que indicarà l’ordre de creació de les pilotes (0 -> primera, 1 -> segona, etc.). Aquest paràmetre servirà per accedir
a   la   taula   global   d’informació   de   les   pilotes,   així   com   per   escriure   el   caràcter
corresponent   (identificador   ‘1’   per   la   primera,   ‘2’   per   la   segona,   etc.). */
void * mou_pilota(void * index)
{
	int f_h, c_h;
	int fi3 = 0;
	char rh, rv, rd, no;
	int in = (intptr_t)index;
	do{									/* Bucle pelota */
		f_h = pos_f[in] + vel_f[in];				/* posicio hipotetica de la pilota (entera) */
		c_h = pos_c[in] + vel_c[in];
		rh = rv = rd = ' ';
		if ((f_h != f_pil[in]) || (c_h != c_pil[in]))
		{
		/* si posicio hipotetica no coincideix amb la posicio actual */
			if (f_h != f_pil[in]) 					/* provar rebot vertical */
			{
				
				rv = win_quincar(f_h, c_pil[in]);			/* veure si hi ha algun obstacle */
				
				if (rv != ' ') 					/* si hi ha alguna cosa */
				{
					comprovar_bloc(f_h, c_pil[in]);
					if (rv == '0')				/* col.lisió amb la paleta? */
					{
						/* XXX: tria la funció que vulgis o implementa'n una millor */
						control_impacte();
						vel_c[in] = control_impacte2(c_pil[in], vel_c[in]);
					}
					vel_f[in] = -vel_f[in];				/* canvia sentit velocitat vertical */
					f_h = pos_f[in] + vel_f[in];			/* actualitza posicio hipotetica */
				}
			}

			if (c_h != c_pil[in]) {	/* provar rebot horitzontal */
				
				rh = win_quincar(f_pil[in], c_h);	/* veure si hi ha algun obstacle */
				

				if (rh != ' ') {	/* si hi ha algun obstacle */
					comprovar_bloc(f_pil[in], c_h);
					/* TODO?: tractar la col.lisio lateral amb la paleta */
					vel_c[in] = -vel_c[in];	/* canvia sentit vel. horitzontal */
					c_h = pos_c[in] + vel_c[in];	/* actualitza posicio hipotetica */
				}
			}

			if ((f_h != f_pil[in]) && (c_h != c_pil[in])) {	/* provar rebot diagonal */
				
				rd = win_quincar(f_h, c_h);
				
				if (rd != ' ') {	/* si hi ha obstacle */
					comprovar_bloc(f_h, c_h);
					/* TODO?: tractar la col.lisio amb la paleta */
					vel_f[in] = -vel_f[in];
					vel_c[in]= -vel_c[in];	/* canvia sentit velocitats */
					f_h = pos_f[in] + vel_f[in];
					c_h = pos_c[in] + vel_c[in];	/* actualitza posicio entera */
				}
			}

			/* mostrar la pilota a la nova posició */
			
			no = win_quincar(f_h, c_h);
			
			if (no==' ')
			{	/* verificar posicio definitiva *//* si no hi ha obstacle */
				
				win_escricar(f_pil[in], c_pil[in], ' ', NO_INV);	/* esborra pilota */
				
				pos_f[in] += vel_f[in];
				pos_c[in] += vel_c[in];
				f_pil[in] = f_h;
				c_pil[in] = c_h;	/* actualitza posicio actual */
				
				if (f_pil[in] != n_fil - 1)	/* si no surt del taulell, */
				{
					
					win_escricar(f_pil[in], c_pil[in], 48+in, INVERS);	/* imprimeix pilota on caracter que es passa es el codi ascii de 0+index*/
					
				}
				else
				{
					
					num_pilotes--;
					
					fi3 = 1;
				}
			}
		}
		else
		{	/* posicio hipotetica = a la real: moure */
			pos_f[in] += vel_f[in];
			pos_c[in] += vel_c[in];
		}
		
		fi2 = (nblocs==0 || num_pilotes==0);
		
		win_retard(retard);

	} while(!fi1 && !fi2 && !fi3);
	vel_f[in]=0.0;
	vel_c[in]=0.0;
	return ((void *) index);
}


/* funcio per moure la paleta segons la tecla premuda */
/* retorna un boolea indicant si l'usuari vol acabar */
void * mou_paleta(void * nul)
{
	int tecla, result;
	do{												/* Bucle paleta */
		result = 0;

		tecla = win_gettec();
		

		if (tecla != 0) {
			if ((tecla == TEC_DRETA) && ((c_pal + MIDA_PALETA) < n_col - 1))
			{
					
					win_escricar(f_pal, c_pal, ' ', NO_INV);			/* esborra primer bloc */

					c_pal++;							/* actualitza posicio */

					win_escricar(f_pal, c_pal + MIDA_PALETA - 1, '0', INVERS);	/*esc. ultim bloc */
					
			}
			if ((tecla == TEC_ESQUER) && (c_pal > 1)) {
					
					win_escricar(f_pal, c_pal + MIDA_PALETA - 1, ' ', NO_INV);	/*esborra ultim bloc */
					c_pal--;							/* actualitza posicio */

					win_escricar(f_pal, c_pal, '0', INVERS);			/* escriure primer bloc */
					
			}
			if (tecla == TEC_RETURN)
				result = 1;							/* final per pulsacio RETURN */
			
			dirPaleta = tecla;							/* per a afectar al moviment de les pilotes*/
			
		}
		
		fi1 = result;
		
		win_retard(5);
	} while(!fi1 && !fi2);

	return ((void *) 0);
}


/* programa principal */
int main(int n_args, char *ll_args[])
{
	int i;
	FILE *fit_conf;

	if ((n_args != 2) && (n_args != 3)) {	/* si numero d'arguments incorrecte */
		i = 0;
		do
			fprintf(stderr, "%s", descripcio[i++]);	/* imprimeix descripcio */
		while (descripcio[i][0] != '*');	/* mentre no arribi al final */
		exit(1);
	}

	fit_conf = fopen(ll_args[1], "rt");	/* intenta obrir el fitxer */
	if (!fit_conf) {
		fprintf(stderr, "Error: no s'ha pogut obrir el fitxer \'%s\'\n",
			ll_args[1]);
		exit(2);
	}

	if (carrega_configuracio(fit_conf) != 0)	/* llegir dades del fitxer  */
		exit(3);	/* aborta si hi ha algun problema en el fitxer */

	if (n_args == 3) {	/* si s'ha especificat parametre de retard */
		retard = atoi(ll_args[2]);	/* convertir-lo a enter */
		if (retard < 10)
			retard = 10;	/* ver(sizeof members[0]) );ificar limits */
		if (retard > 1000)
			retard = 1000;
	} else
		retard = 100;	/* altrament, fixar retard per defecte */

	printf("Joc del Mur: prem RETURN per continuar:\n");
	getchar();

	if (inicialitza_joc() != 0)	/* intenta crear el taulell de joc */
		exit(4);	/* aborta si hi ha algun problema amb taulell */

	pthread_create(&tid[id],NULL, &mou_pilota , (intptr_t *) id);
	id++;

	pthread_create(&tid[0],NULL, &mou_paleta, (void *) NULL);

	clock_t inici_temps = clock();		/* variable tipo tiempo para tiempo inicial */
	clock_t t_actual = clock();		/* variable tipo tiempo para tiempo actual */
	int segons = 0, minuts = 0;		/* variable segundos y minutos inicializados a 0 */
	char tiempo[LONGMISS];			/* variable 'String' para tiempo del tamaño maximo que se puede imprimir por pantalla */

	do {
		t_actual = clock();
		segons = ((((float) t_actual - (float) inici_temps)/CLOCKS_PER_SEC)*100)-60 * minuts;
		if (segons >= 60)
			minuts ++;
		
		memset(tiempo, 0, sizeof tiempo);
		sprintf(tiempo, "Tiempo: %02d:%02d", minuts, segons);
		win_escristr(tiempo);
		
		win_retard(100);		/* retard del joc */
	} while (!fi1 && !fi2);

	t_actual = clock();
	segons = ((((float) t_actual - (float) inici_temps)/CLOCKS_PER_SEC)*100)-60 * minuts;
	if (segons >= 60)
	{
		segons = 0;
		minuts++;
	}	
	memset(tiempo, 0, sizeof tiempo);


	if (nblocs == 0)
	{
		sprintf(tiempo, "YOU WIN! Tiempo: %02d:%02d", minuts, segons);
		mostra_final(tiempo);
	}
	else
	{
		sprintf(tiempo, "GAME OVER, Tiempo: %02d:%02d", minuts, segons);
		mostra_final(tiempo);
	}

	win_fi();		/* tanca les curses */


	return (0);		/* retorna sense errors d'execucio */
}
