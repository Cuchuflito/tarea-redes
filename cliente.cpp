#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 7777

int main(int argc, char const *argv[]) {
    if (argc != 3) {
        std::cerr << "Uso: " << argv[0] << " <IP> <PORT>" << std::endl;
        return -1; //revisar la ip y puerto especificado para jugar
    }

    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};

//crear socket cliente
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "error creacion socket" << std::endl;
        return -1;
    }

//configurar direccion server
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[2]));
//se convierte la ip a binario para almacenarse en serv_addr.sin_addr
    if (inet_pton(AF_INET, argv[1], &serv_addr.sin_addr) <= 0) {
        std::cerr << "Direccion invalida" << std::endl;
        return -1;
    }
//se crea la conexion
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Conexion fallida" << std::endl;
        return -1;
    }

    while (true) { //se limpia el buffer y se lee el tablero del server
        memset(buffer, 0, 1024);
        int valread = read(sock, buffer, 1024);
        if (valread <= 0) {
            std::cerr << "Desconectado del server" << std::endl;
            break;
        }

        std::cout << buffer << std::endl;

        if (strstr(buffer, "Ganaste!") || strstr(buffer, "Perdiste!")) {
            break;
        }

        std::cout << "Ingresa un numero (1-7): ";
        int col;
        std::cin >> col;

        snprintf(buffer, sizeof(buffer), "%d", col);
        send(sock, buffer, strlen(buffer), 0);
    }

    close(sock);
    return 0;
}
