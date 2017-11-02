/*
 * RelayManager.h
 *
 *  Created on: Sep 2017
 *      Author: raulMrello
 *
 *	RelayManager es el módulo encargado de gestionar las callbacks devueltas por el driver de zerocross para actualizar los
 *  los relés que hayan recibido una notificación de conmutación.
 *  Se ejecuta en su propio hilo de ejecución, y publica mensajes pub/sub por medio de la librería MQLib.
 *  Hereda funcionalidades de todos los drivers que es capaz de gestionar, como Zerocross y Relay.
 *
 *  Las solicitudes de cambio de estado de un relé se realizarán mediante la suscripción al topic "relay/value/cmd" en el que
 *  se recibirá el mensaje: 
 *      msg = (Relay::RelayInfo*) 
 *      msg_len = sizeof(Relay::RelayInfo)
 *  Con el campo RelayAction indicando la acción solicitada (TurnOff, TurnOn)
 *
 *
 *  Cuando un relé cambie de estado, lo notificará mediante una publicación al topic "relay/value/stat" con el mismo mensaje:
 *      msg = (Relay::RelayInfo*) 
 *      msg_len = sizeof(Relay::RelayInfo)
 *  Con el campo RelayStat indicando el estado actual (Off, On)
 *
 *  Se podrá configurar el retardo (microsegundos) desde el zerocross para sincronizar las conmutaciones en diferentes condiciones,
 *  mediante la suscripción al topic "relay/sync/cmd" y con el mensaje 
 *      msg = (uint32_t*) 
 *      msg_len = sizeof(unt32_t)
 *  Con el campo msg igual al nuevo retardo de sincronización
 */
 
#ifndef __RelayManager__H
#define __RelayManager__H

#include "mbed.h"

/** Librerías relativas a módulos software */
#include "MQLib.h"
#include "List.h"
#include "Relay.h"
#include "Zerocross.h"

   
class RelayManager{
  public:
    
    /** Estructura de datos que facilita la inserción de argumentos en el constructor cuando hay varios relés que configurar */
    struct RelayEntity{
        uint32_t id;                    /// Identificador del relé
        PinName out_high;               /// Pin gpio asociado a corriente de pico
        PinName out_low;                /// Pin gpio asociado a corriente de mantenimiento
        Relay::RelayLogicLevel level;   /// Nivel lógico de activación
    };
    
    
    /** Constructor
     *  Asocia una lista de relés (puntero a objetos RelayEntity), una entrada de zerocross y los flancos que utilizará
     *  @param relays Lista de relés manejada por el manager
     *  @param zc Entrada de zerocross
     *  @param zc_level Nivel de activación de eventos del zerocross (flancos activos)
     */
    RelayManager(RelayEntity** relays, PinName zc, Zerocross::LogicLevel zc_level);

    
    /** Destructor */
    ~RelayManager();
  
    
	/** task()
     *  Hilo de ejecución del manager
     */
    void task();

    
  protected:
  
    /** Estructura de datos que facilita el manejo de los eventos y estados relativos a cada relé */
    struct RelayHandler{
        Relay* relay;           /// Relé asociado
        Relay::RelayInfo info;  /// Información del relé (id, estado, acciones pendientes)
    };
      
      
    /** Señales RTOS que es capaz de gestionar el módulo */
    enum SigEventFlags{
        RelayActionPendingFlag  = (1<<0),       /// Indica que se ha solicitado un cambio en algún relé
        RelayChangedFlag        = (1<<1),       /// Indica que un relé ha cambiado de estado
        SyncUpdateFlag          = (1<<2),       /// Indica que se solicita una resincronización con el nuevo retardo enviado
    };
  
    
	/** isrZerocrossCb()
     *  Callback invocada al recibir un evento de zerocross. Se ejecuta en contexto ISR. Se deberá actuar sobre los 
     *  relés lo más rápidamente posible, para estar sincronizado con el zc.
     *  @param level Identificador del flanco activo en el zerocross que generó la interrupción
     */
    void isrZerocrossCb(Zerocross::LogicLevel level);        

        
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
    
    /** Propiedades internas */
    
    Thread      _th;                            /// Manejador del thread
    Mutex       _mut;                           /// Mutex de acceso a datos compartidos
    uint32_t    _timeout;                       /// Manejador de timming en la tarea
    uint32_t    _delay_us;                      /// Retardo forzado sobre el ZC para sincronizar las conmutaciones
    uint32_t    _delay_us_req;                  /// Variable auxiliar para recibir la nueva sincronización
         
    MQ::SubscribeCallback   _subscriptionCb;    /// Callback de suscripción a topics
    MQ::PublishCallback     _publicationCb;     /// Callback de publicación en topics
    
    List<RelayHandler> *_relay_list;            /// Lista de relés
    Zerocross *_zc;                             /// Gestor de zerocross
    
};
     
#endif /*__RelayManager__H */

/**** END OF FILE ****/


