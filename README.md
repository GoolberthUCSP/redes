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

Añadido un protocolo para el envío y recibo de mensajes por parte de clientes y servidor.

- Las reglas para el ingreso de datos por el teclado es la siguiente:
    + Para cambiar el username se debe escribir: *N,new_username*.
    + Para enviar un mensaje a una persona se debe escribir: *M,receiver,message*, en el cual *receiver* es el destinatario.
    + Para enviar un mensaje a todos se debe escribir: *W,message*.
    + Para pedir la lista de clientes conectados se debe escribir: *L*.
    + Por último, para desconectarse del servidor se debe escribir: *Q*.

## socketv3

Añadido la opción de enviar archivos menos de 100kB por defecto. Si se envía un tamaño mayor, éste no será enviado en su totalidad. El que envía el archivo podrá ver que se envió correctamente el archivo comparando los *hash_number* del archivo a enviar y del recibido.

- La regla para el envío de datos:
    + Para enviar un archivo a una persona se debe escribir: *F,receiver,filename* en donde filename es el nombre del archivo y este debe contener su extensión.

## socketv4

Añadido la opción de jugar TicTacToe entre 2 clientes, todos los demás clientes solo podrán ver el tablero del juego.

- La regla para iniciar el juego es:
    + Para registrarse un jugador se debe escribir *B*.
    + En caso de que ya existan 2 jugadores, no se podrá registrar.
    + Inicia el primero en registrarse.
    + Los símbolos que se les asigna son random.

- La regla para jugar es:
    + Para realizar una jugada o marcar un casillero se debe escribir: *P,number* en donde *number* es un número entre 1 y 9; éste es el número del casillero.
    + En caso de que no sea su turno, se le enviará una notificación de que no puede realizar la jugada.
    + En caso de que alguien gane o el tablero esté lleno, se les enviará una notificación y se reiniciará el juego; luego, se podrán registrar nuevos jugadores.

- La regla para visualizar el tablero es:
    + Escribir: *V* y podrá visualizar el estado actual del tablero.