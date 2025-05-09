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

#define OPabort 0
#define OPabs 1
#define OPabsf 2
#define OPacos 3
#define OPadd 4
#define OPaddf 5
#define OPand 6
#define OParraylen 7
#define OPasin 8
#define OPatan 9
#define OPatan2 10
#define OPatomic 11
#define OPbreak 12
#define OPcast 13
#define OPcastb 14
#define OPceil 15
#define OPcompact 16
#define OPconst 17
#define OPconstb 18
#define OPcontinue 19
#define OPcos 20
#define OPcosh 21
#define OPdfarray 22
#define OPdfarrayb 23
#define OPdftup 24
#define OPdftupb 25
#define OPdiv 26
#define OPdivf 27
#define OPdrop 28
#define OPdump 29
#define OPdumpd 30
#define OPdup 31
#define OPelse 32
#define OPeor 33
#define OPeq 34
#define OPexec 35
#define OPexecb 36
#define OPexp 37
#define OPfalse 38
#define OPfetch 39
#define OPfetchb 40
#define OPfinal 41
#define OPfirst 42
#define OPfloat 43
#define OPfloor 44
#define OPformat 45
#define OPformatb 46
#define OPge 47
#define OPgef 48
#define OPgoto 49
#define OPgt 50
#define OPgtf 51
#define OPhd 52
#define OPhide 53
#define OPholdon 54
#define OPint 55
#define OPintb 56
#define OPisinf 57
#define OPisnan 58
#define OPlambda 59
#define OPlambdab 60
#define OPle 61
#define OPlef 62
#define OPln 63
#define OPlog 64
#define OPloop 65
#define OPlt 66
#define OPltf 67
#define OPmax 68
#define OPmaxf 69
#define OPmin 70
#define OPminf 71
#define OPmklist 72
#define OPmod 73
#define OPmodf 74
#define OPmul 75
#define OPmulf 76
#define OPne 77
#define OPneg 78
#define OPnegf 79
#define OPnil 80
#define OPnon 81
#define OPnop 82
#define OPnot 83
#define OPor 84
#define OPpick 85
#define OPpickb 86
#define OPpow 87
#define OPpowint 88
#define OPprompt 89
#define OPret 90
#define OPrglob 91
#define OPrglobb 92
#define OPrloc 93
#define OPrlocb 94
#define OPround 95
#define OPsglobi 96
#define OPshl 97
#define OPshr 98
#define OPsin 99
#define OPsinh 100
#define OPskip 101
#define OPskipb 102
#define OPsloc 103
#define OPslocb 104
#define OPsloci 105
#define OPsqr 106
#define OPsqrt 107
#define OPstore 108
#define OPstruct 109
#define OPsub 110
#define OPsubf 111
#define OPsum 112
#define OPswap 113
#define OPtan 114
#define OPtanh 115
#define OPtfc 116
#define OPtfcb 117
#define OPtl 118
#define OPtroff 119
#define OPtron 120
#define OPtrue 121
#define OPtry 122
#define OPupdt 123
#define OPupdtb 124

void opcodePrint(int msk,LINT i,char* p,LINT ind0);
void bytecodePrint(int msk,LB* bytecode);
void bytecodeShowNatives(Buffer* buffer);

void bytecodeOptimize(LB* bytecode);
#endif