/*1er Parcial Electrónica digital 3 2024
 *
 * Ejercicio 2:
 * En una fábrica, hay un sistema de alarma utilizando una LPC1769
 * Rev. D trabajando a una frecuencia de CCLK a 100 [MHz], conecta-
 * do a un sensor de puerta que se activa cuando la puerta se abre.
 * El sensor está conectado al pin P0, el cual genera una interrup-
 * ción externa (EINT) cuando se detecta una apertura (cambio de
 * estado). Al detectar que la puerta se ha abierto, el sistema de-
 * be iniciar un temporizador utilizando el Systick para contar un
 * período de 30 segundos.​

 *Durante estos 30 segundos, el usuario deberá introducir un código
 *de desactivación mediante un DIP switch de 4 entradas conectado a
 *los pines P2 - P2. El código correcto es 0xAA (1010 en binario).
 *El usuario tiene dos intentos para introducir el código correcto.
 *Si después de dos intentos el código ingresado es incorrecto, la
 *alarma se activará, encendiendo un buzzer conectado al pin P1.
 * */
/*----------Compresión del enunciado y modificaciones:----------------
 *
 * 1)El sonsor de la puerto no puede ser P0.6 ya que no corresponde a
 * un pin de EINT. Por lo tanto se usará el P2.10 (EINT0).
 *
 * 2)Con un CCLK en tiempo de delay máximo del Systick es de 167ms.
 * Se toma como diseño un valor de delay de 100ms para que el Systick
 * tenga que interrumpir 300 veces para generar un delay total de 30 seg.
 *
 * 3)El buzzer debería estar conectado a P1.11 pero es un pin noaccesible
 * por lo tanto se usará el pin P0.0.
 *
 * 4)El enunciado dice que el usuario dentro de los 30 segundos tiene dos
 * intentos para ingresar la clave y desactivar la alarma. Sin embargo no
 * especfica como el sistema puede verificar dicha clave. Por lo tanto se
 * implementará un botón extra para que el usuario presione despues de in-
 * troducir la clave en cada intento. De esta forma el sistema podrá detec-
 * tar cuando el usuario realmente realizó un intento de desactivación y
 * validarlo.El boton se implementará en el P2.4. y se lo toma como un boton
 * conectado a Vcc y a la placa
 *
 * 5)Considero el DIPSWITCH conectado a VCC, por lo que apretar cada switch
 * del mismo representa poner en HIGH de la LPC
 *
 */



#ifdef __USE_CMSIS
#include "LPC17xx.h"
#endif

#include <cr_section_macros.h>

//Defines y macros
#define CLAVE (uint8_t)0x0A

//Varibles Globales
uint32_t TickVal = 9999999;
uint16_t StopValue = 300;
uint8_t ClaveAttempt =0;
volatile uint8_t FlagIntento=1;
volatile uint16_t nTicks = 0;
volatile uint8_t FlagOpenDoor = 0;
volatile uint8_t ValidationAttempt =0;


//Prototipo de funciones
void cfgGPIO(void);
void cfgSysTick(uint8_t value);
void cfgEINT(void);
void cfgIntGPIO(void);



int main(void) {

    cfgGPIO();
    cfgSysTick(0X04);
    cfgEINT();
    cfgIntGPIO();

    while(1) {

    }
    return 0 ;
}

void cfgGPIO(void){
	//Configuro P2.10 (SENSOR PUERTA)
	LPC_PINCON->PINSEL4 |=(1<<20);				//Funcion=EINT0
	LPC_PINCON->PINMODE4 |=(3<<20); 			//pull-down ON

	//Configuro P0.0 (Buzzer)
	LPC_GPIO0->FIODIR |=(1<<0);					//OUTPUT
	LPC_GPIO0->FIOCLR |=(1<<0);					//Buzzer OFF

	//Configuro por pines del DIPSWITCH
	LPC_PINCON->PINMODE4 |= (0xFF<<0);			//Pulldown para los 4 SWITCH

	//Configuro el boton extra
	LPC_PINCON->PINMODE4 |= (3<<8);				//Pulldown para P2.4

	/*Considero el resto de configuraciones por defecto (Boton, Buzzer y Dipswitch
	como GPIO) y tambien el boton y el dipswitch como INPUT*/
	return;
}

void cfgSysTick(uint8_t value){

	SysTick->LOAD = TickVal;					//Systick configurado para desbordar cada 100ms
	SysTick->VAL =0;							//Current = 0
	SysTick->CTRL= value;						//Control dependiendo del parametro de entrada
	return;
}

void cfgEINT(void){

	LPC_SC->EXTMODE |= (1<<0); 					//Sensible a flaco
	LPC_SC->EXTPOLAR|=(1<<0);					//De subida
	LPC_SC->EXTINT |=(1<<0);					//Bajo la flag de EINT0

	NVIC_SetPriority(EINT0_IRQn,1);
	NVIC_EnableIRQ(EINT0_IRQn);

	return;
}

void cfgIntGPIO(void){

	LPC_GPIOINT->IO2IntEnF &= ~(1<<4); 			//Deshabilito las interrupciones por flanco de bajada en P2.4
	LPC_GPIOINT->IO2IntEnR |= (1<<4);		//Habilito las interrupciones por flanco de subida en P2.4
	LPC_GPIOINT->IO2IntClr |= (1<<4);           //Limpio la flag
	NVIC_SetPriority(EINT3_IRQn,2);
	NVIC_EnableIRQ(EINT3_IRQn);				//Habilito las interrupcion en el NVIC
	return;
}

void EINT0_IRQHandler(void){

	cfgSysTick(0x07); 							//Activo el Systick(clk source, interrup e inicio)
	FlagOpenDoor=1;


	LPC_SC->EXTINT|=(1<<0); 					//Bajo la flag
	return;
}

void EINT3_IRQHandler(void){
	//Si estoy en los 30seg y no gaste mis dos intentos
	if((FlagOpenDoor)&&(FlagIntento)){
		ClaveAttempt = (LPC_GPIO2->FIOPIN&(0x0F<<0)); 		//Guardo el valor introducido en el DIPSWITCH

			//Si tengo intentos
			if(ValidationAttempt<=1){
					//Si no es la clave
					if(ClaveAttempt!=CLAVE){
							ValidationAttempt++;			//Registro el intento
						}
					//Si es la clave doy acceso
					else{
						LPC_GPIO0->FIOCLR |=(1<<0);			//Buzzer OFF
						cfgSysTick(0x04);					//Paro el Systick
					}
					}
			//Si ya intente mas de dos veces
			else{
				FlagIntento=0;								//Me quedo sin intentos
				LPC_GPIO0->FIOSET |=(1<<0);					//Buzzer ON
				cfgSysTick(0x04);							//Paro el Systick

	}
	}

	LPC_GPIOINT->IO2IntClr|=(1<<4); 			//Bajo la flag
	return;
}

void SysTick_Handler(void){

		//si no han pasado los 30 seg
		if(nTicks<StopValue){
			nTicks++;								//Sigo contando
		}
		//Si me quede sin tiempo
		else{
			LPC_GPIO0->FIOSET |=(1<<0);				//Buzzer ON
			cfgSysTick(0x04);						//Paro el Systick
		}

	SysTick->CTRL &= SysTick->CTRL;					//Bajo la flag
	return;
}






