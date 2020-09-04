/*
 * sim.c
 *
 *  Created on: Jun 17, 2020
 *      Author: Dell
 */

#define _CRT_SECURE_NO_WARNINGS
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>

#define LINE_SIZE 50
#define MEMORY_SIZE (4096)
#define REGISTERS_COUNT (16)
#define IORegister_COUNT (18)
#define SECTOR_NUM (128)
#define SECTOR_SIZE (128)
#define FRAME_CYCLES (1024)
int registers[REGISTERS_COUNT] = { 0 }; //array of registers
int memory[MEMORY_SIZE] = { 0 }; //array of memory
int dis_irq2_clk_pc[4] = { 0,0,0,0 }; //array containing the values of disktimer, irq2Cycle,clk and pc
int diskMemory[SECTOR_NUM][SECTOR_SIZE] = { 0 };
char* IOREGNMAES[IORegister_COUNT] = { "irq0enable","irq1enable","irq2enable","irq0status", "irq1status"
		, "irq2status","irqhandler","irqreturn", "clks", "leds", "display", "timerenable"
		, "timercurrent", "timermax", "diskcmd", "disksector", "diskbuffer", "diskstatus" }; //names of ioregisters
int irqBussy = 0;
int ioregisters[IORegister_COUNT] = { 0 }; //array of ioregisters
void writeTrace(FILE *trace, int cmd[], int PC);
void getCmdsCodes(FILE *memIn, int memInCode[][5]);
void writeRegOut(FILE* regOut);
void exeCommand(int cmd[], int dis_irq2_clk_pc[], int regsArr[], FILE* irq2in, FILE* leds, FILE* display, FILE* hwregtrace);
void writeMemOut(FILE *memOut);
void writeTohwregTrace(FILE* hwregtraceFile, int cmd[], int clk);
void updateIrq0();
void getDiskContent(FILE* diskIn);
void writeToDiskout(FILE* diskOut);
void wrDisk(int dis_irq2_clk_pc[]);
void writeToDisplay(FILE* display, int clk);
void writeToLeds(FILE* leds, int clk);
void getNextIRQ2(int dis_irq2_clk_pc[], FILE* Irq2);
void updateTimer();
void add(int cmd[], int dis_irq2_clk_pc[], int regsArr[]);
void sub(int cmd[], int dis_irq2_clk_pc[], int regsArr[]);
void and(int cmd[], int dis_irq2_clk_pc[], int regsArr[]);
void or (int cmd[], int dis_irq2_clk_pc[], int regsArr[]);
void sll(int cmd[], int dis_irq2_clk_pc[], int regsArr[]);
void sra(int cmd[], int dis_irq2_clk_pc[], int regsArr[]);
void srl(int cmd[], int dis_irq2_clk_pc[], int regsArr[]);
void beq(int cmd[], int dis_irq2_clk_pc[], int regsArr[]);
void bne(int cmd[], int dis_irq2_clk_pc[], int regsArr[]);
void blt(int cmd[], int dis_irq2_clk_pc[], int regsArr[]);
void bgt(int cmd[], int dis_irq2_clk_pc[], int regsArr[]);
void ble(int cmd[], int dis_irq2_clk_pc[], int regsArr[]);
void bge(int cmd[], int dis_irq2_clk_pc[], int regsArr[]);

void jal(int cmd[], int dis_irq2_clk_pc[], int regsArr[]);
void lw(int cmd[], int dis_irq2_clk_pc[], int regsArr[]);
void sw(int cmd[], int dis_irq2_clk_pc[], int regsArr[]);
void reti(int cmd[], int dis_irq2_clk_pc[], int regsArr[]);
void in(int cmd[], int dis_irq2_clk_pc[], int regsArr[]);
void out(int cmd[], int dis_irq2_clk_pc[], int regsArr[]);
void perfomThePseka(int dis_irq2_clk_pc[]);//for the psekot
int signExtention(int num);//sign extension

int main(int argc, char *argv[]) {
	FILE *memIn, *diskIn, *irq2In, *memOut, *hwregtrace, *regOut, *trace, *cycles, *leds, *display, *diskout = NULL;
	int memInCode[MEMORY_SIZE][5];
	int count = 0;
	memIn = fopen(argv[1], "r");//open memIn file
	diskIn = fopen(argv[2], "r");//open memIn file
	irq2In = fopen(argv[3], "r");//open memIn file
	getCmdsCodes(memIn, memInCode);
	memOut = fopen(argv[4], "w");//open memOut file
	regOut = fopen(argv[5], "w");//open reg file
	trace = fopen(argv[6], "w");//open trace file
	hwregtrace = fopen(argv[7], "w");//open hwregtrace file
	cycles = fopen(argv[8], "w");//open cycles file
	leds = fopen(argv[9], "w");//open leds file
	display = fopen(argv[10], "w");//open display file
	diskout = fopen(argv[11], "w");//open diskout file
	while (memInCode[dis_irq2_clk_pc[3]][0] != 19) { //the main fetch-decode-execute loop, continues as long as the current opcode is not halt.
		registers[1] = memInCode[dis_irq2_clk_pc[3]][4]; //reg[1]=imm
		writeTrace(trace, memInCode[dis_irq2_clk_pc[3]], dis_irq2_clk_pc[3]); //write to the trace.
		exeCommand(memInCode[dis_irq2_clk_pc[3]], dis_irq2_clk_pc, registers, irq2In, leds, display, hwregtrace); //execute command .
		dis_irq2_clk_pc[3] &= 0xFFF; //wrap PC
		count++;
	}
	writeTrace(trace, memInCode[dis_irq2_clk_pc[3]], dis_irq2_clk_pc[3]); //write the last line to trace file.
	writeRegOut(regOut);//write regOut to file
	writeMemOut(memOut);//write memOut to file
	writeToDiskout(diskIn);//write diskIn to file
	fprintf(cycles, "%d\n", ++count); //write count to file, count the halt command.
	//close files:
	fclose(memIn);
	fclose(memOut);
	fclose(regOut);
	fclose(trace);
	fclose(cycles);
	fclose(diskout);
	fclose(display);
	fclose(leds);
	fclose(hwregtrace);
	fclose(diskIn);
	return 0;
}

//like the name , getting the Cmd code
void getCmdsCodes(FILE *memIn, int memInCode[][5]) {
	int memory_index = 0;
	int opcode = 0;
	if (memIn == NULL)
		exit(1);
	for (int i = 0; i < 4096; ++i) {
		for (int j = 0; j < 5; ++j) {
			memInCode[i][j] = 0; //filling with zeros
		}
	}
	int i = 0;
	while ((1 == fscanf(memIn, "%x", &opcode)) && (i < 4096)) {
		memory[memory_index++] = opcode;
		memInCode[i][0] = (opcode >> 24) & 0xFF;
		memInCode[i][1] = (opcode >> 20) & 0xF;
		memInCode[i][2] = (opcode >> 16) & 0xF;
		memInCode[i][3] = (opcode >> 12) & 0xF;
		memInCode[i][4] = signExtention(opcode & 0xFFF); //do signextention to imm


		i = i + 1;
	}
}


void writeTrace(FILE *trace, int cmd[], int PC) {
	fprintf(trace, "%08X ", PC & 0xffffffff); //write out the PC value
	fprintf(trace, "%02X%X%X%X%03X", cmd[0] & 0xff, cmd[1] & 0xf, cmd[2] & 0xf, cmd[3] & 0xf, cmd[4] & 0xfff); //write out the memory line in hex
	for (int i = 0; i < 16; i++) fprintf(trace, " %08X", registers[i] & 0xffffffff); //write out the registers values
	fprintf(trace, "\n");
}
void writeMemOut(FILE *memOut) {
	for (uint16_t i = 0; i < MEMORY_SIZE; i++) {
		fprintf(memOut, "%08X\n", memory[i]);
	}
}




void writeRegOut(FILE *regOut) {
	for (int i = 2; i < 16; i++) {
		fprintf(regOut, "%08X\n", (registers[i]) & 0xFFFFFFFF);
	}
}
void writeTohwregTrace(FILE* hwregtraceFile, int cmd[], int clk) {
	if (cmd[0] == 17) {
		fprintf(hwregtraceFile, "%d READ %s %08x\n", clk, IOREGNMAES[registers[cmd[2]] + registers[cmd[3]]], ioregisters[registers[cmd[2]] + registers[cmd[3]]]);
	}
	else if (cmd[0] == 18) {
		fprintf(hwregtraceFile, "%d WRITE %s %08x\n", clk, IOREGNMAES[registers[cmd[2]] + registers[cmd[3]]], ioregisters[registers[cmd[2]] + registers[cmd[3]]]);
	}
}
void writeToDiskout(FILE* diskOut) {
	int i = 0, j = 0;
	int imax = SECTOR_NUM - 1;
	int jmax = SECTOR_SIZE - 1;
	int f = 0;
	for (; imax >= 0 && f == 0; imax--) {
		for (jmax = SECTOR_SIZE - 1; jmax >= 0; jmax--) {
			if (diskMemory[imax][jmax]) {
				f = 1;
				break;
			}
		}
	}
	for (; i <= imax; i++) {
		for (j = 0; j <= jmax; j++) {
			fprintf(diskOut, "%08X\n", diskMemory[i][j]);
		}
	}
}
void writeToDisplay(FILE* display, int clk) {
	fprintf(display, "%d %08x\n", clk, ioregisters[10]);
}
/*******************************************************/
void writeToLeds(FILE* leds, int clk) {
	fprintf(leds, "%d %08x\n", clk, ioregisters[9]);
}


void getDiskContent(FILE* diskIn) {
	char word[LINE_SIZE];
	int i = 0, j = 0;
	while (!feof(diskIn) && fgets(word, LINE_SIZE, diskIn)) {
		if (j == SECTOR_SIZE) {
			j = 0;
			i++;
		}
		diskMemory[i][j++] = strtol(word, NULL, 16) & 0xFFFFFFFF;

	}
}
void exeCommand(int cmd[], int dis_irq2_clk_pc[], int regsArr[], FILE* irq2in, FILE* leds, FILE* display, FILE* hwregtrace) {
	int ledsChanged = 0;
	int displayChanged = 0;
	ioregisters[8] = (ioregisters[8] + 1) % 0xffffffff;

	if (dis_irq2_clk_pc[2] == dis_irq2_clk_pc[1]) {
		ioregisters[5] = 1;
		getNextIRQ2(dis_irq2_clk_pc, irq2in);
	}
	updateTimer();
	updateIrq0();
	switch (cmd[0]) { //execute cmd according to its opcode
	case 0:
		add(cmd, dis_irq2_clk_pc, regsArr);
		break;
	case 1:
		sub(cmd, dis_irq2_clk_pc, regsArr);
		break;
	case 2:
		and (cmd, dis_irq2_clk_pc, regsArr);
		break;
	case 3:
		or (cmd, dis_irq2_clk_pc, regsArr);
		break;
	case 4:
		sll(cmd, dis_irq2_clk_pc, regsArr);
		break;
	case 5:
		sra(cmd, dis_irq2_clk_pc, regsArr);
		break;
	case 6:
		srl(cmd, dis_irq2_clk_pc, regsArr);
		break;
	case 7:
		beq(cmd, dis_irq2_clk_pc, regsArr);
		break;
	case 8:
		bne(cmd, dis_irq2_clk_pc, regsArr);
		break;
	case 9:
		blt(cmd, dis_irq2_clk_pc, regsArr);
		break;
	case 10:
		bgt(cmd, dis_irq2_clk_pc, regsArr);
		break;
	case 11:
		ble(cmd, dis_irq2_clk_pc, regsArr);
		break;
	case 12:
		bge(cmd, dis_irq2_clk_pc, regsArr);
		break;
	case 13:
		jal(cmd, dis_irq2_clk_pc, regsArr);
		break;
	case 14:
		lw(cmd, dis_irq2_clk_pc, regsArr);
		break;
	case 15:
		sw(cmd, dis_irq2_clk_pc, regsArr);
		break;
	case 16:
		reti(cmd, dis_irq2_clk_pc, regsArr);
		break;
	case 17:
		in(cmd, dis_irq2_clk_pc, regsArr);
		break;
	case 18:
		if ((registers[cmd[2]] + registers[cmd[3]]) == 9 && ioregisters[9] != registers[cmd[1]]) {
			ledsChanged = 1;
		}
		if ((registers[cmd[2]] + registers[cmd[3]]) == 10 && ioregisters[10] != registers[cmd[1]]) {
			displayChanged = 1;
		}
		out(cmd, dis_irq2_clk_pc, regsArr);
		break;
	case 19:
		break;

	}
	writeTohwregTrace(hwregtrace, cmd, dis_irq2_clk_pc[2]);
	wrDisk(dis_irq2_clk_pc);
	perfomThePseka(dis_irq2_clk_pc);
	if (ledsChanged) {
		writeToLeds(leds, dis_irq2_clk_pc[2]);
	}
	if (displayChanged) {
		writeToDisplay(display, dis_irq2_clk_pc[2]);
	}
	dis_irq2_clk_pc[2]++;
	if (dis_irq2_clk_pc[2] == 0xffffffff)
		dis_irq2_clk_pc[2] = 0;
}
/*
	 * input arguments:
	 * int cmd[] the command to execute.
	 * int PC - the current PC.
	 * int regsArr[] - the registers array.
	 * This function execute a single add cmd.
	*/
void add(int cmd[], int dis_irq2_clk_pc[], int regsArr[]) {
	regsArr[cmd[1]] = regsArr[cmd[2]] + regsArr[cmd[3]];
	dis_irq2_clk_pc[3]++;

}

/*
 * input arguments:
 * int cmd[] the command to execute.
 * int PC - the current PC.
 * int regsArr[] - the registers array.
 * This function execute a single sub cmd.
*/
void sub(int cmd[], int dis_irq2_clk_pc[], int regsArr[]) {
	regsArr[cmd[1]] = regsArr[cmd[2]] - regsArr[cmd[3]];
	dis_irq2_clk_pc[3]++;

}


/*
 * input arguments:
 * int cmd[] the command to execute.
 * int PC - the current PC.
 * int regsArr[] - the registers array.
 * This function execute a single 'and' cmd.
*/
void and(int cmd[], int dis_irq2_clk_pc[], int regsArr[]) {
	regsArr[cmd[1]] = regsArr[cmd[2]] & regsArr[cmd[3]];
	dis_irq2_clk_pc[3]++;
}

/*
 * input arguments:
 * int cmd[] the command to execute.
 * int PC - the current PC.
 * int regsArr[] - the registers array.
 * This function execute a single 'or' cmd.
*/
void or (int cmd[], int dis_irq2_clk_pc[], int regsArr[]) {
	regsArr[cmd[1]] = regsArr[cmd[2]] | regsArr[cmd[3]];
	dis_irq2_clk_pc[3]++;
}

/*
 * input arguments:
 * int cmd[] the command to execute.
 * int PC - the current PC.
 * int regsArr[] - the registers array.
 * This function execute a single sll cmd.
*/
void sll(int cmd[], int dis_irq2_clk_pc[], int regsArr[]) {
	regsArr[cmd[1]] = regsArr[cmd[2]] << regsArr[cmd[3]];
	dis_irq2_clk_pc[3]++;
}

/*
 * input arguments:
 * int cmd[] the command to execute.
 * int PC - the current PC.
 * int regsArr[] - the registers array.
 * This function execute a single sra cmd.
*/
void sra(int cmd[], int dis_irq2_clk_pc[], int regsArr[]) {
	regsArr[cmd[1]] = regsArr[cmd[2]] >> regsArr[cmd[3]];
	dis_irq2_clk_pc[3]++;
}

/*
*input arguments :
*int cmd[] the command to execute.
* int PC - the current PC.
* int regsArr[] - the registers array.
* This function execute a single srl cmd.
*/
void srl(int cmd[], int dis_irq2_clk_pc[], int regsArr[]) {
	regsArr[cmd[1]] = regsArr[cmd[2]] >> regsArr[cmd[3]];
	dis_irq2_clk_pc[3]++;
}
/*
 * input arguments:
 * int cmd[] the command to execute.
 * int PC - the current PC.
 * int regsArr[] - the registers array.
 * This function execute a single cmd with beq opcode.
*/

void beq(int cmd[], int dis_irq2_clk_pc[], int regsArr[]) {
	dis_irq2_clk_pc[3] = dis_irq2_clk_pc[3] + 1;
	if (regsArr[cmd[2]] == regsArr[cmd[3]])
		dis_irq2_clk_pc[3] = regsArr[cmd[1]];
}
/*
	 * input arguments:
	 * int cmd[] the command to execute.
	 * int PC - the current PC.
	 * int regsArr[] - the registers array.
	 * This function execute a single cmd with bne opcode.
	*/

void bne(int cmd[], int dis_irq2_clk_pc[], int regsArr[]) {
	dis_irq2_clk_pc[3] = dis_irq2_clk_pc[3] + 1;
	if (regsArr[cmd[2]] != regsArr[cmd[3]])
		dis_irq2_clk_pc[3] = regsArr[cmd[1]];
}
/*
	 * input arguments:
	 * int cmd[] the command to execute.
	 * int PC - the current PC.
	 * int regsArr[] - the registers array.
	 * This function execute a single cmd with blt opcode.
	*/
void blt(int cmd[], int dis_irq2_clk_pc[], int regsArr[]) {
	dis_irq2_clk_pc[3] = dis_irq2_clk_pc[3] + 1;
	if (regsArr[cmd[2]] < regsArr[cmd[3]])
		dis_irq2_clk_pc[3] = regsArr[cmd[1]];
}
/*
	 * input arguments:
	 * int cmd[] the command to execute.
	 * int PC - the current PC.
	 * int regsArr[] - the registers array.
	 * This function execute a single cmd with bgt opcode.
	*/
void bgt(int cmd[], int dis_irq2_clk_pc[], int regsArr[]) {
	dis_irq2_clk_pc[3]++;
	if (regsArr[cmd[2]] > regsArr[cmd[3]])
		dis_irq2_clk_pc[3] = regsArr[cmd[1]];
}
/*
	 * input arguments:
	 * int cmd[] the command to execute.
	 * int PC - the current PC.
	 * int regsArr[] - the registers array.
	 * This function execute a single cmd with ble opcode.
	*/
void ble(int cmd[], int dis_irq2_clk_pc[], int regsArr[]) {
	dis_irq2_clk_pc[3]++;
	if (regsArr[cmd[2]] <= regsArr[cmd[3]])
		dis_irq2_clk_pc[3] = regsArr[cmd[1]];
}
/*
	 * input arguments:
	 * int cmd[] the command to execute.
	 * int PC - the current PC.
	 * int regsArr[] - the registers array.
	 * This function execute a single cmd with bge opcode.
	*/
void bge(int cmd[], int dis_irq2_clk_pc[], int regsArr[]) {
	dis_irq2_clk_pc[3]++;
	if (regsArr[cmd[2]] >= regsArr[cmd[3]])
		dis_irq2_clk_pc[3] = regsArr[cmd[1]];
}

/*
* input arguments:
* int cmd[] the command to execute.
* int PC - the current PC.
* int regsArr[] - the registers array.
*
*
* result argument:
* the function returns the PC value after the command execution.
*
* This function execute a single jal cmd.
*/

void jal(int cmd[], int dis_irq2_clk_pc[], int regsArr[]) {
	regsArr[15] = ((dis_irq2_clk_pc[3] + 1) & 0xFFF);
	dis_irq2_clk_pc[3] = regsArr[cmd[1]];
}
/*
	 * input arguments:
	 * int cmd[] the command to execute.
	 * int PC - the current PC.
	 * int regsArr[] - the registers array.
	 * This function execute a single cmd with reti opcode.
	*/
void reti(int cmd[], int dis_irq2_clk_pc[], int regsArr[]) {
	dis_irq2_clk_pc[3] = ioregisters[7];
	irqBussy = 0;
}
/*
	 * input arguments:
	 * int cmd[] the command to execute.
	 * int PC - the current PC.
	 * int regsArr[] - the registers array.
	 * This function execute a single cmd with in opcode.
	*/
void in(int cmd[], int dis_irq2_clk_pc[], int regsArr[]) {
	regsArr[cmd[1]] = ioregisters[regsArr[cmd[2]] + regsArr[cmd[3]]];
	dis_irq2_clk_pc[3]++;
}
/*
	 * input arguments:
	 * int cmd[] the command to execute.
	 * int PC - the current PC.
	 * int regsArr[] - the registers array.
	 * This function execute a single cmd with out opcode.
	*/
void out(int cmd[], int dis_irq2_clk_pc[], int regsArr[]) {
	ioregisters[regsArr[cmd[2]] + regsArr[cmd[3]]] = regsArr[cmd[1]];
	dis_irq2_clk_pc[3]++;
}



/*
 * input arguments:
 * int cmd[] the command to execute.
 * int PC - the current PC.
 * int regsArr[] - the registers array.
 * This function execute a single lw cmd.
*/

void lw(int cmd[], int dis_irq2_clk_pc[], int regsArr[]) {
	int addressToLoadFrom = regsArr[cmd[2]] + regsArr[cmd[3]]; //calculate the index to load from.
	regsArr[cmd[1]] = memory[addressToLoadFrom];
	dis_irq2_clk_pc[3]++;
}


/*
 * input arguments:
 * int cmd[] the command to execute.
 * int PC - the current PC.
 * int regsArr[] - the registers array.
 * This function execute a single sw cmd.
*/
void sw(int cmd[], int dis_irq2_clk_pc[], int regsArr[]) {
	memory[regsArr[cmd[2]] + regsArr[cmd[3]]] = regsArr[cmd[1]] & 0xFFFFFFFF;
	dis_irq2_clk_pc[3]++;
}

void updateIrq0() {
	if (ioregisters[0] == 0) {
		return;
	}
	if (ioregisters[12] == ioregisters[13]) {
		ioregisters[12] = 0;
		ioregisters[3] = 1;
		return;
	}
}
void updateTimer() {
	if (ioregisters[11]) {
		if (ioregisters[13]) {
			if (ioregisters[12] == ioregisters[13]) {
				ioregisters[12] = 0;
				ioregisters[3] = 1;
				return;
			}
			else
				ioregisters[12]++;
		}
		else {
			if (ioregisters[12] == 0xFFFFFFFF) {// might not be needed
				ioregisters[12] = 0;
				return;
			}
			ioregisters[12]++;
		}
	}
}
void wrDisk(int dis_irq2_clk_pc[]) {
	int i = 0;
	if (ioregisters[17] == 0) {
		dis_irq2_clk_pc[0] = 0;
		if (ioregisters[14] == 2) {
			ioregisters[17] = 1;
			for (; i < SECTOR_SIZE; i++)
				diskMemory[ioregisters[15]][i] = memory[ioregisters[16] + i];
		}
		else if (ioregisters[14] == 1) {
			ioregisters[17] = 1;
			for (; i < SECTOR_SIZE; i++)
				memory[ioregisters[16] + i] = diskMemory[ioregisters[15]][i];
		}
	}
	else if (ioregisters[17] == 1) {
		dis_irq2_clk_pc[0] += 1;
	}
	if (dis_irq2_clk_pc[0] == FRAME_CYCLES) {
		dis_irq2_clk_pc[0] = 0;
		ioregisters[17] = 0;
		ioregisters[4] = 1;
		ioregisters[14] = 0;
	}
}
void getNextIRQ2(int dis_irq2_clk_pc[], FILE* Irq2) {
	int x = 0;
	if (!feof(Irq2) && fscanf(Irq2, "%d ", &x)) {
		dis_irq2_clk_pc[1] = x;
	}
}

void perfomThePseka(int dis_irq2_clk_pc[]) {
	int irq = (ioregisters[0] && ioregisters[3]) ||
		(ioregisters[1] && ioregisters[4]) ||
		(ioregisters[2] && ioregisters[5]);
	if (irq && !irqBussy) {
		irqBussy = 1;
		ioregisters[7] = dis_irq2_clk_pc[3];
		dis_irq2_clk_pc[3] = ioregisters[6];
	}
	ioregisters[5] = 0;
}
int signExtention(int num) {
	int mask = 0x800;
	num = num & 0xFFF;
	if (mask & num) {
		num += 0xFFFFF000;
	}
	return num;
}


