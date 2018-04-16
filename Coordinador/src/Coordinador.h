/*
 * Coordinador.h
 *
 *  Created on: 4 abr. 2018
 *      Author: utnso
 */

#define MYPORT 3490    // Puerto al que conectarán los usuarios

#define BACKLOG 10     // Cuántas conexiones pendientes se mantienen en cola

struct parametrosConexion{
	int sockfd;
	int new_fd;
};

void sigchld_handler(int s);
int main(void);
void *conexion(struct parametrosConexion *param);
