	#include "ESI.h"

    int main(int argc, char *argv[])
    {
        int socket_coordinador, socket_planificador;
        FILE * archivoConScripts;

        //Before
    	verificarParametrosAlEjecutar(argc, argv);
    	leerConfiguracion();
    	archivoConScripts = leerArchivo(argv);


    	socket_coordinador = conectarmeYPresentarme(PORT_COORDINADOR);
    	socket_planificador = conectarmeYPresentarme(PORT_PLANIFICADOR);


    	manejarArchivoConScripts(archivoConScripts,socket_coordinador,socket_planificador);

        close(socket_coordinador);
        close(socket_planificador);
        return 0;
    }


    int leerConfiguracion(){
    	char *token;
    	char *search = "=";
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
    	else {
    		puts("Archivo de configuracion vacio");
    	}

    	return EXIT_SUCCESS;
    }

    int verificarParametrosAlEjecutar(int argc, char *argv[]){

        if (argc != 3) {//argc es la cantidad de parametros que recibe el main.
        	puts("Error al ejecutar, para correr este proceso deberias ejecutar: ./ESI ubuntu-server \"nombreArchivo\"");
            exit(1);
        }


        if ((he=gethostbyname(argv[1])) == NULL) {  // obtener información de máquina
        	puts("Error al obtener el hostname, te faltan parametros.");
        	perror("gethostbyname");
            exit(1);
        }
        return EXIT_SUCCESS;
    }

    int conectarmeYPresentarme(int port){
        int sockfd;
        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        	puts("Error al crear el socket");
        	perror("socket");
            exit(1);
        }
        puts("El socket se creo correctamente\n");

        their_addr.sin_family = AF_INET;    // Ordenación de bytes de la máquina
        their_addr.sin_port = htons(port);  // short, Ordenación de bytes de la red
        their_addr.sin_addr = *((struct in_addr *)he->h_addr);
        memset(&(their_addr.sin_zero),'\0', 8);  // poner a cero el resto de la estructura

        if (connect(sockfd, (struct sockaddr *)&their_addr,
                                              sizeof(struct sockaddr)) == -1) {
        	puts("Error al conectarme al servidor.");
        	perror("connect");
            exit(1);
        }
        puts("ESI conectado!\n");

        enviarHeader(sockfd); //Me presento

        return sockfd;

    }

    int enviarHeader(int sockfd){
    	int pid = getpid(); //Los procesos podrian pasarle sus PID al coordinador para que los tenga identificados
    	printf("Mi ID es %d \n",pid);
        tHeader *header = malloc(sizeof(tHeader));
               header->tipoProceso = ESI;
               header->tipoMensaje = CONECTARSE;
               header->idProceso = pid;
               strcpy(header->nombreProceso, "ESI DE PRUEBA" ); // El nombre se da en el archivo de configuracion --> MINOMBRE
			   if (send(sockfd, header, sizeof(tHeader), 0) == -1){
				   puts("Error al enviar mi identificador");
				   perror("Send");
				   free(header);
				   exit(1);
			   }
			   puts("Se envió mi identificador");
			   free(header);
			   return EXIT_SUCCESS;
    }

    int recibirMensaje(int sockfd){
    	int numbytes;
        int buf;

        if ((numbytes=recv(sockfd, buf, sizeof(int), 0)) == -1) {
            perror("recv");
            exit(1);
        }
        return buf;
    }

    int recibirResultadoDelCoordinador(int sockfd, tResultado * resultado){
    	int numbytes;


        if ((numbytes=recv(sockfd, resultado, sizeof(tResultado), 0)) == -1) {
            perror("recv");
            exit(1);
        }
        puts("Recibi el resultado");
        return EXIT_SUCCESS;
    }

    int enviarOperaciontHeader(int sockfd, OperaciontHeader* header){

    	if ((send(sockfd, header, sizeof(OperaciontHeader), 0)) == -1) {
        	puts("Error al enviar el mensaje.");
        	perror("send");
            exit(1);
        }

        puts("El header de la operacion se envio correctamente");
        return EXIT_SUCCESS;
    }

    int enviarMensaje(int sockfd, char* mensaje){

    	printf("Se envió el mensaje: %s \n",mensaje);

    	if ((send(sockfd, mensaje, TAMANIO_CLAVE, 0)) == -1) {
        	puts("Error al enviar el mensaje.");
        	perror("send");
            exit(1);
        }

        printf("El mensaje: \"%s\", se ha enviado correctamente! \n\n",mensaje);
        return EXIT_SUCCESS;
    }

    FILE *leerArchivo(char **argv){

    	//Esto lo copié literal del ejemplo del ParSI. A medida que avancemos, esto en vez de imprimir en pantalla,
    	//va a tener que enviarle los scripts al coordinador (siempre y cuando el planificador me lo indique)

    	FILE * file;
        file = fopen(argv[2], "r");
        if (file == NULL){
            perror("Error al abrir el archivo: ");
            exit(EXIT_FAILURE);
        }


        return file;
    }

    int manejarArchivoConScripts(FILE * file, int socket_coordinador, int socket_planificador){
        char * line = NULL;
        size_t len = 0;
        ssize_t read;

        OperacionAEnviar * operacion;
        OperaciontHeader * header;
        tValidezOperacion * validez = malloc(sizeof(tValidezOperacion));
        int tamanioValor;

        while(getline(&line, &len, file) != -1){ //aca habia un read = getline
            while (!recibirOrdenDeEjecucion(socket_planificador));
                t_esi_operacion parsed = parse(line);

                if(parsed.valido){
                    switch(parsed.keyword){
                        case GET:
                        	validez=OPERACION_VALIDA;
                        	enviarValidez(socket_coordinador,&validez);
							puts("Manejo operacion GET");
							manejarOperacionGET(socket_coordinador, parsed.argumentos.GET.clave, &operacion, &header);
							puts("Se finalizo el manejo de la operacion GET");
                        	break;
                        case SET:
                        	validez=OPERACION_VALIDA;
                        	enviarValidez(socket_coordinador,&validez);
                        	puts("Manejo operacion SET");
							manejarOperacionSET(socket_coordinador, parsed.argumentos.SET.clave, parsed.argumentos.SET.valor, &operacion, &header);
							puts("Se finalizo el manejo de la operacion SET");
                        	break;
                        case STORE:
                        	validez=OPERACION_VALIDA;
                        	enviarValidez(socket_coordinador,&validez);
                        	puts("Manejo operacion STORE");
							manejarOperacionSTORE(socket_coordinador, parsed.argumentos.STORE.clave, &operacion, &header);
							puts("Se finalizo el manejo de la operacion STORE");
                        	break;
                        default:
                        	validez = OPERACION_INVALIDA;
                            fprintf(stderr, "No pude interpretar <%s>\n", line);
                            enviarValidez(socket_coordinador, &validez);
                            exit(EXIT_FAILURE);
                    }
            		tResultado * resultado = malloc(sizeof(tResultado));
                    recibirResultado(socket_coordinador, resultado);
					enviarResultado(socket_planificador,resultado);
                    free(resultado);
                    destruir_operacion(parsed);
                } else {
                	validez = OPERACION_INVALIDA;
                    fprintf(stderr, "La linea <%s> no es valida\n", line);
                    enviarValidez(socket_coordinador, &validez);
                    exit(EXIT_FAILURE);
                }

        }
        fclose(file);
        if (line){
            free(line);
        }
        free(validez);
        return EXIT_SUCCESS;

    }

   int recibirOrdenDeEjecucion(int socket_planificador){
    	puts("llega al recibir ejecucion");
    	char *buffer = malloc(strlen("EJECUTATE"));
    	int tengoPermiso = 0;
    	if(recv(socket_planificador,buffer,strlen("EJECUTATE"),0) <= 0){
    		perror("error al recibir orden de ejecucion");
    		exit(EXIT_FAILURE);
    	}
    	tengoPermiso = strcmp("EJECUTATE",buffer);
    	free(buffer);
    	return tengoPermiso;
    }

    int recibirResultado(int socket_coordinador, tResultado * resultado){
    	puts("Vor a recibir el resultado");
    	recibirResultadoDelCoordinador(socket_coordinador,resultado);

    	switch(resultado->tipoResultado){
    		case OK:
    			puts("La operación salio OK"); //EN ESTOS CASE DEBERIA LOGGEAR O ALGO ASI, PREGUNTAR MAS ADELANTE
    			break;
    		case BLOQUEO:
				puts("La operación se BLOQUEO");
				break;
    		case ERROR:
				puts("La operación tiro ERROR");
				break;
    		default:
    			puts("ERROR AL RECIBIR EL RESULTADO");
    			EXIT_FAILURE;
    			break;
    	}
    	return EXIT_SUCCESS;
    }

	int manejarOperacionGET(int socket_coordinador, char clave[TAMANIO_CLAVE], OperacionAEnviar* operacion, OperaciontHeader * header) {

		header->tipo = OPERACION_GET;
		header->tamanioValor = 0;
		enviarOperaciontHeader(socket_coordinador, header);
		puts("Header de la Operacion GET enviado correctamente");

		clave[TAMANIO_CLAVE]='\0';
		printf("Voy a enviar la clave: %s \n",clave);
		enviarMensaje(socket_coordinador, clave);

		printf("Operacion GET con la clave: %s, enviada correctamente\n" ,clave);

		return EXIT_SUCCESS;
	}

	int manejarOperacionSET(int socket_coordinador, char clave[TAMANIO_CLAVE], char *valor, OperacionAEnviar* operacion, OperaciontHeader *header) {

		header->tipo = OPERACION_SET;
		header->tamanioValor = strlen(valor);

		enviarOperaciontHeader(socket_coordinador, header);
		puts("Header de la Operacion SET enviado correctamente");

		enviarMensaje(socket_coordinador, clave);
		enviarValor(socket_coordinador, valor);

		printf("Operacion SET con clave: %s, y valor {1}, enviada correctamente" ,clave,valor);
		return EXIT_SUCCESS;
	}

	int manejarOperacionSTORE(int socket_coordinador, char clave[TAMANIO_CLAVE], OperacionAEnviar* operacion, OperaciontHeader *header) {

		header->tipo = OPERACION_STORE;
		header->tamanioValor = 0;
		enviarOperaciontHeader(socket_coordinador, header);
		puts("Header de la Operacion STORE enviado correctamente");

		enviarMensaje(socket_coordinador, clave);

		printf("Operacion STORE con la clave: %s, enviada correctamente" ,clave);

		return EXIT_SUCCESS;

	}

    int enviarValor(int sockfd, char* valor){

    	printf("Se envió el valor: %s \n",valor);
    	valor[strlen(valor) + 1] = '\0';

    	if ((send(sockfd, valor, strlen(valor), 0)) <= 0) {
        	puts("Error al enviar el valor.");
        	perror("send");
            exit(1);
        }

        printf("El mensaje: \"%s\", se ha enviado correctamente! \n\n",valor);
        return EXIT_SUCCESS;
    }

    int enviarValidez(int sockfd, tValidezOperacion *validez){
    	if ((send(sockfd, validez, sizeof(tValidezOperacion), 0)) <= 0) {
        	puts("Error al enviar la validez.");
        	perror("send");
            exit(1);
        }
    	puts("Se envió la validez");
        return EXIT_SUCCESS;

    }
	int enviarResultado(int socket,tResultado * resul)
	{
		if ((send(socket, resul, sizeof(tResultado), 0)) <= 0) {
        	puts("Error al enviar resultado al planificador");
        	perror("send");
            exit(1);
        }
		printf("Resultado es: %d \n",resul->tipoResultado);//Envio bien el resultado
		printf("Resultado clave: %s \n",resul->clave);
    	puts("Se envió el resultado al planificador");
        return EXIT_SUCCESS;
	}
