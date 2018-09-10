/*
 scte35_types.h - Data structures for SCTE-35 splice_info_section and dependent types 
 Date: 2018-04-08
 Created by: Oguzhan KATLI
*/
#ifndef _SCTE35_TYPES_H_
#define _SCTE35_TYPES_H_
#include <stdint.h>

#define FAIL  -1

typedef struct {
  uint8_t time_specified_flag : 1;
  union {
    // if time_specified_flag == 1
    struct {
      uint8_t time_specified_flag_reserved : 6;
      uint64_t pts_time : 33;
    };
    // else
    uint8_t reserved : 7;
  };
} splice_time_t;

typedef struct {
  uint8_t auto_return : 1;
  uint8_t reserved    : 6;
  uint64_t duration   : 33;
} break_duration_t;

typedef struct {
  uint32_t splice_event_id;
  uint8_t splice_event_cancel_indicator : 1;
  uint8_t reserved : 7;
  uint8_t out_of_network_indicator : 1;
  uint8_t program_splice_flag : 1;
  uint8_t duration_flag : 1;
  uint8_t splice_immediate_flag : 1;
  uint8_t splice_event_reserved : 4;
  // if program_splice_flag == 1 && splice_immediate_flag == 0
  splice_time_t splice_time;
  uint8_t component_count;
  struct {
    uint8_t component_tag;
    splice_time_t splice_time;
  } component_t[255]; // up to component count
  // if duration_flag == 1
  break_duration_t break_duration;
  uint16_t unique_program_id;
  uint8_t avail_num;
  uint8_t avails_expected;
} splice_insert_t;

// base splice descriptor struct that every descriptor shares
typedef struct {
  uint8_t  splice_descriptor_tag;
  uint8_t  descriptor_length;
  uint32_t identifier;
} splice_descriptor_t;

typedef struct {
  uint8_t component_tag;
  uint8_t reserved : 7;
  uint64_t pts_ofset;
} component_tag_t;

typedef struct {
  uint8_t type;
  uint8_t length;
  long bit_start_index;
} segmentation_upid_t;

typedef struct {
  splice_descriptor_t descriptor;
  uint32_t segmentation_event_id;
  uint8_t segmentation_event_cancel_indicator : 1;
  uint8_t reserved : 7;
  // if segmentation_event_cancel_indicator == 0
  uint8_t program_segmentation_flag    : 1;
  uint8_t segmentation_duration_flag   : 1;
  uint8_t delivery_not_restricted_flag : 1;
  union {
    // if delivery_not_restricted_flag == 0
    struct {
      uint8_t web_delivery_allowed_flag    : 1;
      uint8_t no_regional_blackout_flag    : 1;
      uint8_t archive_allowed_flag         : 1;
      uint8_t device_restrictions          : 2;
    };
    // else
    uint8_t reserved_flags : 5;
  };
  // if segmentation_duration_flag == 0
  uint8_t component_count;
  component_tag_t component_tags[255]; // up to component_count
  // if segmentation_duration_flag == 1
  uint64_t segmentation_duration : 40;
  uint8_t segmentation_upid_type;
  uint8_t segmentation_upid_length;
  uint8_t segmentation_upid_data[256];
  //void *segmentation_upid_ptr;  // up to segmentation_upid_length
  //void(*segmentation_upid_printer)(void *);
  uint8_t segmentation_type_id;
  uint8_t segment_num;
  uint8_t segments_expected;
  // if segmentation_type_id == 0x34 || 0x36
  uint8_t sub_segment_num;
  uint8_t sub_segments_expected;
} segmentation_descriptor_t;

typedef struct {
  uint8_t  table_id;
  uint16_t section_syntax_indicator : 1;
  uint16_t private_indicator : 1;
  uint16_t reserved : 2;
  uint16_t section_length : 12;
  uint8_t  protocol_version;
  uint8_t  encrypted_packet     : 1;
  uint8_t  encryption_algorithm : 6;
  uint64_t pts_adjustment       : 33;
  uint8_t  cw_index;
  uint16_t tier : 12;
  uint16_t splice_command_length : 12;
  uint8_t  splice_command_type;
  void *splice_command_ptr;   // will be casted the type of splice_command_type
  void(*splice_command_printer)(void *);
  uint16_t descriptor_loop_length;
  // parsed only segmentation_descriptor for assignment
  //number of allocated segmentation_descriptor;
  uint16_t segmentation_descriptor_count;
  segmentation_descriptor_t *segmentation_descriptor; // will be descriptor_loop_length
  uint8_t alignment_stuffing_bytes_length;
  uint8_t *alignment_stuffing; // size changes by using encryption algorithm
  int32_t e_crc_32;
  int32_t crc_32;
} splice_info_section_t;

// create list for parsed splice_info_section's
// typedef struct splice_info_section_struct_list_t splice_info_section_struct_list;
typedef struct splice_info_section_struct_list {
  splice_info_section_t                  *splice_info_section;
  struct splice_info_section_struct_list *next;
} splice_info_section_struct_list_t;


// returns number of used bytes for parsing one splice_info_section
// if fails to parse, than returns -1 for indicating error
int ParseSCTE35FromByteArray(
  FILE *inputstream, 
  splice_info_section_struct_list_t **list);

void PrintParsedSCTE35ToFile(
  FILE *data, 
  splice_info_section_struct_list_t *parsed_list);

#endif