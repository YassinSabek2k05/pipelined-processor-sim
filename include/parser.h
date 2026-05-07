#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>
#include <stdint.h>

/*
Package 2 instruction encoding:
R-Type: [ opcode(4) | r1(5) | r2(5) | r3(5) | shamt(13) ]
I-Type: [ opcode(4) | r1(5) | r2(5) | immediate(18) ]
J-Type: [ opcode(4) | address(28) ]

Opcode mapping:
  ADD   = 0
  SUB   = 1
  MUL   = 2
  MOVI  = 3
  JEQ   = 4
  AND   = 5
  ORI   = 6
  JMP   = 7
  LSL   = 8
  LSR   = 9
  MOVR  = 10
  MOVM  = 11

Signed immediates are 18-bit two's complement values.
Shift amount fields are 13 bits and must be in the range 0..8191.
*/

#define OPCODE_ADD   0
#define OPCODE_SUB   1
#define OPCODE_MUL   2
#define OPCODE_MOVI  3
#define OPCODE_JEQ   4
#define OPCODE_AND   5
#define OPCODE_ORI   6
#define OPCODE_JMP   7
#define OPCODE_LSL   8
#define OPCODE_LSR   9
#define OPCODE_MOVR  10
#define OPCODE_MOVM  11

int parse_and_encode_instruction(const char *line,
                                 uint32_t *encoded,
                                 char *error_message,
                                 size_t error_message_size);

#endif // PARSER_H
