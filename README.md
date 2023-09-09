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

## socketv2

El mismo chat del anterior *socket*, pero usando un protocolo para el envío y recibo de mensajes por parte de clientes y servidor. La compilación y la ejecución es igual al la versión anterior.

- Las reglas para el ingreso de datos por el teclado es la siguiente:
    + Para cambiar el username se debe escribir: *N,new_username*.
    + Para enviar un mensaje a una persona se debe escribir: *M,receiver,message*, en el cual *receiver* es el destinatario.
    + Para enviar un mensaje a todos se debe escribir: *W,message*.
    + Para pedir la lista de clientes conectados se debe escribir: *L*.
    + Por último, para desconectarse del servidor se debe escribir: *Q*.