/*Utilizando interrupciones por GPIO realizar un código en C que permita,
 *mediante 4 pines de entrada GPIO, leer y guardar un número compuesto por
 *4 bits. Dicho número puede ser cambiado por un usuario mediante 4 swit-
ches, los cuales cuentan con sus respectivas resistencias de pull up ex-
ternas. El almacenamiento debe realizarse en una variable del tipo array
de forma tal que se asegure tener disponible siempre los últimos 10 núme-
ros elegidos por el usuario, garantizando además que el número ingresado
más antiguo, de este conjunto de 10, se encuentre en el elemento 9 y el
número actual en el elemento 0 de dicho array. La interrupción por GPIO
empezará teniendo la máxima prioridad de interrupción posible y cada 200
números ingresados deberá disminuir en 1 su prioridad hasta alcanzar la
mínima posible. Llegado este momento, el programa deshabilitará todo tipo
de interrupciones producidas por las entradas GPIO. Tener en cuenta que el
código debe estar debidamente comentado.*/

#ifdef __USE_CMSIS
#include "LPC17xx.h"
#endif

#include <cr_section_macros.h>

//Defines
#define COUNT_NUM_MAX 200
#define MAX_PRIORITY 31
#define SIZE_OF_ARRAY 10

//Defino variables globales
uint8_t VectorNUM[10]={0};
volatile uint8_t Contador = 0;
volatile uint32_t PrioridadActual = 0;

//Prototipos de funciones
void cfgGPIO(void);
void cfgIntGPIO(void);


int main(void) {

	cfgGPIO();
	cfgIntGPIO();

    while(1) {

    }
    return 0 ;
}

void cfgGPIO(void){

	/*Configuro con 4 LSB del PORT0 para conectador a los SWITCH
	* Asumo q=configuracion por defecto GPIO, INPUT
	*/
	LPC_PINCON->PINMODE0 |= ((2<<6)|(2<<4)|(2<<2)|(2<<0)); 		//Desactivo sus pull-up/down
	return;
}

void cfgIntGPIO(void){

	LPC_GPIOINT->IO0IntEnR |= (0x0F<<0); 		//Habilito interrupciones por flanco ascendiente
	LPC_GPIOINT->IO0IntEnF |= (0x0F<<0);		//Habilito interrupciones por flanco descendiente
	LPC_GPIOINT->IO0IntClr |= (0x0F<<0);		//Limpio las flags

	NVIC_SetPriority(EINT3_IRQn,PrioridadActual);
	NVIC_EnableIRQ(EINT3_IRQn);

	return;
}

void EINT3_IRQHandler(void){
	volatile static uint8_t NumActual =0;

	NumActual = (LPC_GPIO0->FIOPIN&(0x0F<<0));					//Guardo el valor actual
	Contador = (Contador+1)%COUNT_NUM_MAX; 						//incremento el contador de valores en un buffer circular

	//Si ya guarde 200 valores
	if(Contador==0){
		PrioridadActual++; 										//Aumento la prioridad

		//Si aun no llegue a la menor prioridad
		if(PrioridadActual<MAX_PRIORITY){
			NVIC_SetPriority(EINT3_IRQn, PrioridadActual); 		//Disminuyo la prioridad
		}
		//Si ya llegue a la peor prioridad
		else{
			NVIC_DisableIRQ(EINT3_IRQn);						//Desactivo la interrupcion por GPIO
		}
	}

	//Actualizo lo valores del array
	for(uint8_t i =(SIZE_OF_ARRAY-1);i>0;i--){
		VectorNUM[i]=VectorNUM[i-1];
	}
		VectorNUM[0]=NumActual;

	LPC_GPIOINT->IO0IntClr |= (0x0F<<0);						//Bajo las flags
	return;
}


