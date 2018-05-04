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
/*						*/


/* La funcio retornarà 1 si la pilota surt de la porteria, 0 altrament */
/* funcio per moure la pilota: El valor que es passa pel paràmetre index serà un enter que indicarà l’ordre de creació de les pilotes (0 -> primera, 1 -> segona, etc.). Aquest paràmetre servirà per accedir
a   la   taula   global   d’informació   de   les   pilotes,   així   com   per   escriure   el   caràcter
corresponent   (identificador   ‘1’   per   la   primera,   ‘2’   per   la   segona,   etc.). */
int main(int n_args, char *ll_args[])
//void * mou_pilota(void * index)
{
	int fi2 = 0, fi3 = 0;
	/* Parsing arguments */
	int index = (intptr_t)atoi(ll_args[1]);
	int id_mem_tauler = atoi(ll_args[2]);
	int n_fil = atoi(ll_args[3]);
    int n_col = atoi(ll_args[4]);
    float vel_f = atof(ll_args[5]);
    float vel_c = atof(ll_args[6]);
    float pos_f = atof(ll_args[7]);
    float pos_c = atof(ll_args[8]);
    int f_pil = atoi(ll_args[9]);
    int c_pil = atoi(ll_args[10]);	
    int nblocs = atoi(ll_args[11]);
    int num_pilotes = atoi(ll_args[12]);	
    int retard = atoi(ll_args[13]);
    	
	
    int * addr_tauler = map_mem(id_mem_tauler);
    win_set(addr_tauler, n_fil, n_col);
	
	int f_h, c_h;
	char rh, rv, rd, no;
	//int in = (intptr_t)index;
	do									/* Bucle pelota */
	{
		f_h = pos_f + vel_f;				/* posicio hipotetica de la pilota (entera) */
		c_h = pos_c + vel_c;
		rh = rv = rd = ' ';
		if ((f_h != f_pil) || (c_h != c_pil))
		{
		/* si posicio hipotetica no coincideix amb la posicio actual */
			if (f_h != f_pil) 					/* provar rebot vertical */
			{
				//pthread_mutex_lock(&mutex);
				rv = win_quincar(f_h, c_pil);			/* veure si hi ha algun obstacle */
				//pthread_mutex_unlock(&mutex);
				if (rv != ' ') 					/* si hi ha alguna cosa */
				{
					//pthread_mutex_lock(&mutex);
					comprovar_bloc(f_h, c_pil);
					//pthread_mutex_unlock(&mutex);
					if (rv == '0')				/* col.lisió amb la paleta? */
					{
						/* XXX: tria la funció que vulgis o implementa'n una millor */
						control_impacte();
						vel_c = control_impacte2(c_pil, vel_c);
					}
					vel_f = -vel_f;				/* canvia sentit velocitat vertical */
					f_h = pos_f + vel_f;			/* actualitza posicio hipotetica */
				}
			}

			if (c_h != c_pil) /* provar rebot horitzontal */
			{	
				//pthread_mutex_lock(&mutex);
				rh = win_quincar(f_pil, c_h);	/* veure si hi ha algun obstacle */
				//pthread_mutex_unlock(&mutex);

				if (rh != ' ') /* si hi ha algun obstacle */
				{	
					//pthread_mutex_lock(&mutex);
					comprovar_bloc(f_pil, c_h);
					//pthread_mutex_unlock(&mutex);
					/* TODO?: tractar la col.lisio lateral amb la paleta */
					vel_c = -vel_c;	/* canvia sentit vel. horitzontal */
					c_h = pos_c + vel_c;	/* actualitza posicio hipotetica */
				}
			}

			if ((f_h != f_pil) && (c_h != c_pil)) /* provar rebot diagonal */
			{	
				//pthread_mutex_lock(&mutex);
				rd = win_quincar(f_h, c_h);
				//pthread_mutex_unlock(&mutex);
				if (rd != ' ') /* si hi ha obstacle */
				{	
					//pthread_mutex_lock(&mutex);
					comprovar_bloc(f_h, c_h);
					//pthread_mutex_unlock(&mutex);
					/* TODO?: tractar la col.lisio amb la paleta */
					vel_f = -vel_f;
					vel_c= -vel_c;	/* canvia sentit velocitats */
					f_h = pos_f + vel_f;
					c_h = pos_c + vel_c;	/* actualitza posicio entera */
				}
			}

			/* mostrar la pilota a la nova posició */
			//pthread_mutex_lock(&mutex);
			no = win_quincar(f_h, c_h);
			//pthread_mutex_unlock(&mutex);
			if (no == ' ')
			{	/* verificar posicio definitiva *//* si no hi ha obstacle */
				//pthread_mutex_lock(&mutex);
				win_escricar(f_pil, c_pil, ' ', INVERS);	/* esborra pilota */
				//pthread_mutex_unlock(&mutex);
				pos_f += vel_f;
				pos_c += vel_c;
				f_pil = f_h;
				c_pil = c_h;	/* actualitza posicio actual */
				//pthread_mutex_lock(&mutex);
				if (f_pil != n_fil - 1)	/* si no surt del taulell, */
				{
					//pthread_mutex_lock(&mutex);
					win_escricar(f_pil, c_pil, 48+index, INVERS);	/* imprimeix pilota on caracter que es passa es el codi ascii de 0+index*/
					//pthread_mutex_unlock(&mutex);
				}
				else
				{
					//pthread_mutex_lock(&mutex);
					num_pilotes--;
					//pthread_mutex_unlock(&mutex);
					fi3 = 1;
				}
				//pthread_mutex_unlock(&mutex);
			}
		}
		else
		{	/* posicio hipotetica = a la real: moure */
			pos_f += vel_f;
			pos_c += vel_c;
		}
		//pthread_mutex_lock(&mutex);
		fi2 = (nblocs == 0 || num_pilotes == 0);
		//pthread_mutex_unlock(&mutex);
		win_retard(retard);

	} while(!fi2 && !fi3); /* fer bucle fins que la pilota surti de la porteria i llavors acabar el proces????? */

	vel_f = 0.0;
	vel_c = 0.0;
	
	return (fi3);
}