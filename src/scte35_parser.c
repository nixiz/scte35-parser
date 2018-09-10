#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "scte35_types.h"

extern void splice_insert_printer(splice_insert_t *ptr);
extern void splice_time_printer(splice_time_t *ptr);

extern void ReleaseSpliceInfoSection(splice_info_section_t *splice_info_section) {
  if (splice_info_section == NULL) return;

  if (splice_info_section->alignment_stuffing != NULL)
    free(splice_info_section->alignment_stuffing);

  if (splice_info_section->segmentation_descriptor != NULL)
    free(splice_info_section->segmentation_descriptor);

  if (splice_info_section->splice_command_ptr != NULL)
    free(splice_info_section->splice_command_ptr);
  free(splice_info_section);
}

extern void ReleaseSpliceInfoSectionList(splice_info_section_struct_list_t *splice_info_section_list) {
  if (splice_info_section_list == NULL) return;

  if (splice_info_section_list->splice_info_section != NULL) {
    ReleaseSpliceInfoSection(splice_info_section_list->splice_info_section);
  }

  if (splice_info_section_list->next != NULL) {
    ReleaseSpliceInfoSectionList(splice_info_section_list->next);
  }
  free(splice_info_section_list);
}

// return size of filebuffer_bits array
unsigned long CreateAndParseBitArrayForFile(
  FILE *inputstream,
  unsigned char **filebuffer_bits) {
  // get total amount of bytes in dump file
  fseek(inputstream, 0, SEEK_END);
  unsigned long filesize = ftell(inputstream);
  *filebuffer_bits = (unsigned char *)malloc(filesize * 8); // 8 bits for every byte
                                                            // set working pointer
  unsigned char *filebuffer_bit_arr = *filebuffer_bits;
  if (*filebuffer_bits == NULL) {
    perror("Error taking memory for given file!\nFile size too long, please try with smaller dmup file..");
    return FAIL;
  }
  rewind(inputstream);

  // parse bit array
  // TODO(oguzhank): optimize this parsing
  for (unsigned long i = 0; i < filesize; i++)
  {
    int r = 128;
    int val = fgetc(inputstream);
    for (int j = 0; j < 8; j++) {
      int bVal = 0;
      if (r & val) {
        bVal = 1;
      }
      filebuffer_bit_arr[j + (i * 8)] = bVal;
      r = r >> 1;
    }
  } // for.

    // return size of allocatet bit array
  return filesize * 8;
}

// after every read operation cntx will be updated for indexing bit array
uint64_t ReadBitsFromArray(unsigned char *filebuffer_bits,
                           int bitsize,
                           unsigned long * cntx) {
  int hSigNum = 0;
  if (bitsize > 32) {
    for (int i = 0; i < bitsize - 32; i++) {
      hSigNum = hSigNum << 1;
      int val = filebuffer_bits[(*cntx)++];
      if (val) {
        hSigNum++;
      }
    } // for.
    hSigNum = hSigNum * pow(2, 32);
    bitsize = 32;
  }
  int res = 0;
  for (int i = 0; i < bitsize; i++) {
    res = res << 1;
    int val = filebuffer_bits[(*cntx)++];
    if (val) {
      res++;
    }
  }
  if (bitsize >= 32) {
    res = res >> 0;
  }
  return res + hSigNum;
}

int ParseSCTE35FromByteArray(
  FILE *inputstream,
  splice_info_section_struct_list_t **splice_info_section_struct_list) {

  //checkedfreadresult_t result = success;
  int parseDone = 0;
  // unsigned int total_read_bytes = 0;
  // bit array for dump file
  unsigned long filebuffer_bits_size = 0;
  unsigned long filebuffer_bits_index = 0;
  unsigned char *filebuffer_bits;

  filebuffer_bits_size = CreateAndParseBitArrayForFile(inputstream, &filebuffer_bits);
  if (filebuffer_bits_size == FAIL) {
    goto function_exit;
  }

  // create parsed scte35 splice info list
  *splice_info_section_struct_list = (splice_info_section_struct_list_t *)malloc(sizeof(splice_info_section_struct_list_t));
  // set working pointer
  splice_info_section_struct_list_t *list = *splice_info_section_struct_list;
  list->next = NULL;

  unsigned long remaining_bits = filebuffer_bits_size - filebuffer_bits_index;
  unsigned long read_bits = 0;
  // while there are remaining bits to parse
  while (remaining_bits > 0) {
    // resetting parse ok flag
    parseDone = 0;

    list->splice_info_section = (splice_info_section_t *)calloc(sizeof(splice_info_section_t), sizeof(char));
    // set working pointer
    splice_info_section_t *section = list->splice_info_section;
    
    section->table_id = ReadBitsFromArray(filebuffer_bits, 8, &filebuffer_bits_index);
    section->section_syntax_indicator = ReadBitsFromArray(filebuffer_bits, 1, &filebuffer_bits_index);
    section->private_indicator = ReadBitsFromArray(filebuffer_bits, 1, &filebuffer_bits_index);
    section->reserved = ReadBitsFromArray(filebuffer_bits, 2, &filebuffer_bits_index);
    section->section_length = ReadBitsFromArray(filebuffer_bits, 12, &filebuffer_bits_index);
    section->protocol_version = ReadBitsFromArray(filebuffer_bits, 8, &filebuffer_bits_index);
    section->encrypted_packet = ReadBitsFromArray(filebuffer_bits, 1, &filebuffer_bits_index);
    section->encryption_algorithm = ReadBitsFromArray(filebuffer_bits, 6, &filebuffer_bits_index);
    section->pts_adjustment = ReadBitsFromArray(filebuffer_bits, 33, &filebuffer_bits_index);
    section->cw_index = ReadBitsFromArray(filebuffer_bits, 8, &filebuffer_bits_index);
    section->tier = ReadBitsFromArray(filebuffer_bits, 12, &filebuffer_bits_index);
    section->splice_command_length = ReadBitsFromArray(filebuffer_bits, 12, &filebuffer_bits_index);
    section->splice_command_type = ReadBitsFromArray(filebuffer_bits, 8, &filebuffer_bits_index);

    switch (section->splice_command_type)
    {
    case 5: // splice_insert();
    {
      section->splice_command_ptr = (splice_insert_t *)calloc(sizeof(splice_insert_t), sizeof(char));
      section->splice_command_printer = (void (*)(void *))splice_insert_printer;
      // set working pointer
      splice_insert_t *splice_insert = section->splice_command_ptr;
      splice_insert->splice_event_id = ReadBitsFromArray(filebuffer_bits, 32, &filebuffer_bits_index);
      splice_insert->splice_event_cancel_indicator = ReadBitsFromArray(filebuffer_bits, 1, &filebuffer_bits_index);
      splice_insert->reserved = ReadBitsFromArray(filebuffer_bits, 7, &filebuffer_bits_index);
      if (splice_insert->splice_event_cancel_indicator == 0)
      {
        splice_insert->out_of_network_indicator = ReadBitsFromArray(filebuffer_bits, 1, &filebuffer_bits_index);
        splice_insert->program_splice_flag = ReadBitsFromArray(filebuffer_bits, 1, &filebuffer_bits_index);
        splice_insert->duration_flag = ReadBitsFromArray(filebuffer_bits, 1, &filebuffer_bits_index);
        splice_insert->splice_immediate_flag = ReadBitsFromArray(filebuffer_bits, 1, &filebuffer_bits_index);
        splice_insert->splice_event_reserved = ReadBitsFromArray(filebuffer_bits, 4, &filebuffer_bits_index);
        if (splice_insert->program_splice_flag == 1 &&
            splice_insert->splice_immediate_flag == 0)
        {
          splice_insert->splice_time.time_specified_flag = ReadBitsFromArray(filebuffer_bits, 1, &filebuffer_bits_index);
          if (splice_insert->splice_time.time_specified_flag)
          {
            splice_insert->splice_time.time_specified_flag_reserved = ReadBitsFromArray(filebuffer_bits, 6, &filebuffer_bits_index);
            splice_insert->splice_time.pts_time = ReadBitsFromArray(filebuffer_bits, 33, &filebuffer_bits_index);
          }
          else
          {
            splice_insert->splice_time.reserved = ReadBitsFromArray(filebuffer_bits, 7, &filebuffer_bits_index);
          }
        } // if

        if (splice_insert->program_splice_flag == 0)
        {
          splice_insert->component_count = ReadBitsFromArray(filebuffer_bits, 8, &filebuffer_bits_index);
          for (int i = 0; i < splice_insert->component_count; i++)
          {
            splice_insert->component_t[i].component_tag = ReadBitsFromArray(filebuffer_bits, 8, &filebuffer_bits_index);
            if (splice_insert->splice_immediate_flag == 0)
            {
              splice_insert->component_t[i].splice_time.time_specified_flag = ReadBitsFromArray(filebuffer_bits, 1, &filebuffer_bits_index);
              if (splice_insert->component_t[i].splice_time.time_specified_flag)
              {
                splice_insert->component_t[i].splice_time.time_specified_flag_reserved = ReadBitsFromArray(filebuffer_bits, 6, &filebuffer_bits_index);
                splice_insert->component_t[i].splice_time.pts_time = ReadBitsFromArray(filebuffer_bits, 33, &filebuffer_bits_index);
              }
              else
              {
                splice_insert->component_t[i].splice_time.reserved = ReadBitsFromArray(filebuffer_bits, 7, &filebuffer_bits_index);
              }
            }
          }
        } // if splice_insert->program_splice_flag == 0

        if (splice_insert->duration_flag == 1)
        {
          splice_insert->break_duration.auto_return = ReadBitsFromArray(filebuffer_bits, 1, &filebuffer_bits_index);
          splice_insert->break_duration.reserved = ReadBitsFromArray(filebuffer_bits, 6, &filebuffer_bits_index);
          splice_insert->break_duration.duration = ReadBitsFromArray(filebuffer_bits, 33, &filebuffer_bits_index);
        } // if splice_insert->duration_flag == 1
        splice_insert->unique_program_id = ReadBitsFromArray(filebuffer_bits, 16, &filebuffer_bits_index);
        splice_insert->avail_num = ReadBitsFromArray(filebuffer_bits, 8, &filebuffer_bits_index);
        splice_insert->avails_expected = ReadBitsFromArray(filebuffer_bits, 8, &filebuffer_bits_index);
      } // if splice_event_cancel_indicator == 0
    }
    break;
    case 6: // time_signal();
    {
      section->splice_command_ptr = (splice_time_t *)calloc(sizeof(splice_time_t), sizeof(char));
      section->splice_command_printer = (void (*)(void *))splice_time_printer;
      // set working pointer
      splice_time_t *splice_time = section->splice_command_ptr;
      splice_time->time_specified_flag = ReadBitsFromArray(filebuffer_bits, 1, &filebuffer_bits_index);
      if (splice_time->time_specified_flag)
      {
        splice_time->time_specified_flag_reserved = ReadBitsFromArray(filebuffer_bits, 6, &filebuffer_bits_index);
        splice_time->pts_time = ReadBitsFromArray(filebuffer_bits, 33, &filebuffer_bits_index);
      }
      else
      {
        splice_time->reserved = ReadBitsFromArray(filebuffer_bits, 7, &filebuffer_bits_index);
      }
    }
    break;
    default:
      section->splice_command_ptr = NULL;
      perror("\nUnsupported splice command type! this application only accepts splice_insert and time_signal..");
      goto function_exit;
    } // switch

    // indicates number of bytes following loop
    section->descriptor_loop_length = ReadBitsFromArray(filebuffer_bits, 16, &filebuffer_bits_index);

    // create segmentation_descriptor array with length descriptor_loop_length. assuming munber of descriptors will not exceed the total length in bytes
    section->segmentation_descriptor = (segmentation_descriptor_t *)malloc(section->descriptor_loop_length * sizeof(segmentation_descriptor_t));
    if (section->segmentation_descriptor == NULL) {
      perror("\nCan not allocate space from memory! Insufficent memory size..");
      goto function_exit;
    }

    // after this line, sum of all read bytes should be equal to descriptor_loop_length
    int descriptor_loop_start_bits = filebuffer_bits_index;
    section->segmentation_descriptor_count = 0;
    
    // int read_bytes = (filebuffer_bits_index - descriptor_loop_start_bits) / 8;
    // while shoul read bytes defined in descriptor_loop_length 
    while (((filebuffer_bits_index - descriptor_loop_start_bits) / 8) < section->descriptor_loop_length)
    {
      splice_descriptor_t splice_descriptor;
      splice_descriptor.splice_descriptor_tag = ReadBitsFromArray(filebuffer_bits, 8, &filebuffer_bits_index);
      splice_descriptor.descriptor_length = ReadBitsFromArray(filebuffer_bits, 8, &filebuffer_bits_index);
      splice_descriptor.identifier = ReadBitsFromArray(filebuffer_bits, 32, &filebuffer_bits_index);
      if (splice_descriptor.splice_descriptor_tag != 0x02) // segmentation_descriptor
      {
        /*
        printf("\nWarning! This application only parse segmentation descriptorr! Invalid splice descriptor tag, application will continue without parsing it components..");
        for (int i = 0; i < splice_descriptor.descriptor_length; i++)
        {
          // private bytes
          ReadBitsFromArray(filebuffer_bits, 8, &filebuffer_bits_index);
        }
        continue;
        */
        perror("\nError! This application only parse segmentation descriptorr! Invalid splice descriptor tag..");
        goto function_exit;
      }
      
      section->segmentation_descriptor[section->segmentation_descriptor_count].descriptor = splice_descriptor;
      section->segmentation_descriptor[section->segmentation_descriptor_count].segmentation_event_id = ReadBitsFromArray(filebuffer_bits, 32, &filebuffer_bits_index);
      section->segmentation_descriptor[section->segmentation_descriptor_count].segmentation_event_cancel_indicator = ReadBitsFromArray(filebuffer_bits, 1, &filebuffer_bits_index);
      section->segmentation_descriptor[section->segmentation_descriptor_count].reserved = ReadBitsFromArray(filebuffer_bits, 7, &filebuffer_bits_index);
      if (section->segmentation_descriptor[section->segmentation_descriptor_count].segmentation_event_cancel_indicator == 0)
      {
        section->segmentation_descriptor[section->segmentation_descriptor_count].program_segmentation_flag = ReadBitsFromArray(filebuffer_bits, 1, &filebuffer_bits_index);
        section->segmentation_descriptor[section->segmentation_descriptor_count].segmentation_duration_flag = ReadBitsFromArray(filebuffer_bits, 1, &filebuffer_bits_index);
        section->segmentation_descriptor[section->segmentation_descriptor_count].delivery_not_restricted_flag = ReadBitsFromArray(filebuffer_bits, 1, &filebuffer_bits_index);
        if (section->segmentation_descriptor[section->segmentation_descriptor_count].delivery_not_restricted_flag == 0)
        {
          section->segmentation_descriptor[section->segmentation_descriptor_count].web_delivery_allowed_flag = ReadBitsFromArray(filebuffer_bits, 1, &filebuffer_bits_index);
          section->segmentation_descriptor[section->segmentation_descriptor_count].no_regional_blackout_flag = ReadBitsFromArray(filebuffer_bits, 1, &filebuffer_bits_index);
          section->segmentation_descriptor[section->segmentation_descriptor_count].archive_allowed_flag = ReadBitsFromArray(filebuffer_bits, 1, &filebuffer_bits_index);
          section->segmentation_descriptor[section->segmentation_descriptor_count].device_restrictions = ReadBitsFromArray(filebuffer_bits, 2, &filebuffer_bits_index);
        }
        else
        {
          section->segmentation_descriptor[section->segmentation_descriptor_count].reserved_flags = ReadBitsFromArray(filebuffer_bits, 5, &filebuffer_bits_index);
        }

        if (section->segmentation_descriptor[section->segmentation_descriptor_count].program_segmentation_flag == 0)
        {
          section->segmentation_descriptor[section->segmentation_descriptor_count].component_count = ReadBitsFromArray(filebuffer_bits, 8, &filebuffer_bits_index);
          for (int i = 0; i < section->segmentation_descriptor[section->segmentation_descriptor_count].component_count; i++)
          {
            section->segmentation_descriptor[section->segmentation_descriptor_count].component_tags[section->segmentation_descriptor_count].component_tag = ReadBitsFromArray(filebuffer_bits, 8, &filebuffer_bits_index);
            section->segmentation_descriptor[section->segmentation_descriptor_count].component_tags[section->segmentation_descriptor_count].reserved = ReadBitsFromArray(filebuffer_bits, 7, &filebuffer_bits_index);
            section->segmentation_descriptor[section->segmentation_descriptor_count].component_tags[section->segmentation_descriptor_count].pts_ofset = ReadBitsFromArray(filebuffer_bits, 33, &filebuffer_bits_index);
          }
        }
        if (section->segmentation_descriptor[section->segmentation_descriptor_count].segmentation_duration_flag == 1)
        {
          section->segmentation_descriptor[section->segmentation_descriptor_count].segmentation_duration = ReadBitsFromArray(filebuffer_bits, 40, &filebuffer_bits_index);
        }
        section->segmentation_descriptor[section->segmentation_descriptor_count].segmentation_upid_type = ReadBitsFromArray(filebuffer_bits, 8, &filebuffer_bits_index);
        section->segmentation_descriptor[section->segmentation_descriptor_count].segmentation_upid_length = ReadBitsFromArray(filebuffer_bits, 8, &filebuffer_bits_index);
        for (int ii = 0; ii < section->segmentation_descriptor[section->segmentation_descriptor_count].segmentation_upid_length; ii++) {
          section->segmentation_descriptor[section->segmentation_descriptor_count].segmentation_upid_data[ii] = ReadBitsFromArray(filebuffer_bits, 8, &filebuffer_bits_index);
        }
        /*
        // save segmentation upid
        section->segmentation_descriptor[section->segmentation_descriptor_count].segmentation_upid.bit_start_index = filebuffer_bits_index;
        section->segmentation_descriptor[section->segmentation_descriptor_count].segmentation_upid.type = section->segmentation_descriptor[section->segmentation_descriptor_count].segmentation_upid_type;
        section->segmentation_descriptor[section->segmentation_descriptor_count].segmentation_upid.length = section->segmentation_descriptor[section->segmentation_descriptor_count].segmentation_upid_length;
        // move read pointer as segmentation upid length
        filebuffer_bits_index = filebuffer_bits_index + (section->segmentation_descriptor[section->segmentation_descriptor_count].segmentation_upid.length * 8);
        */
        section->segmentation_descriptor[section->segmentation_descriptor_count].segmentation_type_id = ReadBitsFromArray(filebuffer_bits, 8, &filebuffer_bits_index);
        section->segmentation_descriptor[section->segmentation_descriptor_count].segment_num = ReadBitsFromArray(filebuffer_bits, 8, &filebuffer_bits_index);
        section->segmentation_descriptor[section->segmentation_descriptor_count].segments_expected = ReadBitsFromArray(filebuffer_bits, 8, &filebuffer_bits_index);
        if (section->segmentation_descriptor[section->segmentation_descriptor_count].segmentation_type_id == 0x34 || 
            section->segmentation_descriptor[section->segmentation_descriptor_count].segmentation_type_id == 0x36)
        {
          section->segmentation_descriptor[section->segmentation_descriptor_count].sub_segment_num       = ReadBitsFromArray(filebuffer_bits, 8, &filebuffer_bits_index);
          section->segmentation_descriptor[section->segmentation_descriptor_count].sub_segments_expected = ReadBitsFromArray(filebuffer_bits, 8, &filebuffer_bits_index);
        }
      } // if segmentation_event_cancel_indicator == 0

      section->segmentation_descriptor_count = section->segmentation_descriptor_count + 1;
    } // while descriptor_loop_length

    // be sure we read aligned bits, in here should no remaining bits to complete byte
    if (filebuffer_bits_index % 8 != 0)
    {
      perror("\nparsing error: read/parsed bits doesn't align byte");
      goto function_exit;
    }

    if (section->encrypted_packet)
    {
      long total_read_bytes = (filebuffer_bits_index - read_bits) / 8;
      int remaining_section_bytes = section->section_length + 3 - total_read_bytes;

      section->alignment_stuffing_bytes_length = remaining_section_bytes - 4 - 4; // 4 byte e_crc, 4 byte crc
      section->alignment_stuffing = (uint8_t *)malloc(section->alignment_stuffing_bytes_length);
      if (section->alignment_stuffing == NULL) {
        perror("\nCan not allocate space from memory! Insufficent memory size..");
        goto function_exit;
      }

      for (int i = 0; i < section->alignment_stuffing_bytes_length; i++)
      {
        section->alignment_stuffing[i] = ReadBitsFromArray(filebuffer_bits, 8, &filebuffer_bits_index);
      }
      section->e_crc_32 = ReadBitsFromArray(filebuffer_bits, 32, &filebuffer_bits_index);
    }
    section->crc_32 = ReadBitsFromArray(filebuffer_bits, 32, &filebuffer_bits_index);

    // parse is done if every byte read from stream correctly
    parseDone = (section->section_length + 3 - ((filebuffer_bits_index - read_bits) / 8)) == 0 ? 1 : 0;
    if (parseDone != 1)
    {
      perror("\ninvalid parse!");
      goto function_exit;
    }
    
    // update remaining bits
    read_bits = filebuffer_bits_index;
    remaining_bits = filebuffer_bits_size - filebuffer_bits_index;
    if (remaining_bits)
    {
      // create new list item and set list ptr to next 
      list->next = (splice_info_section_struct_list_t *)malloc(sizeof(splice_info_section_struct_list_t));
      list = list->next;
      if (list == NULL) {
        perror("\nCan not allocate space from memory! Insufficent memory size..");
        goto function_exit;
      }
      list->next = NULL;
    }
  } // while remaining bits in bit stream

    // if end of file reached and couldnt parse a valid splice_info_section
function_exit:
  return parseDone == 1 ? 0 : FAIL; 
}
