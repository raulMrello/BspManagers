/*
 * ProximityManager.h
 *
 *  Created on: Nov 2017
 *      Author: raulMrello
 *
 *  ProximityManager es un módulo C++ que gestiona la detección de objetos cercanos mediante el sensor HC-SR04 de ultrasonidos.
 *  Requiere un par de gpios (salida digital y entrada de interrupción) para el driver HCSR04.
 *
 *  Este gestor puede trabajar como thread independiente o formar parte de threads auxiliares para procesar trabajos puntuales
 *  mediante el servicio <job>
 *
 *  Se puede configurar un par de topics base, para la suscripción $(sub_topic) y para la publicación $(pub_topic). Teniendo esto
 *  en cuenta, las órdenes que acepta el módulo son las siguientes:
 *
 *  Suscripción:
 *      $(sub_topic)/cmd/config D,I,O,F,R,Ei,Er
 *      Permite ajustar los siguientes parámetros:
 *      D: Rango de detección máxima en cm. Por encima de ese valor, no lo tendrá en cuenta.
 *      I: Diferencia en cm entre medidas consecutivas para notificar event de acercamiento.
 *      O: Diferencia en cm entre medidas consecutivas para notificar event de alejamiento.
 *      F: Número de muestras acumuladas para el filtro anti-glitch.
 *      R: Ventana en cm para que el filtro anti-glitch considere dos medidas como similares
 *      Ei:Flag para habilitar o no los eventos de captura instantáneos.
 *      Ei:Flag para habilitar o no los eventos de errores durante la captura.
 *      Por ejemplo, para detectar cambios superiores a 10cm al acercarse y 20cm al alejarse, con un filtro de 3 muestras, y 50cm 
 *      como rango para las muestras del filtro,con todos los eventos activos, se publicaría lo siguiente: 
 *                  $(sub_topic)/cmd/dist 10,20,3,50,1,1
 *
 *      $(sub_topic)/cmd/start T,t
 *      Permite iniciar la captura con una cadencia T(ms) y un timeout de medida de t(ms)
 *
 *      $(sub_topic)/cmd/stop 0
 *      Permite detener la captura
 *
 *  Publicación:
 *      $(sub_topic)/sta/dist E,D
 *      Permite notificar eventos de estado indicando E(tipo de evento: 0 si se acerca, 1 si se aleja, 2 error en medida) y 
 *      D(distancia en cm), así para notificar que un objeto se aproxima y que está a 20cm se publicará: $(pub_topic)/stat/dist 0,20 
 *
 *  NOTA: Esta es la configuración para una medida constante con resolución adecuada:
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
     *  @brief Constructor, que asocia una salida digital y una interrupción entrante
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
    
  
	/** setSubscriptionBase()
     *  Registra el topic base a los que se suscribirá el módulo
     *  @param sub_topic Topic base para la suscripción
     */
    void setSubscriptionBase(const char* sub_topic);          
    
  
	/** setPublicationBase()
     *  Registra el topic base a los que publicará el módulo
     *  @param pub_topic Topic base para la publicación
     */
    void setPublicationBase(const char* pub_topic);     


protected:
    
    /** Flags de tarea (asociados a la máquina de estados) */
    enum SigEventFlags{
        DistEventFlag        =  (1<<0),
        InvalidDistEventFlag =  (1<<1),
        MeasureErrorEventFlag = (1<<2),
    };
      
	Thread _th;                         /// Hilo de ejecución asociado
    char*  _sub_topic;                  /// Topic base para la suscripción
    char*  _pub_topic;                  /// Topic base para la publicación
    char*  _pub_topic_unique;           /// Topic para publicar
    char*  _msg;
    bool   _ready;                      /// Flag de estado disponible
    Logger*     _debug;                 /// Canal de depuración
    MQ::PublishCallback   _publCb;      /// Callback de publicación en topics
    MQ::SubscribeCallback _subscrCb;    /// Callback de suscripción en topics
    HCSR04::DistEventCallback _distCb;  /// Callback de recepción de eventos de medida
    
    
    /** @fn task
     *  @brief Hilo de ejecución asociado al manager
     */
    void task();
    

	/** publicationCb()
     *  Callback invocada al finalizar una publicación
     *  @param topic Identificador del topic
     *  @param result Resultado de la publicación
     */    
     void publicationCb(const char* topic, int32_t result);
    

	/** subscriptionCb()
     *  Callback invocada tras recibir una suscripción
     *  @param topic Identificador del topic
     *  @param msg Mensaje
     *  @param msg_len Tamaño del mensaje
     */    
     void subscriptionCb(const char* name, void* msg, uint16_t msg_len);       
    

	/** distEventCb()
     *  Callback invocada tras recibir una notificación de distancia capturada
     *  @param ev Evento
     *  @param dist Distancia en cm (0,-1 para errores de medida)
     */    
     void distEventCb(HCSR04::DistanceEvent ev, int16_t dist);
};


#endif   /* __ProximityManager_H */
