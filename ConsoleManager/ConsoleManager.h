/*
 * ConsoleManager.h
 *
 *  Created on: Oct 2017
 *      Author: raulMrello
 *
 *	ConsoleManager es el módulo encargado de gestionar las operaciones remotas (RPC) solicitadas desde un 
 *  terminal remoto conectado a un puerto de comunicaciones (en este caso un SerialTerminal). El formato de
 *  las órdenes RPC es en modo texto y de esta forma:
 *
 *        CMD\nNUMARGS\nARG0\nARG1\n...ARGN\n\0
 *
 *  ConsoleManager se encargará de chequear que el formato de la orden RPC es correcto y de publicarlo en el
 *  topic "$SYS/rpc/req/CMD" correspondiente, siendo CMD el nombre de la orden solicitada. Así para una orden cuyo
 *  nombre es "TSPB", se publicará en el topic "$SYS/rpc/req/TSPB" con los datos "NUMARGS\nARG0\nARG1\n...ARGN\n\0"
 *
 *  NOTA: Todos los comandos RPC se pueden consultar en el archivo "Comandos_RPC.txt"
 *        Además, todos los módulos que sean capaces de procesar comandos RPC deberán hacer uso de la librería
 *        <RPCBuilder> para parsear los topics a las estructuras de datos correspondientes, o enviarlos tal cual
 *        si esa librería no existe.
 *
 */
 

#ifndef CONSOLEMANAGER_H
#define CONSOLEMANAGER_H

/** Archivos de cabecera */
#include "mbed.h"
#include "SerialTerminal.h"
#include "MQLib.h"
#include "Heap.h"



class ConsoleManager{

public:
    /** ConsoleManager()
     *  Crea el objeto asignando un puerto serie para la interfaz con el equipo digital
     *  @param sterm Puerto serie asignado
     *  @param recv_buf_size Tamaño a reservar para el buffer de recepción
     *  @param token Caracter de separación de argumentos. Por defecto '\n'
     */
    ConsoleManager(SerialTerminal* sterm, uint16_t recv_buf_size, const char* token = "\n");

    /** task()
     *  Hilo de ejecución asociado para el procesado de las comunicaciones serie
     */
    void task(); 
  
    
protected:
                
	/** subscriptionCb()
     *  Callback invocada al recibir una actualización de un topic al que está suscrito
     *  @param topic Identificador del topic
     *  @param msg Mensaje recibido
     *  @param msg_len Tamaño del mensaje
     */    
    void subscriptionCb(const char* topic, void* msg, uint16_t msg_len);

        
	/** publicationCb()
     *  Callback invocada al finalizar una publicación
     *  @param topic Identificador del topic
     *  @param result Resultado de la publicación
     */    
     void publicationCb(const char* topic, int32_t result);


    /** onRxComplete()
     *  Manejador ISR de datos recibidos vía serie
     */
    void onRxComplete();


    /** onRxTimeout()
     *  Manejador ISR de timeout en la recepción serie
     */
    void onRxTimeout();


    /** onRxOvf()
     *  Manejador ISR de buffer overflow en la recepción serie
     */
    void onRxOvf();


    /** onTxComplete()
     *  Manejador ISR de datos enviados vía serie
     */
    void onTxComplete();


    /** onRxData()
     *  Procesamiento dedicado de los bytes recibidos.
     *  @param buf Buffer de datos recibidos
     *  @param size Número de dato recibidos hasta el momento
     *  @return Indica si el procesado tiene una trama válida (true) o no (false)
     */
    bool onRxData(uint8_t* buf, uint16_t size);   


    /** Flags de tarea (asociados a la máquina de estados) */
    enum SigEventFlags{
        ReceivedData   = (1<<0),
        SentData       = (1<<1),
        TimeoutOnRecv  = (1<<2),
        OverflowOnRecv = (1<<3),
    };
      
    
    Thread      _th;            /// Manejador del thread
    uint32_t    _timeout;       /// Manejador de timming en la tarea
    SerialTerminal* _sterm;     /// Manejador del terminal serie
         
    MQ::SubscribeCallback   _subscriptionCb;    /// Callback de suscripción a topics
    MQ::PublishCallback     _publicationCb;     /// Callback de publicación en topics
    
    char* _databuf;             /// Buffer para la recepción de datos
    uint16_t _bufsize;          /// Tamaño del buffer
    char* _token;               /// Caracter de separación de argumentos

    Callback <void()> _cb_tx;
    Callback <void()> _cb_rx;
    Callback <void()> _cb_rx_tmr;
    
    /** Envía la respuesta */
    void ack();
};


#endif
