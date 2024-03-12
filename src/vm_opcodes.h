/* Copyright (c) 2022, Sylvain Huet, Ambermind
   This program is free software: you can redistribute it and/or modify it
   under the terms of the GNU General Public License, version 2.0, as
   published by the Free Software Foundation.
   This program is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License,
   version 2.0, for more details.
   You should have received a copy of the GNU General Public License along
   with this program. If not, see <https://www.gnu.org/licenses/>. */
#ifndef _OPCODES_
#define _OPCODES_

#define OPCODE_NB 123

#define OPabs 0
#define OPabsf 1
#define OPacos 2
#define OPadd 3
#define OPaddf 4
#define OPand 5
#define OParraylen 6
#define OPasin 7
#define OPatan 8
#define OPatan2 9
#define OPatomic 10
#define OPbreak 11
#define OPcast 12
#define OPcastb 13
#define OPceil 14
#define OPconst 15
#define OPconstb 16
#define OPcos 17
#define OPcosh 18
#define OPdfarray 19
#define OPdfarrayb 20
#define OPdftup 21
#define OPdftupb 22
#define OPdiv 23
#define OPdivf 24
#define OPdrop 25
#define OPdump 26
#define OPdumpd 27
#define OPdup 28
#define OPelse 29
#define OPeor 30
#define OPeq 31
#define OPexec 32
#define OPexecb 33
#define OPexp 34
#define OPfalse 35
#define OPfetch 36
#define OPfetchb 37
#define OPfinal 38
#define OPfirst 39
#define OPfloat 40
#define OPfloor 41
#define OPformat 42
#define OPformatb 43
#define OPge 44
#define OPgef 45
#define OPgoto 46
#define OPgt 47
#define OPgtf 48
#define OPhd 49
#define OPholdon 50
#define OPint 51
#define OPintb 52
#define OPisinf 53
#define OPisnan 54
#define OPlambda 55
#define OPlambdab 56
#define OPle 57
#define OPlef 58
#define OPln 59
#define OPlog 60
#define OPlt 61
#define OPltf 62
#define OPmark 63
#define OPmax 64
#define OPmaxf 65
#define OPmin 66
#define OPminf 67
#define OPmklist 68
#define OPmod 69
#define OPmodf 70
#define OPmul 71
#define OPmulf 72
#define OPne 73
#define OPneg 74
#define OPnegf 75
#define OPnil 76
#define OPnon 77
#define OPnop 78
#define OPnot 79
#define OPor 80
#define OPpick 81
#define OPpickb 82
#define OPpow 83
#define OPpowint 84
#define OPprompt 85
#define OPret 86
#define OPrglob 87
#define OPrglobb 88
#define OPrloc 89
#define OPrlocb 90
#define OPround 91
#define OPsglobi 92
#define OPshl 93
#define OPshr 94
#define OPsin 95
#define OPsinh 96
#define OPskip 97
#define OPskipb 98
#define OPsloc 99
#define OPslocb 100
#define OPsloci 101
#define OPsqr 102
#define OPsqrt 103
#define OPstore 104
#define OPstruct 105
#define OPsub 106
#define OPsubf 107
#define OPsum 108
#define OPswap 109
#define OPtan 110
#define OPtanh 111
#define OPtfc 112
#define OPtfcb 113
#define OPthrow 114
#define OPtl 115
#define OPtroff 116
#define OPtron 117
#define OPtrue 118
#define OPtry 119
#define OPunmark 120
#define OPupdt 121
#define OPupdtb 122

void bytecodeInit();

void opcodePrint(Thread* th, int msk,LINT i,char* p,LINT ind0);
void bytecodePrint(Thread* th, int msk,LB* bytecode);

void bytecodeOptimize(LB* bytecode);
#endif