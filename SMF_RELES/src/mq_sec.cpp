/*******************************************************************************
   File   : mqprin.c  Module that integers the functions of machine states
********************************************************************************
   Description:In this module is called the functions of machine states.
********************************************************************************
   Creation of module: 25 of Apr of 2006
   Last modification:
   Name of creator of module: Ángel Ropero
   Software P2100
   Links with another files. mqprin.c.
   Company: HYC
*******************************************************************************/
/*** Includes ***/
#include <string.h>
#include <stdlib.h>
#include <advancedSerial.h>

#include "mq_sec.h"
#include "maqsec.h"
#include "datos.h"
#include "hardware.h"
#include "protocolo.h"
#include "Lora_imp.h"

#define VACIO     0

//-------------------- GLOBAL VARIABLES AL MODULO ------------------------------
extern MAQFLAGSEC maqsec_bits;
_MQSEC_FLAGS maqflags;
unsigned long T_anterior, timeout_ACK = 0;
int Last_status = INICIAL;

//-------------------- GLOBAL FUNCTIONS ----------------------------------------
static int mqsec_Chech_SN_PCBRELES (void);
static void mqsec_Enviar_COMANDO_LORA (int Comando_Enviar);
static void mqsec_Activar_LEDBOTON (void);
static void mqsec_Activar_RELE (int rele, bool nivel);
//------------------------------------------------------------------------------
//------------------- STATES OF FUNCIONALITIES ---------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// FUNCTION: short mqsec_EstINICIAL_SEC (void)
// DESCRIPTION: Function that executes the functionalities of INICIAL state.
// PARAMETERS: Void.
// RETURNS:  0.
//------------------------------------------------------------------------------
short mqsec_EstINICIAL (unsigned long T_actual) {          //Este estado le dejamos ahí por si hiciera falta
  
  if (maqsec_bits.flag_1A_vez == false) {   //First time that is entried to state.
    maqsec_SetFlag1A ();               //Set the flag of first time.
    T_anterior = T_actual;
    Serial.println (F("ESTADO INICIAL"));
    Last_status = INICIAL;
    mqsec_Activar_LEDBOTON ();
  }
  //------------------------------------ PROCCESS --------------------------------
  int result = mqsec_Chech_SN_PCBRELES ();
  if (result == 0)  maqsec_ActivarCondicion (CS1);   //Enviar al estado de VINCULAR.
  else              maqsec_ActivarCondicion (CS0);   //Enviar al estado de REPOSO.
  return 0;
}

//------------------------------------------------------------------------------
// FUNCTION: short mqsec_EstESPERA (void)
// DESCRIPTION: Function that executes the functionalities of REPOSO state.
// PARAMETERS: Void.
// RETURNS:  0.
//------------------------------------------------------------------------------
short mqsec_EstREPOSO (unsigned long T_actual, int comando_LoRa) {
  //byte valor_EEPROM;

  if (maqsec_bits.flag_1A_vez == false) {   //First time that is entried to state.
    maqsec_SetFlag1A ();               //Set the flag of first time.
    T_anterior = T_actual;
    Serial.println (F("ESTADO REPOSO: "));
    Last_status = REPOSO;
    mqsec_Chech_SN_PCBRELES ();
    mqsec_Activar_RELE (EV_LLENADO          , HIGH);
    mqsec_Activar_RELE (EV_VACIADO          , HIGH);
    mqsec_Activar_RELE (EV_PERSIANA_ESTIRAR , HIGH);
    mqsec_Activar_RELE (EV_PERSIANA_RECOGER , HIGH);
    mqsec_Activar_RELE (EV_BOMBA_VAC        , HIGH);
  }
  //------------------------------------ PROCCESS --------------------------------
  if (comando_LoRa == LORA_PING) {
    maqsec_ActivarCondicion (CS2);      //Enviar al Estado de PING para responder con el ACK.
  }
  return 0;
}
//------------------------------------------------------------------------------
// FUNCTION: short mqsec_EstCALLADO (void)
// DESCRIPTION: Function that executes the functionalities of REPOSO state.
// PARAMETERS: Void.
// RETURNS:  0.
//------------------------------------------------------------------------------
short mqsec_EstVINCULAR (unsigned long T_actual, int comando_LoRa) {

  if (maqsec_bits.flag_1A_vez == false) {   //First time that is entried to state.
    maqsec_SetFlag1A ();               //Set the flag of first time.
    T_anterior = T_actual;
    Serial.println (F("ESTADO VINCULAR"));
    Last_status = VINCULAR;
  }
  if (comando_LoRa == LORA_VINCULAR) {
    maqsec_bits.flag_vincular = true;
    int result = mqsec_Chech_SN_PCBRELES ();
    if (result == 0) {
      Serial.println (F("¡¡¡¡ GRABAR SN_RECEIVER !!!!"));
      lora_imp_GRABAR_SNRECEIVER ();
      maqsec_ActivarCondicion (CS2);        //Enviar al Estado ACK para responder con el ACK.
      Last_status = REPOSO;
    } else {
      Serial.println (F("¡¡¡¡ HACER EL PING AL RECEIVER ALMACENADO !!!!"));
    }
  }
  if (comando_LoRa == LORA_PING) {
    maqsec_ActivarCondicion (CS2);      //Enviar al Estado ACK para responder con el ACK.
  }
  //------------------------------------ PROCCESS --------------------------------
  //if ((T_actual - T_anterior) > TIMEOUT_ESTADOS) maqsec_ActivarCondicion (CS2);
  return 0;
}
//------------------------------------------------------------------------------
// FUNCTION: short mqsec_EstPING (void)
// DESCRIPTION: Estado que sirve para que cuando se reciba un PING se responda un ACK
//              teniendo en cuenta que no se puede enviar inmediatamente.
// PARAMETERS: Recibe el T_actual y el comando LoRa.
// RETURNS:  .
//------------------------------------------------------------------------------
short mqsec_EstPING (unsigned long T_actual) {

  if (maqsec_bits.flag_1A_vez == false) {   //First time that is entried to state.
    maqsec_SetFlag1A ();               //Set the flag of first time.
    T_anterior = T_actual;
    Serial.println (F("ESTADO PING"));
  }
  //------------------------------------ PROCCESS --------------------------------
  return 0;
}
//------------------------------------------------------------------------------
// FUNCTION: short mqsec_EstALARMAS (void)
// DESCRIPTION: Function that executes the functionalities of REPOSO state.
// PARAMETERS: Void.
// RETURNS:  0.
//------------------------------------------------------------------------------
short mqsec_EstACK (unsigned long T_actual, int comando_LoRa) {

  if (maqsec_bits.flag_1A_vez == false) {   //First time that is entried to state.
    maqsec_SetFlag1A ();               //Set the flag of first time.
    T_anterior = T_actual;
    Serial.println (F("ESTADO ACK"));
    timeout_ACK = T_actual;
  }
  //------------------------------------ PROCCESS --------------------------------
  if ((T_actual - timeout_ACK) > TIMEOUT_SEND_ACK) {
    timeout_ACK = T_actual;
    mqsec_Enviar_COMANDO_LORA (LORA_ACK);
    switch (Last_status) {
      case REPOSO:
        maqsec_ActivarCondicion (CS0);    //Devolver al estado de REPOSO.
        Serial.println (F("SALIR PARA REPOSO"));
        break;
      case VINCULAR:
        maqsec_ActivarCondicion (CS1);    //Devolver al estado de VINCULACIÓN.
        Serial.println (F("SALIR PARA VINCULAR"));
        break;
      default:
        Serial.println (F("SALIR PARA DEFAULT"));
        maqsec_ActivarCondicion (CS0);
        break;
    }
  }
  return 0;
}

int mqsec_Chech_SN_PCBRELES (void) {
  byte valor_EEPROM[11];

  memset (valor_EEPROM, 0, sizeof(valor_EEPROM));
  Serial.print (F("El S/N de la PCB_RELES es: "));
  for (int i = 0; i < DATOS_NUMERO_SERIE; i++) {
    valor_EEPROM[i] = datos_EEPROM_Read_BYTE (DAT_SN_RECEIVER + i);
    Serial.print ((char) valor_EEPROM[i]);
  }
  Serial.println ();
  if (valor_EEPROM[0] != 'S' || valor_EEPROM[1] != 'M' || valor_EEPROM[2] != 'F' || valor_EEPROM[3] != 'R') {
    Serial.println (F("¡¡¡¡¡NO HAY S/N PCB RELES!!!!!"));
    return 0;
  } else {
    return 1;
  }
}

void mqsec_Enviar_COMANDO_LORA (int Comando_Enviar) {
  char comando[N_MAXIMO_CARACTERES_MENSAJE];
  char mensaje_local[N_MAXIMO_CARACTERES_MENSAJE];

  memset (comando, 0, sizeof(comando));
  memset (mensaje_local, 0, sizeof(mensaje_local));
  switch (Comando_Enviar) {
  case LORA_ACK:
    strcpy (comando, "#ACK*");
    break;
  case LORA_PING:
    strcpy (comando, "#PING*");
    break;
  default:
    break;
  }
  int total_lenght = protocolo_Mensaje_pruebas (comando, strlen(comando), mensaje_local);
  Lora_imp_SendPacket (mensaje_local, total_lenght);
}

void mqsec_Activar_LEDBOTON (void) {
  digitalWrite  (LED_BOTON, LOW);        //Pulso para apagar el FOCOIRIS.
}

void mqsec_Activar_RELE (int rele, bool nivel) {
  digitalWrite  (rele, nivel);        //Pulso para apagar el FOCOIRIS.
}