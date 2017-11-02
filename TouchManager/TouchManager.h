/*
 * TouchManager.h
 *
 *  Created on: Nov 2017
 *      Author: raulMrello
 *
 *	TouchManager es el m�dulo encargado de gestionar los eventos recibidos de un driver de contactos capacitivos, como
 *  por ejemplo el MPR121_CapTouch driver. Por lo tanto debe extender la funcionalidad de dicho driver
 *
 *  Este m�dulo proporciona un hilo de ejecuci�n propio para realizar las consultas I2C al chip MPR121 proporcionado por
 *  el driver, desde el contexto de tarea. Adem�s proporciona un mecanimo para generar eventos TOUCHED, HOLD, RELEASED.
 *
 *  El uso de este manager a m�s alto nivel puede ser, mediante la librer�a MQLib, mediante la instalaci�n de los topics
 *  de publicaci�n correspondientes y/o por medio de callbacks dedicadas.
 */
 
#ifndef __TouchManager__H
#define __TouchManager__H

#include "mbed.h"

/** Librer�as relativas a m�dulos software */
#include "MQLib.h"
#include "Logger.h"
#include "MPR121_CapTouch.h"


   
class TouchManager : public MPR121_CapTouch{
  public:

  
    /** TouchEvent
     *  Lista de los eventos que puede notificar este manager
     */  
    enum TouchEvent{
        TouchedEvent,
        ReleasedEvent,
    };
    
  
    /** TouchMsg
     *  Estructura de los mensajes que puede publicar este manager
     */  
    struct TouchMsg{
        uint8_t elec;
        TouchEvent evt;
    };
  
    /** Constructor
     *  Asocia los pines gpio para el driver implementado y la direcci�n I2C
     *  @param sda L�nea sda del bus i2c
     *  @param scl L�nea scl del bus i2c
     *  @param irq Entrada de interrupci�n
     *  @param addr Direcci�n i2c, por defecto (5Ah)
     */
    TouchManager(PinName sda, PinName scl, PinName irq, uint8_t addr = MPR121_CapTouch::DefaultAddress);
  
  
    /** Destructor */
    ~TouchManager();
    
  
	/** setDebugChannel()
     *  Instala canal de depuraci�n
     *  @param dbg Logger
     */
    void setDebugChannel(Logger* dbg){ _debug = dbg; }
    
  
	/** attachCallback()
     *  Instala callback de recepci�n de eventos
     *  @param onEventCb Callback a instalar para recbir eventos tipo TouchEvent
     */
    void attachCallback(Callback<void(TouchMsg*)> onEventCb) { _evt_cb = onEventCb; }
    
  
	/** attachTopics()
     *  Registra los topics en los que publicar� los eventos, adjuntando el tipo TouchEvent.
     *  @param pub_topic Topic sobre el que publicar� los eventos
     */
    void attachTopics(const char* pub_topic) { _pub_topic = (char*)pub_topic; }   

    
  protected:
      
    /** Flags de tarea (asociados a la m�quina de estados) */
    enum SigEventFlags{
        IrqFlag  = (1<<0),
    };
    
    Thread      _th;                    /// Manejador del thread
    uint32_t    _timeout;               /// Manejador de timming en la tarea
    char*       _pub_topic;             /// Topic sobre el que publicar         
    Logger*     _debug;                 /// Canal de depuraci�n
    uint16_t    _curr_sns;              /// Valor actual de los sensores

    Callback<void(TouchMsg*)> _evt_cb;      /// Callback a invocar para la notificaci�n de eventos
    MQ::PublishCallback     _publicationCb; /// Callback de publicaci�n en topics
    
	/** task()
     *  Hilo de ejecuci�n del protocolo 
     */
    void task();
  
    
	/** onIrqCb()
     *  Callback invocada tras recibir un evento del chip en la l�nea irq
     */
    void onIrqCb();        
    

	/** publicationCb()
     *  Callback invocada al finalizar una publicaci�n
     *  @param topic Identificador del topic
     *  @param result Resultado de la publicaci�n
     */    
     void publicationCb(const char* topic, int32_t result);    
    
};
     
#endif /*__TouchManager__H */

/**** END OF FILE ****/


