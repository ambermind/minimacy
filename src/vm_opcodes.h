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

#define OPCODE_NB 125

#define OPadd 0
#define OPatomic 1
#define OPbreak 2
#define OPconst 3
#define OPconstb 4
#define OPdrop 5
#define OPdup 6
#define OPelse 7
#define OPeq 8
#define OPfalse 9
#define OPfetch 10
#define OPfetchb 11
#define OPfinal 12
#define OPfirst 13
#define OPfloat 14
#define OPge 15
#define OPgoto 16
#define OPgt 17
#define OPhd 18
#define OPint 19
#define OPintb 20
#define OPle 21
#define OPlef 22
#define OPlt 23
#define OPmark 24
#define OPmklist 25
#define OPmod 26
#define OPmul 27
#define OPne 28
#define OPneg 29
#define OPnil 30
#define OPnon 31
#define OPnop 32
#define OPnot 33
#define OPor 34
#define OPret 35
#define OPrglob 36
#define OPrglobb 37
#define OPrloc 38
#define OPrlocb 39
#define OPsglobi 40
#define OPshl 41
#define OPshr 42
#define OPskip 43
#define OPskipb 44
#define OPsloc 45
#define OPslocb 46
#define OPsloci 47
#define OPstore 48
#define OPstruct 49
#define OPsub 50
#define OPsum 51
#define OPswap 52
#define OPtfc 53
#define OPtfcb 54
#define OPthrow 55
#define OPtl 56
#define OPtrue 57
#define OPtry 58
#define OPunmark 59
#define OPupdt 60
#define OPupdtb 61
#define OPabs 62
#define OPabsf 63
#define OPacos 64
#define OPaddf 65
#define OPand 66
#define OPasin 67
#define OPatan 68
#define OPatan2 69
#define OPcast 70
#define OPcastb 71
#define OPceil 72
#define OPcos 73
#define OPcosh 74
#define OPdftab 75
#define OPdftabb 76
#define OPdiv 77
#define OPdivf 78
#define OPdump 79
#define OPeor 80
#define OPexec 81
#define OPexecb 82
#define OPexp 83
#define OPfloor 84
#define OPgef 85
#define OPgtf 86
#define OPholdon 87
#define OPln 88
#define OPlog 89
#define OPltf 90
#define OPmax 91
#define OPmaxf 92
#define OPmin 93
#define OPminf 94
#define OPmktab 95
#define OPmktabb 96
#define OPmodf 97
#define OPmulf 98
#define OPnegf 99
#define OPpowint 100
#define OPpow 101
#define OPprompt 102
#define OPround 103
#define OPsin 104
#define OPsinh 105
#define OPsqr 106
#define OPsqrt 107
#define OPsubf 108
#define OPtan 109
#define OPtanh 110
#define OPtroff 111
#define OPtron 112
#define OPlambda 113
#define OPlambdab 114
#define OPpick 115
#define OPpickb 116
#define OPdumpd 117
#define OPformat 118
#define OPformatb 119
#define OPdfarray 120
#define OPdfarrayb 121
#define OPtablen 122
#define OPisnan 123
#define OPisinf 124

void bytecodeInit();

void opcodePrint(Thread* th, int msk,LINT i,char* p,LINT ind0);
void bytecodePrint(Thread* th, int msk,LB* bytecode);

void bytecodeOptimize(LB* bytecode);
#endif