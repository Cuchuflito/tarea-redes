#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <thread>
#include <vector>
#include <mutex>

#define PORT 7777
#define ROWS 6
#define COLS 7

std::mutex mtx; // Para proteger el acceso concurrente al tablero

struct Game {
    char board[ROWS][COLS];
    bool player_turn; // true si es el turno del cliente, false si es el turno del servidor
    bool game_over;
    char winner;
};

void init_board(Game &game) {
    // Inicializar el tablero vacío
    for (int i = 0; i < ROWS; ++i) {
        for (int j = 0; j < COLS; ++j) {
            game.board[i][j] = ' ';
        }
    }
    game.player_turn = rand() % 2 == 0; // Decidir aleatoriamente quién empieza
    game.game_over = false;
    game.winner = ' ';
}

std::string get_board_str(const Game &game) {
    std::string board_str = "";
    for (int j = 1; j <= COLS; ++j) {
        board_str += std::to_string(j) + " ";
    }
    board_str += "\n";
    for (int i = 0; i < ROWS; ++i) {
        for (int j = 0; j < COLS; ++j) {
            board_str += game.board[i][j];
            board_str += " ";
        }
        board_str += "\n";
    }
    return board_str;
}

void print_board(const Game &game) {
    std::cout << get_board_str(game);
}

bool check_winner(Game &game, char player) {
    // Verificar filas, columnas y diagonales para detectar un ganador
    for (int i = 0; i < ROWS; ++i) {
        for (int j = 0; j < COLS - 3; ++j) {
            if (game.board[i][j] == player && game.board[i][j+1] == player && game.board[i][j+2] == player && game.board[i][j+3] == player) {
                return true;
            }
        }
    }
    for (int i = 0; i < ROWS - 3; ++i) {
        for (int j = 0; j < COLS; ++j) {
            if (game.board[i][j] == player && game.board[i+1][j] == player && game.board[i+2][j] == player && game.board[i+3][j] == player) {
                return true;
            }
        }
    }
    for (int i = 0; i < ROWS - 3; ++i) {
        for (int j = 0; j < COLS - 3; ++j) {
            if (game.board[i][j] == player && game.board[i+1][j+1] == player && game.board[i+2][j+2] == player && game.board[i+3][j+3] == player) {
                return true;
            }
        }
    }
    for (int i = 3; i < ROWS; ++i) {
        for (int j = 0; j < COLS - 3; ++j) {
            if (game.board[i][j] == player && game.board[i-1][j+1] == player && game.board[i-2][j+2] == player && game.board[i-3][j+3] == player) {
                return true;
            }
        }
    }
    return false;
}

bool make_move(Game &game, int col, char player) {
    if (col < 0 || col >= COLS || game.board[0][col] != ' ') {
        return false;
    }
    for (int i = ROWS - 1; i >= 0; --i) {
        if (game.board[i][col] == ' ') {
            game.board[i][col] = player;
            return true;
        }
    }
    return false;
}

void handle_client(int client_socket) {
    Game game;
    init_board(game);
    char buffer[1024];

    while (true) {
        mtx.lock();
        std::cout << "Estado actual del tablero:" << std::endl;
        print_board(game);
        mtx.unlock();

        if (game.player_turn) {
            // Esperar jugada del cliente
            memset(buffer, 0, 1024);
            int read_size = read(client_socket, buffer, 1024);
            if (read_size <= 0) {
                std::cout << "Cliente desconectado" << std::endl;
                close(client_socket);
                break;
            }

            int col = atoi(buffer) - 1; // Convertir columna a índice
            if (!make_move(game, col, 'C')) {
                std::cout << "Movimiento inválido del cliente" << std::endl;
                write(client_socket, "Movimiento inválido\n", 20);
                continue;
            }

            if (check_winner(game, 'C')) {
                game.game_over = true;
                game.winner = 'C';
            }

            // Enviar tablero actualizado al cliente después del movimiento del cliente
            mtx.lock();
            std::string board_str = get_board_str(game);
            mtx.unlock();
            write(client_socket, board_str.c_str(), board_str.size());

        } else {
            // Jugada del servidor
            int col;
            do {
                col = rand() % COLS;
            } while (!make_move(game, col, 'S'));

            if (check_winner(game, 'S')) {
                game.game_over = true;
                game.winner = 'S';
            }

            // Enviar tablero actualizado al cliente después del movimiento del servidor
            mtx.lock();
            std::string board_str = get_board_str(game);
            mtx.unlock();
            write(client_socket, board_str.c_str(), board_str.size());
        }

        game.player_turn = !game.player_turn;

        if (game.game_over) {
            std::string result = game.winner == 'C' ? "¡Has ganado!\n" : "¡El servidor ha ganado!\n";
            write(client_socket, result.c_str(), result.size());
            close(client_socket);
            break;
        }
    }
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Crear el socket del servidor
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("Falló la creación del socket");
        exit(EXIT_FAILURE);
    }

    // Configurar opciones del socket
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("Falló setsockopt");
        exit(EXIT_FAILURE);
    }

    // Asignar dirección y puerto al socket del servidor
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Vincular el socket del servidor a la dirección y puerto especificados
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Falló bind");
        exit(EXIT_FAILURE);
    }

    // Poner el socket en modo de escucha
    if (listen(server_fd, 3) < 0) {
        perror("Falló listen");
        exit(EXIT_FAILURE);
    }

    std::cout << "Esperando conexiones..." << std::endl;

    while (true) {
        // Aceptar nuevas conexiones de clientes
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (new_socket < 0) {
            perror("Falló accept");
            exit(EXIT_FAILURE);
        }
        std::cout << "Nueva conexión, socket fd es " << new_socket << std::endl;

        // Crear un nuevo hilo para manejar la conexión del cliente
        std::thread(handle_client, new_socket).detach();
    }

    return 0;
}

