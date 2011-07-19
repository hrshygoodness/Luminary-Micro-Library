/**
 * @file - stellarisif.c
 * lwIP Ethernet interface for Stellaris Devices
 *
 */

/**
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

/**
 * Copyright (c) 2008 Texas Instruments Incorporated
 *
 * This file is dervied from the ``ethernetif.c'' skeleton Ethernet network
 * interface driver for lwIP.
 *
 */

#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/sys.h"
#include <lwip/stats.h>
#include <lwip/snmp.h>
#include "netif/etharp.h"
#include "netif/ppp_oe.h"
#include "netif/stellarisif.h"

/**
 * Sanity Check:  This interface driver will NOT work if the following defines
 * are incorrect.
 *
 */
#if (PBUF_LINK_HLEN != 16)
#error "PBUF_LINK_HLEN must be 16 for this interface driver!"
#endif
#if (ETH_PAD_SIZE != 2)
#error "ETH_PAD_SIZE must be 2 for this interface driver!"
#endif
#if (!SYS_LIGHTWEIGHT_PROT)
#error "SYS_LIGHTWEIGHT_PROT must be enabled for this interface driver!"
#endif

/**
 * Number of pbufs supported in low-level tx/rx pbuf queue.
 *
 */
#ifndef STELLARIS_NUM_PBUF_QUEUE
#define STELLARIS_NUM_PBUF_QUEUE    20
#endif

/**
 * Setup processing for PTP (IEEE-1588).
 *
 */
#if LWIP_PTPD
extern void lwIPHostGetTime(u32_t *time_s, u32_t *time_ns);
#endif

/**
 * Stellaris DriverLib Header Files required for this interface driver.
 *
 */
#include "inc/hw_ethernet.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/ethernet.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"

/* Define those to better describe your network interface. */
#define IFNAME0 'l'
#define IFNAME1 'm'

/* Helper struct to hold a queue of pbufs for transmit and receive. */
struct pbufq {
  struct pbuf *pbuf[STELLARIS_NUM_PBUF_QUEUE];
  unsigned long qwrite;
  unsigned long qread;
  unsigned long overflow;
};

/* Helper macros for accessing pbuf queues. */
#define PBUF_QUEUE_EMPTY(q) \
    (((q)->qwrite == (q)->qread) ? true : false)

#define PBUF_QUEUE_FULL(q) \
    ((((((q)->qwrite + 1) % STELLARIS_NUM_PBUF_QUEUE)) == (q)->qread) ? \
    true : false )

/**
 * Helper struct to hold private data used to operate your ethernet interface.
 * Keeping the ethernet address of the MAC in this struct is not necessary
 * as it is already kept in the struct netif.
 * But this is only an example, anyway...
 */
struct stellarisif {
  struct eth_addr *ethaddr;
  /* Add whatever per-interface state that is needed here. */
  struct pbufq txq;
};

/**
 * Global variable for this interface's private data.  Needed to allow
 * the interrupt handlers access to this information outside of the
 * context of the lwIP netif.
 *
 */
static struct stellarisif stellarisif_data;

/**
 * Pop a pbuf packet from a pbuf packet queue
 *
 * @param q is the packet queue from which to pop the pbuf.
 *
 * @return pointer to pbuf packet if available, NULL otherswise.
 */
static struct pbuf *
dequeue_packet(struct pbufq *q)
{
  struct pbuf *pBuf;
  SYS_ARCH_DECL_PROTECT(lev);

  /**
   * This entire function must run within a "critical section" to preserve
   * the integrity of the transmit pbuf queue.
   *
   */
  SYS_ARCH_PROTECT(lev);

  if(PBUF_QUEUE_EMPTY(q)) {
    /* Return a NULL pointer if the queue is empty. */
    pBuf = (struct pbuf *)NULL;
  }
  else {
    /**
     * The queue is not empty so return the next frame from it
     * and adjust the read pointer accordingly.
     *
     */
    pBuf = q->pbuf[q->qread];
    q->qread = ((q->qread + 1) % STELLARIS_NUM_PBUF_QUEUE);
  }

  /* Return to prior interrupt state and return the pbuf pointer. */
  SYS_ARCH_UNPROTECT(lev);
  return(pBuf);
}

/**
 * Push a pbuf packet onto a pbuf packet queue
 *
 * @param p is the pbuf to push onto the packet queue.
 * @param q is the packet queue.
 *
 * @return 1 if successful, 0 if q is full.
 */
static int
enqueue_packet(struct pbuf *p, struct pbufq *q)
{
  SYS_ARCH_DECL_PROTECT(lev);
  int ret;

  /**
   * This entire function must run within a "critical section" to preserve
   * the integrity of the transmit pbuf queue.
   *
   */
  SYS_ARCH_PROTECT(lev);

  if(!PBUF_QUEUE_FULL(q)) {
    /**
     * The queue isn't full so we add the new frame at the current
     * write position and move the write pointer.
     *
     */
    q->pbuf[q->qwrite] = p;
    q->qwrite = ((q->qwrite + 1) % STELLARIS_NUM_PBUF_QUEUE);
    ret = 1;
  }
  else {
    /**
     * The stack is full so we are throwing away this value.  Keep track
     * of the number of times this happens.
     *
     */
    q->overflow++;
    ret = 0;
  }

  /* Return to prior interrupt state and return the pbuf pointer. */
  SYS_ARCH_UNPROTECT(lev);
  return(ret);
}

/**
 * In this function, the hardware should be initialized.
 * Called from stellarisif_init().
 *
 * @param netif the already initialized lwip network interface structure
 *        for this ethernetif
 */
static void
stellarisif_hwinit(struct netif *netif)
{
  u32_t temp;
  //struct stellarisif *stellarisif = netif->state;

  /* set MAC hardware address length */
  netif->hwaddr_len = ETHARP_HWADDR_LEN;

  /* set MAC hardware address */
  EthernetMACAddrGet(ETH_BASE, &(netif->hwaddr[0]));

  /* maximum transfer unit */
  netif->mtu = 1500;

  /* device capabilities */
  /* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
  netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;

  /* Do whatever else is needed to initialize interface. */
  /* Disable all Ethernet Interrupts. */
  EthernetIntDisable(ETH_BASE, (ETH_INT_PHY | ETH_INT_MDIO | ETH_INT_RXER |
     ETH_INT_RXOF | ETH_INT_TX | ETH_INT_TXER | ETH_INT_RX));
  temp = EthernetIntStatus(ETH_BASE, false);
  EthernetIntClear(ETH_BASE, temp);

  /* Initialize the Ethernet Controller. */
  EthernetInitExpClk(ETH_BASE, SysCtlClockGet());

  /*
   * Configure the Ethernet Controller for normal operation.
   * - Enable TX Duplex Mode
   * - Enable TX Padding
   * - Enable TX CRC Generation
   * - Enable RX Multicast Reception
   */
  EthernetConfigSet(ETH_BASE, (ETH_CFG_TX_DPLXEN |ETH_CFG_TX_CRCEN |
    ETH_CFG_TX_PADEN | ETH_CFG_RX_AMULEN));

  /* Enable the Ethernet Controller transmitter and receiver. */
  EthernetEnable(ETH_BASE);

  /* Enable the Ethernet Interrupt handler. */
  IntEnable(INT_ETH);

  /* Enable Ethernet TX and RX Packet Interrupts. */
  EthernetIntEnable(ETH_BASE, ETH_INT_RX | ETH_INT_TX);
}

/**
 * This function should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf might be
 * chained.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @param p the MAC packet to send (e.g. IP packet including MAC addresses and type)
 * @return ERR_OK if the packet could be sent
 *         an err_t value if the packet couldn't be sent
 * @note This function MUST be called with interrupts disabled or with the
 *       Stellaris Ethernet transmit fifo protected.
 */
static err_t
stellarisif_transmit(struct netif *netif, struct pbuf *p)
{
  int iBuf;
  unsigned char *pucBuf;
  unsigned long *pulBuf;
  struct pbuf *q;
  int iGather;
  unsigned long ulGather;
  unsigned char *pucGather;

  /**
   * Fill in the first two bytes of the payload data (configured as padding
   * with ETH_PAD_SIZE = 2) with the total length of the payload data
   * (minus the Ethernet MAC layer header).
   *
   */
  *((unsigned short *)(p->payload)) = p->tot_len - 16;

  /* Initialize the gather register. */
  iGather = 0;
  pucGather = (unsigned char *)&ulGather;
  ulGather = 0;

  /* Copy data from the pbuf(s) into the TX Fifo. */
  for(q = p; q != NULL; q = q->next) {
    /* Intialize a char pointer and index to the pbuf payload data. */
    pucBuf = (unsigned char *)q->payload;
    iBuf = 0;

    /**
     * If the gather buffer has leftover data from a previous pbuf
     * in the chain, fill it up and write it to the Tx FIFO.
     *
     */
    while((iBuf < q->len) && (iGather != 0)) {
      /* Copy a byte from the pbuf into the gather buffer. */
      pucGather[iGather] = pucBuf[iBuf++];

      /* Increment the gather buffer index modulo 4. */
      iGather = ((iGather + 1) % 4);
    }

    /**
     * If the gather index is 0 and the pbuf index is non-zero,
     * we have a gather buffer to write into the Tx FIFO.
     *
     */
    if((iGather == 0) && (iBuf != 0)) {
      HWREG(ETH_BASE + MAC_O_DATA) = ulGather;
      ulGather = 0;
    }

    /* Initialze a long pointer into the pbuf for 32-bit access. */
    pulBuf = (unsigned long *)&pucBuf[iBuf];

    /**
     * Copy words of pbuf data into the Tx FIFO, but don't go past
     * the end of the pbuf.
     *
     */
    while((iBuf + 4) <= q->len) {
      HWREG(ETH_BASE + MAC_O_DATA) = *pulBuf++;
      iBuf += 4;
    }

    /**
     * Check if leftover data in the pbuf and save it in the gather
     * buffer for the next time.
     *
     */
    while(iBuf < q->len) {
      /* Copy a byte from the pbuf into the gather buffer. */
      pucGather[iGather] = pucBuf[iBuf++];

      /* Increment the gather buffer index modulo 4. */
      iGather = ((iGather + 1) % 4);
    }
  }

  /* Send any leftover data to the FIFO. */
  HWREG(ETH_BASE + MAC_O_DATA) = ulGather;

  /* Wakeup the transmitter. */
  HWREG(ETH_BASE + MAC_O_TR) = MAC_TR_NEWTX;

  /* Dereference the pbuf from the queue. */
  pbuf_free(p);

  LINK_STATS_INC(link.xmit);

  return(ERR_OK);
}

/**
 * This function with either place the packet into the Stellaris transmit fifo,
 * or will place the packet in the interface PBUF Queue for subsequent
 * transmission when the transmitter becomes idle.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @param p the MAC packet to send (e.g. IP packet including MAC addresses and type)
 * @return ERR_OK if the packet could be sent
 *         an err_t value if the packet couldn't be sent
 *
 */
static err_t
stellarisif_output(struct netif *netif, struct pbuf *p)
{
  struct stellarisif *stellarisif = netif->state;
  SYS_ARCH_DECL_PROTECT(lev);

  /**
   * This entire function must run within a "critical section" to preserve
   * the integrity of the transmit pbuf queue.
   *
   */
  SYS_ARCH_PROTECT(lev);

  /**
   * Bump the reference count on the pbuf to prevent it from being
   * freed till we are done with it.
   *
   */
  pbuf_ref(p);

  /**
   * If the transmitter is idle, and there is nothing on the queue,
   * send the pbuf now.
   *
   */
  if(PBUF_QUEUE_EMPTY(&stellarisif->txq) &&
    ((HWREG(ETH_BASE + MAC_O_TR) & MAC_TR_NEWTX) == 0)) {
    stellarisif_transmit(netif, p);
  }

  /* Otherwise place the pbuf on the transmit queue. */
  else {
    /* Add to transmit packet queue */
    if(!enqueue_packet(p, &(stellarisif->txq))) {
      /* if no room on the queue, free the pbuf reference and return error. */
      pbuf_free(p);
      SYS_ARCH_UNPROTECT(lev);
      return (ERR_MEM);
    }
  }

  /* Return to prior interrupt state and return. */
  SYS_ARCH_UNPROTECT(lev);
  return ERR_OK;
}

/**
 * This function will read a single packet from the Stellaris ethernet
 * interface, if available, and return a pointer to a pbuf.  The timestamp
 * of the packet will be placed into the pbuf structure.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return pointer to pbuf packet if available, NULL otherswise.
 */
static struct pbuf *
stellarisif_receive(struct netif *netif)
{
  struct pbuf *p, *q;
  u16_t len;
  u32_t temp;
  int i;
  unsigned long *ptr;
#if LWIP_PTPD
  u32_t time_s, time_ns;

  /* Get the current timestamp if PTPD is enabled */
  lwIPHostGetTime(&time_s, &time_ns);
#endif


  /* Check if a packet is available, if not, return NULL packet. */
  if((HWREG(ETH_BASE + MAC_O_NP) & MAC_NP_NPR_M) == 0) {
    return(NULL);
  }

  /**
   * Obtain the size of the packet and put it into the "len" variable.
   * Note:  The length returned in the FIFO length position includes the
   * two bytes for the length + the 4 bytes for the FCS.
   *
   */
  temp = HWREG(ETH_BASE + MAC_O_DATA);
  len = temp & 0xFFFF;

  /* We allocate a pbuf chain of pbufs from the pool. */
  p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);

  /* If a pbuf was allocated, read the packet into the pbuf. */
  if(p != NULL) {
    /* Place the first word into the first pbuf location. */
    *(unsigned long *)p->payload = temp;
    p->payload = (char *)(p->payload) + 4;
    p->len -= 4;

    /* Process all but the last buffer in the pbuf chain. */
    q = p;
    while(q != NULL) {
      /* Setup a byte pointer into the payload section of the pbuf. */
      ptr = q->payload;

      /**
       * Read data from FIFO into the current pbuf
       * (assume pbuf length is modulo 4)
       *
       */
      for(i = 0; i < q->len; i += 4) {
        *ptr++ = HWREG(ETH_BASE + MAC_O_DATA);
      }

      /* Link in the next pbuf in the chain. */
      q = q->next;
    }

    /* Restore the first pbuf parameters to their original values. */
    p->payload = (char *)(p->payload) - 4;
    p->len += 4;

    /* Adjust the link statistics */
    LINK_STATS_INC(link.recv);

#if LWIP_PTPD
    /* Place the timestamp in the PBUF */
    p->time_s = time_s;
    p->time_ns = time_ns;
#endif
  }

  /* If no pbuf available, just drain the RX fifo. */
  else {
    for(i = 4; i < len; i+=4) {
      temp = HWREG(ETH_BASE + MAC_O_DATA);
    }

    /* Adjust the link statistics */
    LINK_STATS_INC(link.memerr);
    LINK_STATS_INC(link.drop);
  }

  return(p);
}

/**
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function stellarisif_hwinit() to do the
 * actual setup of the hardware.
 *
 * This function should be passed as a parameter to netif_add().
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return ERR_OK if the loopif is initialized
 *         ERR_MEM if private data couldn't be allocated
 *         any other err_t on error
 */
err_t
stellarisif_init(struct netif *netif)
{
  LWIP_ASSERT("netif != NULL", (netif != NULL));

#if LWIP_NETIF_HOSTNAME
  /* Initialize interface hostname */
  netif->hostname = "lwip";
#endif /* LWIP_NETIF_HOSTNAME */

  /*
   * Initialize the snmp variables and counters inside the struct netif.
   * The last argument should be replaced with your link speed, in units
   * of bits per second.
   */
  NETIF_INIT_SNMP(netif, snmp_ifType_ethernet_csmacd, 1000000);

  netif->state = &stellarisif_data;
  netif->name[0] = IFNAME0;
  netif->name[1] = IFNAME1;
  /* We directly use etharp_output() here to save a function call.
   * You can instead declare your own function an call etharp_output()
   * from it if you have to do some checks before sending (e.g. if link
   * is available...) */
  netif->output = etharp_output;
  netif->linkoutput = stellarisif_output;

  stellarisif_data.ethaddr = (struct eth_addr *)&(netif->hwaddr[0]);
  stellarisif_data.txq.qread = stellarisif_data.txq.qwrite = 0;
  stellarisif_data.txq.overflow = 0;

  /* initialize the hardware */
  stellarisif_hwinit(netif);

  return ERR_OK;
}

/**
 * Process tx and rx packets at the low-level interrupt.
 *
 * Should be called from the Stellaris Ethernet Interrupt Handler.  This
 * function will read packets from the Stellaris Ethernet fifo and place them
 * into a pbuf queue.  If the transmitter is idle and there is at least one packet
 * on the transmit queue, it will place it in the transmit fifo and start the
 * transmitter.
 *
 */
void
stellarisif_interrupt(struct netif *netif)
{
  struct stellarisif *stellarisif;
  struct pbuf *p;

  /* setup pointer to the if state data */
  stellarisif = netif->state;

  /**
   * Process the transmit and receive queues as long as there is receive
   * data available
   *
   */
  p = stellarisif_receive(netif);
  while(p != NULL) {
    /* process the packet */
#if NO_SYS
    if(ethernet_input(p, netif)!=ERR_OK) {
#else
    if(tcpip_input(p, netif)!=ERR_OK) {
#endif
      /* drop the packet */
      LWIP_DEBUGF(NETIF_DEBUG, ("stellarisif_input: input error\n"));
      pbuf_free(p);

      /* Adjust the link statistics */
      LINK_STATS_INC(link.memerr);
      LINK_STATS_INC(link.drop);
    }

    /* Check if TX fifo is empty and packet available */
    if((HWREG(ETH_BASE + MAC_O_TR) & MAC_TR_NEWTX) == 0) {
      p = dequeue_packet(&stellarisif->txq);
      if(p != NULL) {
        stellarisif_transmit(netif, p);
      }
    }

    /* Read another packet from the RX fifo */
    p = stellarisif_receive(netif);
  }

  /* One more check of the transmit queue/fifo */
  if((HWREG(ETH_BASE + MAC_O_TR) & MAC_TR_NEWTX) == 0) {
    p = dequeue_packet(&stellarisif->txq);
    if(p != NULL) {
      stellarisif_transmit(netif, p);
    }
  }
}

#if NETIF_DEBUG
/* Print an IP header by using LWIP_DEBUGF
 * @param p an IP packet, p->payload pointing to the IP header
 */
void
stellarisif_debug_print(struct pbuf *p)
{
  struct eth_hdr *ethhdr = (struct eth_hdr *)p->payload;
  u16_t *plen = (u16_t *)p->payload;

  LWIP_DEBUGF(NETIF_DEBUG, ("ETH header:\n"));
  LWIP_DEBUGF(NETIF_DEBUG, ("Packet Length:%5"U16_F" \n",*plen));
  LWIP_DEBUGF(NETIF_DEBUG, ("Destination: %02"X8_F"-%02"X8_F"-%02"X8_F"-%02"X8_F"-%02"X8_F"-%02"X8_F"\n",
    ethhdr->dest.addr[0],
    ethhdr->dest.addr[1],
    ethhdr->dest.addr[2],
    ethhdr->dest.addr[3],
    ethhdr->dest.addr[4],
    ethhdr->dest.addr[5]));
  LWIP_DEBUGF(NETIF_DEBUG, ("Source: %02"X8_F"-%02"X8_F"-%02"X8_F"-%02"X8_F"-%02"X8_F"-%02"X8_F"\n",
    ethhdr->src.addr[0],
    ethhdr->src.addr[1],
    ethhdr->src.addr[2],
    ethhdr->src.addr[3],
    ethhdr->src.addr[4],
    ethhdr->src.addr[5]));
  LWIP_DEBUGF(NETIF_DEBUG, ("Packet Type:0x%04"U16_F" \n", ethhdr->type));
}
#endif /* NETIF_DEBUG */
