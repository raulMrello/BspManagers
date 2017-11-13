#include "mbed.h"
#include "MQLib.h"
#include "MQSerialBridge.h"
#include "Logger.h"
#include "ServoManager.h"

// **************************************************************************
// *********** DEFINICIONES *************************************************
// **************************************************************************


/** Macro de impresi�n de trazas de depuraci�n */
#define DEBUG_TRACE(format, ...)    if(logger){Thread::wait(10); logger->printf(format, ##__VA_ARGS__);}


// **************************************************************************
// *********** OBJETOS  *****************************************************
// **************************************************************************

/** Canal de comunicaci�n remota */
static MQSerialBridge* qserial;
/** Canal de depuraci�n */
static Logger* logger;
/** Control de servos */
static ServoManager* servoman;
/** N�mero de servos m�ximo */
static const uint8_t SERVO_COUNT = 3;




// **************************************************************************
// *********** TEST  ********************************************************
// **************************************************************************


//------------------------------------------------------------------------------------
void test_ServoManager(){
            
    // --------------------------------------
    // Inicia el canal de comunicaci�n remota
    //  - Pines USBTX, USBRX a 115200bps y 256 bytes para buffers
    //  - Configurado por defecto en modo texto
    qserial = new MQSerialBridge(USBTX, USBRX, 115200, 256);
    

    // --------------------------------------
    // Inicia el canal de depuraci�n (compartiendo salida remota)
    logger = (Logger*)qserial;    
    DEBUG_TRACE("\r\nIniciando test_ServoManager...\r\n");

    // --------------------------------------
    // Creo manager de control para los servos
    //  - Direcci�n I2C = 0h
    //  - N�mero de servos controlables = 3 (0 al 2)    
    DEBUG_TRACE("\r\nCreando ServoManager...");    
    servoman = new ServoManager(PB_7, PB_6, SERVO_COUNT);
    servoman->setDebugChannel(logger);
    
    // espero a que est� listo
    DEBUG_TRACE("\r\n�Listo?... ");
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
    
    // situo todos a 0� y doy la orden sincronizada
    DEBUG_TRACE("\r\nGirando servos a 0�... ");
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
    DEBUG_TRACE("\r\n- Mover servoX D�:      breathe/cmd/servo X,DEG");    
    DEBUG_TRACE("\r\n- Respiracion servos:   breathe/cmd/move Tus,Nstep,SrvOrg,StDif");    
    
}

