#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>

/***************************************************************************************/
/*                                  Constants                                          */
/***************************************************************************************/


#define MAX_INM_ENTRY 16

/***************************************************************************************/
/*                                  Data Structures                                    */
/***************************************************************************************/

struct reg {
    int value;
    bool busy;
};

typedef enum instr_op_code{
	EADD,
	ESUB,
	EAND,
	EOR,
	ELD
} OPCODE;

char OPSTR[][4] = {
    "ADD",
    "SUB",
    "AND",
    "OR",
    "LD"
};

typedef struct instr_mem_entry {
    OPCODE op;
    int rd;
    int rs;
    int rt;
    //char instr_str[30]; //only 15 required
} inm_entry;

typedef struct instr_mem {
    inm_entry instr[MAX_INM_ENTRY];
    int front;
    int rear;
} INM;

typedef struct intermediate_instr_pipe {
    bool valid;
    OPCODE op;
    int rd;
    int src1_val;
    int src2_val;
    //char instr_str[30]; //only 15 required
} instr_pipe;

typedef struct address_buffer {
    bool valid;
    int rd;
    int addr;
} addr_buf;

typedef struct result_buffer {
    struct res_entry{
        bool valid;
        int rd;
        int val;
    }r_entry[2];
} res_buf;

/***************************************************************************************/
/*                                  Globals                                            */
/***************************************************************************************/

struct reg rgf[8];
int dam[8];

/***************************************************************************************/
/*                                  Function definitions                               */
/***************************************************************************************/

void dump_dam(){
    int i = 0;

    printf("DAM:");
    printf("<%d,%d>", i, dam[i]);
    i++;
    for(; i < 8; i++)
        printf(",<%d,%d>", i, dam[i]);
    printf("\n");
}

void parse_input_to_dam(const char *mem_str) {
    char *copy = strdup(mem_str);
    char *token = NULL;
    int addr;

    // Remove <>
    char *temp = copy + 1;
    temp[strlen(temp) - 1] = '\0';

    // Extract destination address
    token = strsep(&temp, ",");
    if(token){
        addr = atoi(token);
        if((addr >= 0) && (addr <= 8)) {
            dam[addr] = atoi(temp);
        }
    }
}

void initialize_dam(const char *damfile) {
	FILE *fin = fopen (damfile, "r");
	char line[30];

    while (fgets(line, sizeof(line), fin)) {
        //remove the trailing \n
        if (line[strlen(line)-1] == '\n')
            line[strlen(line)-1] = '\0';
        if (line[strlen(line)-1] == '\r')
            line[strlen(line)-1] = '\0';
        printf("read line %s\n", line);
        parse_input_to_dam(line);
    }

    fclose(fin);
}

void dump_rgf(){
    int i = 0;

    printf("RGF:");
    printf("<R%d,%d>", i, rgf[i].value);
    i++;
    for(; i < 8; i++)
        printf(",<R%d,%d>", i, rgf[i].value);
    printf("\n");
}

void parse_input_to_rgf(const char *reg_str) {
    char *copy = strdup(reg_str);
    char *token = NULL;
    int reg;

    // Remove <>
    char *temp = copy + 1;
    temp[strlen(temp) - 1] = '\0';

    // Extract destination register
    token = strsep(&temp, ",");
    if((token) && strlen(token) == 2){
        //Remove preceding R
        token++;
        reg = atoi(token);
        if((reg >= 0) && (reg <= 8)) {
            rgf[reg].value = atoi(temp);
        }
    }
}

void initialize_rgf(const char *regfile) {
	FILE *fin = fopen (regfile, "r");
	char line[30];

    while (fgets(line, sizeof(line), fin)) {
        //remove the trailing \n
        if (line[strlen(line)-1] == '\n')
            line[strlen(line)-1] = '\0';
        if (line[strlen(line)-1] == '\r')
            line[strlen(line)-1] = '\0';
        printf("read line %s\n", line);
        parse_input_to_rgf(line);
    }

    fclose(fin);
}

void enqueue_instr(inm_entry *instr, INM *inm)
{
	if(inm->rear < (MAX_INM_ENTRY - 1)) {
        inm->rear++;
		inm->instr[inm->rear] = *instr; //shallow copy
    }
}

inm_entry* dequeue_instr(INM *inm)
{
	inm_entry *retval = NULL;

	if(inm->front <= inm->rear) {
		retval = &(inm->instr[inm->front]);
		inm->front++;
    }

	return retval;
}

void parse_input_to_inm(INM *inm, const char *instr_str){
    inm_entry t_instr;
    char *copy = strdup(instr_str);
    char *token = NULL;

    // Remove <>
    char *temp = copy + 1;
    temp[strlen(temp) - 1] = '\0';

    // Extract opcode string
    token = strsep(&temp, ",");
    if(token) {
        if(!strcmp(token, OPSTR[EADD]))
            t_instr.op = EADD;
        else if(!strcmp(token, OPSTR[ESUB]))
            t_instr.op = ESUB;
        else if(!strcmp(token, OPSTR[EAND]))
            t_instr.op = EAND;
        else if(!strcmp(token, OPSTR[EOR]))
            t_instr.op = EOR;
        else if(!strcmp(token, OPSTR[ELD]))
            t_instr.op = ELD;
        else {
            printf("Parsing error - unsupported instruction\n");
            goto exit_parse;
        }
    }
    else {
        printf("Parsing error - empty operation string\n");
        goto exit_parse;
    }

    // Extract destination register
    token = strsep(&temp, ",");
    if((token) && strlen(token) == 2){
        //Remove preceding R
        token++;
        t_instr.rd = atoi(token);
        if((t_instr.rd < 0) || (t_instr.rd > 8)) {
            printf("Parsing error - invalid destination register\n");
            goto exit_parse;
        }
    }

    // Extract first source register
    token = strsep(&temp, ",");
    if((token) && strlen(token) == 2){
        //Remove preceding R
        token++;
        t_instr.rs = atoi(token);
        if((t_instr.rs < 0) || (t_instr.rs > 8)) {
            printf("Parsing error - invalid source-1 register\n");
            goto exit_parse;
        }
    }

    // Extract second source register
    token = strsep(&temp, ",");
    if((token) && strlen(token) == 2){
        //Remove preceding R
        token++;
        t_instr.rt = atoi(token);
        if((t_instr.rt < 0) || (t_instr.rt > 8)) {
            printf("Parsing error - invalid source-2 register\n");
            goto exit_parse;
        }
    }

    //strcpy(t_instr.instr_str, instr_str);

    // Now append this instruction to queue
    enqueue_instr(&t_instr, inm);
    printf("Appended <%s,%d,%d,%d> to queue at %d\n", OPSTR[inm->instr[inm->rear].op], inm->instr[inm->rear].rd, inm->instr[inm->rear].rs, inm->instr[inm->rear].rt, inm->rear);

exit_parse:
    free(copy);
}

void populate_inm(const char *instr_file, INM *inm) {
	FILE *fin = fopen (instr_file, "r");
	char line[30];

    while (fgets(line, sizeof(line), fin)) {
        //remove the trailing \n
        if (line[strlen(line)-1] == '\n')
            line[strlen(line)-1] = '\0';
        if (line[strlen(line)-1] == '\r')
            line[strlen(line)-1] = '\0';
        printf("read line %s\n", line);
        parse_input_to_inm(inm, line);
    }

    fclose(fin);
}

void display_inm(const INM *inm) {
    int i = inm->front;
    printf("INM:");
    printf("<%s,%d,%d,%d>", OPSTR[inm->instr[i].op], inm->instr[i].rd, inm->instr[i].rs, inm->instr[i].rt);
    i++;
    for (; i <= inm->rear; i++) {
        printf(",<%s,%d,%d,%d>", OPSTR[inm->instr[i].op], inm->instr[i].rd, inm->instr[i].rs, inm->instr[i].rt);
    }
    printf("\n");
}

bool is_empty_inm(INM *inm) {
    return (inm->front > inm->rear);
}

void decode_instr(INM *inm, instr_pipe *inb) {
 	inm_entry *top = NULL;
   //Check src regs availability for topmost instruction
    if(!(is_empty_inm(inm)) && !(rgf[inm->instr[inm->front].rs].busy) &&
        !(rgf[inm->instr[inm->front].rt].busy)){
        rgf[inm->instr[inm->front].rd].busy = true;
        top = dequeue_instr(inm);
        inb->op = top->op;
        inb->rd = top->rd;
        inb->src1_val = rgf[top->rs].value;
        inb->src2_val = rgf[top->rt].value;
        inb->valid = true;
        //sprintf(inb->instr_str, "<%s,R%d,%d,%d>", OPSTR[inb->op], inb->rd, inb->src1_val, inb->src2_val);
    }
}

void issue1(instr_pipe *inb, instr_pipe *aib){
    if((inb->valid) && (inb->op != ELD)) {
        *aib = *inb;
        // Free up the space
        memset(inb, 0, sizeof(instr_pipe));
    }
}

void issue2(instr_pipe *inb, instr_pipe *lib){
    if((inb->valid) && (inb->op == ELD)) {
        *lib = *inb;
        // Free up the space
        memset(inb, 0, sizeof(instr_pipe));
    }
}

void alu(instr_pipe *aib, res_buf *reb) {
    int val = 0;
    if(aib->valid) {
        switch(aib->op) {
            case EADD:
                val = aib->src1_val + aib->src2_val;
            break;

            case ESUB:
                val = aib->src1_val - aib->src2_val;
            break;

            case EAND:
                val = aib->src1_val & aib->src2_val;
            break;

            case EOR:
                val = aib->src1_val | aib->src2_val;
            break;

            default:
                printf("ALU error - Unsupported operation");
        }
        reb->r_entry[1].val = val;
        reb->r_entry[1].rd = aib->rd;
        reb->r_entry[1].valid = true;

        // Free up space
        memset(aib, 0, sizeof(instr_pipe));
    }
}

void addr_calc(instr_pipe *lib, addr_buf *abuf) {
    if(lib->valid)
    {
        abuf->rd = lib->rd;
        abuf->addr = lib->src1_val + lib->src2_val;
        abuf->valid = true;
    }
    // Free up space
    memset(lib, 0, sizeof(instr_pipe));
}

void load(addr_buf *abuf, res_buf *reb){
    if(abuf->valid) {
        reb->r_entry[0].val = dam[abuf->addr];
        reb->r_entry[0].rd = abuf->rd;
        reb->r_entry[0].valid = true;
    }
    // Free up space
    memset(abuf, 0, sizeof(addr_buf));
}

void write_rgf(res_buf *reb) {
    if(reb->r_entry[0].valid) {
        rgf[reb->r_entry[0].rd].value = reb->r_entry[0].val;
        rgf[reb->r_entry[0].rd].busy = false;
        //reb->r_entry[0].valid = false;
    }
    if(reb->r_entry[1].valid) {
        rgf[reb->r_entry[1].rd].value = reb->r_entry[1].val;
        rgf[reb->r_entry[1].rd].busy = false;
        //reb->r_entry[1].valid = false;
    }
    // Free up space
    memset(reb, 0, sizeof(res_buf));
}

void display_intd_pipe(instr_pipe *ds, const char *str) {
    printf("%s:", str);
    if(ds->valid) {
        printf("<%s,R%d,%d,%d>", OPSTR[ds->op], ds->rd, ds->src1_val, ds->src2_val);
    }
    printf("\n");
}

bool is_empty_reb(res_buf *reb){
    return (!reb->r_entry[0].valid && !reb->r_entry[1].valid);
}

int main() {
	INM inm;
	instr_pipe inb, lib, aib;
	addr_buf adb;
	res_buf reb;
	int step=0;

	memset(&inm, 0, sizeof(inm));
	memset(&inb, 0, sizeof(inb));
	memset(&lib, 0, sizeof(lib));
	memset(&aib, 0, sizeof(aib));
	memset(&adb, 0, sizeof(adb));
	memset(&reb, 0, sizeof(reb));
	inm.rear = -1;
	inm.front = 0;

	// Load instructions to instruction memory
    populate_inm("instructions.txt", &inm);
    //display_inm(&inm);

    initialize_rgf("registers.txt");
    //dump_rgf();

    initialize_dam("datamemory.txt");
    //dump_dam();

    /******************************* Dump initial data structures *****************************/
    printf("STEP %d:\n", step++);
    display_inm(&inm);
    display_intd_pipe(&inb, "INB");
    display_intd_pipe(&aib, "AIB");
    display_intd_pipe(&lib, "LIB");
    printf("ADB:");
    if(adb.valid)
        printf("<R%d,%d>", adb.rd, adb.addr);
    printf("\n");

    printf("REB:");
    if(reb.r_entry[0].valid)
        printf("<R%d,%d>", reb.r_entry[0].rd, reb.r_entry[0].val);
    if(reb.r_entry[1].valid) {
        if(reb.r_entry[0].valid)
            printf(",");
        printf("<R%d,%d>", reb.r_entry[1].rd, reb.r_entry[1].val);
    }
    printf("\n");
    dump_rgf();
    dump_dam();
    /******************************************************************************************/

    while(!is_empty_inm(&inm) || !is_empty_reb(&reb)) {
        write_rgf(&reb);
        load(&adb, &reb);
        alu(&aib, &reb);
        addr_calc(&lib, &adb);
        issue2(&inb, &lib);
        issue1(&inb, &aib);
        decode_instr(&inm, &inb);

        /************************ Dump the data structures at this step ***********************/
        printf("STEP %d:\n", step++);
        display_inm(&inm);
        display_intd_pipe(&inb, "INB");
        display_intd_pipe(&aib, "AIB");
        display_intd_pipe(&lib, "LIB");
        printf("ADB:");
        if(adb.valid)
            printf("<R%d,%d>", adb.rd, adb.addr);
        printf("\n");

        printf("REB:");
        if(reb.r_entry[0].valid)
            printf("<R%d,%d>", reb.r_entry[0].rd, reb.r_entry[0].val);
        if(reb.r_entry[1].valid) {
            if(reb.r_entry[0].valid)
                printf(",");
            printf("<R%d,%d>", reb.r_entry[1].rd, reb.r_entry[1].val);
        }
        printf("\n");
        dump_rgf();
        dump_dam();
        /**************************************************************************************/
    }

    /******************************* Dump final data structures *****************************/
    printf("STEP %d:\n", step++);
    display_inm(&inm);
    display_intd_pipe(&inb, "INB");
    display_intd_pipe(&aib, "AIB");
    display_intd_pipe(&lib, "LIB");
    printf("ADB:");
    if(adb.valid)
        printf("<R%d,%d>", adb.rd, adb.addr);
    printf("\n");

    printf("REB:");
    if(reb.r_entry[0].valid)
        printf("<R%d,%d>", reb.r_entry[0].rd, reb.r_entry[0].val);
    if(reb.r_entry[1].valid) {
        if(reb.r_entry[0].valid)
            printf(",");
        printf("<R%d,%d>", reb.r_entry[1].rd, reb.r_entry[1].val);
    }
    printf("\n");
    dump_rgf();
    dump_dam();
    /******************************************************************************************/

    return 0;
}

