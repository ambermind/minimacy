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

#define OPCODE_NB 124

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
#define OPhide 50
#define OPholdon 51
#define OPint 52
#define OPintb 53
#define OPisinf 54
#define OPisnan 55
#define OPlambda 56
#define OPlambdab 57
#define OPle 58
#define OPlef 59
#define OPln 60
#define OPlog 61
#define OPlt 62
#define OPltf 63
#define OPmark 64
#define OPmax 65
#define OPmaxf 66
#define OPmin 67
#define OPminf 68
#define OPmklist 69
#define OPmod 70
#define OPmodf 71
#define OPmul 72
#define OPmulf 73
#define OPne 74
#define OPneg 75
#define OPnegf 76
#define OPnil 77
#define OPnon 78
#define OPnop 79
#define OPnot 80
#define OPor 81
#define OPpick 82
#define OPpickb 83
#define OPpow 84
#define OPpowint 85
#define OPprompt 86
#define OPret 87
#define OPrglob 88
#define OPrglobb 89
#define OPrloc 90
#define OPrlocb 91
#define OPround 92
#define OPsglobi 93
#define OPshl 94
#define OPshr 95
#define OPsin 96
#define OPsinh 97
#define OPskip 98
#define OPskipb 99
#define OPsloc 100
#define OPslocb 101
#define OPsloci 102
#define OPsqr 103
#define OPsqrt 104
#define OPstore 105
#define OPstruct 106
#define OPsub 107
#define OPsubf 108
#define OPsum 109
#define OPswap 110
#define OPtan 111
#define OPtanh 112
#define OPtfc 113
#define OPtfcb 114
#define OPthrow 115
#define OPtl 116
#define OPtroff 117
#define OPtron 118
#define OPtrue 119
#define OPtry 120
#define OPunmark 121
#define OPupdt 122
#define OPupdtb 123

void opcodePrint(int msk,LINT i,char* p,LINT ind0);
void bytecodePrint(int msk,LB* bytecode);

void bytecodeOptimize(LB* bytecode);
#endif