//*****************************************************************************
//
// lmimaze.java - Java applet to display the full maze in real-time on a web
//                page.
//
// Copyright (c) 2007-2013 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 10636 of the EK-LM3S8962 Firmware Package.
//
//*****************************************************************************

//*****************************************************************************
//
// This file is not automatically built as part of the build process; it
// requires the Java Development Kit which is not installed on a typical
// system.  The kit can be obtained from www.java.com.
//
// Once installed, this file can be compiled with:
//
//     javac -target 1.5 -d html lmimaze.java
//
// Which will produce the Java byte code file and place it into the game
// filesystem.
//
//*****************************************************************************

import java.applet.*;
import java.awt.*;
import java.io.*;
import java.net.*;

//*****************************************************************************
//
// An applet that draws the maze data read from the quick start application.
//
//*****************************************************************************
public class lmimaze extends Applet implements Runnable
{
    byte maze[], monster[], player[];
    int buffer, refresh, refreshOne;
    Image backbuffer1, backbuffer2;
    boolean threadSuspended;
    Graphics backg1, backg2;
    Thread t = null;

    //*************************************************************************
    //
    // Initializes the applet.
    //
    //*************************************************************************
    public void init()
    {
        //
        // Create an image and drawing context for the first backing image.
        //
        backbuffer1 = createImage(635, 470);
        backg1 = backbuffer1.getGraphics();

        //
        // Create an image and drawing context for the second backing image.
        //
        backbuffer2 = createImage(635, 470);
        backg2 = backbuffer2.getGraphics();

        //
        // Set the window background color to black.
        //
        setBackground(Color.black);

        //
        // The first backing image should be displayed on the screen first.
        //
        buffer = 0;

        //
        // Allocate a byte array to hold the maze data, monster position data,
        // and player position data.
        //
        maze = new byte[127 * 94];
        monster = new byte[100 * 4];
        player = new byte[4];

        //
        // Initialize the refresh and single refresh variables.
        //
        refresh = 1;
        refreshOne = 0;
    }

    //*************************************************************************
    //
    // Starts the background thread for the applet.
    //
    //*************************************************************************
    public void start()
    {
        //
        // See if the background thread has been created.
        //
        if(t == null)
        {
            //
            // Create the background thread.
            //
            t = new Thread(this);

            //
            // Indicate that the background thread is allowed to run.
            //
            threadSuspended = false;

            //
            // Start the background thread.
            //
            t.start();
        }
        else
        {
            //
            // See if the background thread is currently suspended.
            //
            if(threadSuspended)
            {
                //
                // Indicate that the background thread is allowed to run.
                //
                threadSuspended = false;

                //
                // Syncronously notify the background thread.
                //
                synchronized(this)
                {
                    notify();
                }
            }
        }
    }

    //*************************************************************************
    //
    // Suspend the background thread.
    //
    //*************************************************************************
    public void stop()
    {
        //
        // Indicate that the background thread is not allowed to run.
        //
        threadSuspended = true;
    }

    //*************************************************************************
    //
    // The thread code for the background thread.
    //
    //*************************************************************************
    public void run()
    {
        //
        // Catch exceptions that may occur during execution of the background
        // thread.
        //
        try
        {
            //
            // Loop forever.
            //
            while(true)
            {
                //
                // See if the background thread is suspended, or if screen
                // refresh has been turned off.
                //
                if(threadSuspended || ((refresh == 0) && (refreshOne == 0)))
                {
                    //
                    // Synchronize with the main thread.
                    //
                    synchronized(this)
                    {
                        //
                        // Wait while the background thread is suspended or if
                        // screen refrehs has been turned off.
                        //
                        while(threadSuspended ||
                              ((refresh == 0) && (refreshOne == 0)))
                        {
                            wait();
                        }
                    }
                }

                //
                // Catch excpetions that may occur during the web accesses.
                //
                try
                {
                    HttpURLConnection connection;
                    InputStream in;
                    int x, y, idx;
                    Graphics g;
                    URL server;

                    //
                    // Construct the URL to the maze data file.
                    //
                    server = new URL(getCodeBase() + "maze.dat");

                    //
                    // Create a connection to the URL.
                    //
                    connection = (HttpURLConnection)server.openConnection();

                    //
                    // Disable caching.
                    //
                    connection.setUseCaches(false);

                    //
                    // Connect to the server.
                    //
                    connection.connect();

                    //
                    // Get an input stream for the maze data file.
                    //
                    in = connection.getInputStream();

                    //
                    // Read the maze data file from the server.  A loop is
                    // needed since all the data may not be returned by a
                    // single read.
                    //
                    idx = 0;
                    while(idx != (127 * 94))
                    {
                        x = in.read(maze, idx, (127 * 94) - idx);
                        if(x == -1)
                        {
                            break;
                        }
                        idx += x;
                    }

                    //
                    // Close the connection to the server.
                    //
                    connection.disconnect();

                    //
                    // Construct the URL to the monster position data file.
                    //
                    server = new URL(getCodeBase() + "monster.dat");

                    //
                    // Create a connection to the URL.
                    //
                    connection = (HttpURLConnection)server.openConnection();

                    //
                    // Disable caching.
                    //
                    connection.setUseCaches(false);

                    //
                    // Connect to the server.
                    //
                    connection.connect();

                    //
                    // Get an input stream for the monster position data file.
                    //
                    in = connection.getInputStream();

                    //
                    // Read the monster position data file from the server.  A
                    // loop is needed since all the data may not be returned by
                    // a single read.
                    //
                    idx = 0;
                    while(idx != (100 * 4))
                    {
                        x = in.read(monster, idx, (100 * 4) - idx);
                        if(x == -1)
                        {
                            break;
                        }
                        idx += x;
                    }

                    //
                    // Close the connection to the server.
                    //
                    connection.disconnect();

                    //
                    // Construct the URL to the player position data file.
                    //
                    server = new URL(getCodeBase() + "player.dat");

                    //
                    // Create a connection to the URL.
                    //
                    connection = (HttpURLConnection)server.openConnection();

                    //
                    // Disable caching.
                    //
                    connection.setUseCaches(false);

                    //
                    // Connect to the server.
                    //
                    connection.connect();

                    //
                    // Get an input stream for the player position data file.
                    //
                    in = connection.getInputStream();

                    //
                    // Read the player position data file from the server.  A
                    // loop is needed since all the data may not be returned by
                    // a single read.
                    //
                    idx = 0;
                    while(idx != 4)
                    {
                        x = in.read(player, idx, 4 - idx);
                        if(x == -1)
                        {
                            break;
                        }
                        idx += x;
                    }

                    //
                    // Close the connection to the server.
                    //
                    connection.disconnect();

                    //
                    // Get the graphics context to use for drawing this maze
                    // data.
                    //
                    if(buffer == 0)
                    {
                        g = backg2;
                    }
                    else
                    {
                        g = backg1;
                    }

                    //
                    // Loop through the rows of the maze.
                    //
                    idx = 0;
                    for(y = 0; y < 470; y += 5)
                    {
                        //
                        // Loop through the columns of the maze.
                        //
                        for(x = 0; x < 635; x += 5)
                        {
                            //
                            // See if there is a wall at this point in the
                            // maze.
                            //
                            if(maze[idx] == 0)
                            {
                                //
                                // Clear this part of the image.
                                //
                                g.setColor(Color.black);
                                g.fillRect(x, y, 5, 5);
                            }
                            else
                            {
                                //
                                // There is a wall, so see if it connects to a
                                // wall segment above it.
                                //
                                if((maze[idx] & 1) == 1)
                                {
                                    //
                                    // Draw the top row of this wall segment
                                    // based on a wall segment being above it.
                                    //
                                    g.setColor(Color.darkGray);
                                    g.drawLine(x, y, x, y);
                                    g.setColor(Color.white);
                                    g.drawLine(x + 1, y, x + 3, y);
                                    g.setColor(Color.lightGray);
                                    g.drawLine(x + 4, y, x + 4, y);
                                }
                                else
                                {
                                    //
                                    // Draw the top row of this wall segment
                                    // based on there not being a wall segment
                                    // above it.
                                    //
                                    g.setColor(Color.darkGray);
                                    g.drawLine(x, y, x + 4, y);
                                }

                                //
                                // See if this wall segment connects to a wall
                                // segment below it.
                                //
                                if((maze[idx] & 2) == 2)
                                {
                                    //
                                    // Draw the bottom row of this wall segment
                                    // based on a wall segment being below it.
                                    //
                                    g.setColor(Color.darkGray);
                                    g.drawLine(x, y + 4, x, y + 4);
                                    g.setColor(Color.white);
                                    g.drawLine(x + 1, y + 4, x + 3, y + 4);
                                    g.setColor(Color.lightGray);
                                    g.drawLine(x + 4, y + 4, x + 4, y + 4);
                                }
                                else
                                {
                                    //
                                    // Draw the bottom row of this wall segment
                                    // based on there not being a wall segment
                                    // below it.
                                    //
                                    g.setColor(Color.lightGray);
                                    g.drawLine(x, y + 4, x + 4, y + 4);
                                }

                                //
                                // See if this wall segment connects to a wall
                                // segment to its right.
                                //
                                if((maze[idx] & 4) == 4)
                                {
                                    //
                                    // The right column should be white.
                                    //
                                    g.setColor(Color.white);
                                }
                                else
                                {
                                    //
                                    // The right column should be light gray.
                                    //
                                    g.setColor(Color.lightGray);
                                }

                                //
                                // Draw the right column of this wall segment
                                // based on the color chosen above.
                                //
                                g.drawLine(x + 4, y + 1, x + 4, y + 3);

                                //
                                // See if this wall segment connects to a wall
                                // segment to its left.
                                //
                                if((maze[idx] & 8) == 8)
                                {
                                    //
                                    // The left column should be white.
                                    //
                                    g.setColor(Color.white);
                                }
                                else
                                {
                                    //
                                    // The left column should be dark gray.
                                    //
                                    g.setColor(Color.darkGray);
                                }

                                //
                                // Draw the left colunn of this wall segment
                                // based on the color chosen above.
                                //
                                g.drawLine(x, y + 1, x, y + 3);

                                //
                                // Draw the interior of the wall segment.
                                //
                                g.setColor(Color.white);
                                g.fillRect(x + 1, y + 1, 3, 3);
                            }

                            //
                            // Go to the next byte of the maze data.
                            //
                            idx++;
                        }
                    }

                    //
                    // Set the drawing color to red.
                    //
                    g.setColor(Color.red);

                    //
                    // Loop through the monsters.
                    //
                    for(idx = 0; idx < 400; idx += 4)
                    {
                        //
                        // Get the position of this monster.
                        //
                        x = ((monster[idx] & 0xff) +
                             ((monster[idx + 1] & 0xff) * 256));
                        y = ((monster[idx + 2] & 0xff) +
                             ((monster[idx + 3] & 0xff) * 256));

                        //
                        // See if this monster exists.
                        //
                        if((x != 0) || (y != 0))
                        {
                            //
                            // Translate the monster position from the full
                            // maze coordinate to the image coordinate.
                            //
                            x = (x * 5) / 12;
                            y = (y * 5) / 12;

                            //
                            // Draw this monster on the image.
                            //
                            g.fillRect(x + 1, y, 2, 4);
                            g.fillRect(x, y + 1, 4, 2);
                        }
                    }

                    //
                    // Set the drawing color to green.
                    //
                    g.setColor(Color.green);

                    //
                    // Get the position of the player.
                    //
                    x = (player[0] & 0xff) + ((player[1] & 0xff) * 256);
                    y = (player[2] & 0xff) + ((player[3] & 0xff) * 256);

                    //
                    // See if the player is located in the maze.
                    //
                    if((x != 0) || (y != 0))
                    {
                        //
                        // Translate the player position from the full maze
                        // coordinates to the image coordinates.
                        //
                        x = (x * 5) / 12;
                        y = (y * 5) / 12;

                        //
                        // Draw the player on the image.
                        //
                        g.fillRect(x + 1, y, 2, 4);
                        g.fillRect(x, y + 1, 4, 2);
                    }

                    //
                    // Indicate that the newly drawn image should be displayed.
                    //
                    buffer ^= 1;

                    //
                    // Request a refresh of the window.
                    //
                    repaint();

                    //
                    // Clear the refresh now flag.
                    //
                    refreshOne = 0;
                }
                catch(Exception e)
                {
                }
            }
        }
        catch(InterruptedException e)
        {
        }
    }

    //*************************************************************************
    //
    // Updates the window.
    //
    //*************************************************************************
    public void update(Graphics g)
    {
        //
        // See which image should be displayed.
        //
        if(buffer == 0)
        {
            //
            // Draw the first image to the window.
            //
            g.drawImage(backbuffer1, 2, 2, this);
        }
        else
        {
            //
            // Draw the second iamge to the window.
            //
            g.drawImage(backbuffer2, 2, 2, this);
        }
    }

    //*************************************************************************
    //
    // Handles window update requests.
    //
    //*************************************************************************
    public void paint(Graphics g)
    {
        //
        // Update the window.
        //
        update(g);
    }

    //*************************************************************************
    //
    // Changes the state of the auto refresh variable.  Called from JavaScript.
    //
    //*************************************************************************
    public void autoRefresh(int state)
    {
        //
        // Save the state of the refresh variable.
        //
        refresh = state;

        //
        // Synhronously notify the background thread.
        //
        synchronized(this)
        {
            notify();
        }
    }

    //*************************************************************************
    //
    // Requests a refresh of the maze.  Called from JavaScript.
    //
    //*************************************************************************
    public void refreshNow()
    {
        //
        // Set the single refresh variable.
        //
        refreshOne = 1;

        //
        // Synchronously notify the background thread.
        //
        synchronized(this)
        {
            notify();
        }
    }
}
