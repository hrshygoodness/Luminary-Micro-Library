/*
	SafeRTOS Copyright (C) Wittenstein High Integrity Systems.

	See projdefs.h for version number information.

	SafeRTOS is distributed exclusively by Wittenstein High Integrity Systems,
	and is subject to the terms of the License granted to your organization,
	including its warranties and limitations on distribution.  It cannot be
	copied or reproduced in any way except as permitted by the License.

	Licenses are issued for each concurrent user working on a specified product
	line.

	WITTENSTEIN high integrity systems is a trading name of WITTENSTEIN
	aerospace & simulation ltd, Registered Office: Brown's Court,, Long Ashton
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



