
#ifndef ASM_H
#define ASM_H
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define MAX_LABEL_LEN 	50
#define MAX_LINE_LEN 	500
#define MEMORY_SIZE 	4096 //Max Lines
#define NUM_OPCODES 	20
#define NUM_REGS 		16
#define WORD_LEN		10 //8 digits + '\n' + '\0'


typedef struct Label {
	char label[MAX_LABEL_LEN];
	int PC;
}Label;

typedef struct Command {
	char opcode[7]; // because the largest opcode string is 6 letters (+'\0')
	char rd[6];
	char rs[6];
	char rt[6];
	char imm[50];
}Command;

typedef struct WordCmd {
	int PC;
	int data;
	int valid;
}WordCmd;

int getCommands(FILE *asm_file, Command Mem[], WordCmd Words[], Label Labels[], int *maxWordAddr, int *numOfLabels);
void parseWordLine(char line[], int PC, WordCmd Words[], int *maxWordAddr);
void parseCmdLine(char line[], Command Mem[], int PC);
void parseLabelLine(char line[], Label Labels[], int PC, int *numOfLabels);
int isRealLine(char line[]);
void toLower(char str[]);
int isLabelLine(char line[]);
int isWordLine(char line[]);
void trim(char line[]);
void writeMem(char memData[][WORD_LEN], Command Mem[], int memSize, Label Labels[], int numOfLabels);
int strToInt(char str[]);
int getImm(char curImm[], Label Labels[], int numOfLabels);
void exeWordCmds(char memData[][WORD_LEN], WordCmd Words[], int memSize);
void writeFile(char memData[][WORD_LEN], const char filename[], int fileSize);
int toNumber(char number[]);
void add_digit(char str_op[], int x);
const char(opcode[NUM_OPCODES])[50] = { "add", "sub", "and", "or", "sll", "sra", "srl", "beq", "bne", "blt", "bgt", "ble", "bge", "jal", "lw" , "sw" , "reti" , "in" , "out" , "halt" };
const char(regs[NUM_REGS])[50] = { "$zero","$imm","$v0","$a0","$a1","$t0","$t1","$t2","$t3","$s0", "$s1","$s2","$gp","$sp","$fp","$ra" };

#endif //ASM_H
#pragma once
