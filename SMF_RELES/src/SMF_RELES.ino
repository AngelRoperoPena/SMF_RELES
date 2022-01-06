#include <Wire.h>
#include <Time.h>

#include "hardware.h"
#include "maqsec.h"
#include "esp_system.h"
#include "Lora_imp.h"
#include "datos.h"
#include "protocolo.h"
#include "AnalogInput.h"
#include "xxtea_Easy.h"

unsigned long t_actual, T_secundario;

static void inicializacion_Pines (void);

void setup() {
  Serial.begin (115200);
  inicializacion_Pines ();
  datos_IniciarEEPROM ();
  //datos_Reset_MemoriaEEPROM ();
  datos_Parametrizar_MemoriaEEPROM ();
  maqsec_IniciacionVbles ();                        //Initias all variables of State Machine.
  Lora_imp_setup ();
  protocolo_Init ();
  AnalogInput_setup ();
  xxtea_Easy_setup ();
}
 
void loop() {
  unsigned long T_actual;
  int comando_LoRa = LORA_NULL;

  T_actual = millis ();                             //Captura de los milisegundos transcurridos.
  //AnalogInput_loop (T_actual);
  //Lora_imp_loop (T_actual);
  comando_LoRa = Lora_imp_CheckPacket ();
  maqsec_SECUNDARIA (T_actual, comando_LoRa);
}
void inicializacion_Pines (void) {
  //pinMode       (ON_OFF,      INPUT);
  pinMode       (BOTON,               INPUT);
  pinMode       (EV_LLENADO,          OUTPUT);
  digitalWrite  (EV_LLENADO,          NIVEL_BAJO);        //Pulso para apagar el FOCOIRIS.
  pinMode       (EV_VACIADO,          OUTPUT);
  digitalWrite  (EV_VACIADO,          NIVEL_BAJO);        //Pulso para apagar el FOCOIRIS.
  pinMode       (EV_PERSIANA_ESTIRAR, OUTPUT);
  digitalWrite  (EV_PERSIANA_ESTIRAR, NIVEL_BAJO);        //Pulso para apagar el FOCOIRIS.
  pinMode       (EV_PERSIANA_RECOGER, OUTPUT);
  digitalWrite  (EV_PERSIANA_RECOGER, NIVEL_BAJO);        //Pulso para apagar el FOCOIRIS.
  pinMode       (EV_BOMBA_VAC,        OUTPUT);
  digitalWrite  (EV_BOMBA_VAC,        NIVEL_BAJO);        //Pulso para apagar el FOCOIRIS.
  pinMode       (LED_BOTON,           OUTPUT);
  digitalWrite  (LED_BOTON,           LOW);        //Pulso para apagar el FOCOIRIS.
}

 
