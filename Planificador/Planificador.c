/*
 ============================================================================
 Name        : Planificador.c
 Author      : Operactivos - Martin Mendez
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */
#include "Planificador.h"

void ejecutarConsola();
int main(void) {

	int socketCoord;
	struct sockaddr_in coordAddr;//informacion del coordinador


	coordAddr.sin_family = AF_INET;         // Ordenación de bytes de la máquina
	coordAddr.sin_port = htons(PORT_COORD);
	coordAddr.sin_addr.s_addr = INADDR_ANY; // Rellenar con mi dirección IP
	memset(&(coordAddr.sin_zero), '\0', 8); // Poner a cero el resto de la estructura


	connect(socketCoord,(struct sockaddr*)&coordAddr,sizeof(struct sockaddr));
	if(socketCoord<=0)
	 puts("error conexion al coordinador");
	//hasta ahora hice lo basico para comunicarme con el coordinador
	//empieza la parte de escuchar al ESI

	int sock_esi, new_fd;
	struct sockaddr_in my_addr;    // Información sobre mi dirección
	struct sockaddr_in esi_addr; // Información sobre la dirección remota
	int sin_size;
	sock_esi = socket(AF_INET, SOCK_STREAM, 0);
	if(sock_esi<=1)
		puts("error de conexion esi");
	my_addr.sin_family = AF_INET;         // Ordenación de máquina
	my_addr.sin_port = htons(PORT);     // short, Ordenación de la red
	my_addr.sin_addr.s_addr = INADDR_ANY; // Rellenar con mi dirección IP
	memset(&(my_addr.sin_zero), '\0', 8); // Poner a cero el resto de la estructura

	bind(sock_esi, (struct sockaddr *)&my_addr, sizeof(struct sockaddr));
	listen(sock_esi, BACKLOG);
	sin_size = sizeof(struct sockaddr_in);
	new_fd = accept(sock_esi, (struct sockaddr *)&esi_addr, &sin_size);

	ejecutarConsola();

	return EXIT_SUCCESS;
}
void ejecutarConsola()
{
/*
 * La consola debe ir haciendo readlines de pantalla
 * e ir haciendo un caso dependiendo lo que se lea,
 * usando manejo de strings.
 * */
	puts("hola mundo");
}
