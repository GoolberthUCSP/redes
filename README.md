# Redes y Comunicación - UCSP 2023-02

- Fredy Goolberth Quispe Neira

## socket

- Compilación
```terminal
make
```
- Ejecución: necesario como parámetro el puerto por el cual se comunicarán el servidor y los clientes
```terminal
./server <Port>
```
```terminal
./client <Port>
```
Servidor y clientes deben usar el mismo puerto para la comunicación, por defecto el IP del servidor es el *localhost* al cual se conectarán los clientes.