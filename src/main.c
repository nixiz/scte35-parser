#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "scte35_types.h"

// implemented in 'scte35_parser.c'
extern void ReleaseSpliceInfoSectionList(splice_info_section_struct_list_t *splice_info_section_list);

int main(int argc, const char *argv[]) {
  int exit_code = 0; // success
  FILE *finput = NULL;
  FILE *foutput = NULL;
  splice_info_section_struct_list_t *splice_info_section_list = NULL;
  // parse passed arguments
  if (argc != 3) {
    exit_code = 1;
    printf("\ninvalid usage of %s", argv[0]);
    printf("\nsample usage: \"%s scte35.dump scte35.txt\"\n", argv[0]);
    goto exit_program;
  }

  finput = fopen(argv[1], "rb"); // try open given input file read only
  if (finput == NULL || ferror(finput)) {
    exit_code = 2;
    printf("\ncan not open given input file \"%s\"\n", argv[1]);
    goto exit_program;
  }

  foutput = fopen(argv[2], "w"); // try open given output file
  if (foutput == NULL || ferror(finput)) {
    exit_code = 2;
    printf("\ncan not open given output file \"%s\"\n", argv[2]);
    goto exit_program;
  }

  if (ParseSCTE35FromByteArray(finput, &splice_info_section_list) == FAIL) {
    perror("\nCould not parse all input scte35 data into memory!");
    exit_code = 3;
    goto exit_program;
  }

  PrintParsedSCTE35ToFile(foutput, splice_info_section_list);
  // want to print console than activate this line below
  PrintParsedSCTE35ToFile(NULL, splice_info_section_list);
exit_program: // The only exit point of the program should be at the end of main().
              // release resources
  if (finput != NULL) {
    fclose(finput);
    finput = NULL;
  }

  if (foutput != NULL) {
    fclose(foutput);
    foutput = NULL;
  }
  ReleaseSpliceInfoSectionList(splice_info_section_list);
  return exit_code;
}