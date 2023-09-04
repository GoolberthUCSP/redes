# Redes y Comunicación - UCSP 2023-02

- Fredy Goolberth Quispe Neira

## socket

Creación de un chat, en el cual el cliente podrá enviar mensajes a un cliente conectado. Al principio el cliente se identifica, luego puede enviar mensajes en formato: *destinatario,mensaje*.
Para cerrar sesión el cliente envía el siguiente mensaje: *END*.

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