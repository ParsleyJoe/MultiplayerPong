# MultiplayerPong

Simple pong game made to run on LAN using Unix sockets

## Building
    ### Build client:
    ```
        make clean
        make
    ```
    ### Build server:
    ```
        cd pongserver
        make clean
        make
    ```

## Running
    1. Run server
        ```
        cd pongserver
        ./server
        ```
    2. Run clients
        ```
        ./pong hostname
        ./pong hostname
        ```
    hostname = linux machine hostname

Currently game only works when both clients are connected

