//*****************************************************************************
//
// CTS_HAL.c - Capacative Sense Library Hardware Abstraction Layer.
//
// Copyright (c) 2012 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
// 
// Texas Instruments (TI) is supplying this software for use solely and
// exclusively on TI's microcontroller products. The software is owned by
// TI and/or its suppliers, and is protected under applicable copyright
// laws. You may not combine this software with "viral" open-source
// software in order to form a larger program.
// 
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
// 
// This is part of revision 9453 of the EK-LM4F120XL Firmware Package.
//
//*****************************************************************************

#include "drivers/CTS_structure.h"
#include "drivers/CTS_Layer.h"
#define RAM_FOR_FLASH

// Global variables for sensing
#ifdef TOTAL_NUMBER_OF_ELEMENTS
unsigned long baseCnt[TOTAL_NUMBER_OF_ELEMENTS];
#ifdef RAM_FOR_FLASH
unsigned long measCnt[MAXIMUM_NUMBER_OF_ELEMENTS_PER_SENSOR];
#endif
unsigned long ctsStatusReg = (DOI_INC+TRADOI_FAST+TRIDOI_SLOW);
#endif

//*****************************************************************************
//
//! \addtogroup CTS_API
//! @{ 
//
//*****************************************************************************


//*****************************************************************************
//
//! Measure the capacitance of each element within the Sensor
//!
//! \param groupOfElements Pointer to Sensor structure to be measured
//!
//! \param counts Address to where the measurements are to be written
//!
//! This function selects the appropriate HAL to perform the capacitance
//! measurement based upon the halDefinition found in the sensor structure. 
//! The order of the elements within the Sensor structure is arbitrary but must
//! be consistent between the application and configuration. The first element
//! in the array (counts) corresponds to the first element within the Sensor 
//! structure.
//!
//! \return None.
//
//*****************************************************************************
void 
TI_CAPT_Raw(const tSensor* groupOfElements, unsigned long * counts)
{
    CapSenseSystickRC(groupOfElements, counts);
}

#ifdef TOTAL_NUMBER_OF_ELEMENTS


//*****************************************************************************
//
//! Make a single capacitance meausrment to initialize baseline tracking
//!
//! \param   groupOfElements Pointer to Sensor structure to be measured
//! 
//! \return  none
//
//*****************************************************************************
void 
TI_CAPT_Init_Baseline(const tSensor* groupOfElements)
{
    TI_CAPT_Raw(groupOfElements, &baseCnt[groupOfElements->ulBaseOffset]);
}

//*****************************************************************************
//
//! Update baseline tracking by averaging several measurements
//!
//! \param   groupOfElements Pointer to Sensor structure to be measured
//!
//! \param   numberOfAverages Number of measurements to be averaged
//!
//! \return  none
//
//*****************************************************************************
void
TI_CAPT_Update_Baseline(const tSensor* groupOfElements, unsigned char numberOfAverages)
{
	unsigned char i,j;
    #ifndef RAM_FOR_FLASH
	unsigned long *measCnt;
    measCnt = (unsigned long *)malloc(groupOfElements->ucNumElements * sizeof(unsigned long));
    if(measCnt ==0)
    {
        while(1);
    }
    #endif
    for(j=0; j < numberOfAverages; j++)
    {
        for(i=0; i < groupOfElements->ucNumElements; i++)
        {
            TI_CAPT_Raw(groupOfElements, measCnt);
            baseCnt[i+groupOfElements->ulBaseOffset] = measCnt[i]/2 + baseCnt[i+groupOfElements->ulBaseOffset]/2;
        }
    }
    #ifndef RAM_FOR_FLASH
    free(measCnt);
    #endif
}

//*****************************************************************************
//! Reset the Baseline Tracking algorithm to the default state
//!
//! \return  none
//
//*****************************************************************************
void 
TI_CAPT_Reset_Tracking(void)
{
    ctsStatusReg = (DOI_INC+TRADOI_FAST+TRIDOI_SLOW);
}

//*****************************************************************************
//
//! Update the Baseline Tracking algorithm Direction of Interest
//!
//! \param   direction Direction of increasing or decreasing capacitance
//!
//! \return  none
//
//*****************************************************************************
void 
TI_CAPT_Update_Tracking_DOI(unsigned char direction)
{
	if(direction)
	{
		ctsStatusReg |= DOI_INC;
	}
	else
	{
		ctsStatusReg &= ~DOI_INC;
	}
}

//*****************************************************************************
//
//! Update the baseling tracking algorithm tracking rates
//!
//! \param   rate Rate of tracking changes in and against direction of intrest
//!
//! \return  none
//
//*****************************************************************************
void 
TI_CAPT_Update_Tracking_Rate(unsigned char rate)
{
  ctsStatusReg &= ~(TRIDOI_FAST+TRADOI_VSLOW); // clear fields
  ctsStatusReg |= (rate & 0xF0); // update fields
}

//*****************************************************************************
//
//! Measure the change in capacitance of the Sensor
//! 
//! \param   groupOfElements Pointer to Sensor structure to be measured
//!
//! \param   deltaCnt Address to where the measurements are to be written
//!
//! This function measures the change in capacitance of each element within a 
//! sensor and updates the baseline tracking in the event that no change 
//! exceeds the detection threshold. The order of the elements within the 
//! Sensor structure is arbitrary but must be consistent between the 
//! application and configuration. The first element in the array (deltaCnt)
//! corresponds to the first element within the Sensor structure.
//!
//! \return  none
//
//*****************************************************************************
void 
TI_CAPT_Custom(const tSensor* groupOfElements, unsigned long * deltaCnt)
{ 
    unsigned char j;
    unsigned long tempCnt;
    ctsStatusReg &= ~ EVNT;
        
    // This section calculates the delta counts*************************************
    //******************************************************************************
    TI_CAPT_Raw(groupOfElements, &deltaCnt[0]); // measure group of sensors
  
    for (j = 0; j < (groupOfElements->ucNumElements); j++)
    {  
        tempCnt = deltaCnt[j];
        if((!(ctsStatusReg & DOI_MASK)))
        { 
            // RO method, interested in an increase in capacitance
            if(baseCnt[j+groupOfElements->ulBaseOffset] < deltaCnt[j])            
            {
                // If capacitance decreases, then measCnt is greater than base
                // , set delta to zero
                deltaCnt[j] = 0;
                // Limit the change in the opposite direction to the threshold
                if(((groupOfElements->Element[j])->ulThreshold)
                &&
                (baseCnt[j+groupOfElements->ulBaseOffset]+(groupOfElements->Element[j])->ulThreshold < tempCnt))
                {
                    tempCnt = baseCnt[j+groupOfElements->ulBaseOffset]+(groupOfElements->Element[j])->ulThreshold;
                }
            }
            else
            {
                // change occuring in our DOI, save result
                deltaCnt[j] = baseCnt[j+groupOfElements->ulBaseOffset]-deltaCnt[j];
            }
        }
        if(((ctsStatusReg & DOI_MASK)))
        { 
            // RO method: interested in a decrease in capactiance
            //  measCnt is greater than baseCnt
            if(baseCnt[j+groupOfElements->ulBaseOffset] > deltaCnt[j])            
            {
                // If capacitance increases, set delta to zero
                deltaCnt[j] = 0;
                // Limit the change in the opposite direction to the threshold
                if(((groupOfElements->Element[j])->ulThreshold)
                &&
                (baseCnt[j+groupOfElements->ulBaseOffset] > tempCnt+(groupOfElements->Element[j])->ulThreshold))
                {
                    tempCnt = baseCnt[j+groupOfElements->ulBaseOffset]-(groupOfElements->Element[j])->ulThreshold;
                }
            }
            else       
            {
                // change occuring in our DOI
                deltaCnt[j] = deltaCnt[j] - baseCnt[j+groupOfElements->ulBaseOffset];
            }         
        }
            
        // This section updates the baseline capacitance****************************
        //**************************************************************************  
        if (deltaCnt[j]==0)
        { // if delta counts is 0, then the change in capacitance was opposite the
          // direction of interest.  The baseCnt[i] is updated with the saved 
          // measCnt value for the current index value 'i'.
          switch ((ctsStatusReg & TRADOI_VSLOW))
          {
            case TRADOI_FAST://Fast
                    tempCnt = tempCnt/2;
                    baseCnt[j+groupOfElements->ulBaseOffset] = (baseCnt[j+groupOfElements->ulBaseOffset]/2);
                    break;
            case TRADOI_MED://Medium
                    tempCnt = tempCnt/4;
                    baseCnt[j+groupOfElements->ulBaseOffset] = 3*(baseCnt[j+groupOfElements->ulBaseOffset]/4);
                    break;
            case TRADOI_SLOW://slow
                  tempCnt = tempCnt/64;
                  baseCnt[j+groupOfElements->ulBaseOffset] = 63*(baseCnt[j+groupOfElements->ulBaseOffset]/64);
                    break;
            case TRADOI_VSLOW://very slow
                  tempCnt = tempCnt/128;
                  baseCnt[j+groupOfElements->ulBaseOffset] = 127*(baseCnt[j+groupOfElements->ulBaseOffset]/128);
              break;
          }
          // set X, Y & Z, then perform calculation for baseline tracking:
          // Base_Capacitance = X*(Measured_Capacitance/Z) + Y*(Base_Capacitance/Z)
          baseCnt[j+groupOfElements->ulBaseOffset] = (tempCnt)+(baseCnt[j+groupOfElements->ulBaseOffset]);
        }
            // delta counts are either 0, less than threshold, or greater than threshold
            // never negative
        else if(deltaCnt[j]<(groupOfElements->Element[j])->ulThreshold && !(ctsStatusReg & PAST_EVNT))
        {    //if delta counts is positive but less than threshold,
          switch ((ctsStatusReg & TRIDOI_FAST))
          {
            case TRIDOI_VSLOW://very slow
              if(deltaCnt[j] > 15)
              {
                  if(tempCnt < baseCnt[j+groupOfElements->ulBaseOffset])
                  {
                     baseCnt[j+groupOfElements->ulBaseOffset] = baseCnt[j+groupOfElements->ulBaseOffset] - 1;               
                  }
                  else
                  {
                     baseCnt[j+groupOfElements->ulBaseOffset] = baseCnt[j+groupOfElements->ulBaseOffset] + 1;               
                  }
              }
              tempCnt = 0;
              break;
            case TRIDOI_SLOW://slow
              if(tempCnt < baseCnt[j+groupOfElements->ulBaseOffset])
              {
                 baseCnt[j+groupOfElements->ulBaseOffset] = baseCnt[j+groupOfElements->ulBaseOffset] - 1;               
              }
              else
              {
                 baseCnt[j+groupOfElements->ulBaseOffset] = baseCnt[j+groupOfElements->ulBaseOffset] + 1;               
              }
              tempCnt = 0;
              break;
            case TRIDOI_MED://medium
                tempCnt = tempCnt/4;
                baseCnt[j+groupOfElements->ulBaseOffset] = 3*(baseCnt[j+groupOfElements->ulBaseOffset]/4);
                break;
            case TRIDOI_FAST://fast
                tempCnt = tempCnt/2;
                baseCnt[j+groupOfElements->ulBaseOffset] = (baseCnt[j+groupOfElements->ulBaseOffset]/2);
                break;
          }
          // set X, Y & Z, then perform calculation for baseline tracking:
          // Base_Capacitance = X*(Measured_Capacitance/Z) + Y*(Base_Capacitance/Z)
          baseCnt[j+groupOfElements->ulBaseOffset] = (tempCnt)+(baseCnt[j+groupOfElements->ulBaseOffset]);
        }
        //if delta counts above the threshold, event has occurred
        else if(deltaCnt[j]>=(groupOfElements->Element[j])->ulThreshold)
        {
          ctsStatusReg |= EVNT;
          ctsStatusReg |= PAST_EVNT;
        }
    }// end of for-loop
    if(!(ctsStatusReg & EVNT))
    {
      ctsStatusReg &= ~PAST_EVNT;
    }
}

//*****************************************************************************
//
//! Determine if a button is being pressed
//!
//! \param   groupOfElements Pointer to button to be scanned
//!
//! \return  result Indication if button is (1) or is not (0) being pressed
//!
//
//*****************************************************************************
unsigned char 
TI_CAPT_Button(const tSensor * groupOfElements)
{
    unsigned char result = 0;
    
    #ifndef RAM_FOR_FLASH
    unsigned long *measCnt;
    measCnt = (unsigned long *)malloc(groupOfElements->ucNumElements * sizeof(unsigned long));
    if(measCnt ==0)
    {
        while(1);
    }
    #endif
    
    TI_CAPT_Custom(groupOfElements, measCnt);
    #ifndef RAM_FOR_FLASH
    free(measCnt);
    #endif
    if(ctsStatusReg & EVNT)
    {
        result = 1;
    }    
    return result;
}

//*****************************************************************************
//
//! \brief   Determine which button if any is being pressed
//!
//! \param   groupOfElements Pointer to buttons to be scanned
//!
//! \return  result pointer to element (button) being pressed or 0 none
//
//*****************************************************************************
const tCapTouchElement *
TI_CAPT_Buttons(const tSensor *groupOfElements)
{
    unsigned char index;
    #ifndef RAM_FOR_FLASH
    unsigned long *measCnt;
    measCnt = (unsigned long *)malloc(groupOfElements->ucNumElements * sizeof(unsigned long));
    if(measCnt ==0)
    {
        while(1);
    }
    #endif
    TI_CAPT_Custom(groupOfElements, measCnt);
    
    if(ctsStatusReg & EVNT)
    {
        index = Dominant_Element(groupOfElements, measCnt);
        //ctsStatusReg &= ~EVNT;
        index++;
    }
    else 
    {
        index = 0;
    }
    #ifndef RAM_FOR_FLASH
    free(measCnt);
    #endif
    if(index)
    {
      return groupOfElements->Element[index-1];
    }
    return 0;
}

#ifdef SLIDER
//*****************************************************************************
//
//! Determine the position on a slider
//
//! \param   groupOfElements Pointer to slider
//
//! \return  result position on slider or illegal value if no touch
//
//*****************************************************************************
unsigned long 
TI_CAPT_Slider(const tSensor* groupOfElements)
{
    unsigned char index;
    signed long position;
    // allocate memory for measurement
    #ifndef RAM_FOR_FLASH
    unsigned long *measCnt;
    measCnt = (unsigned long *)malloc(groupOfElements->ucNumElements * sizeof(unsigned long));
    if(measCnt ==0)
    {
        while(1);
    }
    #endif
    position = ILLEGAL_SLIDER_WHEEL_POSITION;
    //make measurement
    TI_CAPT_Custom(groupOfElements, measCnt);
    
    // Use EVNT flag to determine if slider was touched.
    // The EVNT flag is a global variable and managed within the TI_CAPT_Custom function.
    if(ctsStatusReg & EVNT)
    {
        index = Dominant_Element(groupOfElements, &measCnt[0]);
        // The index represents the element within the array with the highest return.
        if(index == 0)
        {
          // Special case of 1st element in slider, add 1st, last, and 2nd
          position = measCnt[0] + measCnt[1];
        }
        else if(index == (groupOfElements->ucNumElements -1))
        {
          // Special case of Last element in slider, add last, 1st, and 2nd to last
          position = measCnt[groupOfElements->ucNumElements -1] + measCnt[groupOfElements->ucNumElements -2];
        }
        else
        {
          position = measCnt[index] + measCnt[index+1] + measCnt[index-1];
        } 
        // Determine if sensor threshold criteria is met
        if(position > groupOfElements->ulSensorThreshold)
        {
            // calculate position
    	    position = index*(groupOfElements->ucPoints/groupOfElements->ucNumElements);
            position += (groupOfElements->ucPoints/groupOfElements->ucNumElements)/2;
            if(index == 0)
            {
              // Special case of 1st element in slider, which only has one 
              // neighbor, measCnt[1]. measCnt is limited to ulMaxResponse 
              // within dominantElement function
              if(measCnt[1])
              {
                  position += (measCnt[1]*(groupOfElements->ucPoints/groupOfElements->ucNumElements))/100;
              }
              else
              {
                  position = (measCnt[0]*(groupOfElements->ucPoints/groupOfElements->ucNumElements)/2)/100;
              }
            }
            else if(index == (groupOfElements->ucNumElements -1))
            {
              // Special case of Last element in slider, which only has one 
              // neighbor, measCnt[x-1] or measCnt[ucNumElements-1]
              if(measCnt[index-1])
              {
                  position -= (measCnt[index-1]*(groupOfElements->ucPoints/groupOfElements->ucNumElements))/100;
              }
              else
              {
                  position = groupOfElements->ucPoints;
                  position -= (measCnt[index]*(groupOfElements->ucPoints/groupOfElements->ucNumElements)/2)/100;
              }
            }
            else
            {
                  position += (measCnt[index+1]*(groupOfElements->ucPoints/groupOfElements->ucNumElements))/100;
                  position -= (measCnt[index-1]*(groupOfElements->ucPoints/groupOfElements->ucNumElements))/100;
            }  
            if((position > groupOfElements->ucPoints) || (position < 0))
            {
                  position = ILLEGAL_SLIDER_WHEEL_POSITION;
            }     
        }
        else
        {
            position = ILLEGAL_SLIDER_WHEEL_POSITION;
        }
    }
    #ifndef RAM_FOR_FLASH
    free(measCnt);
    #endif
    return position;
}
#endif

#ifdef WHEEL
//*****************************************************************************
//
//! Determine the position on a wheel
//
//! \param   groupOfElements Pointer to wheel
//
//! \return  result position on wheel or illegal value if no touch
//
//*****************************************************************************
unsigned long 
TI_CAPT_Wheel(const tSensor* groupOfElements)
{
    unsigned char index;
    signed long position;
    // allocate memory for measurement
    #ifndef RAM_FOR_FLASH
    unsigned long *measCnt;
    measCnt = (unsigned long *)malloc(groupOfElements->ucNumElements * sizeof(unsigned long));
    if(measCnt ==0)
    {
        while(1);
    }
    #endif
    position = ILLEGAL_SLIDER_WHEEL_POSITION;
    //make measurement
    TI_CAPT_Custom(groupOfElements, measCnt);
    // Translate the EVNT flag from an element level EVNT to a sensor level EVNT.
    // The sensor must read at least 75% cumulative response before indicating a 
    // touch.
    if(ctsStatusReg & EVNT)
    {
        index = Dominant_Element(groupOfElements, &measCnt[0]);
        // The index represents the element within the array with the highest return.
        // 
        if(index == 0)
        {
          // Special case of 1st element in slider, add 1st, last, and 2nd
          position = measCnt[0] + measCnt[groupOfElements->ucNumElements -1] + measCnt[1];
        }
        else if(index == (groupOfElements->ucNumElements -1))
        {
          // Special case of Last element in slider, add last, 1st, and 2nd to last
          position = measCnt[index] + measCnt[0] + measCnt[index-1];
        }
        else
        {
          position = measCnt[index] + measCnt[index+1] + measCnt[index-1];
        } 
        if(position > groupOfElements->ulSensorThreshold)
        {
            //index = Dominant_Element(groupOfElements, &measCnt[0]);
            // The index represents the element within the array with the highest return.
            // 
            position = index*(groupOfElements->ucPoints/groupOfElements->ucNumElements);
            position += (groupOfElements->ucPoints/groupOfElements->ucNumElements)/2;
            if(index == 0)
            {
              // Special case of 1st element in slider, which only has one neighbor, measCnt[1]
              // measCnt is limited to ulMaxResponse within dominantElement function
              position += (measCnt[1]*(groupOfElements->ucPoints/groupOfElements->ucNumElements))/100;
              position -= (measCnt[groupOfElements->ucNumElements -1]*(groupOfElements->ucPoints/groupOfElements->ucNumElements))/100;
              if(position < 0)
              {
                position = position + (unsigned long)groupOfElements->ucPoints;
              }
            }
            else if(index == (groupOfElements->ucNumElements -1))
            {
              // Special case of Last element in slider, which only has one neighbor, measCnt[x-1] or measCnt[ucNumElements-1]
              // measCnt is limited to ulMaxResponse within dominantElement function
              position += (measCnt[0]*(groupOfElements->ucPoints/groupOfElements->ucNumElements))/100;
              position -= (measCnt[index-1]*(groupOfElements->ucPoints/groupOfElements->ucNumElements))/100;
              if(position > (groupOfElements->ucPoints -1))
              {
                position = position - (unsigned long)groupOfElements->ucPoints;
              }
            }
            else
            {
              position += (measCnt[index+1]*(groupOfElements->ucPoints/groupOfElements->ucNumElements))/100;
              position -= (measCnt[index-1]*(groupOfElements->ucPoints/groupOfElements->ucNumElements))/100;
            } 
            if((position > groupOfElements->ucPoints) || position < 0)
            {
                  position = ILLEGAL_SLIDER_WHEEL_POSITION;
            }     
        }
        else 
        {
            position = ILLEGAL_SLIDER_WHEEL_POSITION;
        }
    }
    #ifndef RAM_FOR_FLASH
    free(measCnt);
    #endif
    return position;
}
#endif

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************


//*****************************************************************************
//
//
//*****************************************************************************
unsigned char 
Dominant_Element(const tSensor* groupOfElements, unsigned long* deltaCnt)
{
    unsigned char i;
    unsigned long percentDelta=0; 
    unsigned char dominantElement=0;
    for(i=0;i<groupOfElements->ucNumElements;i++)
    {  
        if(deltaCnt[i]>=(groupOfElements->Element[i])->ulThreshold)
        {
            if(deltaCnt[i] > ((groupOfElements->Element[i])->ulMaxResponse))
            {
                deltaCnt[i] = (groupOfElements->Element[i])->ulMaxResponse;
                // limit response to the maximum
            }
            // (ulMaxResponse - threshold) cannot exceed 655
            // 100*(delta - threshold) / (ulMaxResponse - threshold)
            deltaCnt[i] = (100*(deltaCnt[i]-(groupOfElements->Element[i])->ulThreshold))/((groupOfElements->Element[i])->ulMaxResponse - (groupOfElements->Element[i])->ulThreshold);
            if(deltaCnt[i] > percentDelta)
            {
                //update percentDelta
                percentDelta = deltaCnt[i];
                dominantElement = i;
            }
        }
        else
        {
            deltaCnt[i] = 0;
        }
    } // end for loop    
    return dominantElement;
}
#endif
