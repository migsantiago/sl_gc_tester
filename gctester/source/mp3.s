   .rodata
   .globl mp3data
   .globl mp3length
   .balign 32
   mp3length:	.long  mp3end - mp3data
   mp3data:
   .incbin "../source/test.mp3"
   mp3end: