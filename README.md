# computer-organization_project
#include "asm.h"

//variables are global because they need a lot of memory.
Label _Labels[MEMORY_SIZE]; //label array
Command _Commands[MEMORY_SIZE]; //commands array
WordCmd _Words[MEMORY_SIZE]; //word commands array. Index in array is the address of the command
char _memData[MEMORY_SIZE][WORD_LEN]; //array of memory to be written into memIn

int main(int argc, char* argv[]) {
	FILE *asm_file = NULL; //asm file pointer
	int memSize = 0; //number of commands in the program
	int numOfLabels = 0; //number of labels in the program
	int maxWordAddr = 0; //max address of a word command
	int fileSize = 0; //number of lines in memIn

	asm_file = fopen(argv[1], "r"); //open program file
	memSize = getCommands(asm_file, _Commands, _Words, _Labels, &maxWordAddr, &numOfLabels); //fill command and label array from program
	fclose(asm_file); //close program file
	writeMem(_memData, _Commands, memSize, _Labels, numOfLabels); //write the memory to memData array
	exeWordCmds(_memData, _Words, memSize); //execute all word commands (editing memData)
	fileSize = memSize > (maxWordAddr + 1) ? memSize : (maxWordAddr + 1);
	writeFile(_memData, argv[2], fileSize); //write the memory data to memIn file

	return 0;
}

/*
 * input arguments:
 * FILE* asm_file - the assembly program file to get commands from.
 * Command Commands[] - the commands array (will be updated).
 * WordCmd Words[] - the .word commands array (will be updated).
 * Label Labels[] - the labels array (will be updated).
 * int *maxWordAddr - max address of a .word command (will be updated).
 * int *numOfLabels - number of labels in the program (will be updated).
 *
 * result argument:
 * the fuction returns number of commands.
 *
 * This function fills the Commands, Words, Labels arrays according to the program file.
*/
int getCommands(FILE *asm_file, Command Commands[], WordCmd Words[], Label Labels[], int *maxWordAddr, int *numOfLabels) {
	int cmdNum = 0;
	char line[MAX_LINE_LEN];
	if (asm_file == NULL) exit(1); //no file found

	while (!feof(asm_file) && fgets(line, MAX_LINE_LEN, asm_file)) { //not an empty row, or NULL pointer
		if (isLabelLine(line))
			parseLabelLine(line, Labels, cmdNum, numOfLabels); //count the label, parse the label from the line and keep it in the labels array.
		if (!isRealLine(line)) continue; //if the line only had a label, continue
		if (isWordLine(line))  //if the line is a .word command - write it to the Words array
			parseWordLine(line, cmdNum, Words, maxWordAddr);
		else {
			parseCmdLine(line, Commands, cmdNum); //line is a command line - write it to the Commands array and count it.
			cmdNum++;
		}
	}
	return cmdNum;
}

/*
 * input arguments:
 * char line[] - a line from program file.
 *
 * result argument:
 * the function returns 1 if the line is not empty or a comment line and 0 otherwise.
 *
 * This function checks if a line has a command in it
*/
int isRealLine(char line[]) {
	char *p = line;
	while (isspace(*p)) p++;
	if (p == NULL || *p == '\0' || *p == '\n' || *p == '#') return 0;
	return 1;
}

/*
 * input arguments:
 * char str[]- a string to change.
 *
 * This fuction changes a string letters to lower case.
*/
void toLower(char str[]) {
	for (int i = 0; str[i]; i++) {
		str[i] = tolower(str[i]);
	}
}

/*
 * input arguments:
 * char line[] - a line from program file.
 *
 * This function removes all spaces from string.
*/
void trim(char line[]) {
	char cpy[MAX_LINE_LEN], c = 0;
	int i = 0, j = 0;
	do {
		while (isspace(line[i])) i++;
		c = cpy[j++] = line[i++];
	} while (c);
	strcpy(line, cpy); //keep the trimmed line only
}

/*
 * input arguments:
 * char line[] - a line from program file.
 *
 * result argument:
 * the function returns 1 if the line starts with a label and 0 otherwise.
 *
 * This function checks if a line is labeled.
*/
int isLabelLine(char line[]) {
	if ((strstr(line, ":") != NULL) && ((strstr(line, "#") == NULL) || (strstr(line, "#") > strstr(line, ":"))))
		return 1;
	return 0;
}

/*
 * input arguments:
 * char line[] - a line from program file.
 *
 * result argument:
 * the function returns 1 if the line starts with a '.' and 0 otherwise.
 *
 * This function checks if a line is a .word command.
*/
int isWordLine(char line[]) {
	if ((strstr(line, ".") != NULL) && ((strstr(line, "#") == NULL) || (strstr(line, "#") > strstr(line, "."))))
		return 1;
	return 0;
}

/*
 * input arguments:
 * char line[] - a line from program file.
 * Label Labels[] - the label array (to be updated).
 * int PC - current program counter (not counting immediate lines).
 * int *numOfLabels - pointer to total number of labels (will increase by 1).
 *
 * This function fills the labels array in labels and their PC's.
*/
void parseLabelLine(char line[], Label Labels[], int PC, int *numOfLabels) {
	char label[MAX_LABEL_LEN];
	strcpy(label, strtok(line, ":")); //set label to the string in line till (:)
	trim(label); //remove spaces from label.
	strcpy(line, (strtok(NULL, ""))); //save the rest of the line into line
	strcpy(Labels[*numOfLabels].label, label); //copy to struct
	Labels[*numOfLabels].PC = PC;
	(*numOfLabels)++;
}

/*
 * input arguments:
 * char line[] - the line in the program that has the .word command.
 * int PC - the current program counter (not including immediate lines).
 * WordCmd Words[] - the .word commands array (to be updated).
 * int *maxWordAddr - the biggest address of a word command (to be updated).
 *
 * This function fills the word array with word commands.
*/
void parseWordLine(char line[], int PC, WordCmd Words[], int *maxWordAddr) {
	int addr = 0, data = 0;
	strtok(line, " \n\t\v\f\r"); // token is .word
	addr = toNumber(strtok(NULL, " \n\t\v\f\r,")); //look number after the .word
	data = toNumber(strtok(NULL, " \n\t\v\f\r,#")); //look for second number
	if ((addr >= 0) && (addr < MEMORY_SIZE)) { //if addr is legal
		Words[addr].data = data;
		Words[addr].PC = PC;
		Words[addr].valid = 1;
		if (addr > *maxWordAddr) *maxWordAddr = addr; //update max
	}
}

/*
 * input arguments:
 * char line[] - the line in the program that has the command.
 * Command Commands[] - the commands array (to be updated).
 * int PC - the current program counter (not including immediate lines).
 *
 * This function fills the Commands array.
 * the function parses a program line and updates the command at PC index.
 * assuming the line is: opcode rd,$rs,$rt,imm:
 * 1. parse opcode as the line till the first whitespace and keep the rest.
 * 2. parse rd as the rest till the next comma (,) and keep the rest.
 * 3. parse rs as the rest till the next comma (,) and keep the rest.
 * 3. parse rt as the rest till the next comma (,) and keep the rest.
 * 4. parse imm to be the rest of the line.
*/
void parseCmdLine(char line[], Command Commands[], int PC) {
	strcpy(Commands[PC].opcode, strtok(line, " \n\t\v\f\r,#")); //save opcode.
	trim(Commands[PC].opcode);
	strcpy(line, (strtok(NULL, "#"))); // line is text between opcode and #
	trim(line);
	strcpy(Commands[PC].rd, strtok(line, " \n\t\v\f\r,"));
	strcpy(Commands[PC].rs, strtok(NULL, " \n\t\v\f\r,"));
	strcpy(Commands[PC].rt, strtok(NULL, " \n\t\v\f\r,"));
	strcpy(Commands[PC].imm, strtok(NULL, " \n\t\v\f\r,"));
}


/*
 * input arguments:
 * char memData[][] - the memory commands array to be written to memIn file.
 * Command Commands[] - the commands array with all the commands from the program.
 * int memSize - the actual memory size (number of commands)
 * Label Labels[] - the labels array with all the labels from the program file.
 * int numOfLabels - total number of label in the command
 *
 * This function writes out the memory into memData array without executing the .word commands.
 * It does that by converting the string values to integers, and formatting them as text.
*/
void writeMem(char memData[][WORD_LEN], Command Commands[], int memSize, Label Labels[], int numOfLabels) {
	int i = 0, opcode = 0, rd = 0, rs = 0, rt = 0, imm = 0;
	for (int PC = 0; PC < memSize; PC++) {
		opcode = strToInt(Commands[PC].opcode) & 0xFF;
		char str_op[50] = "";
		sprintf(str_op, "%X", opcode);
		if (strlen(str_op) == 1)
			add_digit(str_op, 1);
		rd = strToInt(Commands[PC].rd) & 0xF;
		rs = strToInt(Commands[PC].rs) & 0xF;
		rt = strToInt(Commands[PC].rt) & 0xF;
		imm = getImm(Commands[PC].imm, Labels, numOfLabels) & 0xFFF;
		char str_imm[50] = "";
		sprintf(str_imm, "%X", imm);
		if (strlen(str_imm) == 1)
			add_digit(str_imm, 2);
		else
			if (strlen(str_imm) == 2)
				add_digit(str_imm, 1);

		sprintf(memData[i++], "%s%X%X%X%s\n", str_op, rd, rs, rt, str_imm); //write out the memory line in hex
	}
}


/*
 * input arguments:
 * char memData[][] - the memory commands array to be written to memIn file.
 * WordCmd Words[] - the word commands array.
 * int memSize - the current memory size.
 *
 * This function will execute all .word commands by updating memData array.
*/
void exeWordCmds(char memData[][WORD_LEN], WordCmd Words[], int memSize) {
	//FIXME memSize changed
	for (int PC = 0; PC <= MEMORY_SIZE; PC++) {
		if (Words[PC].valid && (Words[PC].PC >= PC || PC > memSize))  //if .word command is the most recent, write it
			sprintf(memData[PC], "%08X\n", Words[PC].data);
		else if (PC >= memSize)
			strcpy(memData[PC], "00000000\n");
	}
}

/*
 * input arguments:
 * char memData[][] - the memory commands array to be written to memIn file.
 * char filename[] - name of the memIn file.
 * int fileSize - number of rows to write into file.
 *
 * This function will write memData array into memIn file.
*/
void writeFile(char memData[][WORD_LEN], const char filename[], int fileSize) {
	FILE *mem = fopen(filename, "w"); //open memIn file
	if (mem == NULL) exit(1);
	for (int i = 0; i < fileSize; i++)
		fprintf(mem, "%s", memData[i]);
	fclose(mem); //close memIn file
}

/*
 * input arguments:
 * char str[] - the string that need to be converted to a number value.
 *
 * result:
 * the corresponding integer to the string value or -1 if a value was not found.
 *
 * This function will convert a string representing opcode, reg or branch rd value to an integer.
*/
int strToInt(char str[]) {
	toLower(str);
	for (int i = 0; i < NUM_OPCODES; i++) { //search in the opcodes, regs, and branchOpcodes array the current str. the value is its index (NUM_BRANCH_OPS<NUM_OPCODES).
		if (!strcmp(str, opcode[i]) ||
			!strcmp(str, regs[i])) {
			return i;
		}
	}
	return 0; //not found
}


/*
 * input arguments:
 * curImm[] - the current immediate as a string.
 * Labels[] - the labels array.
 * int numOfLabels - the total number of labels in the program.
 *
 * result argument:
 * the function returns the immediate integer value.
 *
 * checks if its a label, otherwise its a number.
*/
int getImm(char curImm[], Label Labels[], int numOfLabels) {
	for (int i = 0; i <= numOfLabels; i++) { //check if theirs a match for a label and the current immediate.
		if (!strcmp(curImm, Labels[i].label)) {
			return Labels[i].PC;
		}
	}
	return toNumber(curImm);
}

/*
 * input arguments:
 * number[] - the number as a string.
 *
 * result argument:
 * the function returns the number integer value for base 10 and 16.
 *
 * checks if its base16, otherwise it is base 10.
*/
int toNumber(char number[]) {
	if (strstr(number, "0x") != NULL || strstr(number, "0X") != NULL)  //if the data is in hex
		return strtol(number, NULL, 16) & 0xFFFF;
	return strtol(number, NULL, 10) & 0xFFFF;
}

void add_digit(char str[], int x) {
	char zero[50] = "";
	if (x == 1)
		strcpy(zero, "0");
	else
		if (x == 2)
			strcpy(zero, "00");
	strcat(zero, str);
	strcpy(str, zero);
}
