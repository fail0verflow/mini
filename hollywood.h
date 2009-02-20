#ifndef __HOLLYWOOD_H__
#define __HOLLYWOOD_H__

/* Hollywood Registers */

#define		HW_PPC_REG_BASE		0xd000000
#define		HW_REG_BASE			0xd800000

// The PPC can only see the first three IPC registers
#define		HW_IPC_PPCMSG		(HW_REG_BASE + 0x000) //PPC to ARM
#define		HW_IPC_PPCCTRL		(HW_REG_BASE + 0x004)
#define		HW_IPC_ARMMSG		(HW_REG_BASE + 0x008) //ARM to PPC
#define		HW_IPC_ARMCTRL		(HW_REG_BASE + 0x00c)

// Write one to send a message. Cleared when peer writes one to IPC_CTRL_RECV.
#define		IPC_CTRL_SEND		0x01
// Set by peer to acknowledge a message. Write one to clear.
#define		IPC_CTRL_SENT		0x02
// Set by peer to send a message. Write one to clear.
#define		IPC_CTRL_RECV		0x04
// Write one acknowledge a message. Cleared when peer writes one to IPC_CTRL_SENT.
#define		IPC_CTRL_RECVD		0x08
// Enable interrupt when a message is received
#define		IPC_CTRL_INT_RECV	0x10
// Enable interrupt when a sent message is acknowledged
#define		IPC_CTRL_INT_SENT	0x20

/*
	The IPC registers are connected to each other.
	Both registers are identical and this works for
	both peers. Flag bits are cleared by writing a one
	to them.
	
	When Peer A sets this		Peer B sees this set
	IPC_CTRL_SEND				IPC_CTRL_RECV
	IPC_CTRL_RECVD				IPC_CTRL_SENT
	
	In fact, bit _SEND on Peer A and bit _RECV on peer B are the same bit,
	and the same goes for _RECVD and _SENT, except writing one from A _sets_
	the bit, and writing one from B _clears_ the bit. The same, of course,
	is true for the other pair of bits in the other direction.

	The flow, therefore, goes as follows, for a message
	from A to B. Steps with the same number can be taken
	in any order.
	
	1. Peer A writes the message address to the register
	2. Peer A sets IPC_CTRL_SEND
	3. Peer B sees IPC_CTRL_RECV
	4. Peer B writes one to IPC_CTRL_RECV to clear it (A's IPC_CTRL_SEND is cleared at this point)
	4. Peer B reads its message address register
	5. Peer B sets IPC_CTRL_RECVD
	6. Peer A sees IPC_CTRL_SENT
	7. Peer A writes one to IPC_CTRL_SENT to clear it (B's IPC_CTRL_RECVD is cleared at this point)
	7. Peer A may now write to the message address register again
	
	The same is true for a message from Peer B to Peer A.
	
	In the particular case of IOS IPC, the PPC is always the "master"
	(it sends requests as messages) and the ARM is always the "slave"
	(it replies to PPC's requests). IOS can handle up to 16(15?)
	simultaneously pending transactions (the PPC can send up to 16(15?)
	messages without getting any replies - for example, due to
	asynchronous requests or multithreaded execution of blocking
	requests)
	
*/

#define		HW_TIMER			(HW_REG_BASE + 0x010)
#define		HW_ALARM			(HW_REG_BASE + 0x014)

// maybe?
#define		HW_FIQFLAG			(HW_REG_BASE + 0x030)
#define		HW_FIQENABLE		(HW_REG_BASE + 0x034)

#define		HW_IRQFLAG			(HW_REG_BASE + 0x038)
#define		HW_IRQENABLE		(HW_REG_BASE + 0x03c)

#define		HW_MEMMIRR			(HW_REG_BASE + 0x060)

// something to do with PPCBOOT
// and legacy DI it seems ?!?
#define		HW_EXICTRL			(HW_REG_BASE + 0x070)
#define		EXICTRL_ENABLE_EXI		1

// PPC side of GPIO1 (Starlet can access this too)
// Output state
#define		HW_GPIO1BOUT		(HW_REG_BASE + 0x0c0)
// Direction (1=output)
#define		HW_GPIO1BDIR		(HW_REG_BASE + 0x0c4)
// Input state
#define		HW_GPIO1BIN			(HW_REG_BASE + 0x0c8)
// Interrupt level
#define		HW_GPIO1BINTLVL		(HW_REG_BASE + 0x0cc)
// Interrupt flags (write 1 to clear)
#define		HW_GPIO1BINTFLAG	(HW_REG_BASE + 0x0d0)
// Interrupt propagation enable
// Do these interrupts go anywhere???
#define		HW_GPIO1BINTENABLE	(HW_REG_BASE + 0x0d4)
//??? seems to be a mirror of inputs at some point... power-up state?
#define		HW_GPIO1BINMIR		(HW_REG_BASE + 0x0d8)
// 0xFFFFFF by default, if cleared disables respective outputs. Top bits non-settable.
#define		HW_GPIO1ENABLE		(HW_REG_BASE + 0x0dc)

#define		HW_GPIO1_SLOT		0x000020
#define		HW_GPIO1_DEBUG		0xFF0000
#define		HW_GPIO1_DEBUG_SH	16

// Starlet side of GPIO1
// Output state
#define		HW_GPIO1OUT			(HW_REG_BASE + 0x0e0)
// Direction (1=output)
#define		HW_GPIO1DIR			(HW_REG_BASE + 0x0e4)
// Input state
#define		HW_GPIO1IN			(HW_REG_BASE + 0x0e8)
// Interrupt level
#define		HW_GPIO1INTLVL		(HW_REG_BASE + 0x0ec)
// Interrupt flags (write 1 to clear)
#define		HW_GPIO1INTFLAG		(HW_REG_BASE + 0x0f0)
// Interrupt propagation enable (interrupts go to main interrupt 0x800)
#define		HW_GPIO1INTENABLE	(HW_REG_BASE + 0x0f4)
//??? seems to be a mirror of inputs at some point... power-up state?
#define		HW_GPIO1INMIR		(HW_REG_BASE + 0x0f8)
// Owner of each GPIO bit. If 1, GPIO1B registers assume control. If 0, GPIO1 registers assume control.
#define		HW_GPIO1OWNER		(HW_REG_BASE + 0x0fc)

// ????
#define		HW_DIFLAGS			(HW_REG_BASE + 0x180)
#define		DIFLAGS_BOOT_CODE		0x100000

// maybe a GPIO???
#define		HW_RESETS			(HW_REG_BASE + 0x194)

#define		HW_CLOCKS			(HW_REG_BASE + 0x1b4)

#define		HW_GPIO2OUT			(HW_REG_BASE + 0x1c8)
#define		HW_GPIO2DIR			(HW_REG_BASE + 0x1cc)
#define		HW_GPIO2IN			(HW_REG_BASE + 0x1d0)

#define		HW_OTPCMD			(HW_REG_BASE + 0x1ec)
#define		HW_OTPDATA			(HW_REG_BASE + 0x1f0)
#define		HW_VERSION			(HW_REG_BASE + 0x214)


/* NAND Registers */

#define		NAND_REG_BASE				0xd010000

#define		NAND_CMD				(NAND_REG_BASE + 0x000)
#define		NAND_STATUS				NAND_CMD
#define		NAND_CONF				(NAND_REG_BASE + 0x004)
#define		NAND_ADDR0				(NAND_REG_BASE + 0x008)
#define		NAND_ADDR1				(NAND_REG_BASE + 0x00c)
#define		NAND_DATA				(NAND_REG_BASE + 0x010)
#define		NAND_ECC				(NAND_REG_BASE + 0x014)
#define		NAND_UNK1				(NAND_REG_BASE + 0x018)
#define		NAND_UNK2				(NAND_REG_BASE + 0x01c)



/* AES Registers */

#define		AES_REG_BASE			0xd020000

#define		AES_CMD					(AES_REG_BASE + 0x000)
#define		AES_SRC					(AES_REG_BASE + 0x004)
#define		AES_DEST				(AES_REG_BASE + 0x008)
#define		AES_KEY					(AES_REG_BASE + 0x00c)
#define		AES_IV					(AES_REG_BASE + 0x010)



/* SHA-1 Registers */

#define		SHA_REG_BASE			0xd030000

#define		SHA_CMD					(SHA_REG_BASE + 0x000)
#define		SHA_SRC					(SHA_REG_BASE + 0x004)
#define		SHA_H0					(SHA_REG_BASE + 0x008)
#define		SHA_H1					(SHA_REG_BASE + 0x00c)
#define		SHA_H2					(SHA_REG_BASE + 0x010)
#define		SHA_H3					(SHA_REG_BASE + 0x014)
#define		SHA_H4					(SHA_REG_BASE + 0x018)



/* SD Host Controller Registers */

#define		SD_REG_BASE				0xd070000

#define		SDHC_SDMA_ADDR				(0x000)
#define		SDHC_BLOCK_SIZE				(0x004)
#define		SDHC_BLOCK_COUNT			(0x006)
#define		SDHC_CMD_ARG				(0x008)
#define		SDHC_CMD_TRANSFER_MODE			(0x00c)
#define		SDHC_CMD				(0x00e)
#define		SDHC_RESPONSE				(0x010)
#define		SDHC_DATA				(0x020)
#define		SDHC_PRESENT_STATE			(0x024)
#define		SDHC_HOST_CONTROL			(0x028)
#define		SDHC_POWER_CONTROL			(0x029)
#define		SDHC_BLOCK_GAP_CONTROL			(0x02a)
#define		SDHC_WAKEUP_CONTROL			(0x02b)
#define		SDHC_CLOCK_CONTROL			(0x02c)
#define		SDHC_TIMEOUT_CONTROL			(0x02e)
#define		SDHC_SOFTWARE_RESET			(0x02f)
#define		SDHC_NORMAL_INTERRUPT_STATUS		(0x030)
#define		SDHC_ERROR_INTERRUPT_STATUS		(0x032)
#define		SDHC_NORMAL_INTERRUPT_ENABLE		(0x034)
#define		SDHC_ERROR_INTERRUPT_ENABLE		(0x036)
#define		SDHC_NORMAL_INTERRUPT_SIGNAL_ENABLE	(0x038)
#define		SDHC_ERROR_INTERRUPT_SIGNAL_ENABLE	(0x03a)
#define		SDHC_AMCD12_ERROR_STATUS		(0x03c)
#define		SDHC_CAPABILITIES			(0x040)
#define		SDHC_MAX_CAPABILITIES			(0x048)
#define		SDHC_FORCE_ERROR_EVENT_ACMD12		(0x050)
#define		SDHC_FORCE_ERROR_EVENT_INTERRUPT	(0x052)
#define		SDHC_ADMA_ERROR_STATUS			(0x054)
#define		SDHC_ADMA_SYSTEM_ADDR			(0x058)
#define		SDHC_SLOT_INTERRUPT_STATUS		(0x0fc)
#define		SDHC_VERSION				(0x0fe)

/* EXI Registers */

#define		EXI_REG_BASE			0xd806800
#define		EXI0_REG_BASE			(EXI_REG_BASE+0x000)
#define		EXI1_REG_BASE			(EXI_REG_BASE+0x014)
#define		EXI2_REG_BASE			(EXI_REG_BASE+0x028)

#define		EXI0_CSR				(EXI0_REG_BASE+0x000)
#define		EXI0_MAR				(EXI0_REG_BASE+0x004)
#define		EXI0_LENGTH				(EXI0_REG_BASE+0x008)
#define		EXI0_CR					(EXI0_REG_BASE+0x00c)
#define		EXI0_DATA				(EXI0_REG_BASE+0x010)

#define		EXI1_CSR				(EXI1_REG_BASE+0x000)
#define		EXI1_MAR				(EXI1_REG_BASE+0x004)
#define		EXI1_LENGTH				(EXI1_REG_BASE+0x008)
#define		EXI1_CR					(EXI1_REG_BASE+0x00c)
#define		EXI1_DATA				(EXI1_REG_BASE+0x010)

#define		EXI2_CSR				(EXI2_REG_BASE+0x000)
#define		EXI2_MAR				(EXI2_REG_BASE+0x004)
#define		EXI2_LENGTH				(EXI2_REG_BASE+0x008)
#define		EXI2_CR					(EXI2_REG_BASE+0x00c)
#define		EXI2_DATA				(EXI2_REG_BASE+0x010)

#define		EXI_BOOT_BASE			(EXI_REG_BASE+0x040)



/* MEMORY CONTROLLER Registers */

#define		MEM_REG_BASE			0xd8b4000
#define		MEM_PROT				(MEM_REG_BASE+0x20a)
#define		MEM_PROT_START			(MEM_REG_BASE+0x20c)
#define		MEM_PROT_END			(MEM_REG_BASE+0x20e)
#define		MEM_FLUSHREQ			(MEM_REG_BASE+0x228)
#define		MEM_FLUSHACK			(MEM_REG_BASE+0x22a)

#endif
