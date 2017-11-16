/*
 * TouchManager.h
 *
 *  Created on: Nov 2017
 *      Author: raulMrello
 *
 *	TouchManager es el módulo encargado de gestionar los eventos recibidos de un driver de contactos capacitivos, como
 *  por ejemplo el MPR121_CapTouch driver. Por lo tanto debe extender la funcionalidad de dicho driver
 *
 *  Este gestor puede trabajar como thread independiente o formar parte de threads auxiliares para procesar trabajos puntuales
 *  mediante el servicio <job>. Además proporciona un mecanimo para generar eventos TOUCHED, HOLD, RELEASED.
 *
 *  El uso de este manager a más alto nivel puede ser, mediante la librería MQLib, mediante la instalación de los topics
 *  de publicación correspondientes y/o por medio de callbacks dedicadas. Así la publicación de topics será:
 *
 *      $(pub_topic) ELEC,1     // Para notificar pulsación en nodo ELEC
 *      $(pub_topic) ELEC,0    // Para notificar liberación en nodo ELEC
 */
 
#ifndef __TouchManager__H
#define __TouchManager__H

#include "mbed.h"

/** Librerías relativas a módulos software */
#include "MQLib.h"
#include "Logger.h"
#include "MPR121_CapTouch.h"


   
class TouchManager : public MPR121_CapTouch{
  public:

  
    /** TouchEvent
     *  Lista de los eventos que puede notificar este manager
     */  
    enum TouchEvent{
        ReleasedEvent,
        TouchedEvent,
    };
    
  
    /** TouchMsg
     *  Estructura de los mensajes que puede publicar este manager
     */  
    struct TouchMsg{
        uint8_t elec;
        TouchEvent evt;
    };
		
    /** Callback definida para la notificación de eventos */
    typedef Callback<void(TouchMsg*)> TouchEventCallback;
  
    /** Constructor
     *  Asocia los pines gpio para el driver implementado y la dirección I2C
     *  @param sda Línea sda del bus i2c
     *  @param scl Línea scl del bus i2c
     *  @param irq Entrada de interrupción
     *  @param elec_mask Máscara de bits de los electrodos activos
     *  @param addr Dirección i2c, por defecto (5Ah)
     *  @param run_thread Flag para indicar si debe iniciarse como un thread o no
     */
    TouchManager(PinName sda, PinName scl, PinName irq, uint16_t elec_mask, uint8_t addr = MPR121_CapTouch::DefaultAddress, bool run_thread = true);
  
  
    /** Destructor */
    ~TouchManager();
    
  
	/** ready()
     *  Devuelve el estado de ejecución
     *  @return True, False
     */
    bool ready() {return _ready; }
    
  
	/** setDebugChannel()
     *  Instala canal de depuración
     *  @param dbg Logger
     */
    void setDebugChannel(Logger* dbg){ _debug = dbg; }
    
	
    /** @fn job
     *  @brief Rutina de ejecución para procesar eventos de forma asíncrona, suele utilizarse
     *  en threads de control externos que se encargan de lanzar trabajos a otros módulos que
     *  carecen de thread propio.
     *  @param signals Flags activos
     */
    void job(uint32_t signals);
    
  
	/** attachCallback()
     *  Instala callback de recepción de eventos
     *  @param onEventCb Callback a instalar para recbir eventos tipo TouchEvent
     */
    void attachCallback(TouchEventCallback onEventCb) { _evt_cb = onEventCb; }
    
  
	/** setPublicationBase()
     *  Registra el topic base a los que publicará el módulo
     *  @param pub_topic Topic base para la publicación
     */
    void setPublicationBase(const char* pub_topic);  

    
  protected:
    static const uint32_t AntiGlitchTimeout = 30000;    /// Filtro anti-glitch de 30ms
  
    /** Flags de tarea (asociados a la máquina de estados) */
    enum SigEventFlags{
        IrqFlag         = (1<<0),       /// Flag para notificar interrupción del driver
        AntiGlitchFlag  = (1<<1),       /// Flag para notificar interrupción del filtro anti-glitch
    };
    
    Thread      _th;                    /// Manejador del thread
    Ticker      _tick_glitch;           /// Ticker del filtro anti-glitch
    uint32_t    _timeout;               /// Manejador de timming en la tarea
    char*       _pub_topic;             /// Topic base para la publicación
    char        _msg[8];                /// Mensaje a publicar
    Logger*     _debug;                 /// Canal de depuración
    uint16_t    _curr_sns;              /// Valor actual de los sensores
    uint16_t    _sns;                   /// Valor de la medida en curso
    bool   _ready;                      /// Flag de estado disponible    

    TouchEventCallback _evt_cb;         /// Callback a invocar para la notificación de eventos
    MQ::PublishCallback _publicationCb; /// Callback de publicación en topics
    
	/** task()
     *  Hilo de ejecución del protocolo 
     */
    void task();
  
    
	/** onIrqCb()
     *  Callback invocada tras recibir un evento del chip en la línea irq
     */
    void onIrqCb();        
  
    
	/** isrTickCb()
     *  Callback invocada tras la temporización del filtro antiglitch
     */
    void isrTickCb();        
    

	/** publicationCb()
     *  Callback invocada al finalizar una publicación
     *  @param topic Identificador del topic
     *  @param result Resultado de la publicación
     */    
     void publicationCb(const char* topic, int32_t result);    
    
};
     
#endif /*__TouchManager__H */

/**** END OF FILE ****/


