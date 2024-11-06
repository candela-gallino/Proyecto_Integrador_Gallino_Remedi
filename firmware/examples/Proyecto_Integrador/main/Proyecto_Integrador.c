/*! @mainpage Dosificador de alimento para terneros 
 *
 * @section genDesc Descripcion General 
 *
 * Este proyecto permite abrir unas compuertas controlando la liberación de cantidad de alimento.
 *   Se debe tener en cuenta que, dependiendo del peso del animal, se le da una dosis que después
 *  puede ir variando. 
 *
 * @section changelog Canales y perifericos 
 * 
 * |    Peripheral  |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	Vcc      	| 	5V 		    |
 * | 	GND  	 	| 	GND     	|
 * | 	PIN_26	 	| 	GPIO_		|
 * | 	SG90 PWM	| 	GPIO_22		|
 * | 	SDK HX711   | 	GPIO_18		|
 * |    DT	HX711   | 	GPIO_19		|
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 02/10/2024 | Creacion del proyecto                      |
 * | 06/11/2024 | Finalizacion del proyecto                      |
 *
 * @author Gallino Candela y Remedi Juan Cruz  (candela.gallino@uner.edu.ar- juan.remedi@ingenieria.uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "hx711.h"
#include "servo_sg90.h"
#include "timer_mcu.h"
#include "gpio_mcu.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "led.h"
#include "neopixel_stripe.h"
#include "ble_mcu.h"
#include "delay_mcu.h"

#include "fft.h"
#include "iir_filter.h"
/*==================[macros and definitions]=================================*/
#define CONFIG_BLINK_PERIOD 500
#define LED_BT LED_1
#define BUFFER_SIZE 256
#define SAMPLE_FREQ 220
/**
 * @def DIFERENCIA_PESO_OBJETIVO
 * @brief Peso del alimento que se le debe dar al animal
 */
#define DIFERENCIA_PESO_OBJETIVO 40.0

// #define PESO_INICIAL 80.0
/*==================[internal data definition]===============================*/
gpio_t pd_sck = 1;				   // Pin de reloj
gpio_t dout = 2;				   // Pin de datos
float scale = 2.4;				   // Factor de escala determinado por calibración 
double offset = 0.0;			   // Valor de la tara
volatile float peso_actual = 0.0;  // Peso actual medido
volatile float peso_inicial = 0.0; // peso inicial
TaskHandle_t CELDA_DE_CARGA_TASKHANDLE = NULL;
TaskHandle_t SERVO_TASKHANDLE = NULL;

/*==================[internal functions declaration]=========================*/
/**
 * @brief Función de tarea para manejar la celda de carga, encargada de medir el peso
 * 
 * @param param Parámetro de la tarea (no utilizado)
 */
void celda_de_carga(void *param)
{
	peso_inicial = HX711_get_units(5);
	
	while (1)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		peso_actual = HX711_get_units(5);
        printf("la diferencia  es %f\n", peso_actual-peso_inicial);
		printf("el peso actual es %f\n", peso_actual);
	}
}
/**
 * @brief Función de tarea para mover el servo, permitiendo abrir y cerrar compuertas 
 * 
 * @param param Parámetro de la tarea (no utilizado)
 */
void mover_servo(void *param)
{
	while (1)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		// Verificamos si la diferencia es igual o mayor a 15kg

		if (peso_actual - peso_inicial <= DIFERENCIA_PESO_OBJETIVO)
		{ 

			ServoMove(SERVO_0, -45);
		}
		else 
		ServoMove(SERVO_0, 90);
	}
}
/**
 * @brief Función de retardo que notifica a las tareas
 */
void funcion_delay()
{
	vTaskNotifyGiveFromISR(CELDA_DE_CARGA_TASKHANDLE, pdFALSE);
	vTaskNotifyGiveFromISR(SERVO_TASKHANDLE, pdFALSE);
}
/*==================[external functions definition]==========================*/

/**
 * @brief Función principal de la aplicación
 */
void app_main(void)
{
	HX711_Init(128, GPIO_18, GPIO_19);
	HX711_tare(5);
	HX711_tare(5);
	HX711_setScale(scale);
	ServoInit(SERVO_0, GPIO_22);
	ServoMove(SERVO_0, 90);

	timer_config_t timer_delay =
		{
			.timer = TIMER_A,
			.period = 1000000,
			.func_p = funcion_delay,
			.param_p = NULL,
		};
	TimerInit(&timer_delay);

	xTaskCreate(&celda_de_carga, "celda de carga", 2048, NULL, 5, &CELDA_DE_CARGA_TASKHANDLE);
	xTaskCreate(&mover_servo, "servo compuerta", 2048, NULL, 5, &SERVO_TASKHANDLE);

	TimerStart(timer_delay.timer);
}

/*==================[end of file]============================================*/
