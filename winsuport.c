/*****************************************************************************/
/*                                                                           */
/*                                   winsuport.c                             */
/*                                                                           */
/* Rutines de suport al control de pantalla, utilitzant la llibreria 'curses'*/
/*      (gcc -c winsuport.c -o winsuport.o)                                  */
/*                                                                           */
/* La descripcio general del funcionament de les rutines, aixi com la decla- */
/* racio de constants, es troben al fitxer 'winsuport.h'		     */
/*									     */
/*****************************************************************************/

#include <stdio.h>
#include <signal.h>
#include <curses.h>
#include "winsuport.h"


WINDOW* win;                /* finestra d'acces al terminal */
void    (*oldsig)(int);     /* referencia a l'antiga rutina de signal */
bool    hi_ha_win = FALSE;  /* flag de finestra creada */
short   ufil;               /* numero de l'ultima fila de la finestra */
short	ucol;		    /* numero de l'ultima columna de la finestra */
int		cursor;


int win_ini(int *fil, int *col, char creq, unsigned int inv)
{
  int p_fil,p_col;
  chtype wc;

  if (hi_ha_win) return(-1);       /* error si ja s'havia creat la finestra */
  if (initscr() == NULL) return(-2);    /* error en iniciar les curses */
  /* resize per si depureu amb el DDD */
//  resize_term(*fil, *col);
  if ((*fil > LINES) || (*col > COLS))    /* error en les mides */
  { endwin();
    return(-3);
  }
  if (*fil == 0) *fil = LINES;	/* cas de mida maxima */
  if (*col == 0) *col = COLS;
  p_fil = (LINES - *fil)/ 2;		/* posicio de la finestra centrada */
  p_col = (COLS - *col) / 2;
                              /* canviar interrupcio de final de proces */
  oldsig = signal(SIGINT,win_fi);
	                     /* configuracio preliminar de les curses */
  cbreak();			/* rep tecles sense buffering */
  noecho();			/* no fa echo de les tecles llegides */
  nonl();			/* el newline \n no es tradueix a CR i LF */
  intrflush(stdscr,FALSE);	/* ^C no s'esborra del buffer d'entrada */
  keypad(stdscr,TRUE);		/* s'activa el teclat numeric i mov. cursor */

  win = newwin(*fil, *col, p_fil, p_col);

  if (win == NULL)            /* error en la creacio de la finestra */
  { endwin();
    return(-4);
  }
  hi_ha_win = TRUE;

  leaveok(win,TRUE);	/* dibuixar en pantalla no modificara el cursor */
  cursor= curs_set(0);		/* fixa el cursor com a invisible */
  nodelay(win,TRUE);
  refresh();		/* refresca la pantalla (inicialment l'esborra) */

  ufil = *fil;			/* memoritza numero files */
  ucol = *col;			/* memoritza numero columnes */
  wc = creq;			/* dibuixa el requadre */
  if (inv) wc |= A_REVERSE;
  mvwhline(win,0,0,wc,ucol);		/* fila superior */
  mvwhline(win,ufil-2,0,wc,ucol);	/* fila inferior */
  mvwvline(win,1,0,wc,ufil-2);		/* columna dreta */
  mvwvline(win,1,ucol-1,wc,ufil-2);	/* columna esquerra */
  wrefresh(win);		/* redibuixa el contingut de la finestra */
  return(0);			/* retorna OK */
}


void win_fi()
{
  curs_set(cursor);		/* fixa el cursor com a visible */
  delwin(win);			/* allibera recursos de la finestra */
  refresh();
  endwin();			/* finalitza l'entorn de curses */
  setvbuf(stdout,NULL, _IOLBF, BUFSIZ);
  signal(SIGINT,oldsig);                /* restaura antiga rutina de signal */
  hi_ha_win = FALSE;
}


void win_escricar(int f, int c, char car, unsigned int invers)
{
  chtype wc;

  if (hi_ha_win)
  { wc = car;
    if (invers) wc |= A_REVERSE;
    mvwaddch(win,f,c,wc);
    wrefresh(win);
    refresh();
  }
}


char win_quincar(int f, int c)
{
  char result;
  chtype wc;

  result = 0;			/* resultat per defecte */
  if (hi_ha_win)
  { wc = mvwinch(win,f,c);		/* captura caracter */
    result = wc & A_CHARTEXT;		/* elimina atributs */
    if ((result < ' ') || (result > '~')) /* filtra valors fora del rang */
	result = ' ';	/* retorna blanc si no es un caracter ASCII */
  }
  return(result);
}


int win_quinatri(int f, int c)
{
  int result = 0;
  chtype wc;

  if (hi_ha_win)
  { wc = mvwinch(win,f,c);	/* captura caracter */
    result = wc & A_REVERSE;	/* determina si es invers o no */
  }
  return(result);
}


void win_escristr(char *str)
{
  if (hi_ha_win)
  { mvwaddstr(win,ufil-1,0,str);    /* escriu string en la ultima linia */
    wclrtoeol(win);                 /* esborra fins al final de la linia */
    wrefresh(win);
    refresh();
  }
}


int win_gettec(void)
{
  int result = 0;
  
  if (hi_ha_win)
  { result = wgetch(win);
    if (result == ERR) result = 0;
  }
  return(result);
}


int win_retard(int ms)
{
  napms(ms);
  return(0);
}


int win_carregatauler(char *nom_fitxer, int n_f, int n_c, char creq)
{
  int i,j, error;
  chtype wc;
  FILE *fit_ent;

  error = 0;
  if (hi_ha_win)        /* si ja s'ha creat la finestra */
  {
    if (n_f > (ufil-1)) return(-4);	/* numero de files massa gran */
    if (n_c > ucol) return(-3);		/* numero de columnes massa gran */

    fit_ent = fopen(nom_fitxer,"rt");    /* obrir fitxer text per lectura */
    if (fit_ent)                         /* si ha pogut obrir el fitxer */
    {
      i = 0;
      while (!feof(fit_ent) && (i < n_f) && (error == 0))
      { j = 0;
        do
        { wc = fgetc(fit_ent);          /* captar un caracter */
          if (wc == creq)               /* si es igual al caracter paret */
             wc |= A_REVERSE;           /* imprimir-lo invertit */
          mvwaddch(win,i,j,wc);
          j++;                          /* repetir mentre hi hagi linia */
        } while ((j < n_c) && (wc != EOL) && (wc != EOF));
        if (j == n_c)
        { wc = fgetc(fit_ent);          /* capturar caracter fi de linia */
          if (wc != EOL) error = -2;	/* error en format de linia */
        }
        else error = -2;		/* error en format de taulell */
        i++;
      }
      fclose(fit_ent);
      if (error == 0)                   /* si tot correcte */
      { wrefresh(win);                  /* actualitza dibuix tauler */
        refresh();
      }
    }
    else error = -1;            /* error en nom del fitxer */
  }
  else error = -5;              /* error finestra no oberta */
  return(error);                /* retorna 0 si no hi ha error */
}


