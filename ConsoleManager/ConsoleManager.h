/*
 * ConsoleManager.h
 *
 *  Created on: Oct 2017
 *      Author: raulMrello
 *
 *	ConsoleManager es el m�dulo encargado de gestionar las operaciones remotas (RPC) solicitadas desde un 
 *  terminal remoto conectado a un puerto de comunicaciones (en este caso un SerialTerminal). El formato de
 *  las �rdenes RPC es en modo texto y de esta forma:
 *
 *        CMD\nNUMARGS\nARG0\nARG1\n...ARGN\n\0
 *
 *  ConsoleManager se encargar� de chequear que el formato de la orden RPC es correcto y de publicarlo en el
 *  topic "$SYS/rpc/req/CMD" correspondiente, siendo CMD el nombre de la orden solicitada. As� para una orden cuyo
 *  nombre es "TSPB", se publicar� en el topic "$SYS/rpc/req/TSPB" con los datos "NUMARGS\nARG0\nARG1\n...ARGN\n\0"
 *
 *  NOTA: Todos los comandos RPC se pueden consultar en el archivo "Comandos_RPC.txt"
 *        Adem�s, todos los m�dulos que sean capaces de procesar comandos RPC deber�n hacer uso de la librer�a
 *        <RPCBuilder> para parsear los topics a las estructuras de datos correspondientes, o enviarlos tal cual
 *        si esa librer�a no existe.
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
     *  @param recv_buf_size Tama�o a reservar para el buffer de recepci�n
     *  @param token Caracter de separaci�n de argumentos. Por defecto '\n'
     */
    ConsoleManager(SerialTerminal* sterm, uint16_t recv_buf_size, const char* token = "\n");

    /** task()
     *  Hilo de ejecuci�n asociado para el procesado de las comunicaciones serie
     */
    void task(); 
  
    
protected:
                
	/** subscriptionCb()
     *  Callback invocada al recibir una actualizaci�n de un topic al que est� suscrito
     *  @param topic Identificador del topic
     *  @param msg Mensaje recibido
     *  @param msg_len Tama�o del mensaje
     */    
    void subscriptionCb(const char* topic, void* msg, uint16_t msg_len);

        
	/** publicationCb()
     *  Callback invocada al finalizar una publicaci�n
     *  @param topic Identificador del topic
     *  @param result Resultado de la publicaci�n
     */    
     void publicationCb(const char* topic, int32_t result);


    /** onRxComplete()
     *  Manejador ISR de datos recibidos v�a serie
     */
    void onRxComplete();


    /** onRxTimeout()
     *  Manejador ISR de timeout en la recepci�n serie
     */
    void onRxTimeout();


    /** onRxOvf()
     *  Manejador ISR de buffer overflow en la recepci�n serie
     */
    void onRxOvf();


    /** onTxComplete()
     *  Manejador ISR de datos enviados v�a serie
     */
    void onTxComplete();


    /** onRxData()
     *  Procesamiento dedicado de los bytes recibidos.
     *  @param buf Buffer de datos recibidos
     *  @param size N�mero de dato recibidos hasta el momento
     *  @return Indica si el procesado tiene una trama v�lida (true) o no (false)
     */
    bool onRxData(uint8_t* buf, uint16_t size);   


    /** Flags de tarea (asociados a la m�quina de estados) */
    enum SigEventFlags{
        ReceivedData   = (1<<0),
        SentData       = (1<<1),
        TimeoutOnRecv  = (1<<2),
        OverflowOnRecv = (1<<3),
    };
      
    
    Thread      _th;            /// Manejador del thread
    uint32_t    _timeout;       /// Manejador de timming en la tarea
    SerialTerminal* _sterm;     /// Manejador del terminal serie
         
    MQ::SubscribeCallback   _subscriptionCb;    /// Callback de suscripci�n a topics
    MQ::PublishCallback     _publicationCb;     /// Callback de publicaci�n en topics
    
    char* _databuf;             /// Buffer para la recepci�n de datos
    uint16_t _bufsize;          /// Tama�o del buffer
    char* _token;               /// Caracter de separaci�n de argumentos

    Callback <void()> _cb_tx;
    Callback <void()> _cb_rx;
    Callback <void()> _cb_rx_tmr;
    
    /** Env�a la respuesta */
    void ack();
};


#endif
