/*
 * ServoManager.h
 *
 *  Created on: Nov 2017
 *      Author: raulMrello
 *
 *	ServoManager es el módulo encargado de gestionar los servos por medio del driver PCA9685 y proporcionar funcionalidades
 *  de alto nivel, en el contexto de tarea.
 *
 *  El uso de este manager a más alto nivel puede ser, mediante la librería MQLib, mediante la instalación de los topics
 *  de publicación correspondientes y/o por medio de callbacks dedicadas.
 *
 *  Los topics en los que escucha este módulo son los siguientes: 
 *
 *  ${pub_topic}/cmd/servo S,A
 *      Mueve el servo S al ángulo A (limitado por rangos min,max)
 *
 *  ${pub_topic}/cmd/duty S,D
 *      Mueve el servo S al duty D (sin limitación por rango)
 *
 *  ${pub_topic}/cmd/move/start StepTimeUs,NumSteps,ServoOrigin,StepDif,AngIni,AngEnd
 *      Genera un patrón de movimiento senoidal(-1,1,-1) con una cadencia de paso StepTimeUs a completar en NumSteps pasos y 
 *      centrado en el servo ServoOrigin. Los servos adyacentes replican el movimiento variando StepDif pasos del servo
 *      origen.
 *
 *  ${pub_topic}/cmd/move/stop 0
 *      Detiene el patrón de movimiento
 *
 *  ${pub_topic}/cmd/info S
 *      Obtiene información sobre el servo S
 *
 *  ${pub_topic}/cmd/cal S,Ai,Af,Di,Df
 *      Calibra los rangos del servo S, con ángulo minmax Ai,Af y duty minmax Di,Df.
 *
 *  ${pub_topic}/cmd/save 0
 *      Guarda los datos de calibración de todos los servos en NVFlash
 */
 
#ifndef __ServoManager__H
#define __ServoManager__H

#include "mbed.h"

/** Librerías relativas a módulos software */
#include "MQLib.h"
#include "Logger.h"
#include "PCA9685_ServoDrv.h"
#include "NVFlash.h"


   
class ServoManager : public PCA9685_ServoDrv{
  public:

  
    /** Constructor
     *  Asocia los pines gpio para el driver implementado. Utiliza la dirección i2c por defecto
     *  @param sda Línea sda del bus i2c
     *  @param scl Línea scl del bus i2c
     *  @param num_servos Número de servos
     */
    ServoManager(PinName sda, PinName scl, uint8_t num_servos);
  
  
    /** Destructor */
    ~ServoManager();
    
  
	/** setDebugChannel()
     *  Instala canal de depuración
     *  @param dbg Logger
     */
    void setDebugChannel(Logger* dbg){ _debug = dbg; }
    
  
	/** setSubscriptionBase()
     *  Registra el topic base a los que se suscribirá el módulo
     *  @param sub_topic Topic base para la suscripción
     */
    void setSubscriptionBase(const char* sub_topic);  
    
  
	/** startMovement()
     *  Establece un movimiento repetitivo
     *  @param duty Array de movimientos en cuentas pwm
     *  @param steps Número de pasos en el movimiento
     *  @param step_tick_us Tiempo entre paso y paso en us
     *  @param servo_zero Servo que inicia el movimiento. 
     *  @param step_dif Diferencia de paso entre servos adyacentes
     */
    void startMovement(uint16_t* duty, uint8_t steps, uint32_t step_tick_us, uint8_t servo_zero, uint8_t step_dif);
    
  
	/** stopMovement()
     *  Detiene el movimiento repetitivo
     */
    void stopMovement();  

        
    /** Devuelve el estado del driver
     *  @return Estado del chip
     */
    bool ready() { return ((PCA9685_ServoDrv::getState() == PCA9685_ServoDrv::Ready)? true : false); }    
    
  
	/** getNVDataSize()
     *  Obtiene el tamaño de los datos NV
     */
    uint32_t getNVDataSize(){ return(sizeof(NVData_t)); }
    
  
	/** setNVData()
     *  Actualiza los datos NVData
     *  @param data Datos de recuperación
     *  @return 0 si es correcto, !=0 si los datos son incorrectos o inválidos
     */
    int setNVData(void* data);
    
  
	/** getNVData()
     *  Lee los datos NVData
     *  @param data Recibe los datos NV de recuperación
     */
    void getNVData(uint32_t* data); 
    
    
  protected:
      
    /** Flags de tarea (asociados a la máquina de estados) */
    enum SigEventFlags{
        TickMoveFlag  = (1<<0),
    };
      
    /** Estructura de ejecución de movimientos repetitivos */
    struct RepetitiveMovement_t{
        uint16_t *duty;
        uint8_t steps;
        uint32_t step_tick_us;
    };
    
    /** Estructura de datos con los parámetros de calibración */    
    struct NVData_t{
        int16_t minAngle[PCA9685_ServoDrv::ServoCount];
        int16_t maxAngle[PCA9685_ServoDrv::ServoCount];
        uint16_t minDuty[PCA9685_ServoDrv::ServoCount];
        uint16_t maxDuty[PCA9685_ServoDrv::ServoCount];
        uint32_t crc32;
    };
    
    Thread      _th;                    /// Manejador del thread
    uint32_t    _timeout;               /// Manejador de timming en la tarea
    char*       _sub_topic;             /// Topic base para la suscripción
    Logger*     _debug;                 /// Canal de depuración
    uint8_t     _num_servos;            /// Número de servos
    RepetitiveMovement_t _rmove;        /// Movimiento repetitivo
    uint8_t _move_step[PCA9685_ServoDrv::ServoCount];
    Ticker _tick_move;

    MQ::SubscribeCallback     _subscrCb;    /// Callback de suscripción en topics
    
	/** task()
     *  Hilo de ejecución del protocolo 
     */
    void task();
  
    
	/** onIrqCb()
     *  Callback invocada tras recibir un evento de temporización
     */
    void onTickCb();        
    

	/** subscriptionCb()
     *  Callback invocada tras recibir una suscripción
     *  @param topic Identificador del topic
     *  @param msg Mensaje
     *  @param msg_len Tamaño del mensaje
     */    
     void subscriptionCb(const char* name, void* msg, uint16_t msg_len);    
    
};
     
#endif /*__ServoManager__H */

/**** END OF FILE ****/


