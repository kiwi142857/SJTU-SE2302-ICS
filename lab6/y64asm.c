#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "y64asm.h"

line_t *line_head = NULL;
line_t *line_tail = NULL;
int lineno = 0;

#define err_print(_s, _a...)                                                                                           \
    do {                                                                                                               \
        if (lineno < 0)                                                                                                \
            fprintf(stderr,                                                                                            \
                    "[--]: "_s                                                                                         \
                    "\n",                                                                                              \
                    ##_a);                                                                                             \
        else                                                                                                           \
            fprintf(stderr,                                                                                            \
                    "[L%d]: "_s                                                                                        \
                    "\n",                                                                                              \
                    lineno, ##_a);                                                                                     \
    } while (0);

int64_t vmaddr = 0; /* vm addr */
int is_invalid_imm = 0;
/* register table */
const reg_t reg_table[REG_NONE] = {
    {"%rax", REG_RAX, 4}, {"%rcx", REG_RCX, 4}, {"%rdx", REG_RDX, 4}, {"%rbx", REG_RBX, 4}, {"%rsp", REG_RSP, 4},
    {"%rbp", REG_RBP, 4}, {"%rsi", REG_RSI, 4}, {"%rdi", REG_RDI, 4}, {"%r8", REG_R8, 3},   {"%r9", REG_R9, 3},
    {"%r10", REG_R10, 4}, {"%r11", REG_R11, 4}, {"%r12", REG_R12, 4}, {"%r13", REG_R13, 4}, {"%r14", REG_R14, 4}};
const reg_t *find_register(char *name)
{
    int i;
    for (i = 0; i < REG_NONE; i++)
        if (!strncmp(name, reg_table[i].name, reg_table[i].namelen))
            return &reg_table[i];
    return NULL;
}

/* instruction set */
instr_t instr_set[] = {
    {"nop", 3, HPACK(I_NOP, F_NONE), 1},
    {"halt", 4, HPACK(I_HALT, F_NONE), 1},
    {"rrmovq", 6, HPACK(I_RRMOVQ, F_NONE), 2},
    {"cmovle", 6, HPACK(I_RRMOVQ, C_LE), 2},
    {"cmovl", 5, HPACK(I_RRMOVQ, C_L), 2},
    {"cmove", 5, HPACK(I_RRMOVQ, C_E), 2},
    {"cmovne", 6, HPACK(I_RRMOVQ, C_NE), 2},
    {"cmovge", 6, HPACK(I_RRMOVQ, C_GE), 2},
    {"cmovg", 5, HPACK(I_RRMOVQ, C_G), 2},
    {"irmovq", 6, HPACK(I_IRMOVQ, F_NONE), 10},
    {"rmmovq", 6, HPACK(I_RMMOVQ, F_NONE), 10},
    {"mrmovq", 6, HPACK(I_MRMOVQ, F_NONE), 10},
    {"addq", 4, HPACK(I_ALU, A_ADD), 2},
    {"subq", 4, HPACK(I_ALU, A_SUB), 2},
    {"andq", 4, HPACK(I_ALU, A_AND), 2},
    {"xorq", 4, HPACK(I_ALU, A_XOR), 2},
    {"jmp", 3, HPACK(I_JMP, C_YES), 9},
    {"jle", 3, HPACK(I_JMP, C_LE), 9},
    {"jl", 2, HPACK(I_JMP, C_L), 9},
    {"je", 2, HPACK(I_JMP, C_E), 9},
    {"jne", 3, HPACK(I_JMP, C_NE), 9},
    {"jge", 3, HPACK(I_JMP, C_GE), 9},
    {"jg", 2, HPACK(I_JMP, C_G), 9},
    {"call", 4, HPACK(I_CALL, F_NONE), 9},
    {"ret", 3, HPACK(I_RET, F_NONE), 1},
    {"pushq", 5, HPACK(I_PUSHQ, F_NONE), 2},
    {"popq", 4, HPACK(I_POPQ, F_NONE), 2},
    {".byte", 5, HPACK(I_DIRECTIVE, D_DATA), 1},
    {".word", 5, HPACK(I_DIRECTIVE, D_DATA), 2},
    {".long", 5, HPACK(I_DIRECTIVE, D_DATA), 4},
    {".quad", 5, HPACK(I_DIRECTIVE, D_DATA), 8},
    {".pos", 4, HPACK(I_DIRECTIVE, D_POS), 0},
    {".align", 6, HPACK(I_DIRECTIVE, D_ALIGN), 0},
    {NULL, 1, 0, 0} // end
};

instr_t *find_instr(char *name)
{
    int i;
    for (i = 0; instr_set[i].name; i++)
        if (strncmp(instr_set[i].name, name, instr_set[i].len) == 0)
            return &instr_set[i];
    return NULL;
}

/* symbol table (don't forget to init and finit it) */
symbol_t *symtab = NULL;

/*
 * find_symbol: scan table to find the symbol
 * args
 *     name: the name of symbol
 *
 * return
 *     symbol_t: the 'name' symbol
 *     NULL: not exist
 */
symbol_t *find_symbol(char *name)
{
    symbol_t *sym = symtab->next;
    for (; sym != NULL; sym = sym->next)
        if (strcmp(sym->name, name) == 0)
            return sym;
    if (sym == NULL)
        return NULL;
    return sym;
}

/*
 * add_symbol: add a new symbol to the symbol table
 * args
 *     name: the name of symbol
 *
 * return
 *     0: success
 *     -1: error, the symbol has exist
 */
int add_symbol(char *name)
{
    /* check duplicate */
    symbol_t *sym = find_symbol(name);
    if (sym) {
        err_print("Dup symbol:%s", name);
        return -1;
    }
    /* create new symbol_t (don't forget to free it)*/
    symbol_t *newsym = (symbol_t *)malloc(sizeof(symbol_t));
    newsym->name = name;
    newsym->addr = vmaddr;
    if (symtab == NULL) {
        symtab = newsym;
        return 0;
    }
    newsym->next = symtab->next;
    /* add the new symbol_t to symbol table */
    symtab->next = newsym;
    return 0;
}

/* relocation table (don't forget to init and finit it) */
reloc_t *reltab = NULL;

/*
 * add_reloc: add a new relocation to the relocation table
 * args
 *     name: the name of symbol
 */
void add_reloc(char *name, bin_t *bin)
{
    /* create new reloc_t (don't forget to free it)*/
    reloc_t *newReloct = (reloc_t *)malloc(sizeof(reloc_t));
    newReloct->name = name;
    newReloct->y64bin = bin;
    /* add the new reloc_t to relocation table */
    if (reltab == NULL) {
        reltab = newReloct;
        return;
    }
    reloc_t *reloc = reltab;
    for (; reloc->next != NULL; reloc = reloc->next)
        ;

    reloc->next = newReloct;
}

/* macro for parsing y64 assembly code */
#define IS_DIGIT(s) ((*(s) >= '0' && *(s) <= '9') || *(s) == '-' || *(s) == '+')
#define IS_LETTER(s) ((*(s) >= 'a' && *(s) <= 'z') || (*(s) >= 'A' && *(s) <= 'Z'))
#define IS_COMMENT(s) (*(s) == '#')
#define IS_REG(s) (*(s) == '%')
#define IS_IMM(s) (*(s) == '$')

#define IS_BLANK(s) (*(s) == ' ' || *(s) == '\t')
#define IS_END(s) (*(s) == '\0')

#define SKIP_BLANK(s)                                                                                                  \
    do {                                                                                                               \
        while (!IS_END(s) && IS_BLANK(s))                                                                              \
            (s)++;                                                                                                     \
    } while (0);

/* return value from different parse_xxx function */
typedef enum
{
    PARSE_ERR = -1,
    PARSE_REG,
    PARSE_DIGIT,
    PARSE_SYMBOL,
    PARSE_MEM,
    PARSE_DELIM,
    PARSE_INSTR,
    PARSE_LABEL
} parse_t;

/*
 * parse_instr: parse an expected data token (e.g., 'rrmovq')
 * args
 *     ptr: point to the start of string
 *     inst: point to the inst_t within instr_set
 *
 * return
 *     PARSE_INSTR: success, move 'ptr' to the first char after token,
 *                            and store the pointer of the instruction to 'inst'
 *     PARSE_ERR: error, the value of 'ptr' and 'inst' are undefined
 */
parse_t parse_instr(char **ptr, instr_t **inst)
{
    char *tmp_ptr = *ptr;
    /* skip the blank */
    SKIP_BLANK(tmp_ptr);
    /* find_instr and check end */
    instr_t *instr = find_instr(tmp_ptr);
    if (instr == NULL)
        return PARSE_ERR;

    /* set 'ptr' and 'inst' */
    *inst = instr;
    *ptr = tmp_ptr + instr->len;
    return PARSE_INSTR;
}

/*
 * parse_delim: parse an expected delimiter token (e.g., ',')
 * args
 *     ptr: point to the start of string
 *
 * return
 *     PARSE_DELIM: success, move 'ptr' to the first char after token
 *     PARSE_ERR: error, the value of 'ptr' and 'delim' are undefined
 */
parse_t parse_delim(char **ptr, char delim)
{
    char *tmp_ptr = *ptr;
    /* skip the blank and check */
    SKIP_BLANK(tmp_ptr);
    if (*tmp_ptr != delim) {
        err_print("Invalid '%c'",delim) return PARSE_ERR;
    }
    /* set 'ptr' */
    *ptr = tmp_ptr + 1;
    return PARSE_DELIM;
}

/*
 * parse_reg: parse an expected register token (e.g., '%rax')
 * args
 *     ptr: point to the start of string
 *     regid: point to the regid of register
 *
 * return
 *     PARSE_REG: success, move 'ptr' to the first char after token,
 *                         and store the regid to 'regid'
 *     PARSE_ERR: error, the value of 'ptr' and 'regid' are undefined
 */
parse_t parse_reg(char **ptr, regid_t *regid)
{
    char *tmp_ptr = *ptr;
    /* skip the blank and check */
    SKIP_BLANK(tmp_ptr);
    /* find register */
    const reg_t *reg = find_register(tmp_ptr);
    if (reg == NULL) {
        err_print("Invalid REG");
        return PARSE_ERR;
    }
    /* set 'ptr' and 'regid' */
    *ptr = tmp_ptr + reg->namelen;
    *regid = reg->id;
    return PARSE_REG;
}

/*
 * parse_symbol: parse an expected symbol token (e.g., 'Main')
 * args
 *     ptr: point to the start of string
 *     name: point to the name of symbol (should be allocated in this function)
 *
 * return
 *     PARSE_SYMBOL: success, move 'ptr' to the first char after token,
 *                               and allocate and store name to 'name'
 *     PARSE_ERR: error, the value of 'ptr' and 'name' are undefined
 */
parse_t parse_symbol(char **ptr, char **name)
{
    char *tmp_ptr = *ptr;
    /* skip the blank and check */
    SKIP_BLANK(tmp_ptr);
    symbol_t *sym = find_symbol(tmp_ptr);
    if (sym == NULL)
        return PARSE_ERR;
    /* allocate name and copy to it */

    /* cout the length of name*/
    int len = strlen(sym->name);
    *name = malloc(len + 1);

    /* set all the space 0*/

    strcpy(*name, sym->name);
    (*name)[len] = 0;

    /* set 'ptr' and 'name' */
    *ptr = tmp_ptr + len;
    return PARSE_SYMBOL;
}

/*
 * parse_digit: parse an expected digit token (e.g., '0x100')
 * args
 *     ptr: point to the start of string
 *     value: point to the value of digit
 *
 * return
 *     PARSE_DIGIT: success, move 'ptr' to the first char after token
 *                            and store the value of digit to 'value'
 *     PARSE_ERR: error, the value of 'ptr' and 'value' are undefined
 */
parse_t parse_digit(char **ptr, long *value)
{
    char *tmp_ptr = *ptr;
    /* skip the blank and check */
    SKIP_BLANK(tmp_ptr);
    /* since there maybe exist minus before the number*/
    /* since there maybe exist 0x before the number */
    int isMinus = 0, isHex = 0;
    if (*tmp_ptr == '-') {
        tmp_ptr++;
        isMinus = 1;
    }
    if (*tmp_ptr == '0' && *(tmp_ptr + 1) == 'x') {
        tmp_ptr += 2;
        isHex = 1;
    }
    if (IS_DIGIT(tmp_ptr) == 0 && (isHex && IS_LETTER(tmp_ptr)) == 0)
        return PARSE_ERR;

    /* calculate the digit, (NOTE: see strtoll()) */
    *value = strtoull(tmp_ptr, NULL, isHex ? 16 : 10);
    /* move tmp_ptr to the end of the num*/
    while (IS_DIGIT(tmp_ptr) || (isHex && IS_LETTER(tmp_ptr)))
        tmp_ptr++;
    /* set 'ptr' and 'value' */
    if (isMinus)
        *value = -(*value);
    *ptr = tmp_ptr;
    return PARSE_DIGIT;
}

/*
 * parse_imm: parse an expected immediate token (e.g., '$0x100' or 'STACK')
 * args
 *     ptr: point to the start of string
 *     name: point to the name of symbol (should be allocated in this function)
 *     value: point to the value of digit
 *
 * return
 *     PARSE_DIGIT: success, the immediate token is a digit,
 *                            move 'ptr' to the first char after token,
 *                            and store the value of digit to 'value'
 *     PARSE_SYMBOL: success, the immediate token is a symbol,
 *                            move 'ptr' to the first char after token,
 *                            and allocate and store name to 'name'
 *     PARSE_ERR: error, the value of 'ptr', 'name' and 'value' are undefined
 */
parse_t parse_imm(char **ptr, char **name, long *value)
{
    char *tmp_ptr = *ptr;
    /* skip the blank and check */
    SKIP_BLANK(tmp_ptr);
    /* if IS_DIGIT, then parse the digit */
    if (tmp_ptr[0] == '$') {
        tmp_ptr++;
        if (parse_digit(&tmp_ptr, value) == PARSE_ERR) {
            err_print("Invalid Immediate");
            is_invalid_imm = 1;
            return PARSE_ERR;
        }
        *ptr = tmp_ptr;
        return PARSE_DIGIT;
    }
    /* if IS_IMM, then parse the digit */
    else if (IS_LETTER(tmp_ptr)) {
        if (parse_symbol(&tmp_ptr, name) == PARSE_ERR) {
            return PARSE_ERR;
        }
        *ptr = tmp_ptr;
        return PARSE_SYMBOL;
    }
    /* if IS_LETTER, then parse the symbol */

    /* set 'ptr' and 'name' or 'value' */

    err_print("Invalid Immediate");
    return PARSE_ERR;
}

/*
 * parse_mem: parse an expected memory token (e.g., '8(%rbp)')
 * args
 *     ptr: point to the start of string
 *     value: point to the value of digit
 *     regid: point to the regid of register
 *
 * return
 *     PARSE_MEM: success, move 'ptr' to the first char after token,
 *                          and store the value of digit to 'value',
 *                          and store the regid to 'regid'
 *     PARSE_ERR: error, the value of 'ptr', 'value' and 'regid' are undefined
 */
parse_t parse_mem(char **ptr, long *value, regid_t *regid)
{
    char *tmp_ptr = *ptr;
    long tmp_value;
    regid_t tmp_regid;
    /* skip the blank and check */
    SKIP_BLANK(tmp_ptr);
    /* calculate the digit and register, (ex: (%rbp) or 8(%rbp)) */
    if (IS_DIGIT(tmp_ptr)) {
        if (parse_digit(&tmp_ptr, &tmp_value) == PARSE_ERR) {
            err_print("Invalid MEM");
            return PARSE_ERR;
        }
    } else
        tmp_value = 0;
    if (*tmp_ptr == '(')
        tmp_ptr++;
    else {
        err_print("Invalid MEM");
        return PARSE_ERR;
    }
    if (parse_reg(&tmp_ptr, &tmp_regid) == PARSE_ERR) {
        err_print("Invalid MEM");
        return PARSE_ERR;
    }
    if (*tmp_ptr == ')')
        tmp_ptr++;
    else {
        err_print("Invalid MEM");
        return PARSE_ERR;
    }
    /* set 'ptr', 'value' and 'regid' */
    *ptr = tmp_ptr;
    *value = tmp_value;
    *regid = tmp_regid;
    return PARSE_MEM;
}

/*
 * parse_data: parse an expected data token (e.g., '0x100' or 'array')
 * args
 *     ptr: point to the start of string
 *     name: point to the name of symbol (should be allocated in this function)
 *     value: point to the value of digit
 *
 * return
 *     PARSE_DIGIT: success, data token is a digit,
 *                            and move 'ptr' to the first char after token,
 *                            and store the value of digit to 'value'
 *     PARSE_SYMBOL: success, data token is a symbol,
 *                            and move 'ptr' to the first char after token,
 *                            and allocate and store name to 'name'
 *     PARSE_ERR: error, the value of 'ptr', 'name' and 'value' are undefined
 */
parse_t parse_data(char **ptr, char **name, long *value)
{
    char *tmp_ptr = *ptr;
    /* skip the blank and check */
    SKIP_BLANK(tmp_ptr);
    /* if IS_DIGIT, then parse the digit */
    if (parse_digit(&tmp_ptr, value) != PARSE_ERR) {
        *ptr = tmp_ptr;
        return PARSE_DIGIT;
    }
    /* if IS_LETTER, then parse the symbol */
    if (parse_symbol(&tmp_ptr, name) != PARSE_ERR) {
        *ptr = tmp_ptr;
        return PARSE_SYMBOL;
    } else
        return PARSE_ERR;

    /* set 'ptr', 'name' and 'value' */

    return PARSE_ERR;
}

/*
 * parse_label: parse an expected label token (e.g., 'Loop:')
 * args
 *     ptr: point to the start of string
 *     name: point to the name of symbol (should be allocated in this function)
 *
 * return
 *     PARSE_LABEL: success, move 'ptr' to the first char after token
 *                            and allocate and store name to 'name'
 *     PARSE_ERR: error, the value of 'ptr' is undefined
 */
parse_t parse_label(char **ptr, char **name)
{
    char *tmp_ptr = *ptr;
    /* skip the blank and check */
    SKIP_BLANK(tmp_ptr);
    if (IS_LETTER(tmp_ptr) == 0)
        return PARSE_ERR;
    /* allocate name and copy to it */
    char *tmp_name = tmp_ptr;
    while (IS_LETTER(tmp_ptr) || IS_DIGIT(tmp_ptr))
        tmp_ptr++;
    if (*tmp_ptr != ':')
        return PARSE_ERR;
    int len = tmp_ptr - tmp_name;
    *name = malloc(len + 1);
    strncpy(*name, tmp_name, len);
    (*name)[len] = 0;
    /* set 'ptr' and 'name' */
    *ptr = tmp_ptr + 1;
    return PARSE_LABEL;
}

/*
 * parse_line: parse a line of y64 code (e.g., 'Loop: mrmovq (%rcx), %rsi')
 * (you could combine above parse_xxx functions to do it)
 * args
 *     line: point to a line_t data with a line of y64 assembly code
 *
 * return
 *     TYPE_XXX: success, fill line_t with assembled y64 code
 *     TYPE_ERR: error, try to print err information
 */
type_t parse_line(line_t *line)
{

    /* when finish parse an instruction or lable, we still need to continue check
     * e.g.,
     *  Loop: mrmovl (%rbp), %rcx
     *           call SUM  #invoke SUM function */

    /* skip blank and check IS_END */
    if (IS_END(line->y64asm)) {
        line->type = TYPE_COMM;
        return line->type;
    }
    char *tmp_ptr = line->y64asm;
    line->type = TYPE_ERR;
    SKIP_BLANK(tmp_ptr);
    /* is a comment ? */
    if (IS_COMMENT(tmp_ptr)) {
        line->type = TYPE_COMM;
        return line->type;
    }
    /* is a label ? */
    char *name;
    if (parse_label(&tmp_ptr, &name) == PARSE_LABEL) {
        if (add_symbol(name) == -1) {
            line->type = TYPE_ERR;
            return line->type;
        }

        line->type = TYPE_INS;
        line->y64bin.addr = vmaddr;
    }
    /* is an instruction ? */
    instr_t *inst;
    regid_t rA = REG_NONE, rB = REG_NONE;
    char *immName;
    long value = 0;
    char *symbol;
    parse_t type;

    if (parse_instr(&tmp_ptr, &inst) == PARSE_INSTR) {
        switch (HIGH(inst->code)) {
        case I_HALT:
            line->y64bin.addr = vmaddr;
            line->y64bin.bytes = 1;
            line->y64bin.codes[0] = inst->code;
            line->type = TYPE_INS;
            vmaddr += 1;
            return line->type;
        case I_NOP:
            line->y64bin.addr = vmaddr;
            line->y64bin.bytes = 1;
            line->y64bin.codes[0] = inst->code;
            line->type = TYPE_INS;
            vmaddr += 1;
            return line->type;
        case I_RRMOVQ:
        case I_ALU:
            if (parse_reg(&tmp_ptr, &rA) == PARSE_REG && parse_delim(&tmp_ptr, ',') == PARSE_DELIM &&
                parse_reg(&tmp_ptr, &rB) == PARSE_REG) {
                line->y64bin.addr = vmaddr;
                line->type = TYPE_INS;
                line->y64bin.bytes = 2;
                line->y64bin.codes[0] = inst->code;
                line->y64bin.codes[1] = HPACK(rA, rB);
                vmaddr += 2;
                return line->type;
            }
            break;
        case I_IRMOVQ:
            type = parse_imm(&tmp_ptr, &immName, &value);
            if (is_invalid_imm) {
                line->type = TYPE_ERR;
                return line->type;
            }
            if (type == PARSE_ERR) {
                SKIP_BLANK(tmp_ptr);
                char *old_ptr = tmp_ptr;
                while (IS_LETTER(tmp_ptr) || IS_DIGIT(tmp_ptr))
                    tmp_ptr++;
                char *symbolName = malloc(tmp_ptr - old_ptr + 1);
                strncpy(symbolName, old_ptr, tmp_ptr - old_ptr);
                add_reloc(symbolName, &line->y64bin);
                parse_delim(&tmp_ptr, ',');
                parse_reg(&tmp_ptr, &rA);
                line->type = TYPE_INS;
                line->y64bin.addr = vmaddr;
                vmaddr += 10;
                line->y64bin.bytes = 10;
                line->y64bin.codes[0] = inst->code;
                line->y64bin.codes[1] = HPACK(REG_NONE, rA);
                return line->type;
                /* tmp_ptr skip the Symbol's length*/
            }
            if (parse_delim(&tmp_ptr, ',') == PARSE_DELIM && parse_reg(&tmp_ptr, &rA) == PARSE_REG) {
                if (type == PARSE_SYMBOL)
                    add_reloc(immName, &line->y64bin);
                line->type = TYPE_INS;
                line->y64bin.addr = vmaddr;
                vmaddr += 10;
                line->y64bin.bytes = 10;
                line->y64bin.codes[0] = inst->code;
                line->y64bin.codes[1] = HPACK(REG_NONE, rA);
                for (int i = 0; i < 8; ++i) {
                    line->y64bin.codes[i + 2] = (value >> (i * 8)) & 0xFF;
                }
                return line->type;
            }
            break;
        case I_RMMOVQ:
            if (parse_reg(&tmp_ptr, &rA) == PARSE_REG && parse_delim(&tmp_ptr, ',') == PARSE_DELIM &&
                parse_mem(&tmp_ptr, &value, &rB) == PARSE_MEM) {
                line->type = TYPE_INS;
                line->y64bin.addr = vmaddr;
                vmaddr += 10;
                line->y64bin.bytes = 10;
                line->y64bin.codes[0] = inst->code;
                line->y64bin.codes[1] = HPACK(rA, rB);
                for (int i = 0; i < 8; ++i) {
                    line->y64bin.codes[i + 2] = (value >> (i * 8)) & 0xFF;
                }
                return line->type;
            }
            break;
        case I_MRMOVQ:
            if (parse_mem(&tmp_ptr, &value, &rB) == PARSE_MEM && parse_delim(&tmp_ptr, ',') == PARSE_DELIM &&
                parse_reg(&tmp_ptr, &rA) == PARSE_REG) {
                line->type = TYPE_INS;
                line->y64bin.addr = vmaddr;
                vmaddr += 10;
                line->y64bin.bytes = 10;
                line->y64bin.codes[0] = inst->code;
                line->y64bin.codes[1] = HPACK(rA, rB);
                for (int i = 0; i < 8; ++i) {
                    line->y64bin.codes[i + 2] = (value >> (i * 8)) & 0xFF;
                }
                return line->type;
            }
            break;
        case I_JMP:
            if (parse_digit(&tmp_ptr, &value) == PARSE_DIGIT) {
                if (value < 0 || value > 100) {
                    err_print("Invalid DEST");
                    line->type = TYPE_ERR;
                    return line->type;
                }
                line->type = TYPE_INS;
                line->y64bin.addr = vmaddr;
                vmaddr += 9;
                line->y64bin.bytes = 9;
                line->y64bin.codes[0] = inst->code;
                for (int i = 0; i < 8; ++i) {
                    line->y64bin.codes[i + 1] = (value >> (i * 8)) & 0xFF;
                }
                return line->type;
            }
            type = parse_symbol(&tmp_ptr, &symbol);
            if (type == PARSE_SYMBOL || type == PARSE_ERR) {
                if (type == PARSE_SYMBOL)
                    add_reloc(symbol, &line->y64bin);
                else {
                    SKIP_BLANK(tmp_ptr);
                    char *old_ptr = tmp_ptr;
                    while (IS_LETTER(tmp_ptr) || IS_DIGIT(tmp_ptr))
                        tmp_ptr++;
                    char *symbolName = malloc(tmp_ptr - old_ptr + 1);
                    strncpy(symbolName, old_ptr, tmp_ptr - old_ptr);
                    add_reloc(symbolName, &line->y64bin);
                }
                line->type = TYPE_INS;
                line->y64bin.addr = vmaddr;
                vmaddr += 9;
                line->y64bin.bytes = 9;
                line->y64bin.codes[0] = inst->code;
                return line->type;
            }
            line->type = TYPE_ERR;
            return line->type;
        case I_CALL:
            if (parse_digit(&tmp_ptr, &value) == PARSE_DIGIT) {
                line->type = TYPE_INS;
                line->y64bin.addr = vmaddr;
                vmaddr += 9;
                line->y64bin.bytes = 9;
                line->y64bin.codes[0] = inst->code;
                for (int i = 0; i < 8; ++i) {
                    line->y64bin.codes[i + 1] = (value >> (i * 8)) & 0xFF;
                }
                return line->type;
            }
            type = parse_symbol(&tmp_ptr, &symbol);
            if (type == PARSE_SYMBOL)
                add_reloc(symbol, &line->y64bin);
            else if (type == PARSE_ERR) {
                SKIP_BLANK(tmp_ptr);
                char *old_ptr = tmp_ptr;
                while (IS_LETTER(tmp_ptr) || IS_DIGIT(tmp_ptr))
                    tmp_ptr++;
                char *symbolName = malloc(tmp_ptr - old_ptr + 1);
                strncpy(symbolName, old_ptr, tmp_ptr - old_ptr);
                add_reloc(symbolName, &line->y64bin);
            }
            line->type = TYPE_INS;
            line->y64bin.addr = vmaddr;
            vmaddr += 9;
            line->y64bin.bytes = 9;
            line->y64bin.codes[0] = inst->code;
            return line->type;

            break;
        case I_RET:
            line->y64bin.addr = vmaddr;
            line->y64bin.bytes = 1;
            line->y64bin.codes[0] = inst->code;
            line->type = TYPE_INS;
            vmaddr += 1;
            return line->type;
        case I_PUSHQ:
        case I_POPQ:
            if (parse_reg(&tmp_ptr, &rA) == PARSE_REG) {
                line->y64bin.addr = vmaddr;
                line->type = TYPE_INS;
                line->y64bin.bytes = 2;
                line->y64bin.codes[0] = inst->code;
                line->y64bin.codes[1] = HPACK(rA, rB);
                vmaddr += 2;
                return line->type;
            }
            break;
        case I_DIRECTIVE:
            if (inst->code == HPACK(I_DIRECTIVE, D_DATA)) {
                type = parse_data(&tmp_ptr, &name, &value);

                if (type == PARSE_SYMBOL)
                    add_reloc(name, &line->y64bin);
                else if (type == PARSE_ERR) {
                    SKIP_BLANK(tmp_ptr);
                    char *old_ptr = tmp_ptr;
                    while (IS_LETTER(tmp_ptr) || IS_DIGIT(tmp_ptr))
                        tmp_ptr++;
                    char *symbolName = malloc(tmp_ptr - old_ptr + 1);
                    strncpy(symbolName, old_ptr, tmp_ptr - old_ptr);
                    add_reloc(symbolName, &line->y64bin);
                }
                line->y64bin.addr = vmaddr;

                if (strcmp(inst->name, ".byte") == 0) {
                    line->y64bin.codes[0] = value;
                    line->y64bin.bytes = 1;
                    vmaddr += 1;
                }

                else if (strcmp(inst->name, ".word") == 0) {
                    line->y64bin.codes[0] = value & 0xff;
                    line->y64bin.codes[1] = (value >> 8) & 0xff;
                    line->y64bin.bytes = 2;
                    vmaddr += 2;
                }

                else if (strcmp(inst->name, ".long") == 0) {
                    for (int i = 0; i < 4; ++i) {
                        line->y64bin.codes[i] = (value >> (i * 8)) & 0xFF;
                    }
                    line->y64bin.bytes = 4;
                    vmaddr += 4;
                }

                else if (strcmp(inst->name, ".quad") == 0) {
                    for (int i = 0; i < 8; ++i) {
                        line->y64bin.codes[i] = (value >> (i * 8)) & 0xFF;
                    }
                    line->y64bin.bytes = 8;
                    vmaddr += 8;
                }

                line->type = TYPE_INS;
                return line->type;

            } else if (inst->code == HPACK(I_DIRECTIVE, D_POS)) {
                if (parse_data(&tmp_ptr, &name, &value) == PARSE_DIGIT) {
                    vmaddr = value;
                    line->type = TYPE_INS;
                    line->y64bin.addr = vmaddr;
                    return line->type;
                }
            } else if (inst->code == HPACK(I_DIRECTIVE, D_ALIGN)) {
                if (parse_data(&tmp_ptr, &name, &value) == PARSE_DIGIT) {
                    vmaddr = ((vmaddr / value) + ((vmaddr % value) != 0)) * value;
                    line->type = TYPE_INS;
                    line->y64bin.addr = vmaddr;
                    return line->type;
                }
            }
            break;
        default:
            break;
        }
    }

    /* set type and y64bin */

    /* update vmaddr */

    /* parse the rest of instruction according to the itype */
    return line->type;
}

/*
 * assemble: assemble an y64 file (e.g., 'asum.ys')
 * args
 *     in: point to input file (an y64 assembly file)
 *
 * return
 *     0: success, assmble the y64 file to a list of line_t
 *     -1: error, try to print err information (e.g., instr type and line number)
 */
int assemble(FILE *in)
{
    static char asm_buf[MAX_INSLEN]; /* the current line of asm code */
    line_t *line;
    int slen;
    char *y64asm;

    /* read y64 code line-by-line, and parse them to generate raw y64 binary code list */
    while (fgets(asm_buf, MAX_INSLEN, in) != NULL) {
        slen = strlen(asm_buf);
        while ((asm_buf[slen - 1] == '\n') || (asm_buf[slen - 1] == '\r')) {
            asm_buf[--slen] = '\0'; /* replace terminator */
        }

        /* store y64 assembly code */
        y64asm = (char *)malloc(sizeof(char) * (slen + 1)); // free in finit
        strcpy(y64asm, asm_buf);

        line = (line_t *)malloc(sizeof(line_t)); // free in finit
        memset(line, '\0', sizeof(line_t));

        line->type = TYPE_COMM;
        line->y64asm = y64asm;
        line->next = NULL;

        line_tail->next = line;
        line_tail = line;
        lineno++;

        if (parse_line(line) == TYPE_ERR) {
            return -1;
        }
    }

    lineno = -1;
    return 0;
}

/*
 * relocate: relocate the raw y64 binary code with symbol address
 *
 * return
 *     0: success
 *     -1: error, try to print err information (e.g., addr and symbol)
 */
int relocate(void)
{
    reloc_t *rtmp = NULL;

    rtmp = reltab->next;
    while (rtmp) {
        /* find symbol */
        int i = 1;
        int j = 1;
        /* relocate y64bin according itype */
        switch (HIGH(rtmp->y64bin->codes[0])) {
        case I_IRMOVQ:
            i = 2;
            j = 2;
        case I_JMP:
        case I_CALL:
            if (find_symbol(rtmp->name) == NULL) {
                err_print("Unknown symbol:'%s'", rtmp->name);
                return -1;
            }
            for (; i < rtmp->y64bin->bytes; ++i) {
                rtmp->y64bin->codes[i] = find_symbol(rtmp->name)->addr >> (8 * (i - j)) & 0xFF;
            }
            break;
        case I_DIRECTIVE:
        default:
            i = j = 0;
            if (LOW(rtmp->y64bin->codes[0]) == D_DATA) {
                if (find_symbol(rtmp->name) == NULL) {
                    err_print("Unknown symbol:'%s'", rtmp->name);
                    return -1;
                }
                for (; i < rtmp->y64bin->bytes; ++i) {
                    rtmp->y64bin->codes[i] = find_symbol(rtmp->name)->addr >> (8 * (i - j)) & 0xFF;
                }
            }
            break;
        }

        /* next */
        rtmp = rtmp->next;
    }
    return 0;
}

/*char tmp_str[512]; // 创建一个临时字符串变量
int des = 0;*/
/*
 * binfile: generate the y64 binary file
 * args
 *     out: point to output file (an y64 binary file)
 *
 * return
 *     0: success
 *     -1: error
 */
int binfile(FILE *out)
{
    /* prepare image with y64 binary code */
    line_t *tmp = line_head->next;
    int old_vmaddr = 0;
    while (tmp != NULL) {
        if (tmp->type == TYPE_INS && tmp->y64bin.bytes > 0) {
            /*for (int i = 0; i < tmp->y64bin.bytes; i++) {
                sprintf(tmp_str + i*2 + des, "%02x", tmp->y64bin.codes[i]); //
            将tmp->y64bin.codes中的内容转换为十六进制字符串，并存储在tmp_str中
            }
            des += tmp->y64bin.bytes * 2;*/
            // 将tmp->y64bin.codes中的内容写入到out中
            if (tmp->y64bin.addr - old_vmaddr > 0)
                for (int i = 0; i < tmp->y64bin.addr - old_vmaddr; i++)
                    fputc(0, out);
            if (fwrite(tmp->y64bin.codes, 1, tmp->y64bin.bytes, out) != tmp->y64bin.bytes) {
                err_print("Write %s failed", tmp->y64asm);
                return -1;
            }
            old_vmaddr = tmp->y64bin.addr + tmp->y64bin.bytes;
        }
        tmp = tmp->next;
    }

    return 0;
}

/* whether print the readable output to screen or not ? */
bool_t screen = FALSE;

static void hexstuff(char *dest, int value, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        char c;
        int h = (value >> 4 * i) & 0xF;
        c = h < 10 ? h + '0' : h - 10 + 'a';
        dest[len - i - 1] = c;
    }
}

void print_line(line_t *line)
{
    char buf[64];

    /* line format: 0xHHH: cccccccccccc | <line> */
    if (line->type == TYPE_INS) {
        bin_t *y64bin = &line->y64bin;
        int i;

        strcpy(buf, "  0x000:                      | ");

        hexstuff(buf + 4, y64bin->addr, 3);
        if (y64bin->bytes > 0)
            for (i = 0; i < y64bin->bytes; i++)
                hexstuff(buf + 9 + 2 * i, y64bin->codes[i] & 0xFF, 2);
    } else {
        strcpy(buf, "                              | ");
    }

    printf("%s%s\n", buf, line->y64asm);
}

/*
 * print_screen: dump readable binary and assembly code to screen
 * (e.g., Figure 4.8 in ICS book)
 */
void print_screen(void)
{
    line_t *tmp = line_head->next;
    while (tmp != NULL) {
        print_line(tmp);
        tmp = tmp->next;
    }
}

/* init and finit */
void init(void)
{
    reltab = (reloc_t *)malloc(sizeof(reloc_t)); // free in finit
    memset(reltab, 0, sizeof(reloc_t));

    symtab = (symbol_t *)malloc(sizeof(symbol_t)); // free in finit
    memset(symtab, 0, sizeof(symbol_t));

    line_head = (line_t *)malloc(sizeof(line_t)); // free in finit
    memset(line_head, 0, sizeof(line_t));
    line_tail = line_head;
    lineno = 0;
}

void finit(void)
{
    reloc_t *rtmp = NULL;
    do {
        rtmp = reltab->next;
        if (reltab->name)
            free(reltab->name);
        free(reltab);
        reltab = rtmp;
    } while (reltab);

    symbol_t *stmp = NULL;
    do {
        stmp = symtab->next;
        if (symtab->name)
            free(symtab->name);
        free(symtab);
        symtab = stmp;
    } while (symtab);

    line_t *ltmp = NULL;
    do {
        ltmp = line_head->next;
        if (line_head->y64asm)
            free(line_head->y64asm);
        free(line_head);
        line_head = ltmp;
    } while (line_head);
}

static void usage(char *pname)
{
    printf("Usage: %s [-v] file.ys\n", pname);
    printf("   -v print the readable output to screen\n");
    exit(0);
}

int main(int argc, char *argv[])
{
    int rootlen;
    char infname[512];
    char outfname[512];
    int nextarg = 1;
    FILE *in = NULL, *out = NULL;

    if (argv[nextarg][0] == '-') {
        char flag = argv[nextarg][1];
        switch (flag) {
        case 'v':
            screen = TRUE;
            nextarg++;
            break;
        default:
            usage(argv[0]);
        }
    }

    /* parse input file name */
    rootlen = strlen(argv[nextarg]) - 3;
    /* only support the .ys file */
    if (strcmp(argv[nextarg] + rootlen, ".ys"))
        usage(argv[0]);

    if (rootlen > 500) {
        err_print("File name too long");
        exit(1);
    }

    /* init */
    init();

    /* assemble .ys file */
    strncpy(infname, argv[nextarg], rootlen);
    strcpy(infname + rootlen, ".ys");
    in = fopen(infname, "r");
    if (!in) {
        err_print("Can't open input file '%s'", infname);
        exit(1);
    }

    if (assemble(in) < 0) {
        err_print("Assemble y64 code error");
        fclose(in);
        exit(1);
    }

    fclose(in);

    /* relocate binary code */
    if (relocate() < 0) {
        err_print("Relocate binary code error");
        exit(1);
    }

    /* generate .bin file */
    strncpy(outfname, argv[nextarg], rootlen);
    strcpy(outfname + rootlen, ".bin");
    out = fopen(outfname, "wb");
    if (!out) {
        err_print("Can't open output file '%s'", outfname);
        exit(1);
    }

    if (binfile(out) < 0) {
        err_print("Generate binary file error");
        fclose(out);
        exit(1);
    }
    fclose(out);

    /* print to screen (.yo file) */
    if (screen)
        print_screen();

    /* finit */
    finit();
    return 0;
}
