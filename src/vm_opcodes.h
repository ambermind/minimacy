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
#define OPcompact 15
#define OPconst 16
#define OPconstb 17
#define OPcontinue 18
#define OPcos 19
#define OPcosh 20
#define OPdfarray 21
#define OPdfarrayb 22
#define OPdftup 23
#define OPdftupb 24
#define OPdiv 25
#define OPdivf 26
#define OPdrop 27
#define OPdump 28
#define OPdumpd 29
#define OPdup 30
#define OPelse 31
#define OPeor 32
#define OPeq 33
#define OPexec 34
#define OPexecb 35
#define OPexp 36
#define OPfalse 37
#define OPfetch 38
#define OPfetchb 39
#define OPfinal 40
#define OPfirst 41
#define OPfloat 42
#define OPfloor 43
#define OPformat 44
#define OPformatb 45
#define OPge 46
#define OPgef 47
#define OPgoto 48
#define OPgt 49
#define OPgtf 50
#define OPhd 51
#define OPhide 52
#define OPholdon 53
#define OPint 54
#define OPintb 55
#define OPisinf 56
#define OPisnan 57
#define OPlambda 58
#define OPlambdab 59
#define OPle 60
#define OPlef 61
#define OPln 62
#define OPlog 63
#define OPloop 64
#define OPlt 65
#define OPltf 66
#define OPmax 67
#define OPmaxf 68
#define OPmin 69
#define OPminf 70
#define OPmklist 71
#define OPmod 72
#define OPmodf 73
#define OPmul 74
#define OPmulf 75
#define OPne 76
#define OPneg 77
#define OPnegf 78
#define OPnil 79
#define OPnon 80
#define OPnop 81
#define OPnot 82
#define OPor 83
#define OPpick 84
#define OPpickb 85
#define OPpow 86
#define OPpowint 87
#define OPprompt 88
#define OPret 89
#define OPrglob 90
#define OPrglobb 91
#define OPrloc 92
#define OPrlocb 93
#define OPround 94
#define OPsglobi 95
#define OPshl 96
#define OPshr 97
#define OPsin 98
#define OPsinh 99
#define OPskip 100
#define OPskipb 101
#define OPsloc 102
#define OPslocb 103
#define OPsloci 104
#define OPsqr 105
#define OPsqrt 106
#define OPstore 107
#define OPstruct 108
#define OPsub 109
#define OPsubf 110
#define OPsum 111
#define OPswap 112
#define OPtan 113
#define OPtanh 114
#define OPtfc 115
#define OPtfcb 116
#define OPthrow 117
#define OPtl 118
#define OPtroff 119
#define OPtron 120
#define OPtrue 121
#define OPtry 122
#define OPupdt 123
#define OPupdtb 124

void opcodePrint(int msk,LINT i,char* p,LINT ind0);
void bytecodePrint(int msk,LB* bytecode);

void bytecodeOptimize(LB* bytecode);
#endif