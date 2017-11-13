#include "mbed.h"
#include "MQLib.h"
#include "MQSerialBridge.h"
#include "Logger.h"
#include "ServoManager.h"

// **************************************************************************
// *********** DEFINICIONES *************************************************
// **************************************************************************


/** Macro de impresión de trazas de depuración */
#define DEBUG_TRACE(format, ...)    if(logger){Thread::wait(10); logger->printf(format, ##__VA_ARGS__);}


// **************************************************************************
// *********** OBJETOS  *****************************************************
// **************************************************************************

/** Canal de comunicación remota */
static MQSerialBridge* qserial;
/** Canal de depuración */
static Logger* logger;
/** Control de servos */
static ServoManager* servoman;
/** Número de servos máximo */
static const uint8_t SERVO_COUNT = 3;




// **************************************************************************
// *********** TEST  ********************************************************
// **************************************************************************


//------------------------------------------------------------------------------------
void test_ServoManager(){
            
    // --------------------------------------
    // Inicia el canal de comunicación remota
    //  - Pines USBTX, USBRX a 115200bps y 256 bytes para buffers
    //  - Configurado por defecto en modo texto
    qserial = new MQSerialBridge(USBTX, USBRX, 115200, 256);
    

    // --------------------------------------
    // Inicia el canal de depuración (compartiendo salida remota)
    logger = (Logger*)qserial;    
    DEBUG_TRACE("\r\nIniciando test_ServoManager...\r\n");

    // --------------------------------------
    // Creo manager de control para los servos
    //  - Dirección I2C = 0h
    //  - Número de servos controlables = 3 (0 al 2)    
    DEBUG_TRACE("\r\nCreando ServoManager...");    
    servoman = new ServoManager(PB_7, PB_6, SERVO_COUNT);
    servoman->setDebugChannel(logger);
    
    // espero a que esté listo
    DEBUG_TRACE("\r\n¿Listo?... ");
    do{
        Thread::yield();
    }while(!servoman->ready());
    DEBUG_TRACE(" OK");
    
    // establezco rangos de funcionamiento
    DEBUG_TRACE("\r\nAjustando rangos... ");
    for(uint8_t i=0;i<SERVO_COUNT;i++){
        if(servoman->setServoRanges(i, 0, 180, 1000, 2000) != PCA9685_ServoDrv::Success){
            DEBUG_TRACE("ERR_servo_%d\r\n...", i);
        }            
    }
    DEBUG_TRACE("OK");
    
    // situo todos a 0º y doy la orden sincronizada
    DEBUG_TRACE("\r\nGirando servos a 0º... ");
    for(uint8_t i=0;i<SERVO_COUNT;i++){
        if(servoman->setServoAngle(i, 0) != PCA9685_ServoDrv::Success){
            DEBUG_TRACE("ERR_servo_%d\r\n...", i);
        }               
    }
    if(servoman->updateAll() != PCA9685_ServoDrv::Success){
        DEBUG_TRACE("ERR_update");
    }                   
    DEBUG_TRACE("OK");
    
    // establezco topic base 'breathe'
    servoman->setSubscriptionBase("breathe");
    
    // --------------------------------------
    // Arranca el test
    DEBUG_TRACE("\r\n...INICIO DEL TEST...\r\n");    
    DEBUG_TRACE("\r\nComandos MQTT disponibles:");    
    DEBUG_TRACE("\r\n- Mover servoX Dº:      breathe/cmd/servo X,DEG");    
    DEBUG_TRACE("\r\n- Respiracion servos:   breathe/cmd/move Tus,Nstep,SrvOrg,StDif");    
    
}

