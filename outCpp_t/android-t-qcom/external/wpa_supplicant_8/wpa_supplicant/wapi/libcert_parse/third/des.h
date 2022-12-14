/*
 This archive contains the following files, besides this one -

 d3des.c - THINK C* source for DES
 d3des.h - Header file for d3des.c
 enkeys3.c - Simple UNIX-like DES calculator

 Please make sure that you read and understand the "OBLIGATORY IMPORTANT
 DISCLAIMERS, WARNINGS AND RESTRICTIONS" section at the end of this file.  

 D3DES.C, D3DES.H, D3DES68.C & ENKEYS3.C
 =======================================

 These files are useful for playing around with DES cryptography,
 especially if you neither need or love assembler.

 "d3des" is an implementation of DES written in portable 'C'.  While
 originally written on the Macintosh, it has been run correctly, without
 change, on the PC and on UNIX platforms.  It only assumes that "long"
 integers are at least 32 bits wide.  

 The reason it's called "d3"des is that it actually performs five (5)
 different forms of the DES: (1) Single key, single block; (2) Double
 key, single block; (3) Double key, double block; (4) Triple key, single
 block; and (5) Triple key, double block.  Some auxilliary support code
 is included, for hashing keys out of passwords for instance.

 "enkeys3" is a UNIX-like desktop DES ECB-mode calculator.  With some
 changes to enkeys3.c (the header files to be #include'd and the end-of-line
 filtering) the package compiles and runs (correctly) using Symantec THINK C
 on the Macintosh and Microsoft Quick C on the IBM PC.  It should also run
 (more or less correctly) on any version of UNIX.


 OBLIGATORY IMPORTANT DISCLAIMERS, WARNINGS AND RESTRICTIONS
 ===========================================================

 [1] This software is provided "as is" without warranty of fitness for use or
 suitability for any purpose, express or implied.  Use at your own risk or not
 at all.  It does, however, "do" DES.

 [2] This software (comprised of the files listed above) is "freeware".  It may
 be freely used and distributed (but see [3], below).  The copyright in this
 software has not been abandoned, and is hereby asserted.

 [3] ENCRYPTION SOFTWARE MAY ONLY BE EXPORTED FROM NORTH AMERICA UNDER THE
 AUTHORITY OF A VALID EXPORT LICENSE OR PERMIT.  CONSULT THE APPROPRIATE
 BRANCH(ES) OF YOUR FEDERAL GOVERNMENT FOR MORE INFORMATION.

 Try asking them >why<, for starters.

 ---------------------------------------------------------------
 Richard Outerbridge                GEnie: OUTER
 50 Walmer Road, #111        CompuServe: [71755,204]
 Toronto, Ontario                (416) 961-4757
 Canada   M5R 2X4                9210.06 / October 6th, 1992
 ---------------------------------------------------------------

 This release of the code is dedicated to the memory of Michael Allen
 Ridler (1954-1989), Visual Artist, exemplary friend, victim of AIDS.
 */

/* d3des.h -
 *
 * Headers and defines for d3des.c
 * Graven Imagery, 1992.
 *
 * THIS SOFTWARE PLACED IN THE PUBLIC DOMAIN BY THE AUTHOUR
 * 920825 19:42 EDST
 *
 * Copyright (c) 1988,1989,1990,1991,1992 by Richard Outerbridge
 *        (GEnie : OUTER; CIS : [71755,204])
 */

#ifndef _DES3__H
#define _DES3__H

#ifdef  __cplusplus
extern "C"
{
#endif

#define EN0        0                /* MODE == encrypt */
#define DE1        1                /* MODE == decrypt */

    void
    Ddes ( unsigned char *from , unsigned char *into ,
            unsigned long KnL [ 32 ] , unsigned long KnR [ 32 ] ,
            unsigned long Kn3 [ 32 ] );

    void
    des3key ( unsigned char *hexkey , short mode , unsigned long KnL [ 32 ] ,
            unsigned long KnR [ 32 ] , unsigned long Kn3 [ 32 ] );

    void
    des_3key_encrypt_cbc ( unsigned char *plain , int len , unsigned char *iv ,
            unsigned char *cipher , unsigned long KnL [ 32 ] ,
            unsigned long KnR [ 32 ] , unsigned long Kn3 [ 32 ] );

    void
    des_3key_decrypt_cbc ( unsigned char *cipher , int len , unsigned char *iv ,
            unsigned char *plain , unsigned long KnL [ 32 ] ,
            unsigned long KnR [ 32 ] , unsigned long Kn3 [ 32 ] );

    int
    p12_3des_cbc_encrypt ( unsigned char *indata , int in_len ,
            unsigned char *iv , int iv_len , unsigned char *key , int key_len ,
            unsigned char *outdata , int *out_len );

    int
    p12_3des_cbc_decrypt ( unsigned char *indata , int in_len ,
            unsigned char *iv , int iv_len , unsigned char *key , int key_len ,
            unsigned char *outdata , int *out_len );

#ifdef  __cplusplus
}
#endif

#endif /*_DES3__H*/

/*********************************************************************/
