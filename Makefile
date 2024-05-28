# Este es el Makefile para compilar los programas servidor y cliente

# Variables del compilador y las banderas
CXX = g++
CXXFLAGS = -pthread

# Objetivos a construir
all: servidor cliente

# Regla para construir el servidor
servidor: servidor.cpp
	$(CXX) $(CXXFLAGS) -o servidor servidor.cpp

# Regla para construir el cliente
cliente: cliente.cpp
	$(CXX) $(CXXFLAGS) -o cliente cliente.cpp

# Limpiar archivos compilados
clean:
	rm -f servidor cliente
