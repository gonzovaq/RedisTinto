	#include "Planificador.h"


    int ejecutarConsola();
    int main(int argc, char *argv[])
    {
    	// Ejemplo para leer archivo de configuracion en formato clave=valor por linea
    	char *token;
    	char *search = "=";
    	 static const char filename[] = "/home/utnso/workspace2/tp-2018-1c-Sistemas-Operactivos/Instancia/configuracion.config";
    	FILE *file = fopen ( filename, "r" );
    	if ( file != NULL )
    	{
    		puts("Leyendo archivo de configuracion");
    	  char line [ 128 ]; /* or other suitable maximum line size */
    	  while ( fgets ( line, sizeof line, file ) != NULL ) /* read a line */
    	  {
    	    // Token will point to the part before the =.
    	    token = strtok(line, search);
    	    puts(token);
    	    // Token will point to the part after the =.
    	    token = strtok(NULL, search);
    	    puts(token);
    	  }
    	  fclose ( file );
    	}
    	else
    		puts("Archivo de configuracion vacio");


        int socketCoordinador, numbytes;
        char buf[MAXDATASIZE];
        struct hostent *he;
        struct sockaddr_in their_addr; // información de la dirección de destino

        if (argc != 2) {
            fprintf(stderr,"usage: client hostname\n");
            exit(1);
        }

        if ((he=gethostbyname(argv[1])) == NULL) {  // obtener información de máquina
            perror("gethostbyname");
            exit(1);
        }

        if ((socketCoordinador = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            perror("socket");
            exit(1);
        }

        their_addr.sin_family = AF_INET;    // Ordenación de bytes de la máquina
        their_addr.sin_port = htons(PORT);  // short, Ordenación de bytes de la red
        their_addr.sin_addr = *((struct in_addr *)he->h_addr);
        memset(&(their_addr.sin_zero),'\0', 8);  // poner a cero el resto de la estructura

        if (connect(socketCoordinador, (struct sockaddr *)&their_addr,
                                              sizeof(struct sockaddr)) == -1) {
            perror("connect");
            exit(1);
        }

//        //Me identifico con el coordinador
//        char *mensaje = "i";
//        int longitud_mensaje = strlen(mensaje);
//        if (send(socketCoordinador, mensaje, longitud_mensaje, 0) == -1) {
//        	puts("Error al enviar el mensaje.");
//        	perror("send");
//            exit(1);
//        }

    	int pid = getpid();
    	printf("Mi ID es %d \n",pid);

        tHeader *header = malloc(sizeof(tHeader));
        header->tipoProceso = INSTANCIA;
        header->tipoMensaje = CONECTARSE;
        header->idProceso = pid;
            if (send(socketCoordinador, header, sizeof(tHeader), 0) == -1){
           	   puts("Error al enviar mi identificador");
           	   perror("Send");
           	   exit(1);
           	   free(header);
              }


        if ((numbytes=recv(socketCoordinador, buf, MAXDATASIZE-1, 0)) == -1) {
            perror("recv");
            exit(1);
        }

        buf[numbytes] = '\0';

        printf("Received: %s",buf);

        free(header);
        close(socketCoordinador);

        int a=ejecutarConsola();
        return 0;
    }

    int ejecutarConsola()
{
  while(1)
	{
		//char * readline(const char * linea);
		char * linea = readline(": ");


		if(strncmp(linea,"pausar",7)==0)
		{
			puts("va a pausar");
		}

		if(strncmp(linea,"continuar",9)==0)
		{
			puts("va a continuar");
			puts(linea);
		}
		if(strncmp(linea,"bloquear",8)==0)
		{

			char* clave1;
			int id;
			int flag=0;
			int j=0;
			int i=9;
			while(linea[i]!=' ')
			{
				j++;
				i++;
			}
			i=9;
			clave1=malloc(sizeof(char[j]));
			j=0;
			while(linea[i]!=' ')
						{	//printf("%d \n",i);
							clave1[j]=linea[i];
							j++;
							i++;
							if(linea[i]=='\0')
								{
									puts("Falta que ingrese el id");
									flag=1;
									break;
								}
						}
			
			clave1[j]='\0'; //Agrega un /0 al final de la clave y descarta toda la basura
			printf("Clave: %s \n",clave1);
			j=0;
			i++;
			if(flag==0)
			{
				id=obtenerId(i,linea);
			}			
			printf("id: %d \n",id);
			free(clave1);
		}
		if(strncmp(linea,"desbloquear",11)==0)
		{
			//obtener id
			int id=obtenerId(11,linea);
			printf("id: %d \n",id);
		}
		if(strncmp(linea,"kill",4)==0)
		{
			//obtener id
			int id=obtenerId(4,linea);
			printf("id: %d \n",id);
		}
		if(strncmp(linea,"status",6)==0)
		{
			int id=obtenerId(6,linea);
			printf("id: %d \n",id);
		}
		if(strncmp(linea,"deadlocks",9)==0)
		{
			puts("Mostrar bloqueos mutuos");
		}

	}

return 0;
}
int obtenerId(int size, char * linea)
{
	int id;
	char * cid = malloc(sizeof(int));
	int j=0;
	int i=size;
	while(linea[i]!='\0')
	{
		cid[j]=linea[i];
		j++;i++;
	}
	id=atoi(cid);
	return id;
}  

