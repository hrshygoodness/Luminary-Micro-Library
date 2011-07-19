;******************************************************************************
;
; lmi-button.scm - GIMP script for creating a push button background image.
;
; Copyright (c) 2008-2011 Texas Instruments Incorporated.  All rights reserved.
; Software License Agreement
; 
; Texas Instruments (TI) is supplying this software for use solely and
; exclusively on TI's microcontroller products. The software is owned by
; TI and/or its suppliers, and is protected under applicable copyright
; laws. You may not combine this software with "viral" open-source
; software in order to form a larger program.
; 
; THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
; NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
; NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
; A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
; CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
; DAMAGES, FOR ANY REASON WHATSOEVER.
; 
; This is part of revision 7611 of the Stellaris Firmware Development Package.
;
;******************************************************************************

(define (script-fu-lmi-button width
                              height
                              radius
                              thickness
                              color
                              pressed)
  (let* (
        (index 1)
        (greyness 0)
        (thickness (abs thickness))
        (img (car (gimp-image-new width height RGB)))
        (button (car (gimp-layer-new img
                                     width height RGBA-IMAGE
                                     "Background" 100 NORMAL-MODE)))
        (bumpmap (car (gimp-layer-new img
                                      (+ width 2) (+ height 2) RGB-IMAGE
                                      "Bumpmap" 100 NORMAL-MODE)))
        )

    (gimp-context-push)
    (gimp-image-undo-disable img)

    (gimp-image-add-layer img button 1)
    (gimp-image-add-layer img bumpmap 2)

    (gimp-layer-set-offsets bumpmap -1 -1)

    (gimp-context-set-foreground color)
    (gimp-context-set-background '(0 0 0))

    (gimp-edit-fill button BACKGROUND-FILL)

    (gimp-round-rect-select img
                            0 0 width height
                            radius radius
                            CHANNEL-OP-REPLACE TRUE FALSE 0 0)

    (gimp-edit-fill button FOREGROUND-FILL)

    (gimp-drawable-fill bumpmap BACKGROUND-FILL)

    (while (< index thickness)
           (set! greyness (/ (* index 255) thickness))
           (gimp-context-set-background (list greyness greyness greyness))
           (gimp-edit-bucket-fill bumpmap BG-BUCKET-FILL NORMAL-MODE
                                  100 0 FALSE 0 0)
           (gimp-selection-shrink img 1)
           (set! index (+ index 1))
    )
    (gimp-context-set-background '(255 255 255))
    (gimp-edit-bucket-fill bumpmap BG-BUCKET-FILL NORMAL-MODE
                           100 0 FALSE 0 0)

    (gimp-selection-none img)

    (plug-in-bump-map RUN-NONINTERACTIVE img button bumpmap 125 45 3 0 0 0 0
                      TRUE FALSE 1)

    (if (= pressed TRUE) (gimp-image-rotate img ROTATE-180))

    (gimp-image-remove-layer img bumpmap)

    (gimp-image-flatten img)
    (gimp-selection-none img)
    (gimp-image-undo-enable img)
    (gimp-display-new img)

    (gimp-context-pop)
  )
)

(script-fu-register "script-fu-lmi-button"
  _"_LMI Button..."
  _"Create a push button for the Stellaris Graphics Library"
  "support_lmi@ti.com"
  "support_lmi@ti.com"
  "March 2008"
  ""
  SF-ADJUSTMENT _"Width"         '(50 1 1000 1 1 0 1)
  SF-ADJUSTMENT _"Height"        '(50 1 1000 1 1 0 1)
  SF-ADJUSTMENT _"Corner Radius" '(15 0 50 1 2 0 0)
  SF-ADJUSTMENT _"Thickness"     '(5 0 30 1 2 0 0)
  SF-COLOR      _"Color"         "green"
  SF-TOGGLE     _"Pressed"       FALSE
)

(script-fu-menu-register "script-fu-lmi-button"
                         "<Toolbox>/Xtns/Buttons")
