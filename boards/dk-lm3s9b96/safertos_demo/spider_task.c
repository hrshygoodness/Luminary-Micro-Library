//*****************************************************************************
//
// spider_task.c - Tasks to animate a set of spiders on the LCD, one task per
//                 spider.
//
// Copyright (c) 2009-2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 9107 of the DK-LM3S9B96 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_types.h"
#include "driverlib/interrupt.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "SafeRTOS/SafeRTOS_API.h"
#include "drivers/touch.h"
#include "display_task.h"
#include "idle_task.h"
#include "images.h"
#include "priorities.h"
#include "random.h"
#include "spider_task.h"

//*****************************************************************************
//
// The following define the screen area in which the spiders are allowed to
// roam
//
//*****************************************************************************
#define AREA_X                  0
#define AREA_Y                  24
#define AREA_WIDTH              320
#define AREA_HEIGHT             (240 - 24 - 20)

//*****************************************************************************
//
// The size of the spider images.
//
//*****************************************************************************
#define SPIDER_WIDTH            24
#define SPIDER_HEIGHT           24

//*****************************************************************************
//
// The following define the extents of the centroid of the spiders.
//
//*****************************************************************************
#define SPIDER_MIN_X            (AREA_X + (SPIDER_WIDTH / 2))
#define SPIDER_MAX_X            (AREA_X + AREA_WIDTH - (SPIDER_WIDTH / 2))
#define SPIDER_MIN_Y            (AREA_Y + (SPIDER_HEIGHT / 2))
#define SPIDER_MAX_Y            (AREA_Y + AREA_HEIGHT - (SPIDER_HEIGHT / 2))

//*****************************************************************************
//
// The item size, queue size, and memory size for the spider control message
// queue.
//
//*****************************************************************************
#define CONTROL_ITEM_SIZE       sizeof(unsigned long)
#define CONTROL_QUEUE_SIZE      10
#define CONTROL_MEM_SIZE        ((CONTROL_ITEM_SIZE * CONTROL_QUEUE_SIZE) +   \
                                 portQUEUE_OVERHEAD_BYTES)

//*****************************************************************************
//
// A buffer to contain the contents of the spider control message queue.
//
//*****************************************************************************
static unsigned long g_pulControlQueueMem[(CONTROL_MEM_SIZE + 3) / 4];

//*****************************************************************************
//
// The queue that holds messages sent to the spider control task.
//
//*****************************************************************************
static xQueueHandle g_pControlQueue;

//*****************************************************************************
//
// The stack for the spider control task.
//
//*****************************************************************************
static unsigned long g_pulControlTaskStack[128];

//*****************************************************************************
//
// The stacks for the spider tasks.
//
//*****************************************************************************
static unsigned long g_ppulSpiderTaskStack[MAX_SPIDERS][128];

//*****************************************************************************
//
// The amount the spider moves horizontally for each direction of movement.
//
// For this and all subsequent arrays that are indexed by direction of
// movement, the indices are as follows:
//
//     0 => right
//     1 => right and down
//     2 => down
//     3 => left and down
//     4 => left
//     5 => left and up
//     6 => up
//     7 => right and up
//
//*****************************************************************************
static const long g_plSpiderStepX[8] =
{
    1, 1, 0, -1, -1, -1, 0, 1
};

//*****************************************************************************
//
// The amount the spider moves vertically for each direction of movement.
//
//*****************************************************************************
static const long g_plSpiderStepY[8] =
{
    0, 1, 1, 1, 0, -1, -1, -1
};

//*****************************************************************************
//
// The animation images for the spider, two per direction of movement.  In
// other words, entries 0 and 1 correspond to direction 0 (right), entries 2
// and 3 correspond to direction 1 (right and down), etc.
//
//*****************************************************************************
static const unsigned char *g_ppucSpiderImage[16] =
{
    g_pucSpiderR1Image,
    g_pucSpiderR2Image,
    g_pucSpiderDR1Image,
    g_pucSpiderDR2Image,
    g_pucSpiderD1Image,
    g_pucSpiderD2Image,
    g_pucSpiderDL1Image,
    g_pucSpiderDL2Image,
    g_pucSpiderL1Image,
    g_pucSpiderL2Image,
    g_pucSpiderUL1Image,
    g_pucSpiderUL2Image,
    g_pucSpiderU1Image,
    g_pucSpiderU2Image,
    g_pucSpiderUR1Image,
    g_pucSpiderUR2Image
};

//*****************************************************************************
//
// The number of ticks to delay a spider task based on the direction of
// movement.  This array has only two entries; the first corresponding to
// horizontal and vertical movement, and the second corresponding to diagonal
// movement.  By having the second entry be 1.4 times the first, the spiders
// are updated slower when moving along the diagonal to compensate for the
// fact that each step is further (since it is moving one step in both the
// horizontal and vertical).
//
//*****************************************************************************
unsigned long g_pulSpiderDelay[2];

//*****************************************************************************
//
// The horizontal position of the spiders.
//
//*****************************************************************************
static long g_plSpiderX[MAX_SPIDERS];

//*****************************************************************************
//
// The vertical position of the spiders.
//
//*****************************************************************************
static long g_plSpiderY[MAX_SPIDERS];

//*****************************************************************************
//
// A bitmap that indicates which spiders are alive (which corresponds to a
// running task for that spider).
//
//*****************************************************************************
static unsigned long g_ulSpiderAlive;

//*****************************************************************************
//
// A bitmap that indicates which spiders have been killed (by touching them).
//
//*****************************************************************************
static unsigned long g_ulSpiderDead;

//*****************************************************************************
//
// Determines if a given point collides with one of the spiders.  The spider
// specified is ignored when doing collision detection in order to prevent a
// false collision with itself (when checking to see if it is safe to move the
// spider).
//
//*****************************************************************************
static long
SpiderCollide(long lSpider, long lX, long lY)
{
    long lIdx, lDX, lDY;

    //
    // Loop through all the spiders.
    //
    for(lIdx = 0; lIdx < MAX_SPIDERS; lIdx++)
    {
        //
        // Skip this spider if it is not alive or is the spider that should be
        // ignored.
        //
        if((HWREGBITW(&g_ulSpiderAlive, lIdx) == 0) || (lIdx == lSpider))
        {
            continue;
        }

        //
        // Compute the horizontal and vertical difference between this spider's
        // position and the point in question.
        //
        lDX = ((g_plSpiderX[lIdx] > lX) ? (g_plSpiderX[lIdx] - lX) :
               (lX - g_plSpiderX[lIdx]));
        lDY = ((g_plSpiderY[lIdx] > lY) ? (g_plSpiderY[lIdx] - lY) :
               (lY - g_plSpiderY[lIdx]));

        //
        // Return this spider index if the point in question collides with it.
        //
        if((lDX < SPIDER_WIDTH) && (lDY < SPIDER_HEIGHT))
        {
            return(lIdx);
        }
    }

    //
    // No collision was detected.
    //
    return(-1);
}

//*****************************************************************************
//
// This task manages the scurring about of a spider.
//
//*****************************************************************************
static void
SpiderTask(void *pvParameters)
{
    unsigned long ulDir, ulImage, ulTemp;
    long lX, lY, lSpider;

    //
    // Get the spider number from the parameter.
    //
    lSpider = (long)pvParameters;

    //
    // Add the current tick count to the random entropy pool.
    //
    RandomAddEntropy(xTaskGetTickCount());

    //
    // Reseed the random number generator.
    //
    RandomSeed();

    //
    // Indicate that this spider is alive.
    //
    HWREGBITW(&g_ulSpiderAlive, lSpider) = 1;

    //
    // Indicate that this spider is not dead yet.
    //
    HWREGBITW(&g_ulSpiderDead, lSpider) = 0;

    //
    // Get a local copy of the spider's starting position.
    //
    lX = g_plSpiderX[lSpider];
    lY = g_plSpiderY[lSpider];

    //
    // Choose a random starting direction for the spider.
    //
    ulDir = RandomNumber() >> 29;

    //
    // Start by displaying the first of the two spider animation images.
    //
    ulImage = 0;

    //
    // Loop forever.
    //
    while(1)
    {
        //
        // See if this spider has been killed.
        //
        if(HWREGBITW(&g_ulSpiderDead, lSpider) == 1)
        {
            //
            // Wait for 2 seconds.
            //
            xTaskDelay((1000 / portTICK_RATE_MS) * 2);

            //
            // Clear the spider from the display.
            //
            DisplayImage(lX - (SPIDER_WIDTH / 2), lY - (SPIDER_HEIGHT / 2),
                         g_pucSpiderBlankImage);

            //
            // Indicate that this spider is not alive.
            //
            HWREGBITW(&g_ulSpiderAlive, lSpider) = 0;

            //
            // Delete the current task.  This should never return.
            //
            xTaskDelete(NULL);

            //
            // In case it does return, loop forever.
            //
            while(1)
            {
            }
        }

        //
        // Enter a critical section while the next move for the spider is
        // determined.  Having more than one spider trying to move at a time
        // (via preemption) would make the collision detection check fail.
        //
        taskENTER_CRITICAL();

        //
        // Move the spider.
        //
        lX += g_plSpiderStepX[ulDir];
        lY += g_plSpiderStepY[ulDir];

        //
        // See if the spider has cross the boundary of its area, if it has
        // collided with another spider, or if random chance says that the
        // spider should turn despite not having collided with anything.
        //
        if((lX < SPIDER_MIN_X) || (lX > SPIDER_MAX_X) ||
           (lY < SPIDER_MIN_Y) || (lY > SPIDER_MAX_Y) ||
           (SpiderCollide(lSpider, lX, lY) != -1) ||
           (RandomNumber() < 0x08000000))
        {
            //
            // Undo the previous movement of the spider.
            //
            lX -= g_plSpiderStepX[ulDir];
            lY -= g_plSpiderStepY[ulDir];

            //
            // Get a random number to determine the turn to be made.
            //
            ulTemp = RandomNumber();

            //
            // Determine how to turn the spider based on the random number.
            // Half the time the spider turns to the left and half the time it
            // turns to the right.  Of each half, it turns a quarter of a turn
            // 12.5% of the time and an eighth of a turn 87.5% of the time.
            //
            if(ulTemp < 0x10000000)
            {
                ulDir = (ulDir + 2) & 7;
            }
            else if(ulTemp < 0x80000000)
            {
                ulDir = (ulDir + 1) & 7;
            }
            else if(ulTemp < 0xf0000000)
            {
                ulDir = (ulDir - 1) & 7;
            }
            else
            {
                ulDir = (ulDir - 2) & 7;
            }
        }

        //
        // Update the position of the spider.
        //
        g_plSpiderX[lSpider] = lX;
        g_plSpiderY[lSpider] = lY;

        //
        // Exit the critical section now that the spider has been moved.
        //
        taskEXIT_CRITICAL();

        //
        // Have the display task draw the spider at the new position.  Since
        // there is a one pixel empty border around all the images, and the
        // position of the spider is incremented by only one pixel, this also
        // erases any traces of the spider in its previous position.
        //
        DisplayImage(lX - (SPIDER_WIDTH / 2), lY - (SPIDER_HEIGHT / 2),
                     g_ppucSpiderImage[(ulDir * 2) + ulImage]);

        //
        // Toggle the spider animation index.
        //
        ulImage ^= 1;

        //
        // Delay this task for an amount of time based on the direction the
        // spider is moving.
        //
        xTaskDelay(g_pulSpiderDelay[ulDir & 1]);

        //
        // Add the new tick count to the random entropy pool.
        //
        RandomAddEntropy(xTaskGetTickCount());

        //
        // Reseed the random number generator.
        //
        RandomSeed();
    }
}

//*****************************************************************************
//
// Creates a spider task.
//
//*****************************************************************************
static unsigned long
CreateSpider(long lX, long lY)
{
    unsigned long lSpider;

    //
    // Search to see if there is a spider task available.
    //
    for(lSpider = 0; lSpider < MAX_SPIDERS; lSpider++)
    {
        if(HWREGBITW(&g_ulSpiderAlive, lSpider) == 0)
        {
            break;
        }
    }

    //
    // Return a failure if no spider tasks are available (in other words, the
    // maximum number of spiders are already alive).
    //
    if(lSpider == MAX_SPIDERS)
    {
        return(1);
    }

    //
    // Adjust the starting horizontal position to make sure it is inside the
    // allowable area for the spiders.
    //
    if(lX < SPIDER_MIN_X)
    {
        lX = SPIDER_MIN_X;
    }
    else if(lX > SPIDER_MAX_X)
    {
        lX = SPIDER_MAX_X;
    }

    //
    // Adjust the starting vertical position to make sure it is inside the
    // allowable area for the spiders.
    //
    if(lY < SPIDER_MIN_Y)
    {
        lY = SPIDER_MIN_Y;
    }
    else if(lY > SPIDER_MAX_Y)
    {
        lY = SPIDER_MAX_Y;
    }

    //
    // Save the starting position for this spider.
    //
    g_plSpiderX[lSpider] = lX;
    g_plSpiderY[lSpider] = lY;

    //
    // Create a task to animate this spider.
    //
    if(xTaskCreate(SpiderTask, (signed portCHAR *)"Spider",
                   (signed portCHAR *)(g_ppulSpiderTaskStack[lSpider]),
                   sizeof(g_ppulSpiderTaskStack[0]), (void *)lSpider, 0,
                   NULL) != pdPASS)
    {
        return(1);
    }
    TaskCreated();

    //
    // Success.
    //
    return(0);
}

//*****************************************************************************
//
// The callback function for messages from the touch screen driver.
//
//*****************************************************************************
static long
ControlTouchCallback(unsigned long ulMessage, long lX, long lY)
{
    portBASE_TYPE bTaskWaken;

    //
    // Ignore all messages other than pointer down messages.
    //
    if(ulMessage != WIDGET_MSG_PTR_DOWN)
    {
        return(0);
    }

    //
    // Pack the position into a message to send to the spider control task.
    //
    ulMessage = ((lX & 65535) << 16) | (lY & 65535);

    //
    // Send the position message to the spider control task.
    //
    xQueueSendFromISR(g_pControlQueue, &ulMessage, &bTaskWaken);

    //
    // Perform a task yield if necessary.
    //
    taskYIELD_FROM_ISR(bTaskWaken);

    //
    // This message has been handled.
    //
    return(0);
}

//*****************************************************************************
//
// Determines if a given touch screen point collides with one of the spiders.
//
//*****************************************************************************
static long
SpiderTouchCollide(long lX, long lY)
{
    long lIdx, lDX, lDY, lBest, lDist;

    //
    // Until a collision is found, there is no best spider choice.
    //
    lBest = -1;
    lDist = 1000000;

    //
    // Loop through all the spiders.
    //
    for(lIdx = 0; lIdx < MAX_SPIDERS; lIdx++)
    {
        //
        // Skip this spider if it is not alive.
        //
        if((HWREGBITW(&g_ulSpiderAlive, lIdx) == 0) ||
           (HWREGBITW(&g_ulSpiderDead, lIdx) == 1))
        {
            continue;
        }

        //
        // Compute the horizontal and vertical difference between this spider's
        // position and the point in question.
        //
        lDX = ((g_plSpiderX[lIdx] > lX) ? (g_plSpiderX[lIdx] - lX) :
               (lX - g_plSpiderX[lIdx]));
        lDY = ((g_plSpiderY[lIdx] > lY) ? (g_plSpiderY[lIdx] - lY) :
               (lY - g_plSpiderY[lIdx]));

        //
        // See if the point in question collides with this spider.
        //
        if((lDX < (SPIDER_WIDTH + 4)) && (lDY < (SPIDER_HEIGHT + 4)))
        {
            //
            // Compute distance (squared) between this point and the spider.
            //
            lDX = (lDX * lDX) + (lDY * lDY);

            //
            // See if this spider is closer to the point in question than any
            // other spider encountered.
            //
            if(lDX < lDist)
            {
                //
                // Save this spider as the new best choice.
                //
                lBest = lIdx;
                lDist = lDX;
            }
        }
    }

    //
    // Return the best choice, if one was found.
    //
    if(lBest != -1)
    {
        return(lBest);
    }

    //
    // Loop through all the spiders.  This time, the spiders that are dead but
    // not cleared from the screen are not ignored.
    //
    for(lIdx = 0; lIdx < MAX_SPIDERS; lIdx++)
    {
        //
        // Skip this spider if it is not alive.
        //
        if(HWREGBITW(&g_ulSpiderAlive, lIdx) == 0)
        {
            continue;
        }

        //
        // Compute the horizontal and vertical difference between this spider's
        // position and the point in question.
        //
        lDX = ((g_plSpiderX[lIdx] > lX) ? (g_plSpiderX[lIdx] - lX) :
               (lX - g_plSpiderX[lIdx]));
        lDY = ((g_plSpiderY[lIdx] > lY) ? (g_plSpiderY[lIdx] - lY) :
               (lY - g_plSpiderY[lIdx]));

        //
        // See if the point in question collides with this spider.
        //
        if((lDX < (SPIDER_WIDTH + 4)) && (lDY < (SPIDER_HEIGHT + 4)))
        {
            //
            // Compute distance (squared) between this point and the spider.
            //
            lDX = (lDX * lDX) + (lDY * lDY);

            //
            // See if this spider is closer to the point in question than any
            // other spider encountered.
            //
            if(lDX < lDist)
            {
                //
                // Save this spider as the new best choice.
                //
                lBest = lIdx;
                lDist = lDX;
            }
        }
    }

    //
    // Return the best choice, if one was found.
    //
    return(lBest);
}

//*****************************************************************************
//
// This task provides overall control of the spiders, spawning and killing them
// in response to presses on the touch screen.
//
//*****************************************************************************
static void
ControlTask(void *pvParameters)
{
    unsigned long ulMessage;
    long lX, lY, lSpider;

    //
    // Initialize the touch screen driver and register a callback function.
    //
    TouchScreenInit();
    TouchScreenCallbackSet(ControlTouchCallback);

    //
    // Lower the priority of the touch screen interrupt handler.  This is
    // required so that the interrupt handler can safely call the interrupt-
    // safe SafeRTOS functions (specifically to send messages to the queue).
    //
    IntPrioritySet(INT_ADC0SS3, 0xc0);

    //
    // Loop forever.
    //
    while(1)
    {
        //
        // Read the next message from the queue.
        //
        if(xQueueReceive(g_pControlQueue, &ulMessage, portMAX_DELAY) == pdPASS)
        {
            //
            // Extract the position of the screen touch from the message.
            //
            lX = ulMessage >> 16;
            lY = ulMessage & 65535;

            //
            // Ignore this screen touch if it is not inside the spider area.
            //
            if((lX >= AREA_X) && (lX < (AREA_X + AREA_WIDTH)) &&
               (lY >= AREA_Y) && (lY < (AREA_Y + AREA_HEIGHT)))
            {
                //
                // See if this position collides with any of the spiders.
                //
                lSpider = SpiderTouchCollide(lX, lY);
                if(lSpider == -1)
                {
                    //
                    // There is no collision, so create a new spider (if
                    // possible) at this position.
                    //
                    CreateSpider(lX, lY);
                }
                else
                {
                    //
                    // There is a collision, so kill this spider.
                    //
                    HWREGBITW(&g_ulSpiderDead, lSpider) = 1;
                }
            }
        }
    }
}

//*****************************************************************************
//
// Sets the speed of the spiders by specifying the number of milliseconds
// between updates to the spider's position.
//
//*****************************************************************************
void
SpiderSpeedSet(unsigned long ulSpeed)
{
    //
    // Convert the update rate from milliseconds to ticks.  The second entry
    // of the array is 1.4 times the first so that updates when moving along
    // the diagonal, which are longer steps, are done less frequently by a
    // proportional amount.
    //
    g_pulSpiderDelay[0] = (ulSpeed * (1000 / portTICK_RATE_MS)) / 1000;
    g_pulSpiderDelay[1] = (ulSpeed * 14 * (1000 / portTICK_RATE_MS)) / 10000;
}

//*****************************************************************************
//
// Initializes the spider tasks.
//
//*****************************************************************************
unsigned long
SpiderTaskInit(void)
{
    unsigned long ulIdx;

    //
    // Set the initial speed of the spiders.
    //
    SpiderSpeedSet(10);

    //
    // Create a queue for sending messages to the spider control task.
    //
    if(xQueueCreate((signed portCHAR *)g_pulControlQueueMem, CONTROL_MEM_SIZE,
                    CONTROL_QUEUE_SIZE, CONTROL_ITEM_SIZE,
                    &g_pControlQueue) != pdPASS)
    {
        return(1);
    }

    //
    // Create the spider control task.
    //
    if(xTaskCreate(ControlTask, (signed portCHAR *)"Control",
                   (signed portCHAR *)g_pulControlTaskStack,
                   sizeof(g_pulControlTaskStack), NULL, PRIORITY_CONTROL_TASK,
                   NULL) != pdPASS)
    {
        return(1);
    }
    TaskCreated();

    //
    // Create eight spiders initially.
    //
    for(ulIdx = 0; ulIdx < 8; ulIdx++)
    {
        //
        // Create a spider that is centered vertically and equally spaced
        // horizontally across the display.
        //
        if(CreateSpider((ulIdx * (AREA_WIDTH / 8)) + (AREA_WIDTH / 16),
                        (AREA_HEIGHT / 2) + AREA_Y) == 1)
        {
            return(1);
        }

        //
        // Provide an early indication that this spider is alive.  The task is
        // not running yet (since this function is called before the scheduler
        // has been started) so this variable is not set by the task (yet).
        // Manually setting it allows the remaining initial spiders to be
        // created properly.
        //
        HWREGBITW(&g_ulSpiderAlive, ulIdx) = 1;
    }

    //
    // Success.
    //
    return(0);
}
