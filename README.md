# BspManagers

BspManagers es una librería orientada a MBED, que proporciona una serie de manejadores de alto nivel, para
uso avanzado de drivers. Generalmente estos módulos hacen uso de servicios del sistema operativo (tareas,
señales, etc...), así como de otras librerías como MQLib (publicación-suscripción) proporcionando una gestión
de los eventos de los drivers en un contexto de tarea (fuera de las ISRs)


  
## Changelog

----------------------------------------------------------------------------------------------
##### 02.11.2017 ->commit:"Incluyo managers por defecto"
- [x] Incluyo los drivers que ya tengo hechos, que son:
	* TouchManager: Gestor que implementa la parte de alto nivel del driver MPR121_CapTouch I2C
	* CryptoManager: Gestor que implementa funcionalidades de acceso a un chip criptográfico I2C
	* ConsoleManager: Gestor que implementa un terminal remoto de comandos mediante un canal serie UART
	* FSManager: Gestor que implementa un manejador del sistema de ficheros
	* RelayManager: Gestor que maneja la activación de varios relés, sincronizados con paso por cero 
	

