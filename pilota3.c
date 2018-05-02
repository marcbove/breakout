/* Include de les llibreries de mur3.c */
#include <stdio.h>		/* incloure definicions de funcions estandard */
#include <stdint.h>		/* intptr_t for 64bits machines */
#include <stdlib.h>
#include <string.h>
#include "winsuport2.h"		/* incloure definicions de funcions propies */
#include <pthread.h>		/* incloure threads */	
#include "memoria.h"
#include "semafor.h"
#include "missatge.h"

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

/*Si hi ha una col.lisió pilota-bloci esborra el bloc */
void comprovar_bloc(int f, int c)
{
	int col;
	//pthread_mutex_lock(&mutex);
	char quin = win_quincar(f, c);
	//pthread_mutex_unlock(&mutex);

	if (quin == BLKCHAR || quin == FRNTCHAR)
	{
		col = c;
		while (win_quincar(f, col) != ' ')
		{
			//pthread_mutex_lock(&mutex);
			win_escricar(f, col, ' ', NO_INV);
			//pthread_mutex_unlock(&mutex);
			col++;
		}
		col = c - 1;
		while (win_quincar(f, col) != ' ')
		{
			//pthread_mutex_lock(&mutex);
			win_escricar(f, col, ' ', NO_INV);
			//pthread_mutex_unlock(&mutex);
			col--;
		}
		nblocs--;

		if (quin == BLKCHAR)
		{
			//pthread_mutex_lock(&mutex);
			id++;
			pthread_create(&tid[id],NULL, &mou_pilota , (intptr_t *) id);
			pos_f[id] = f;
			pos_c[id] = c;
			vel_f[id] = (float)rand()/(float)(RAND_MAX/2)-1;
			vel_c[id] = (float)rand()/(float)(RAND_MAX/2)-1;
			num_pilotes++;
			//pthread_mutex_unlock(&mutex);
		}
	}
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

/* funcio per moure la pilota: El valor que es passa pel paràmetre index serà un enter que indicarà l’ordre de creació de les pilotes (0 -> primera, 1 -> segona, etc.). Aquest paràmetre servirà per accedir
a   la   taula   global   d’informació   de   les   pilotes,   així   com   per   escriure   el   caràcter
corresponent   (identificador   ‘1’   per   la   primera,   ‘2’   per   la   segona,   etc.). */
int main(int n_args, char *ll_args[])
//void * mou_pilota(void * index)
{




	int f_h, c_h;
	int fi3 = 0;
	char rh, rv, rd, no;
	int in = (intptr_t)index;
	do									/* Bucle pelota */
	{
		f_h = pos_f[in] + vel_f[in];				/* posicio hipotetica de la pilota (entera) */
		c_h = pos_c[in] + vel_c[in];
		rh = rv = rd = ' ';
		if ((f_h != f_pil[in]) || (c_h != c_pil[in]))
		{
		/* si posicio hipotetica no coincideix amb la posicio actual */
			if (f_h != f_pil[in]) 					/* provar rebot vertical */
			{
				pthread_mutex_lock(&mutex);
				rv = win_quincar(f_h, c_pil[in]);			/* veure si hi ha algun obstacle */
				pthread_mutex_unlock(&mutex);
				if (rv != ' ') 					/* si hi ha alguna cosa */
				{
					pthread_mutex_lock(&mutex);
					comprovar_bloc(f_h, c_pil[in]);
					pthread_mutex_unlock(&mutex);
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

			if (c_h != c_pil[in]) /* provar rebot horitzontal */
			{	
				pthread_mutex_lock(&mutex);
				rh = win_quincar(f_pil[in], c_h);	/* veure si hi ha algun obstacle */
				pthread_mutex_unlock(&mutex);

				if (rh != ' ') /* si hi ha algun obstacle */
				{	
					pthread_mutex_lock(&mutex);
					comprovar_bloc(f_pil[in], c_h);
					pthread_mutex_unlock(&mutex);
					/* TODO?: tractar la col.lisio lateral amb la paleta */
					vel_c[in] = -vel_c[in];	/* canvia sentit vel. horitzontal */
					c_h = pos_c[in] + vel_c[in];	/* actualitza posicio hipotetica */
				}
			}

			if ((f_h != f_pil[in]) && (c_h != c_pil[in])) /* provar rebot diagonal */
			{	
				pthread_mutex_lock(&mutex);
				rd = win_quincar(f_h, c_h);
				pthread_mutex_unlock(&mutex);
				if (rd != ' ') /* si hi ha obstacle */
				{	
					pthread_mutex_lock(&mutex);
					comprovar_bloc(f_h, c_h);
					pthread_mutex_unlock(&mutex);
					/* TODO?: tractar la col.lisio amb la paleta */
					vel_f[in] = -vel_f[in];
					vel_c[in]= -vel_c[in];	/* canvia sentit velocitats */
					f_h = pos_f[in] + vel_f[in];
					c_h = pos_c[in] + vel_c[in];	/* actualitza posicio entera */
				}
			}

			/* mostrar la pilota a la nova posició */
			pthread_mutex_lock(&mutex);
			no = win_quincar(f_h, c_h);
			pthread_mutex_unlock(&mutex);
			if (no==' ')
			{	/* verificar posicio definitiva *//* si no hi ha obstacle */
				pthread_mutex_lock(&mutex);
				win_escricar(f_pil[in], c_pil[in], ' ', INVERS);	/* esborra pilota */
				pthread_mutex_unlock(&mutex);
				pos_f[in] += vel_f[in];
				pos_c[in] += vel_c[in];
				f_pil[in] = f_h;
				c_pil[in] = c_h;	/* actualitza posicio actual */
				pthread_mutex_lock(&mutex);
				if (f_pil[in] != n_fil - 1)	/* si no surt del taulell, */
				{
					//pthread_mutex_lock(&mutex);
					win_escricar(f_pil[in], c_pil[in], 48+in, INVERS);	/* imprimeix pilota on caracter que es passa es el codi ascii de 0+index*/
					//pthread_mutex_unlock(&mutex);
				}
				else
				{
					//pthread_mutex_lock(&mutex);
					num_pilotes--;
					//pthread_mutex_unlock(&mutex);
					fi3 = 1;
				}
				pthread_mutex_unlock(&mutex);
			}
		}
		else
		{	/* posicio hipotetica = a la real: moure */
			pos_f[in] += vel_f[in];
			pos_c[in] += vel_c[in];
		}
		pthread_mutex_lock(&mutex);
		fi2 = (nblocs==0 || num_pilotes==0);
		pthread_mutex_unlock(&mutex);
		win_retard(retard);

	} while(!fi1 && !fi2 && !fi3);

	vel_f[in]=0.0;
	vel_c[in]=0.0;
	
	return ((void *) index);
}