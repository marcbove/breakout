/* Include de les llibreries de mur3.c */
#include <stdio.h>		/* incloure definicions de funcions estandard */
#include <stdint.h>		/* intptr_t for 64bits machines */
#include <stdlib.h>
#include <string.h>
#include <pthread.h>		/* incloure threads */
#include <unistd.h>	/* fork */
#include "winsuport2.h"		/* incloure definicions de funcions propies */
#include "memoria.h"
#include "semafor.h"
#include "missatge.h"
#include <sys/wait.h>

/* definicio de constants */
#define SIZE_ARRAY 32
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
pid_t tpid[MAXBALLS];
int *c_pal, *f_pal, *nblocs, *num_pilotes, *dirPaleta, retard, ind, c_pil, f_pil, num_fills;
int id_mem_tauler, n_fil, n_col;
float vel_f, vel_c, pos_f, pos_c;


/*Si hi ha una col.lisió pilota-bloc i esborra el bloc */
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
		(*nblocs)--;

		if (quin == BLKCHAR)
		{
			//pthread_mutex_lock(&mutex);
			char id_str[SIZE_ARRAY], fil_str[SIZE_ARRAY], col_str[SIZE_ARRAY];
			char id_mem_tauler_str[SIZE_ARRAY], vel_f_str[SIZE_ARRAY], vel_c_str[SIZE_ARRAY];
			char f_pil_str[SIZE_ARRAY], c_pil_str[SIZE_ARRAY];
			char pos_f_str[SIZE_ARRAY], pos_c_str[SIZE_ARRAY];
			char nblocs_str[SIZE_ARRAY], npils_str[SIZE_ARRAY], retard_str[SIZE_ARRAY];
			tpid[num_fills] = fork();
			//pthread_create(&tid[id],NULL, &mou_pilota , (intptr_t *) id);
			if (tpid[num_fills] == 0)   /* Es tracta del proces fill */
		  {
		  		sprintf (id_str, "%d", num_fills);
		      sprintf (id_mem_tauler_str, "%d", id_mem_tauler);
		      sprintf (fil_str, "%d", n_fil);
		      sprintf (col_str, "%d", n_col);
		      sprintf (vel_f_str, "%f", (float)rand()/(float)(RAND_MAX/2)-1);
		      sprintf (vel_c_str, "%f", (float)rand()/(float)(RAND_MAX/2)-1);
		      sprintf (pos_f_str, "%f", (float)f);
		      sprintf (pos_c_str, "%f", (float)c);
		      sprintf (f_pil_str, "%d", f);
		      sprintf (c_pil_str, "%d", c);
		      sprintf (nblocs_str, "%d", *nblocs);
		      sprintf (npils_str, "%d", *num_pilotes);
		      sprintf (retard_str, "%d", retard);
					execlp("./pilota3", "pilota3", id_str, id_mem_tauler_str, fil_str,
					col_str, vel_f_str, vel_c_str, pos_f_str, pos_c_str, f_pil_str,
					c_pil_str, nblocs_str, npils_str, retard_str, (char *)0);
		      fprintf(stderr, "Error: No puc executar el proces fill \'pilota3\' \n");
		      exit(1);  /* Retornem error */
			}
		  else if (tpid[num_fills] <  0 )     /* ERROR*/
		  {
		    	fprintf(stderr, "Hi ha hagut un error en la creacio del proces");
		  }

			
			(*num_pilotes)++;
			num_fills++;
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
	for(i = 1; i <= (intptr_t) *num_pilotes; i++)
	{
		if ((intptr_t)*dirPaleta == TEC_DRETA)
		{
			if (vel_c <= 0.0)					/* pilota cap a l'esquerra */
				vel_c = -vel_c - 0.2;				/* xoc: canvi de sentit i reduir velocitat */
			else
			{							/* a favor: incrementar velocitat */
				if (vel_c <= 0.8)
					vel_c += 0.2;
			}
		}
		else
		{
			if ((intptr_t) *dirPaleta == TEC_ESQUER)
			{
				if (vel_c >= 0.0)				/* pilota cap a la dreta */
					vel_c = -vel_c + 0.2;			/* xoc: canvi de sentit i reduir la velocitat */
				else
				{						/* a favor: incrementar velocitat */
					if (vel_c >= -0.8)
						vel_c -= 0.2;
				}
			}
			else
			{							/* XXX trucs no documentats */
				if ((intptr_t) *dirPaleta == TEC_AMUNT)
					vel_c = 0.0;				/* vertical */
				else
				{
					if ((intptr_t) *dirPaleta == TEC_AVALL)
						if (vel_f <= 1.0)
							vel_f -= 0.2;		/* frenar */
				}
			}
		}
		// Aqui otra seccion critica
		(*dirPaleta)=0;							/* reset perque ja hem aplicat l'efecte */
	}
}

float control_impacte2(int c_pil, float velc0)
{
	int distApal;
	float vel_c;

	distApal = c_pil - (intptr_t) *c_pal;
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


/* La funcio retornarà 1 si la pilota surt de la porteria, 0 altrament */
/* funcio per moure la pilota: El valor que es passa pel paràmetre ind serà un enter que indicarà l’ordre de creació de les pilotes (0 -> primera, 1 -> segona, etc.). Aquest paràmetre servirà per accedir
a   la   taula   global   d’informació   de   les   pilotes,   així   com   per   escriure   el   caràcter
corresponent   (identificador   ‘1’   per   la   primera,   ‘2’   per   la   segona,   etc.). */
int main(int n_args, char *ll_args[])
//void * mou_pilota(void * ind)
{
	int fi2 = 0, fi3 = 0;
	/* Parsing arguments */
	ind = (intptr_t)atoi(ll_args[1]);
	id_mem_tauler = atoi(ll_args[2]);
	n_fil = atoi(ll_args[3]);
    n_col = atoi(ll_args[4]);
    vel_f = atof(ll_args[5]);
    vel_c = atof(ll_args[6]);
    pos_f = atof(ll_args[7]);
    pos_c = atof(ll_args[8]);
    f_pil = atoi(ll_args[9]);
    c_pil = atoi(ll_args[10]);
    * nblocs = atoi(ll_args[11]);
    * num_pilotes =  atoi(ll_args[12]);
    retard = atoi(ll_args[13]);


    int * addr_tauler = map_mem(id_mem_tauler);
    win_set(addr_tauler, n_fil, n_col);

	int f_h, c_h;
	char rh, rv, rd, no;
	//int in = (intptr_t)ind;

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
					win_escricar(f_pil, c_pil, 49+ind, INVERS);	/* imprimeix pilota on caracter que es passa es el codi ascii de 0+ind*/
					//pthread_mutex_unlock(&mutex);
				}
				else
				{
					//pthread_mutex_lock(&mutex);
					(*num_pilotes)--;
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
		fi2 = ((*nblocs) == 0 || (*num_pilotes) == 0);
		//pthread_mutex_unlock(&mutex);
		win_retard(retard);

	} while(!fi3 && !fi2); /* fer bucle fins que la pilota surti de la porteria i llavors acabar el proces????? */
	int i;
	int stat;
	for (i=0; i< num_fills; i++){
		waitpid(tpid[i], &stat, 0);
	}

	return (fi3);
}
