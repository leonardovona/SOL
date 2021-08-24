#ifndef UTIL_H_
#define UTIL_H_

#define ec_meno1(s,m) \
	if( (s) == -1 ) {perror(m); exit(EXIT_FAILURE);}

/*controlla NULL; stampa errore e termina */
#define ec_null(s,m) \
	if( (s) == NULL ) {perror(m); exit(EXIT_FAILURE);}

/*controlla -1; stampa errore ed esegue c*/
#define ec_meno1_c(s, m, c) \
	if( (s) == -1 ) {perror(m); c;}

/*controlla se != 0; stampa errore e termina*/
#define ec_non0(s, m) \
	if( (s) != 0 ) {perror(m); exit(EXIT_FAILURE);}
		
#endif /* UTIL_H_ */
