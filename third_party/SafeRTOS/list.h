/*
      SafeRTOS Copyright (C) Wittenstein High Integrity Systems.

      See projdefs.h for version number information.

      SafeRTOS has been licensed by Wittenstein High Integrity Systems to
      Texas Instruments to be embedded within the ROM of certain processors.
      If SafeRTOS is embedded in the ROM of your TI processor you may copy and
      use this header file to facilitate the use of SafeRTOS as described in the
      User Manual.

      If you use SafeRTOS in a safety related application or it is required to meet
      high dependability requirements you must use SafeRTOS in accordance with the
      Safety Manual which forms a part of the Design Assurance Pack (DAP). The
      DAP and support may be purchased separately from WITTENSTEIN high integrity
      systems.

      WITTENSTEIN high integrity systems is a trading name of WITTENSTEIN
      aerospace & simulation ltd, Registered Office: Brown's Court, Long Ashton
      Business Park, Yanley Lane, Long Ashton, Bristol, BS41 9LB, UK.
      Tel: +44 (0) 1275 395 600, fax: +44 (0) 1275 393 630.
      E-mail: info@HighIntegritySystems.com
      Registered in England No. 3711047; VAT No. GB 729 1583 15

      http://www.SafeRTOS.com
*/
/*
	Functions for internal use only.  These functions are documented within
	the software design description.  Do not refer to this file for
	documentation.
*/

#ifndef LIST_H
#define LIST_H

struct xLIST_ITEM
{
	portTickType xItemValue;				
	volatile struct xLIST_ITEM * pxNext;	
	volatile struct xLIST_ITEM * pxPrevious;
	void * pvOwner;							
	void * pvContainer;						
};
typedef struct xLIST_ITEM xListItem;		

typedef struct xLIST
{
	volatile unsigned portBASE_TYPE uxNumberOfItems;
	volatile xListItem * pxIndex;			
	volatile xListItem xListEnd;			
} xList;


#define listSET_LIST_ITEM_OWNER( pxListItem, pxOwner )		( ( pxListItem )->pvOwner = ( void * ) pxOwner )
#define listSET_LIST_ITEM_VALUE( pxListItem, xValue )		( ( pxListItem )->xItemValue = xValue )
#define listGET_LIST_ITEM_VALUE( pxListItem )				( ( pxListItem )->xItemValue )
#define listLIST_IS_EMPTY( pxList )							( ( portBASE_TYPE ) ( ( pxList )->uxNumberOfItems == ( unsigned portBASE_TYPE ) 0 ) )
#define listCURRENT_LIST_LENGTH( pxList )					( ( pxList )->uxNumberOfItems )
#define listGET_OWNER_OF_HEAD_ENTRY( pxList )  				( ( pxList->uxNumberOfItems != ( unsigned portBASE_TYPE ) 0 ) ? ( (&( pxList->xListEnd ))->pxNext->pvOwner ) : ( NULL ) )
#define listGUARANTEED_GET_OWNER_OF_HEAD_ENTRY( pxList ) 	( ( &( pxList->xListEnd ))->pxNext->pvOwner )
#define listIS_CONTAINED_WITHIN( pxList, pxListItem ) 		( ( portBASE_TYPE ) ( ( pxListItem )->pvContainer == ( void * ) pxList ) )


#define listGET_OWNER_OF_NEXT_ENTRY( pxTCB, pxList )									\
	/* Increment the index to the next item and return the item, ensuring */			\
	/* we don't return the marker used at the end of the list.  */						\
	( pxList )->pxIndex = ( pxList )->pxIndex->pxNext;									\
	if( ( pxList )->pxIndex == &( ( pxList )->xListEnd ) )								\
	{																					\
		( pxList )->pxIndex = ( pxList )->pxIndex->pxNext;								\
	}																					\
	pxTCB = ( xTCB * ) ( pxList )->pxIndex->pvOwner


void vListInitialise( xList *pxList );
void vListInitialiseItem( xListItem *pxItem );
void vListInsertOrdered( xList *pxList, xListItem *pxNewListItem );
void vListInsertEnd( xList *pxList, xListItem *pxNewListItem );
void vListRemove( xListItem *pxItemToRemove );



#endif /* LIST_H */



