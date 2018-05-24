

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned int read32 ( unsigned int );

unsigned int read_register ( unsigned int );

#define DBUGFETCH   1
#define DBUGRAM     1
#define DBUGRAMW    1
#define DBUGREG     1
#define DBUG        1
#define DISS        1

#define ROMADDMASK 0xFFFFF
#define RAMADDMASK 0xFFFFF

#define ROMSIZE (ROMADDMASK+1)
#define RAMSIZE (RAMADDMASK+1)

unsigned short rom[ROMSIZE>>1];
unsigned short ram[RAMSIZE>>1];

#define MEMSPACE 0xFF000000
#define ROMSPACE 0x00000000
#define RAMSPACE 0x01000000
#define PERSPACE 0xF0000000
#define DEBSPACE 0xF0000000

#define CPSR_N (1<<31)
#define CPSR_Z (1<<30)
#define CPSR_C (1<<29)
#define CPSR_V (1<<28)
#define CPSR_Q (1<<27)

unsigned long instructions;
unsigned long fetches;
unsigned long reads;
unsigned long writes;

const unsigned char regnames[16][4]=
{
    "fp",
    "sp",
    "r0",
    "r1",
    "r2",
    "r3",
    "r4",
    "r5",
    "r6",
    "r7",
    "r8",
    "r9",
    "r10",
    "r11",
    "r12",
    "r13",
};

unsigned int pc_now;
unsigned int pc_next;
unsigned int reg_norm[16]; //normal execution mode

void dump_counters ( void )
{
    printf("\n\n");
    printf("instructions %lu\n",instructions);
    printf("fetches      %lu\n",fetches);
    printf("reads        %lu\n",reads);
    printf("writes       %lu\n",writes);
    printf("memcycles    %lu\n",fetches+reads+writes);
}

unsigned int fetch16 ( unsigned int addr )
{
    unsigned int data;

    fetches++;

if(DBUGFETCH) fprintf(stderr,"fetch16(0x%08X)=",addr);
if(DBUG) fprintf(stderr,"fetch16(0x%08X)=",addr);
    switch(addr&MEMSPACE)
    {
        case ROMSPACE: //ROM
            addr&=ROMADDMASK;
            addr>>=1;
            data=rom[addr];
if(DBUGFETCH) fprintf(stderr,"0x%04X\n",data);
if(DBUG) fprintf(stderr,"0x%04X\n",data);
            return(data);
        case RAMSPACE: //RAM
            addr&=RAMADDMASK;
            addr>>=1;
            data=ram[addr];
if(DBUGFETCH) fprintf(stderr,"0x%04X\n",data);
if(DBUG) fprintf(stderr,"0x%04X\n",data);
            return(data);
    }
    fprintf(stderr,"fetch16(0x%08X), abort pc = 0x%04X\n",addr,pc_now);
    exit(1);
}

unsigned int fetch32 ( unsigned int addr )
{
    unsigned int data;

if(DBUGFETCH) fprintf(stderr,"fetch32(0x%08X)=",addr);
if(DBUG) fprintf(stderr,"fetch32(0x%08X)=",addr);
    switch(addr&MEMSPACE)
    {
        case ROMSPACE: //ROM
            //addr&=ROMADDMASK;
            //addr>>=1;
            //data=rom[addr+1];
            //data<<=16;
            //data|=rom[addr+0];
            data=fetch16(addr+0);
            data|=fetch16(addr+2)<<16;
if(DBUGFETCH) fprintf(stderr,"0x%08X\n",data);
if(DBUG) fprintf(stderr,"0x%08X\n",data);
            return(data);
        case RAMSPACE: //RAM
            //addr&=RAMADDMASK;
            //addr>>=1;
            //data=ram[addr+1];
            //data<<=16;
            //data|=ram[addr+0];
            data=fetch16(addr+0);
            data|=fetch16(addr+2)<<16;
if(DBUGFETCH) fprintf(stderr,"0x%08X\n",data);
if(DBUG) fprintf(stderr,"0x%08X\n",data);
            return(data);
    }
    fprintf(stderr,"fetch32(0x%08X), abort pc 0x%04X\n",addr,pc_now);
    exit(1);
}

void write16 ( unsigned int addr, unsigned int data )
{

    writes++;


if(DBUG) fprintf(stderr,"write16(0x%08X,0x%04X)\n",addr,data);
    switch(addr&0xMEMSPACE)
    {
        case RAMSPACE: //RAM
if(DBUGRAM) fprintf(stderr,"write16(0x%08X,0x%04X)\n",addr,data);
            addr&=RAMADDMASK;
            addr>>=1;
            ram[addr]=data&0xFFFF;
            return;
    }
    fprintf(stderr,"write16(0x%08X,0x%04X), abort pc 0x%04X\n",addr,data,pc_now);
    exit(1);
}

void write32 ( unsigned int addr, unsigned int data )
{
if(DBUG) fprintf(stderr,"write32(0x%08X,0x%08X)\n",addr,data);
    switch(addr&MEMSPACE)
    {
        case DEBSPACE: //debug
            switch(addr&0xFF)
            {
                //case 0x00:
                //{
                    //fprintf(stderr,"[0x%08X][0x%08X] 0x%08X\n",read_register(14),addr,data);
                    //return;
                //}
                case 0x10:
                {
                    printf("0x%08X ",data);
                    return;
                }
                case 0x20:
                {
                    printf("0x%08X\n",data);
                    return;
                }
                break;
            }
        case PERSPACE:
            dump_counters();
            exit(0);
        case RAMSPACE: //RAM
if(DBUGRAMW) fprintf(stderr,"write32(0x%08X,0x%08X)\n",addr,data);
            write16(addr+0,(data>> 0)&0xFFFF);
            write16(addr+2,(data>>16)&0xFFFF);
            return;
    }
    fprintf(stderr,"write32(0x%08X,0x%08X), abort pc 0x%04X\n",addr,data,pc_now);
    exit(1);
}
//-----------------------------------------------------------------
unsigned int read16 ( unsigned int addr )
{
    unsigned int data;

    reads++;

if(DBUG) fprintf(stderr,"read16(0x%08X)=",addr);
    switch(addr&MEMSPACE)
    {
        case ROMSPACE: //ROM
            addr&=ROMADDMASK;
            addr>>=1;
            data=rom[addr];
if(DBUG) fprintf(stderr,"0x%04X\n",data);
            return(data);
        case RAMSPACE: //RAM
if(DBUGRAM) fprintf(stderr,"read16(0x%08X)=",addr);
            addr&=RAMADDMASK;
            addr>>=1;
            data=ram[addr];
if(DBUG) fprintf(stderr,"0x%04X\n",data);
if(DBUGRAM) fprintf(stderr,"0x%04X\n",data);
            return(data);
    }
    fprintf(stderr,"read16(0x%08X), abort pc 0x%04X\n",addr,pc_now);
    exit(1);
}

unsigned int read32 ( unsigned int addr )
{
    unsigned int data;

if(DBUG) fprintf(stderr,"read32(0x%08X)=",addr);
    switch(addr&MEMMASK)
    {
        case ROMMASK: //ROM
        case RAMMASK: //RAM
if(DBUGRAMW) fprintf(stderr,"read32(0x%08X)=",addr);
            data =read16(addr+0);
            data|=((unsigned int)read16(addr+2))<<16;
if(DBUG) fprintf(stderr,"0x%08X\n",data);
if(DBUGRAMW) fprintf(stderr,"0x%08X\n",data);
            return(data);
    }
    fprintf(stderr,"read32(0x%08X), abort pc 0x%04X\n",addr,pc_now);
    exit(1);
}

unsigned int read_register ( unsigned int reg )
{
    unsigned int data;

    reg&=0xF;
if(DBUG) fprintf(stderr,"read_register(%u(%s))=",reg,reg_names[reg]);
if(DBUGREG) fprintf(stderr,"read_register(%u(%s))=",reg,reg_names[reg]);
    data=reg_norm[reg];
if(DBUG) fprintf(stderr,"0x%08X\n",data);
if(DBUGREG) fprintf(stderr,"0x%08X\n",data);
    return(data);
}

void write_register ( unsigned int reg, unsigned int data )
{
    reg&=0xF;
if(DBUG) fprintf(stderr,"write_register(%u(%s),0x%08X)\n",reg,reg_names[reg],data);
if(DBUGREG) fprintf(stderr,"write_register(%u(%s),0x%08X)\n",reg,reg_names[reg],data);
    reg_norm[reg]=data;
}
