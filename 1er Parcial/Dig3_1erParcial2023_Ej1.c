/*1er Parcial Electrónica Digital 3 2023
 *Ejercicio 1:
 *Utilizando Systick e interrupciones externas escribir un código en C que cuente
 *indefinidamente de 0 a 9. Un pulsador conectado a Eint0 reiniciará la cuenta a
 *0 y se mantendrá en ese valor mientras el pulsador se encuentre presionado. Un
 *pulsador conectado a Eint1 permitirá detener o continuar la cuenta cada vez
 *que sea presionado. Un pulsador conectado a Eint2 permitirá modificar la ve-
 *locidad de incremento del contador. En este sentido, cada vez que se presione
 *ese pulsador el contador pasará a incrementar su cuenta de cada 1 segundo a
 *cada 1 milisegundo y viceversa. Considerar que el microcontrolador se encuentra
 *funcionando con un reloj (cclk) de 16 Mhz. El código debe estar debidamente comentado
 *y los cálculos realizados claramente expresados. En la siguiente figura se muestra una
 * tabla que codifica el display y el esquema del hardware sobre el que funcionará el
 * programa.*/

/*RESUMEN DE LOS DATOS DE LA FIGURA
 * - P2.10 tiene un pulsador conesctado a VCC
 * - P2.11 y P2.12 tienen cada uno un pulsador conectado a masa
 * - El display es de 7 segmento de cátdo común por lo que para escribir cada dig se tiene:
 * 0-->0x3F
 * 1-->0x06
 * 2-->0x5B
 * 3-->0x4F
 * 4-->0x66
 * 5-->0x6D
 * 6-->0x7D
 * 7-->0x07
 * 8-->0x7F
 * 9-->0x67
 * Los segmentos del display estan conectados a los 7 bits menos significativos del PORT0
 * */

#ifdef __USE_CMSIS
#include "LPC17xx.h"
#endif

#include <cr_section_macros.h>

//Variables globales
uint32_t TickVal1 = 15999999;
uint32_t TickVal2 = 15999;
uint8_t NumDisplay[10]={0x3F,
						0x06,
						0x5B,
						0x4F,
						0x66,
						0x6D,
						0x7D,
						0x07,
						0x7F,
						0x67};
volatile uint8_t index =0;
volatile uint8_t StopFlag = 0;

//Prototipo de funciones
void cfgGPIO(void);
void cfgSysTick(uint32_t value);
void cfgEINT(void);


int main(void) {

    cfgGPIO();
    cfgSysTick(TickVal1);
    cfgEINT();

    while(1) {
    	LPC_GPIO0->FIOPIN0=NumDisplay[index];
    }
    return 0 ;
}

void cfgGPIO(void){

	//configuro pines P2.10 P2.11 y P2.12
	LPC_PINCON->PINSEL4 |= (0b010101<<20);				//Funcion 3 para 3 pines (todos EINT)
	LPC_PINCON->PINMODE4 |= (0b11<<20);					//Pull-down para P2.10
	//Supongo los otros dos pines con pull-up por defecto

	//configuro los 7 pines del port0 menos significativos
	LPC_GPIO0->FIODIR |= (0x7F<<0);						//Los declaro como OUTPUTS
	LPC_GPIO0->FIOMASK0 = 0x80; 						//Del byte menos significativo del port 0 enmascaro el pin7
	LPC_GPIO0->FIOCLR |= (0x7F);						//Inicializo con el display apagado

	return;
}

void cfgSysTick(uint32_t value){

	SysTick->LOAD = value;			//El valor inicial dependera del parametro de entrada
	Systick -> Val = 0;				//Current=0
	SysTick -> CTRL = 0x07;			//Selecciono clk source, habilito int. y doy inicio al Systick

	return;
}

void cfgEINT(void){

	//Configuro EINT0 (Asumo por defecto EINT0 sensible por nivel)
	LPC_SC->EXTPOLAR|=(1<<0); 					//Sensible a nivel LOW

	//Configuro EINT1 y EINT2
	LPC_SC->EXTMODE |=(3<<1); 					//Ambos comandados por flanco(de bajada por defecto)

	//Bajo las flags de las 3 interrupciones
	LPC_SC->EXTINT |= (0B111<<0);

	NVIC_SetPriority(EINT0_IRQn,1);
	NVIC_SetPriority(EINT1_IRQn,2);
	NVIC_SetPriority(EINT2_IRQn,3);
	NVIC_Enable(EINT0_IRQn);
	NVIC_Enable(EINT1_IRQn);
	NVIC_Enable(EINT2_IRQn);
	return;
}

void EINT0_IRQHandler(void){

	//Fuerzo el indice del array a 0 para que muestre el 0
	index =0;

	LPC_SC->EXTINT |= (1<<0); 					//Bajo la flag de EINT0
	return;
}

void EINT1_IRQHandler(void){
	//Togleo la variable para alternar entre Stop (=1) y Run (=0)
	StopFlag = (StopFlag+1)%2;

	LPC_SC->EXTINT |= (1<<1);					//Bajo la flag de EINT1
	return;
}

void EINT2_IRQHandler(void){

	volatile static uint8_t FrecChange = 0;
	FrecChange = (FrecChange+1)%2;


	if(FrecChange){
		cfgSysTick(TickVal2);					//Actualizo el display cada 1ms
	}
	else{
		cfgSysTick(TickVal1);					//Actualizo el display cada 1s
	}

	LPC_SC->EXTINT |= (1<<2);					//Bajo la flag de EINT1
	return;
}

void SysTick_Handler(void){

	if(!(LPC_GPIO2->FIOPIN&(1<<10))){

		if(StopFlag!=1){
			index = (index+1)%10;
		}
	}

	SysTick->CTRL &= SysTick->CTRL;
	return;
}




