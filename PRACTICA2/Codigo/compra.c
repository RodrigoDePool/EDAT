#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sql.h>
#include <sqlext.h>
#include <time.h>
#include "odbc.h"
#define TRUE 1
#define FALSE 0



/*FUNCIONES AUXILIARES UTILIZADAS*/
 /* reverse:  reverse string s in place */
 void reverse(char s[])
 {
     int i, j;
     char c;

     for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
         c = s[i];
         s[i] = s[j];
         s[j] = c;
     }
 }


 /* itoa:  convert n to characters in s */
 void itoa(int n, char *s)
 {
     int i, sign;

     if ((sign = n) < 0)  /* record sign */
         n = -n;          /* make n positive */
     i = 0;
     do {       /* generate digits in reverse order */
         s[i++] = n % 10 + '0';   /* get next digit */
     } while ((n /= 10) > 0);     /* delete it */
     if (sign < 0)
         s[i++] = '-';
     s[i] = '\0';
     reverse(s);
 }

/*
  Funcion que devuelve un string con el siguiente formato 'AAAA-MM-DD'
  donde AAAA es el anio actual, MM el mes actual y DD el dia actual
  Returns:
    La string que se devuelve DEBE SER LIBERADA FUERA DE ESTA FUNCION
*/
char *fecha(){
  char *fecha;
  int anio,mes,dia;
  time_t t=time(NULL);
	struct tm tm=*localtime(&t);

  fecha=(char *)malloc(sizeof(char)*11);
  anio=tm.tm_year+1900;
  mes=tm.tm_mon+1;
  dia=tm.tm_mday;

  itoa(anio,fecha,10);
  fecha[4]='-';
  itoa(mes,fecha+5,10);
  fecha[7]='-';
  itoa(dia,fecha+8,10);

  return fecha;
}
/*FIN DE FUNCIONES AUXILIARES*/





int main(int argc, char **argv){
  SQLHENV env;
  SQLHDBC dbc;
  SQLRETURN ret; /* ODBC API return status */
  SQLCHAR outstr[1024];
  SQLSMALLINT outstrlen;
  SQLHSTMT stmt;
  int descuento,precio,total,boolean,aux;
  char buff[1000],scrname[100],titulo[100],*actual;

  /*Revisamos cosas basicas de los argumentos*/
  if(argc<3){
    printf("ERROR, argumentos insuficientes.\n");
    return 0;
  }
  strcpy(scrname,argv[1]);


  /*CONNECTING TO DATABASE*/
  /* Allocate an environment handle */
  SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
  /* We want ODBC 3 support */
  SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, (void *) SQL_OV_ODBC3, 0);
  /* Allocate a connection handle */
  SQLAllocHandle(SQL_HANDLE_DBC, env, &dbc);
  /* Connect to the DSN mydsn */
  ret = SQLDriverConnect(dbc, NULL, (SQLCHAR*) "DRIVER=PostgreSQL ANSI;DATABASE=PRACT_2;SERVER=localhost;PORT=5432;UID=alumnodb;PWD=alumnodb;", SQL_NTS,
                         outstr, sizeof(outstr), &outstrlen,
                         SQL_DRIVER_NOPROMPT);

  /*Probamos que se haya conectado con exito*/
  if(SQL_SUCCEEDED(ret)){
    printf("Conectado a la base de datos\n\n");
  }
  else{
    fprintf(stderr, "Error al conectarse a la base de datos\n");
    odbc_extract_error("SQLDriverConnect", dbc, SQL_HANDLE_DBC);
    /* disconnect from database and free up allocated handles */
    SQLDisconnect(dbc);
    SQLFreeHandle(SQL_HANDLE_DBC, dbc);
    SQLFreeHandle(SQL_HANDLE_ENV, env);
    return 0;
  }



  /*Revisemos que el screen_name esta en la tabla*/
  /*Creamos un string con la consulta*/
  strcpy(buff,"select \"Screen_name\" from public.\"Fidelizado\" where \"Screen_name\"=?");
  SQLPrepare(stmt,(SQLCHAR*)buff,SQL_NTS);
  SQLBindParameter(stmt,1,SQL_PARAM_INPUT,SQL_C_CHAR,SQL_CHAR,0,0,scrname,0,NULL);

  /*Realizamos la consulta y la guardamos en stmt*/
  ret=SQLExecute(stmt);
  if(!SQL_SUCCEEDED(ret)){
    printf("Error en la ejecucion de la consulta\n");
    SQLDisconnect(dbc);
    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    SQLFreeHandle(SQL_HANDLE_DBC, dbc);
    SQLFreeHandle(SQL_HANDLE_ENV, env);
    return 0;
  }
  /*Revisamos si la tabla tiene al menos una fila*/
  ret = SQLFetch(stmt);
  /*liberamos la tabla para utilizar despues*/
  SQLFreeHandle(SQL_HANDLE_STMT, stmt);
  /*si no tiene una fila*/
  if(!SQL_SUCCEEDED(ret)){
    printf("Error, el screen name no esta en la base de datos.\n");
    SQLDisconnect(dbc);
    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    SQLFreeHandle(SQL_HANDLE_DBC, dbc);
    SQLFreeHandle(SQL_HANDLE_ENV, env);
    return 0;
  }




  /*Revisemos que todos los isbns estan en nuestra base de datos*/
  strcpy(buff,"select \"ISBN\" from public.\"Edicion\" where \"ISBN\"=?");
  boolean=TRUE;
  for(i=2;i<argc && boolean==TRUE;i++){
    /*asignamos el isbn a una variable auxiliar*/
    aux=atoi(argv[i]);

    /*Guardamos memoria para guardar la tabla en stmt*/
    ret=SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);
    if(!SQL_SUCCEEDED(ret)){
      printf("Error allocating statement\n");
      SQLDisconnect(dbc);
      SQLFreeHandle(SQL_HANDLE_DBC, dbc);
      SQLFreeHandle(SQL_HANDLE_ENV, env);
      return 0;
    }

    /*preparamos la consulta*/
    SQLPrepare(stmt,(SQLCHAR*)buff,SQL_NTS);
    SQLBindParameter(stmt,1,SQL_PARAM_INPUT,SQL_C_SLONG,SQL_INTEGER,0,0,&aux,0,NULL);
    SQLExecute(stmt);

    /*hacemos un fetch*/
    ret=SQLFetch(stmt);
    /*si el fetch falla, el isbn no esta en la tabla*/
    if(!SQL_SUCCEEDED(ret)){
      boolean=FALSE;
    }
    /*liberamos la tabla para utilizar despues*/
    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
  }

  /*si algun isbn no esta en la tabla, sal del programa*/
  if(boolean==FALSE){
    /*Desconectamos y liberamos*/
    SQLDisconnect(dbc);
    SQLFreeHandle(SQL_HANDLE_DBC, dbc);
    SQLFreeHandle(SQL_HANDLE_ENV, env);
    printf("ERROR, algun isbn no esta en nuestra base de datos.\n");
    return 0;
  }



  /*Calculemos el precio total y vayamos imprimiendo uno a uno los precios de cada libro*/
  for(i=2;i<argc;i++){
      /*asignamos el isbn a una variable auxiliar*/
      aux=atoi(argv[i]);

      /*Busquemos primero el TITULO del libro*/
      /*Guardamos memoria para guardar la tabla en stmt*/
      ret=SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);
      if(!SQL_SUCCEEDED(ret)){
        printf("Error allocating statement\n");
        SQLDisconnect(dbc);
        SQLFreeHandle(SQL_HANDLE_DBC, dbc);
        SQLFreeHandle(SQL_HANDLE_ENV, env);
        return 0;
      }

      strcpy(buff,"select \"Titulo\" from public.\"Edicion\" where \"ISBN\"=?");
      /*preparamos la consulta*/
      SQLPrepare(stmt,(SQLCHAR*)buff,SQL_NTS);
      SQLBindParameter(stmt,1,SQL_PARAM_INPUT,SQL_C_SLONG,SQL_INTEGER,0,0,&aux,0,NULL);
      SQLBindCol(stmt,1,SQL_C_CHAR,titulo,sizeof(titulo),NULL);
      /*Hacemos el fetch para tener el resultado de la consulta en titulo*/
      SQLFetch(stmt);
      /*liberamos la tabla para utilizar despues*/
      SQLFreeHandle(SQL_HANDLE_STMT, stmt);

      /*Saquemos ahora el PRECIO del isbn*/
      ret=SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);
      if(!SQL_SUCCEEDED(ret)){
        printf("Error allocating statement\n");
        SQLDisconnect(dbc);
        SQLFreeHandle(SQL_HANDLE_DBC, dbc);
        SQLFreeHandle(SQL_HANDLE_ENV, env);
        return 0;
      }

      strcpy(buff,"select \"Precio\" from public.\"Edicion\" where \"ISBN\"=?");
      /*preparamos la consulta*/
      SQLPrepare(stmt,(SQLCHAR*)buff,SQL_NTS);
      SQLBindParameter(stmt,1,SQL_PARAM_INPUT,SQL_C_SLONG,SQL_INTEGER,0,0,&aux,0,NULL);
      SQLBindCol(stmt,1,SQL_C_SLONG,precio,sizeof(precio),NULL);
      /*Hacemos el fetch para tener el resultado de la consulta en precio*/
      SQLFetch(stmt);
      /*liberamos la tabla para utilizar despues*/
      SQLFreeHandle(SQL_HANDLE_STMT, stmt);


      /*Revisemos si hay alguna OFERTA en la fecha actual*/
      /*si hay fecha se guardara en descuento y si no hay descuento sera -1*/
      ret=SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);
      if(!SQL_SUCCEEDED(ret)){
        printf("Error allocating statement\n");
        SQLDisconnect(dbc);
        SQLFreeHandle(SQL_HANDLE_DBC, dbc);
        SQLFreeHandle(SQL_HANDLE_ENV, env);
        return 0;
      }
      actual=fecha();


  }


  free(actual);
}