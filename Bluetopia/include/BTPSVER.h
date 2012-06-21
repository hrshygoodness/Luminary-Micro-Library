/*****< btpsver.h >************************************************************/
/*      Copyright 2009 - 2011 Stonestreet One.                                */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*  BTPSVER - Stonestreet One Bluetooth Protocol Stack Version Information.   */
/*                                                                            */
/*  Author:  Damon Lange                                                      */
/*                                                                            */
/*** MODIFICATION HISTORY *****************************************************/
/*                                                                            */
/*   mm/dd/yy  F. Lastname    Description of Modification                     */
/*   --------  -----------    ------------------------------------------------*/
/*   08/03/09  D. Lange       Initial creation.                               */
/******************************************************************************/
#ifndef __BTPSVERH__
#define __BTPSVERH__

   /* Bluetooth Protocol Stack Major and Minor Version Numbers.         */
#define BTPS_VERSION_MAJOR_VERSION_NUMBER                   2
#define BTPS_VERSION_MINOR_VERSION_NUMBER                   1

   /* Bluetooth Protocol Stack Release Number.                          */
#ifndef BTPS_VERSION_RELEASE_NUMBER
   #define BTPS_VERSION_RELEASE_NUMBER                      3
#endif

   /* Bluetooth Protocol Stack Revision Number.                         */
#ifndef BTPS_VERSION_REVISION_NUMBER
   #define BTPS_VERSION_REVISION_NUMBER                     2
#endif

   /* Constants used to convert numeric constants to string constants   */
   /* (used in MACRO's below).                                          */
#define BTPS_VERSION_NUMBER_TO_STRING(_x)                   #_x
#define BTPS_VERSION_CONSTANT_TO_STRING(_y)                 BTPS_VERSION_NUMBER_TO_STRING(_y)

   /* Bluetooth Protocol Stack Constant Version String of the form      */
   /*    "a.b.c.d"                                                      */
   /* where:                                                            */
   /*    a - BTPS_VERSION_MAJOR_VERSION_NUMBER                          */
   /*    b - BTPS_VERSION_MINOR_VERSION_NUMBER                          */
   /*    c - BTPS_VERSION_RELEASE_NUMBER                                */
   /*    d - BTPS_VERSION_REVISION_NUMBER                               */
#define BTPS_VERSION_VERSION_STRING                         BTPS_VERSION_CONSTANT_TO_STRING(BTPS_VERSION_MAJOR_VERSION_NUMBER) "." BTPS_VERSION_CONSTANT_TO_STRING(BTPS_VERSION_MINOR_VERSION_NUMBER) "." BTPS_VERSION_CONSTANT_TO_STRING(BTPS_VERSION_RELEASE_NUMBER) "." BTPS_VERSION_CONSTANT_TO_STRING(BTPS_VERSION_REVISION_NUMBER)

   /* File/Product/Company Name Copyrights and Descriptions.            */
#define BTPS_VERSION_PRODUCT_NAME_STRING                    "Bluetooth Protocol Stack"

#define BTPS_VERSION_COMPANY_NAME_STRING                    "Stonestreet One"

#define BTPS_VERSION_COPYRIGHT_STRING                       "Copyright (C) 2000-11 Stonestreet One"

   /* Windows specific common extensions for Resource Files.            */
#define VER_PRODUCTNAME_STR                                 BTPS_VERSION_PRODUCT_NAME_STRING "\000\000"
#define VER_PRODUCTNAME_STR                                 BTPS_VERSION_PRODUCT_NAME_STRING "\000\000"

#define VER_PRODUCTVERSION                                  BTPS_VERSION_MAJOR_VERSION_NUMBER, BTPS_VERSION_MINOR_VERSION_NUMBER, BTPS_VERSION_RELEASE_NUMBER, BTPS_VERSION_REVISION_NUMBER
#define VER_PRODUCTVERSION_STR                              BTPS_VERSION_VERSION_STRING

#define VER_COMPANYNAME_STR                                 BTPS_VERSION_COMPANY_NAME_STRING "\000\000"

#define VER_LEGALCOPYRIGHT_STR                              BTPS_VERSION_COPYRIGHT_STRING "\000\000"

#endif
