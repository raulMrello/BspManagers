/*
 * RelayManager.h
 *
 *  Created on: Sep 2017
 *      Author: raulMrello
 *
 *	RelayManager es el m�dulo encargado de gestionar las callbacks devueltas por el driver de zerocross para actualizar los
 *  los rel�s que hayan recibido una notificaci�n de conmutaci�n.
 *  Se ejecuta en su propio hilo de ejecuci�n, y publica mensajes pub/sub por medio de la librer�a MQLib.
 *  Hereda funcionalidades de todos los drivers que es capaz de gestionar, como Zerocross y Relay.
 *
 *  Las solicitudes de cambio de estado de un rel� se realizar�n mediante la suscripci�n al topic "relay/value/cmd" en el que
 *  se recibir� el mensaje: 
 *      msg = (Relay::RelayInfo*) 
 *      msg_len = sizeof(Relay::RelayInfo)
 *  Con el campo RelayAction indicando la acci�n solicitada (TurnOff, TurnOn)
 *
 *
 *  Cuando un rel� cambie de estado, lo notificar� mediante una publicaci�n al topic "relay/value/stat" con el mismo mensaje:
 *      msg = (Relay::RelayInfo*) 
 *      msg_len = sizeof(Relay::RelayInfo)
 *  Con el campo RelayStat indicando el estado actual (Off, On)
 *
 *  Se podr� configurar el retardo (microsegundos) desde el zerocross para sincronizar las conmutaciones en diferentes condiciones,
 *  mediante la suscripci�n al topic "relay/sync/cmd" y con el mensaje 
 *      msg = (uint32_t*) 
 *      msg_len = sizeof(unt32_t)
 *  Con el campo msg igual al nuevo retardo de sincronizaci�n
 */
 
#ifndef __RelayManager__H
#define __RelayManager__H

#include "mbed.h"

/** Librer�as relativas a m�dulos software */
#include "MQLib.h"
#include "List.h"
#include "Relay.h"
#include "Zerocross.h"

   
class RelayManager{
  public:
  
    /** Tiempo en ms por defecto, que durar� la activaci�n a m�xima corriente antes de bajar a corriente de mantenimiento */
    static const uint32_t DefaultMaxCurrentTime = 100;
  
  
    /** Estructura de datos que facilita la inserci�n de argumentos en el constructor cuando hay varios rel�s que configurar */
    struct RelayDefinition{
        uint32_t id;                    /// Identificador del rel�
        PinName out_high;               /// Pin gpio asociado a corriente de pico
        PinName out_low;                /// Pin gpio asociado a corriente de mantenimiento
        Relay::RelayLogicLevel level;   /// Nivel l�gico de activaci�n
    };

    
    /** Estructura de datos para la publicaci�n de mensajes relativos al rel� (comando y estados) */
    struct RelayMsg {
        RelayMsg(uint32_t xid, Relay::RelayStat xstat) : id(xid), stat(xstat){}
        uint32_t id;                    /// Identificador del rel�
        Relay::RelayStat stat;          /// Comando a realizar o Estado a notificar
    };
    
    
    /** Constructor
     *  Crea un manejador de rel�s asociando por defecto la entrada de zerocross y los flancos que utilizar�
     *  @param zc Entrada de zerocross
     *  @param zc_level Nivel de activaci�n de eventos del zerocross (flancos activos)
     */
    RelayManager(PinName zc, Zerocross::LogicLevel zc_level);

    
    /** Destructor */
    ~RelayManager();
    
    
    /** addRelay
     *  A�ade un rel� a la lista
     *  @param relaydef Definici�n del rel� a a�adir a la lista
     *  @param high2lowtime Tiempo en ms con corriente m�xima, antes de bajar a mantenimiento
     *  @return Puntero al rel� creado o NULL en caso de error
     */
    Relay* addRelay(RelayDefinition* relaydef, uint32_t high2lowtime = DefaultMaxCurrentTime);
    
   
    
  protected:

    /** Tipos de acciones que se pueden solicitar a un rel� */
    enum RelayAction{
        RelayTurnOff,       /// Apagar rel�
        RelayTurnOnHigh,    /// Activar rel� a m�xima corriente
        RelayTurnOnLow,     /// Activar rel� a corriente de mantenimiento
        ActionCompleted,    /// Acci�n completada (puede ser TurnOff o TurnOn previo)
        NoActions,          /// No hay acciones a realizar
    };
		  
    /** Estructura de datos que facilita el manejo de los eventos y estados relativos a cada rel� */
    struct RelayHandler{
        Relay* relay;               /// Rel� asociado
        RelayAction action;         /// Acciones solicitadas al rel�        
    };
      
      
    /** Se�ales RTOS que es capaz de gestionar el m�dulo */
    enum SigEventFlags{
        RelayActionPendingFlag  = (1<<0),       /// Indica que se ha solicitado un cambio en alg�n rel�
        RelayChangedFlag        = (1<<1),       /// Indica que un rel� ha cambiado de estado
        SyncUpdateFlag          = (1<<2),       /// Indica que se solicita una resincronizaci�n con el nuevo retardo enviado
        RelayToLowLevel         = (1<<3),       /// Indica que alg�n rel� debe bajar a corriente de mantenimiento
    };
     
    
    /** Propiedades internas */
    
    Thread      _th;                            /// Manejador del thread
    Mutex       _mut;                           /// Mutex de acceso a datos compartidos
    uint32_t    _timeout;                       /// Manejador de timming en la tarea
    uint32_t    _delay_us;                      /// Retardo forzado sobre el ZC para sincronizar las conmutaciones
    uint32_t    _delay_us_req;                  /// Variable auxiliar para recibir la nueva sincronizaci�n
         
    MQ::SubscribeCallback   _subscriptionCb;    /// Callback de suscripci�n a topics
    MQ::PublishCallback     _publicationCb;     /// Callback de publicaci�n en topics
    
    List<RelayHandler> *_relay_list;            /// Lista de rel�s
    Zerocross *_zc;                             /// Gestor de zerocross                               
    RelayHandler* _rhnext;                      /// Pr�xima acci�n a realizar
    
    Callback<void(uint32_t)> _timeoutCb;        /// Callback para notificar paso a bajo nivel
    
	/** isrZerocrossCb()
     *  Callback invocada al recibir un evento de zerocross. Se ejecuta en contexto ISR. Se deber� actuar sobre los 
     *  rel�s lo m�s r�pidamente posible, para estar sincronizado con el zc.
     *  @param level Identificador del flanco activo en el zerocross que gener� la interrupci�n
     */
    void isrZerocrossCb(Zerocross::LogicLevel level);        
    
	/** isrMaxCurrTimeout()
     *  Callback invocada al recibir una notificaci�n de rel� para bajar a doble nivel
     *  @param id Identificador del rel� que lo solicita
     */
    void isrMaxCurrTimeout(uint32_t id);        
    
        
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


    /** task()
     *  Hilo de ejecuci�n del manager
     */
    void task();    
};
     
#endif /*__RelayManager__H */

/**** END OF FILE ****/


