/*******************************************************************************
*  SDLGLSTR.H                                                                  *
*      - Output of text in an OpenGL-Window.                                   *
*                                                                              *
*  Copyright (C) 2002-2010  Paul M�ller <pmtech@swissonline.ch>                *
*                                                                              *
*  This program is free software; you can redistribute it and/or modify        *
*  it under the terms of the GNU General Public License as published by        *
*  the Free Software Foundation; either version 2 of the License, or           *
*  (at your option) any later version.                                         *
*                                                                              *
*  This program is distributed in the hope that it will be useful,             *
*  but WITHOUT ANY WARRANTY; without even the implied warranty of              *
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               *
*  GNU Library General Public License for more details.                        *
*                                                                              *
*  You should have received a copy of the GNU General Public License           *
*  along with this program; if not, write to the Free Software                 *
*  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  * 
*******************************************************************************/

#ifndef _SDLGLSTR_H_
#define _SDLGLSTR_H_

/*******************************************************************************
* INCLUDES         							                                   *
*******************************************************************************/

#include "sdlgl.h"

/******************************************************************************
* DEFINES								      								  *
******************************************************************************/

#define SDLGLSTR_FONT6     0    /* Font 6 Points width and 8 Points height */
#define SDLGLSTR_FONT8	   1	/* Font 8 Points width and 8 Points height */
#define SDLGLSTR_FONTSPACE 2    /* Font 8 Points width and 8 Points height */

/* Flags for drawing routines */
#define SDLGLSTR_FINVISIBLE   0x80000000    /* Is invisible                 */
#define SDLGLSTR_FHIGHLIGHT   0x40000000    /* is highlighted               */
#define SDLGLSTR_FINVERTED    0x20000000	/* Draw an inverted button 	    */
#define SDLGLSTR_FHIGHLIGHTED 0x10000000	/* Use highlight color for text	*/
#define SDLGLSTR_FHCENTER     0x08000000	/* Center the text horizontally */
                					        /* in given rectangle		    */
#define SDLGLSTR_FVCENTER     0x04000000	/* Center the text vertically 	*/
#define SDLGLSTR_FRIGHTADJUST 0x02000000    /* Right adjust string          */
#define SDLGLSTR_FHIDEBUTTON  0x01000000	/* Don't draw the button	    */
#define SDLGLSTR_FTEXTURED    0x00800000    /* Draw button with texture     */
#define SDLGLSTR_FEMPTYBUTTON 0x00400000	/* Draw now middle color for button */

#define SDLGLSTR_FBUTTONSTATE 0x00400000        /* State for radio/push buttons */
#define SDLGLSTR_FCENTERED    (SDLGLSTR_FHCENTER | SDLGLSTR_FVCENTER)

/* The basic styles available	*/
#define SDLGLSTR_STYLEBLUE   0
#define SDLGLSTR_STYLEEUROPE 1
#define SDLGLSTR_STYLEWOOD   2

/******************************************************************************
* TYPEDEFS    								      *
******************************************************************************/

typedef struct {

    int fontno;				/* Font to use for drawing         */
    unsigned char buttonmid[3],		/* Color of buttons mid            */
		  buttontop[3],		/* Color of buttons topside        */
                  buttonbottom[3];	/* Color of buttons bottomside     */
    unsigned char textlo[3],		/* Color of non highlighted text   */
                  hotkeylo[3];          /* Color of non highlighted hotkey */
    unsigned char texthi[3],		/* Color of highlighted text       */
                  hotkeyhi[3];          /* Color of highlighted hotkey     */
    unsigned char label[3];	        /* Color for label text in dialog  */
    unsigned char scrollbk[3];		/* Color for scrollbox background  */
    unsigned char maplabel[3];		/* Color for labels in map	   */
    unsigned char maplabelshadow[3];	/* Color for the shadow for mapl.  */
    int           texbklistno;		/* Number of the texture which	   */
					/* holds the background texture	   */
    int 	  texbkno;		/* Number of texture in tex-list   */

} SDLGLSTR_STYLE;


/******************************************************************************
* ROUTINES								      								  *
******************************************************************************/

/* Initializing an shutdown */
int  sdlglstrInit(void);
void sdlglstrShutdown(void);

/* Size and calc functions */
void sdlglstrGetStringSize(char *text, SDLGL_RECT *rect);
void sdlglstrGetButtonSize(char *string, SDLGL_RECT *rect);
int  sdlglstrGetCharPos(char *string, int pixelx);

/* Drawing function */
void sdlglstrString(SDLGL_RECT *pos, char *text);
void sdlglstrStringF(SDLGL_RECT *pos, char *text, ...);
void sdlglstrChar(SDLGL_RECT *pos, char text);
void sdlglstrStringBackground(SDLGL_RECT *pos, int len, int color);
void sdlglstrStringPos(SDLGL_RECT *rect, char *text, int flags);

/* Additional text drawing functions for dialog boxes, using TEXTOUTSTYLE */
void sdlglstrStringStyle(SDLGL_RECT *pos, char *text, int highlight);
void sdlglstrStringToRect(SDLGL_RECT *rect, char *string);
void sdlglstrDrawButton(SDLGL_RECT *rect, char *text, int flags);
void sdlglstrDrawEditField(SDLGL_RECT *rect, char *text, int curpos);
void sdlglstrDrawSpecial(SDLGL_RECT *rect, char *text, int which, int flags);
SDLGL_FIELD *sdlglstrDrawField(SDLGL_FIELD *field);
void sdlglstrPrintValue(SDLGL_RECT *rect, void *value, int which); 

/* Additional function for user defined fonts and styles */
int  sdlglstrSetFont(int fontno);
void sdlglstrSetColorNo(int colorno);
void sdlglstrSetColor(unsigned char r, unsigned char g, unsigned char b);
void sdlglstrAddFont(unsigned char *font, int fontw, int fonth, int fontno);
void sdlglstrInitDrawStyle(int styleno, SDLGLSTR_STYLE *style);
void sdlglstrSetDrawStyle(int styleno);

/* Additional drawing functions */
void sdlglstrDrawRect(SDLGL_RECT *rect, unsigned char *color, int filled);
void sdlglstrDrawRectColNo(SDLGL_RECT *rect,int colorno, int filled);

#endif /* _SDLGLSTR_H_ */

