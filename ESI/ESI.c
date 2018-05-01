	#include "ESI.h"

    int main(int argc, char *argv[])
    {
        int socket_coordinador, socket_planificador;
        char *mensaje_coordinador = "Hola Coordinador, como va?";
        char *mensaje_planificador = "Que onda Planificador?";
        char *buffer_mensaje_recibido;

        //Before
    	verificarParametrosAlEjecutar(argc, argv);
    	leerConfiguracion();
    	leerArchivoEImprimirloEnConsola(argv);

    	//Comunicación con el Coordinador
    	socket_coordinador = conectarSocket(PORT_COORDINADOR);
    	buffer_mensaje_recibido = recibirMensaje(socket_coordinador);
        printf("Received: %s \n",buffer_mensaje_recibido);
    	enviarMensaje(socket_coordinador, mensaje_coordinador);

    	//Comunicación con el Planificador
    	socket_planificador = conectarSocket(PORT_PLANIFICADOR);
    	enviarMensaje(socket_planificador, mensaje_planificador);

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

    int conectarSocket(int port){
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

        enviarHeader(sockfd);

        return sockfd;

    }

    int enviarHeader(int sockfd){
    	int pid = getpid(); //Los procesos podrian pasarle sus PID al coordinador para que los tenga identificados
    	printf("Mi ID es %d \n",pid);
        tHeader *header = malloc(sizeof(tHeader));
               header->tipoProceso = ESI;
               header->tipoMensaje = CONECTARSE;
               header->idProceso = pid;
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

    char* recibirMensaje(int sockfd){
    	int numbytes;
        int tamanio_buffer=100;
        char* buf = malloc(tamanio_buffer); //Seteo el maximo del buffer en 100 para probar. Debe ser variable.

        if ((numbytes=recv(sockfd, buf, tamanio_buffer-1, 0)) == -1) {
            perror("recv");
            exit(1);
        }

        buf[numbytes] = '\0';
        return buf;
    }

    int enviarMensaje(int sockfd, char* mensaje){
    	int longitud_mensaje, bytes_enviados;

        longitud_mensaje = strlen(mensaje);
        if ((bytes_enviados=send(sockfd, mensaje, longitud_mensaje , 0)) == -1) {
        	puts("Error al enviar el mensaje.");
        	perror("send");
            exit(1);
        }

        printf("El mensaje: \"%s\", se ha enviado correctamente! \n\n",mensaje);
        return EXIT_SUCCESS;
    }

    int leerArchivoEImprimirloEnConsola(char **argv){

    	//Esto lo copié literal del ejemplo del ParSI. A medida que avancemos, esto en vez de imprimir en pantalla,
    	//va a tener que enviarle los scripts al coordinador (siempre y cuando el planificador me lo indique)

    	FILE * file;
        char * line = NULL;
        size_t len = 0;
        ssize_t read;

        file = fopen(argv[2], "r");
        if (file == NULL){
            perror("Error al abrir el archivo: ");
            exit(EXIT_FAILURE);
        }

        while ((read = getline(&line, &len, file)) != -1) {
            t_esi_operacion parsed = parse(line);

            if(parsed.valido){
                switch(parsed.keyword){
                    case GET:
                        printf("GET\tclave: <%s>\n", parsed.argumentos.GET.clave);
                        break;
                    case SET:
                        printf("SET\tclave: <%s>\tvalor: <%s>\n", parsed.argumentos.SET.clave, parsed.argumentos.SET.valor);
                        break;
                    case STORE:
                        printf("STORE\tclave: <%s>\n", parsed.argumentos.STORE.clave);
                        break;
                    default:
                        fprintf(stderr, "No pude interpretar <%s>\n", line);
                        exit(EXIT_FAILURE);
                }

                destruir_operacion(parsed);
            } else {
                fprintf(stderr, "La linea <%s> no es valida\n", line);
                exit(EXIT_FAILURE);
            }
        }

        fclose(file);
        if (line){
            free(line);
        }

        return EXIT_SUCCESS;
    }
