#include "mbed.h"
#include "MQLib.h"
#include "MQSerialBridge.h"
#include "Logger.h"
#include "ProximityManager.h"


// **************************************************************************
// *********** DEFINICIONES *************************************************
// **************************************************************************


/** Macro de impresión de trazas de depuración */
#define DEBUG_TRACE(format, ...)    if(logger){Thread::wait(2); logger->printf(format, ##__VA_ARGS__);}


// **************************************************************************
// *********** OBJETOS  *****************************************************
// **************************************************************************

/** Canal de comunicación remota */
static MQSerialBridge* qserial;
/** Canal de depuración */
static Logger* logger;
/** Driver control detector */
static ProximityManager* distdrv;


// **************************************************************************
// *********** TEST  ********************************************************
// **************************************************************************



//------------------------------------------------------------------------------------
void distEvtSubscription(const char* name, void* msg, uint16_t msg_len){
    DEBUG_TRACE("%s %s\r\n", name, msg);
}


//------------------------------------------------------------------------------------
void test_ProximityManager(){
            
    // --------------------------------------
    // Inicia el canal de comunicación remota
    //  - Pines USBTX, USBRX a 115200bps y 256 bytes para buffers
    //  - Configurado por defecto en modo texto
    qserial = new MQSerialBridge(USBTX, USBRX, 115200, 256);
    

    // --------------------------------------
    // Inicia el canal de depuración (compartiendo salida remota)
    logger = (Logger*)qserial;    
    DEBUG_TRACE("\r\nIniciando test_ProximityManager...\r\n");


    // --------------------------------------
    // Creo driver de control para el medidor de distancia
    DEBUG_TRACE("\r\nCreando Driver de proximidad...");    
    distdrv = new ProximityManager(PA_0, PA_1);
    distdrv->setDebugChannel(logger);
    while(!distdrv->ready()){
        Thread::yield();
    }
    DEBUG_TRACE("OK!");    
    
    // situo todos a 0º y doy la orden sincronizada
    DEBUG_TRACE("\r\nAjustando distancias a 10cm... ");
    distdrv->config(10, 10, 3);
    
    // establezco topic base 'prox'
    distdrv->setPublicationBase("prox");
    distdrv->setSubscriptionBase("prox");
    
    DEBUG_TRACE("\r\nSuscripción a prox/sta/# ...");
    MQ::MQClient::subscribe("prox/sta/#", new MQ::SubscribeCallback(&distEvtSubscription));
    DEBUG_TRACE("OK!\r\n");
    
    // --------------------------------------
    // Arranca el test
    DEBUG_TRACE("\r\n...................INICIO DEL TEST.........................\r\n");    
    DEBUG_TRACE("\r\n- Ajustar eventos: prox/cmd/config D,I,O,F,R,Ei,Er");    
    DEBUG_TRACE("\r\n- Iniciar captura: prox/cmd/start T");    
    DEBUG_TRACE("\r\n- Detener captura: prox/cmd/stop 0\r\n");    
}

