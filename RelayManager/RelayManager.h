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
  
    /** Tiempo en ms por defecto, que durará la activación a máxima corriente antes de bajar a corriente de mantenimiento */
    static const uint32_t DefaultMaxCurrentTime = 100;
  
  
    /** Estructura de datos que facilita la inserción de argumentos en el constructor cuando hay varios relés que configurar */
    struct RelayDefinition{
        uint32_t id;                    /// Identificador del relé
        PinName out_high;               /// Pin gpio asociado a corriente de pico
        PinName out_low;                /// Pin gpio asociado a corriente de mantenimiento
        Relay::RelayLogicLevel level;   /// Nivel lógico de activación
    };

    
    /** Estructura de datos para la publicación de mensajes relativos al relé (comando y estados) */
    struct RelayMsg {
        RelayMsg(uint32_t xid, Relay::RelayStat xstat) : id(xid), stat(xstat){}
        uint32_t id;                    /// Identificador del relé
        Relay::RelayStat stat;          /// Comando a realizar o Estado a notificar
    };
    
    
    /** Constructor
     *  Crea un manejador de relés asociando por defecto la entrada de zerocross y los flancos que utilizará
     *  @param zc Entrada de zerocross
     *  @param zc_level Nivel de activación de eventos del zerocross (flancos activos)
     */
    RelayManager(PinName zc, Zerocross::LogicLevel zc_level);

    
    /** Destructor */
    ~RelayManager();
    
    
    /** addRelay
     *  Añade un relé a la lista
     *  @param relaydef Definición del relé a añadir a la lista
     *  @param high2lowtime Tiempo en ms con corriente máxima, antes de bajar a mantenimiento
     *  @return Puntero al relé creado o NULL en caso de error
     */
    Relay* addRelay(RelayDefinition* relaydef, uint32_t high2lowtime = DefaultMaxCurrentTime);
    
   
    
  protected:

    /** Tipos de acciones que se pueden solicitar a un relé */
    enum RelayAction{
        RelayTurnOff,       /// Apagar relé
        RelayTurnOnHigh,    /// Activar relé a máxima corriente
        RelayTurnOnLow,     /// Activar relé a corriente de mantenimiento
        ActionCompleted,    /// Acción completada (puede ser TurnOff o TurnOn previo)
        NoActions,          /// No hay acciones a realizar
    };
		  
    /** Estructura de datos que facilita el manejo de los eventos y estados relativos a cada relé */
    struct RelayHandler{
        Relay* relay;               /// Relé asociado
        RelayAction action;         /// Acciones solicitadas al relé        
    };
      
      
    /** Señales RTOS que es capaz de gestionar el módulo */
    enum SigEventFlags{
        RelayActionPendingFlag  = (1<<0),       /// Indica que se ha solicitado un cambio en algún relé
        RelayChangedFlag        = (1<<1),       /// Indica que un relé ha cambiado de estado
        SyncUpdateFlag          = (1<<2),       /// Indica que se solicita una resincronización con el nuevo retardo enviado
        RelayToLowLevel         = (1<<3),       /// Indica que algún relé debe bajar a corriente de mantenimiento
    };
     
    
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
    RelayHandler* _rhnext;                      /// Próxima acción a realizar
    
    Callback<void(uint32_t)> _timeoutCb;        /// Callback para notificar paso a bajo nivel
    
	/** isrZerocrossCb()
     *  Callback invocada al recibir un evento de zerocross. Se ejecuta en contexto ISR. Se deberá actuar sobre los 
     *  relés lo más rápidamente posible, para estar sincronizado con el zc.
     *  @param level Identificador del flanco activo en el zerocross que generó la interrupción
     */
    void isrZerocrossCb(Zerocross::LogicLevel level);        
    
	/** isrMaxCurrTimeout()
     *  Callback invocada al recibir una notificación de relé para bajar a doble nivel
     *  @param id Identificador del relé que lo solicita
     */
    void isrMaxCurrTimeout(uint32_t id);        
    
        
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


    /** task()
     *  Hilo de ejecución del manager
     */
    void task();    
};
     
#endif /*__RelayManager__H */

/**** END OF FILE ****/


