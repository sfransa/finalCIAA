
/*==================[inclusions]=============================================*/
#include "os.h"               /* <= operating system header */
#include "ciaaPOSIX_stdio.h"  /* <= device handler header */
#include "ciaaPOSIX_string.h" /* <= string header */
#include "ciaak.h"            /* <= ciaa kernel header */
#include "final.h"         /* <= own header */

/*==================[macros and definitions]=================================*/

/*==================[internal data declaration]==============================*/

/*==================[internal functions declaration]=========================*/

/*==================[internal data definition]===============================*/

/** \brief File descriptor for digital output ports
 *
 * Device path /dev/dio/out/0
 */
static int32_t fd_out, fd_in;
uint8_t led, teclas;
int contador, frecuencia, contadorLedWhite, LedWhiteON;
static uint8_t estadoTeclas, teclasFlancoUP;

/*==================[external data definition]===============================*/

/*==================[internal functions definition]==========================*/

/*==================[external functions definition]==========================*/
/** \brief Main function
 *
 * This is the main entry point of the software.
 *
 * \returns 0
 *
 * \remarks This function never returns. Return value is only to avoid compiler
 *          warnings or errors.
 */
int main(void){
   /* Starts the operating system in the Application Mode 1 */
   /* This example has only one Application Mode */
   StartOS(AppMode1);

   /* StartOs shall never returns, but to avoid compiler warnings or errors
    * 0 is returned */
   return 0;
}

/** \brief Error Hook function
 *
 * This fucntion is called from the os if an os interface (API) returns an
 * error. Is for debugging proposes. If called this function triggers a
 * ShutdownOs which ends in a while(1).
 *
 * The values:
 *    OSErrorGetServiceId
 *    OSErrorGetParam1
 *    OSErrorGetParam2
 *    OSErrorGetParam3
 *    OSErrorGetRet
 *
 * will provide you the interface, the input parameters and the returned value.
 * For more details see the OSEK specification:
 * http://portal.osek-vdx.org/files/pdf/specs/os223.pdf
 *
 */
void ErrorHook(void){
   ciaaPOSIX_printf("ErrorHook was called\n");
   ciaaPOSIX_printf("Service: %d, P1: %d, P2: %d, P3: %d, RET: %d\n", OSErrorGetServiceId(), OSErrorGetParam1(), OSErrorGetParam2(), OSErrorGetParam3(), OSErrorGetRet());
   ShutdownOS(0);
}


//
void blink(uint8_t valor){
   uint8_t outputs;

   ciaaPOSIX_read(fd_out, &outputs, 1);
   outputs ^= valor;
   ciaaPOSIX_write(fd_out, &outputs, 1);
}

void cambiarLed(char accion){
  switch(accion){
    case 'i':
      switch (led){
        case VERDE: led = ROJO;
        break;
        case ROJO: led = AMARILLO;
        break;
        case AMARILLO: led = RGBVERDE;
        break;
        case RGBVERDE: led = RGBROJO;
        break;
        case RGBROJO: led = RGBAZUL;
        break;
        case RGBAZUL: led = VERDE;
        break; 
      }
    break;
    case 'd':
      switch (led){
        case VERDE: led = RGBAZUL;
        break;
        case RGBAZUL: led = RGBROJO;
        break;
        case RGBROJO: led = RGBVERDE;
        break;
        case RGBVERDE: led = AMARILLO;
        break;
        case AMARILLO: led = ROJO;
        break;
        case ROJO: led = VERDE;
        break; 
      }
    break;
  }
  ciaaPOSIX_write(fd_out, &led, 1);
}

void cambiarFrecuencia(char accion){
  switch(accion){
    case '-':
      if (frecuencia >= (MINFREC + INCFREC)) 
        frecuencia = frecuencia - INCFREC;
      else{ 
        frecuencia = MINFREC;   
        blink(RGBBLANCO);
        LedWhiteON = 1;        
      }
    break;
    case '+':
      if (frecuencia < (MAXFREC - INCFREC)) 
        frecuencia = frecuencia + INCFREC;
      else{ 
        frecuencia = MAXFREC;
        blink(RGBBLANCO);
        LedWhiteON = 1;  
      }
    break;
  }
}

void checkLedWhite(void){
   uint8_t outputs;
  
   if (LedWhiteON == 1){
     if (contadorLedWhite >= 100){
        blink(RGBBLANCO);

        contadorLedWhite = 0;

        LedWhiteON = 0;
     }
     else{
        contadorLedWhite += 10;
     }
   }
}

void checkLed(void){
   uint8_t outputs;
   
   if (contador >= frecuencia){
      //Blink selected led!
      blink(led);

      contador = 0;
   }
   else{
      contador += 10;
   }
}

void cambiar(uint8_t teclas){
   GetResource(BLOCK);

   uint8_t outputs;

   // tecla 1 y 2 cambia el led, el RGB pasa por todos, menos el blanco
   if (TECLA1 & teclas){
      cambiarLed('i');
   }

   if (TECLA2 & teclas){
      cambiarLed('d');
   }

   // tecla 3 y 4 cambia la frecuecia, si llega al minimo o maximo, prende blanco
   if (TECLA3 & teclas){
      cambiarFrecuencia('-');
   }

   // Led blinking frecuency increment
   if (TECLA4 & teclas){
      cambiarFrecuencia('+');
   }

   ReleaseResource(BLOCK);
}

uint8_t teclado_task(void){
   uint8_t inputs, ret;

   /* lee el estado de las entradas */
   ciaaPOSIX_read(fd_in, &inputs, 1);

   /* detecta flanco ascendente */
   teclasFlancoUP |= (inputs ^ estadoTeclas) & inputs;

   ret = teclasFlancoUP;
   
   teclasFlancoUP = 0;

   /* guarda el nuevo estado de las teclas */
   estadoTeclas = inputs;

   return ret;
}

/*uint8_t teclado_getFlancos(void){
   uint8_t ret = teclasFlancoUP;

   teclasFlancoUP = 0;

   return ret;
}*/

/** \brief Initial task
 *
 * This task is started automatically in the application mode 1.
 */
TASK(InitTask){
   uint8_t outputs;
   /* init CIAA kernel and devices */
   ciaak_start();

   /* print message (only on x86) */
   ciaaPOSIX_printf("Init Task...\n");

   /* open CIAA digital outputs */
   fd_out = ciaaPOSIX_open("/dev/dio/out/0", ciaaPOSIX_O_RDWR);
   fd_in = ciaaPOSIX_open("/dev/dio/in/0", ciaaPOSIX_O_RDWR);

   //led = 0B100000;
   led = VERDE;

   frecuencia = 200;   

   contador = 0;

   contadorLedWhite = 0;

   LedWhiteON = 0;

   //Write the led register
   ciaaPOSIX_write(fd_out, &led, 1);

   //no hay tareas ya que arrancan solas
  
   /* terminate task */
   TerminateTask();
}


TASK(LedsTask){
   checkLed();
   
   checkLedWhite();  

   TerminateTask();
}


TASK(TecladoTask){
   // lee los flancos de las teclas
   teclas = teclado_task();

   cambiar(teclas);

   TerminateTask();
}

/** @} doxygen end group definition */
/** @} doxygen end group definition */
/** @} doxygen end group definition */
/*==================[end of file]============================================*/

