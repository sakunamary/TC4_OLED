/*  TC4-WB for Artisan Coffee Roaster Application

    Released under BSD-NC-3.0 License

    Created by Sakunamary on 2022


    ThermalMeter Task
    The default sample rate of Artisan is 3 seconds, although the setting value can be modified by user.
    I think this value is generated from lots of experimental, so keeps polling-time as 3 seconds.
    Thus, In the ThermalMeter Task will (750ms x 4 = 3s)
    (1) read MAX6675 temperature evey 750ms, and
    (2) Average-Array length is 4.
    That is, the measured temperature is avaraged from 4 times of MAX6675 temperature reading.

*/
#ifndef __TC4_THERMALMETER_H__
#define __TC4_THERMALMETER_H__

#include <Arduino.h>
#include "TC4.h"
#include "max6675.h"


#define ABNORMAL_TEMPERATURE_DEGREE 100 // defin abnormal temperature value



float BT_TempArray[TEMPERATURE_ARRAY_LENGTH] = {0.0}; // temperature array
float ET_TempArray[TEMPERATURE_ARRAY_LENGTH] = {0.0}; // temperature array
int BT_ArrayIndex = 0;                                // A pointer of temperature array
float BT_CurTemp = 0.0;
float ET_CurTemp = 0.0;


bool bReady = false;         // flag to indicate temperature array whether is ready or not
bool bUnit_C = true;         // flag to indicate temperature unit from Artisan requested
bool bAbnormalValue = false; // indicate temperature value is unexpect or not

int thermoDO = 19;
int thermoCLK = 5;
int thermoCS_ET = 16;
int thermoCS_BT = 17;


SemaphoreHandle_t xThermoDataMutex = NULL;

// Create Variables and Object for Environment Temperature (ET) if exist
MAX6675 thermocouple_ET(thermoCLK, thermoCS_ET, thermoDO);

// Create object for Beam Temperature (BT)
MAX6675 thermocouple_BT(thermoCLK, thermoCS_BT, thermoDO);

float averageTemperature(float *pTemp);



void TaskThermalMeter(void *pvParameters)
{ //function 

    /* Variable Definition */
    (void)pvParameters;
    TickType_t xLastWakeTime;

    const TickType_t xIntervel = (user_wifi.sampling_time * 1000) / portTICK_PERIOD_MS;

    /* Task Setup and Initialize */
    // Initial the xLastWakeTime variable with the current time.
    xLastWakeTime = xTaskGetTickCount();

    for (;;) // A Task shall never return or exit.
    { //for loop
        // Wait for the next cycle (intervel 750ms).
        vTaskDelayUntil(&xLastWakeTime, xIntervel);

        // Perform task actions from here
        // Read BT from MAX6675 thermal couple

            // The ET is reference temperature, don't need averaging
            // read ET from MAX6675 thermal couple
            //ET_CurTemp = thermocouple_ET.readCelsius() + user_wifi.etemp_fix;
            // Serial.printf("ET:%f ,ET compensate:%f",ET_CurTemp,etemp_fix_in);
            // Serial.println("");

     
            BT_CurTemp = thermocouple_BT.readCelsius() + user_wifi.btemp_fix;  //get BT data
            ET_CurTemp = thermocouple_ET.readCelsius() + user_wifi.etemp_fix;
     vTaskDelay(100);
            if (bReady) // bReady = false -- a new loop
            {
                // Means, first round of temperature array is done,
                // The averaged temperyure is ready for reading

                if (xSemaphoreTake(xThermoDataMutex, xIntervel) == pdPASS) 
                {//lock the  mutex       
                        temperature_data.BT_AvgTemp  = averageTemperature(&BT_TempArray[0]);
                        temperature_data.ET_AvgTemp  = averageTemperature(&ET_TempArray[0]);
                        xSemaphoreGive(xThermoDataMutex);  //end of lock mutex
                }   

                // Filter out abnormal temperature-up only (Bypass temperature-down)
                // Because in "CHARGE" period, the temperature-down may large than 10 degree
                if (BT_CurTemp < (temperature_data.BT_AvgTemp + ABNORMAL_TEMPERATURE_DEGREE) 
                && ET_CurTemp < (temperature_data.ET_AvgTemp + ABNORMAL_TEMPERATURE_DEGREE) 
                )
                            {
                            // temperature is in-arrange, store it
                            BT_TempArray[BT_ArrayIndex] = BT_CurTemp;
                            ET_TempArray[BT_ArrayIndex] = ET_CurTemp;
                            }
                        else
                            {
                                // set abnormal flag
                                bAbnormalValue = true;
                            }

            }
        
            else // bReady = true
            {   
                 // just read current temperature
                 BT_TempArray[BT_ArrayIndex] = BT_CurTemp;
                 ET_TempArray[BT_ArrayIndex] = ET_CurTemp;
            }
///////////////////////////////////////////////////////////////////////////////////////////////////////////
            if (!bAbnormalValue)
            {
                // Normal temperature will into this loop
                // print MAX6675 reading value on serial monitor

                BT_ArrayIndex++;
                if (BT_ArrayIndex >= TEMPERATURE_ARRAY_LENGTH)
                {
                    BT_ArrayIndex = 0;

                    if (!bReady)
                    {
                        bReady = true;
                    }

                    }
                }
            else //bAbnormalValue
                {
                    // After bypass abnormal value, reset flag here
                    bAbnormalValue = false;
                }
    } 
}//function 



// A function to average temperature array
float averageTemperature(float *pTemp)
{
    float avg = *pTemp;

    pTemp++;
    avg = (avg + *pTemp) / 2;
    pTemp++;
    avg = (avg + *pTemp) / 2;
    pTemp++;
    avg = (avg + *pTemp) / 2;
    return avg;
}




#endif
