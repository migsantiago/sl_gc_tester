.rodata
.balign 32
.globl piclength
.globl picdata

piclength:	.long	picdataend - picdata
picdata:
.incbin "../source/test.jpg"
picdataend:


