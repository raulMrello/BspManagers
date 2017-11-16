/*
 * ProximityManager.h
 *
 *  Created on: Nov 2017
 *      Author: raulMrello
 *
 *  ProximityManager es un m�dulo C++ que gestiona la detecci�n de objetos cercanos mediante el sensor HC-SR04 de ultrasonidos.
 *  Requiere un par de gpios (salida digital y entrada de interrupci�n) para el driver HCSR04.
 *
 *  Este gestor puede trabajar como thread independiente o formar parte de threads auxiliares para procesar trabajos puntuales
 *  mediante el servicio <job>
 *
 *  Se puede configurar un par de topics base, para la suscripci�n $(sub_topic) y para la publicaci�n $(pub_topic). Teniendo esto
 *  en cuenta, las �rdenes que acepta el m�dulo son las siguientes:
 *
 *  Suscripci�n:
 *      $(sub_topic)/cmd/config D,I,O,F,R,Ei,Er
 *      Permite ajustar los siguientes par�metros:
 *      D: Rango de detecci�n m�xima en cm. Por encima de ese valor, no lo tendr� en cuenta.
 *      I: Diferencia en cm entre medidas consecutivas para notificar event de acercamiento.
 *      O: Diferencia en cm entre medidas consecutivas para notificar event de alejamiento.
 *      F: N�mero de muestras acumuladas para el filtro anti-glitch.
 *      R: Ventana en cm para que el filtro anti-glitch considere dos medidas como similares
 *      Ei:Flag para habilitar o no los eventos de captura instant�neos.
 *      Ei:Flag para habilitar o no los eventos de errores durante la captura.
 *      Por ejemplo, para detectar cambios superiores a 10cm al acercarse y 20cm al alejarse, con un filtro de 3 muestras, y 50cm 
 *      como rango para las muestras del filtro,con todos los eventos activos, se publicar�a lo siguiente: 
 *                  $(sub_topic)/cmd/dist 10,20,3,50,1,1
 *
 *      $(sub_topic)/cmd/start T,t
 *      Permite iniciar la captura con una cadencia T(ms) y un timeout de medida de t(ms)
 *
 *      $(sub_topic)/cmd/stop 0
 *      Permite detener la captura
 *
 *  Publicaci�n:
 *      $(sub_topic)/sta/dist E,D
 *      Permite notificar eventos de estado indicando E(tipo de evento: 0 si se acerca, 1 si se aleja, 2 error en medida) y 
 *      D(distancia en cm), as� para notificar que un objeto se aproxima y que est� a 20cm se publicar�: $(pub_topic)/stat/dist 0,20 
 *
 *  NOTA: Esta es la configuraci�n para una medida constante con resoluci�n adecuada:
 *  $/cmd/config 100,3,3,3,10,0,1 
 *  $/cmd/start 200,150 
 */
 
 
#ifndef __ProximityManager_H
#define __ProximityManager_H

 
#include "mbed.h"
#include "MQLib.h"
#include "Logger.h"
#include "HCSR04.h"


//------------------------------------------------------------------------------------
//- CLASS ProximityManager -------------------------------------------------------------
//------------------------------------------------------------------------------------



class ProximityManager : public HCSR04{
public:		
    
    /** @fn ProximityManager()
     *  @brief Constructor, que asocia una salida digital y una interrupci�n entrante
     *  @param trig Pin de salida digital
     *  @param echo Pin de entrada con el echo devuelto
     *  @param run_thread Flag para indicar si debe iniciarse como un thread o no
     */
    ProximityManager(PinName trig, PinName echo, bool run_thread = true);

	
    /** @fn ~ProximityManager()
     *  @brief Destructor por defecto
     */
    virtual ~ProximityManager(){}
    
  
	/** ready()
     *  Devuelve el estado de ejecuci�n
     *  @return True, False
     */
    bool ready() {return _ready; }
    
  
	/** setDebugChannel()
     *  Instala canal de depuraci�n
     *  @param dbg Logger
     */
    void setDebugChannel(Logger* dbg){ _debug = dbg; }
    
	
    /** @fn job
     *  @brief Rutina de ejecuci�n para procesar eventos de forma as�ncrona, suele utilizarse
     *  en threads de control externos que se encargan de lanzar trabajos a otros m�dulos que
     *  carecen de thread propio.
     *  @param signals Flags activos
     */
    void job(uint32_t signals);
    
  
	/** setSubscriptionBase()
     *  Registra el topic base a los que se suscribir� el m�dulo
     *  @param sub_topic Topic base para la suscripci�n
     */
    void setSubscriptionBase(const char* sub_topic);          
    
  
	/** setPublicationBase()
     *  Registra el topic base a los que publicar� el m�dulo
     *  @param pub_topic Topic base para la publicaci�n
     */
    void setPublicationBase(const char* pub_topic);     


protected:
    
    /** Flags de tarea (asociados a la m�quina de estados) */
    enum SigEventFlags{
        DistEventFlag        =  (1<<0),
        InvalidDistEventFlag =  (1<<1),
        MeasureErrorEventFlag = (1<<2),
    };
      
	Thread _th;                         /// Hilo de ejecuci�n asociado
    char*  _sub_topic;                  /// Topic base para la suscripci�n
    char*  _pub_topic;                  /// Topic base para la publicaci�n
    char*  _pub_topic_unique;           /// Topic para publicar
    char*  _msg;
    bool   _ready;                      /// Flag de estado disponible
    Logger*     _debug;                 /// Canal de depuraci�n
    MQ::PublishCallback   _publCb;      /// Callback de publicaci�n en topics
    MQ::SubscribeCallback _subscrCb;    /// Callback de suscripci�n en topics
    HCSR04::DistEventCallback _distCb;  /// Callback de recepci�n de eventos de medida
    
    
    /** @fn task
     *  @brief Hilo de ejecuci�n asociado al manager
     */
    void task();
    

	/** publicationCb()
     *  Callback invocada al finalizar una publicaci�n
     *  @param topic Identificador del topic
     *  @param result Resultado de la publicaci�n
     */    
     void publicationCb(const char* topic, int32_t result);
    

	/** subscriptionCb()
     *  Callback invocada tras recibir una suscripci�n
     *  @param topic Identificador del topic
     *  @param msg Mensaje
     *  @param msg_len Tama�o del mensaje
     */    
     void subscriptionCb(const char* name, void* msg, uint16_t msg_len);       
    

	/** distEventCb()
     *  Callback invocada tras recibir una notificaci�n de distancia capturada
     *  @param ev Evento
     *  @param dist Distancia en cm (0,-1 para errores de medida)
     */    
     void distEventCb(HCSR04::DistanceEvent ev, int16_t dist);
};


#endif   /* __ProximityManager_H */
