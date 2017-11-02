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
    
    /** Estructura de datos que facilita la inserci�n de argumentos en el constructor cuando hay varios rel�s que configurar */
    struct RelayEntity{
        uint32_t id;                    /// Identificador del rel�
        PinName out_high;               /// Pin gpio asociado a corriente de pico
        PinName out_low;                /// Pin gpio asociado a corriente de mantenimiento
        Relay::RelayLogicLevel level;   /// Nivel l�gico de activaci�n
    };
    
    
    /** Constructor
     *  Asocia una lista de rel�s (puntero a objetos RelayEntity), una entrada de zerocross y los flancos que utilizar�
     *  @param relays Lista de rel�s manejada por el manager
     *  @param zc Entrada de zerocross
     *  @param zc_level Nivel de activaci�n de eventos del zerocross (flancos activos)
     */
    RelayManager(RelayEntity** relays, PinName zc, Zerocross::LogicLevel zc_level);

    
    /** Destructor */
    ~RelayManager();
  
    
	/** task()
     *  Hilo de ejecuci�n del manager
     */
    void task();

    
  protected:
  
    /** Estructura de datos que facilita el manejo de los eventos y estados relativos a cada rel� */
    struct RelayHandler{
        Relay* relay;           /// Rel� asociado
        Relay::RelayInfo info;  /// Informaci�n del rel� (id, estado, acciones pendientes)
    };
      
      
    /** Se�ales RTOS que es capaz de gestionar el m�dulo */
    enum SigEventFlags{
        RelayActionPendingFlag  = (1<<0),       /// Indica que se ha solicitado un cambio en alg�n rel�
        RelayChangedFlag        = (1<<1),       /// Indica que un rel� ha cambiado de estado
        SyncUpdateFlag          = (1<<2),       /// Indica que se solicita una resincronizaci�n con el nuevo retardo enviado
    };
  
    
	/** isrZerocrossCb()
     *  Callback invocada al recibir un evento de zerocross. Se ejecuta en contexto ISR. Se deber� actuar sobre los 
     *  rel�s lo m�s r�pidamente posible, para estar sincronizado con el zc.
     *  @param level Identificador del flanco activo en el zerocross que gener� la interrupci�n
     */
    void isrZerocrossCb(Zerocross::LogicLevel level);        

        
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
    
};
     
#endif /*__RelayManager__H */

/**** END OF FILE ****/


