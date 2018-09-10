#include <stdio.h>
#include "scte35_types.h"

enum IndentLevel {
  LVL_0 = 0,
  LVL_1 = 2,
  LVL_2 = 4,
  LVL_3 = 6,
  LVL_4 = 8,
  LVL_5 = 10,
};

// global output pointer
FILE *gb_output = NULL;

void print_with_indent(FILE *output, int indent)
{
  // fprintf(output, "\n");
  // while (indent-- > 0)
  // {
  //   fprintf(output, " ");
  // }
  switch ((enum IndentLevel)indent) 
  {    
    case LVL_1:
      fprintf(output, "\n  ");
    break;
    case LVL_2:
      fprintf(output, "\n    ");
    break;
    case LVL_3:
      fprintf(output, "\n      ");
    break;
    case LVL_4:
      fprintf(output, "\n        ");
    break;
    case LVL_5:
      fprintf(output, "\n          ");
    break;
    case LVL_0:
    default:
      fprintf(output, "\n");
    break;
  }
}

#define indent_print(lvl, frmt,...)  \
  print_with_indent(gb_output, lvl);    \
  fprintf(gb_output, frmt, ##__VA_ARGS__)


extern void splice_insert_printer(splice_insert_t *ptr) {
  indent_print(LVL_1, "splice_insert() {");
  indent_print(LVL_2, "splice_event_id : %d", ptr->splice_event_id);
  indent_print(LVL_2, "splice_event_cancel_indicator : %d", ptr->splice_event_cancel_indicator);
  indent_print(LVL_2, "reserved : %d", ptr->reserved);
  indent_print(LVL_2, "out_of_network_indicator : %d", ptr->out_of_network_indicator);
  indent_print(LVL_2, "program_splice_flag : %d", ptr->program_splice_flag);
  indent_print(LVL_2, "duration_flag : %d", ptr->duration_flag);
  indent_print(LVL_2, "splice_immediate_flag : %d", ptr->splice_immediate_flag);
  indent_print(LVL_2, "splice_event_reserved : %d", ptr->splice_event_reserved);
  if (ptr->program_splice_flag == 1 && ptr->splice_immediate_flag == 0)
  {
    indent_print(LVL_2, "splice_time() {");
    indent_print(LVL_3, "time_specified_flag : %d", ptr->splice_time.time_specified_flag);
    if (ptr->splice_time.time_specified_flag == 1)
    {
      indent_print(LVL_3, "reserved : %d", ptr->splice_time.time_specified_flag_reserved);
      indent_print(LVL_3, "pts_time : %d", ptr->splice_time.pts_time);
    }
    else
    {
      indent_print(LVL_3, "reserved : %d", ptr->splice_time.reserved);
    }
    indent_print(LVL_2, "}"); // splice_time()
  }
  if (ptr->program_splice_flag == 1)
  {
    indent_print(LVL_2, "component_count : %d", ptr->component_count);
    for (int i = 0; i < ptr->component_count; i++)
    {
      indent_print(LVL_2, "component_tag : %d", ptr->component_t[i].component_tag);
      if (ptr->splice_immediate_flag)
      {
        indent_print(LVL_2, "splice_time() {");
        indent_print(LVL_3, "time_specified_flag : %d", ptr->splice_time.time_specified_flag);
        if (ptr->splice_time.time_specified_flag == 1)
        {
          indent_print(LVL_3, "reserved : %d", ptr->splice_time.time_specified_flag_reserved);
          indent_print(LVL_3, "pts_time : %d", ptr->splice_time.pts_time);
        }
        else
        {
          indent_print(LVL_3, "reserved : %d", ptr->splice_time.reserved);
        }
        indent_print(LVL_2, "}"); // splice_time()
      }
    }
    if (ptr->duration_flag == 1)
    {
      indent_print(LVL_2, "break_duration() {");

      indent_print(LVL_3, "auto_return : %d", ptr->break_duration.auto_return);
      indent_print(LVL_3, "reserved : %d", ptr->break_duration.reserved);
      indent_print(LVL_3, "duration : %d", ptr->break_duration.duration);

      indent_print(LVL_2, "}"); // break_duration()
    }
    indent_print(LVL_2, "unique_program_id : %d", ptr->unique_program_id);
    indent_print(LVL_2, "avail_num : %d", ptr->avail_num);
    indent_print(LVL_2, "avails_expected : %d", ptr->avails_expected);
  }
  indent_print(LVL_1, "}");  // splice_insert() 
}

extern void splice_time_printer(splice_time_t *ptr) {
  indent_print(LVL_1, "time_signal() {");
  indent_print(LVL_2, "splice_time() {");
  indent_print(LVL_3, "time_specified_flag : %d", ptr->time_specified_flag);
  if (ptr->time_specified_flag == 1)
  {
    indent_print(LVL_3, "reserved : %d", ptr->time_specified_flag_reserved);
    indent_print(LVL_3, "pts_time : %d", ptr->pts_time);
  }
  else
  {
    indent_print(LVL_3, "reserved : %d", ptr->reserved);
  }
  indent_print(LVL_2, "}"); // splice_time()
  indent_print(LVL_1, "}"); // time_signal() 
}


void PrintParsedSCTE35ToFile(
  FILE *output, 
  splice_info_section_struct_list_t *parsed_list) {
  if (output == NULL) { output = stdout; }
  // set global output pointer for printing;
  gb_output = output;

  while (parsed_list != NULL)
  {
    splice_info_section_t *splice_info_section = parsed_list->splice_info_section;
    indent_print(LVL_0, "splice_info_section() {");
    
    indent_print(LVL_1, "table_id : %d", splice_info_section->table_id);
    indent_print(LVL_1, "section_syntax_indicator : %d", splice_info_section->section_syntax_indicator);
    indent_print(LVL_1, "private_indicator : %d", splice_info_section->private_indicator);
    indent_print(LVL_1, "reserved : %d", splice_info_section->reserved);
    indent_print(LVL_1, "section_length : %d", splice_info_section->section_length);
    indent_print(LVL_1, "protocol_version : %d", splice_info_section->protocol_version);
    indent_print(LVL_1, "encrypted_packet : %d", splice_info_section->encrypted_packet);
    indent_print(LVL_1, "encryption_algorithm : %d", splice_info_section->encryption_algorithm);
    indent_print(LVL_1, "pts_adjustment : %d", splice_info_section->pts_adjustment);
    indent_print(LVL_1, "cw_index : %d", splice_info_section->cw_index);
    indent_print(LVL_1, "tier : %d", splice_info_section->tier);
    indent_print(LVL_1, "splice_command_length : %d", splice_info_section->splice_command_length);
    indent_print(LVL_1, "splice_command_type : %d", splice_info_section->splice_command_type);
    
    // print splice command by type
    splice_info_section->splice_command_printer(splice_info_section->splice_command_ptr);

    indent_print(LVL_1, "descriptor_loop_length : %d", splice_info_section->descriptor_loop_length);
    indent_print(LVL_1, "splice_descriptor() {");
    
    for (int i = 0; i < splice_info_section->segmentation_descriptor_count; i++)
    {
      indent_print(LVL_2, "segmentation_descriptor() {");
      
      indent_print(LVL_3, "splice_descriptor_tag : %d", splice_info_section->segmentation_descriptor[i].descriptor.splice_descriptor_tag);
      indent_print(LVL_3, "descriptor_length : %d", splice_info_section->segmentation_descriptor[i].descriptor.descriptor_length);
      indent_print(LVL_3, "identifier : %d", splice_info_section->segmentation_descriptor[i].descriptor.identifier);
      indent_print(LVL_3, "segmentation_event_id : %d", splice_info_section->segmentation_descriptor[i].segmentation_event_id);
      indent_print(LVL_3, "segmentation_event_cancel_indicator : %d", splice_info_section->segmentation_descriptor[i].segmentation_event_cancel_indicator);
      indent_print(LVL_3, "reserved : %d", splice_info_section->segmentation_descriptor[i].reserved);
      if (splice_info_section->segmentation_descriptor[i].segmentation_event_cancel_indicator == 0)
      {
        indent_print(LVL_3, "program_segmentation_flag : %d", splice_info_section->segmentation_descriptor[i].program_segmentation_flag);
        indent_print(LVL_3, "segmentation_duration_flag : %d", splice_info_section->segmentation_descriptor[i].segmentation_duration_flag);
        indent_print(LVL_3, "delivery_not_restricted_flag : %d", splice_info_section->segmentation_descriptor[i].delivery_not_restricted_flag);
        if (splice_info_section->segmentation_descriptor[i].delivery_not_restricted_flag == 0)
        {
          indent_print(LVL_3, "web_delivery_allowed_flag : %d", splice_info_section->segmentation_descriptor[i].web_delivery_allowed_flag);
          indent_print(LVL_3, "no_regional_blackout_flag : %d", splice_info_section->segmentation_descriptor[i].no_regional_blackout_flag);
          indent_print(LVL_3, "archive_allowed_flag : %d", splice_info_section->segmentation_descriptor[i].archive_allowed_flag);
          indent_print(LVL_3, "device_restrictions : %d", splice_info_section->segmentation_descriptor[i].device_restrictions);
        }
        else
        {
          indent_print(LVL_3, "reserved_flags : %d", splice_info_section->segmentation_descriptor[i].reserved_flags);
        }
        if (splice_info_section->segmentation_descriptor[i].program_segmentation_flag == 0)
        {
          indent_print(LVL_3, "component_count : %d", splice_info_section->segmentation_descriptor[i].component_count);
          for (int j = 0; j < splice_info_section->segmentation_descriptor[i].component_count; j++)
          {
            indent_print(LVL_3, "{");
            indent_print(LVL_4, "component_tag : %d", splice_info_section->segmentation_descriptor[i].component_tags[j].component_tag);
            indent_print(LVL_4, "reserved : %d", splice_info_section->segmentation_descriptor[i].component_tags[j].reserved);
            indent_print(LVL_4, "pts_ofset : %d", splice_info_section->segmentation_descriptor[i].component_tags[j].pts_ofset);
            indent_print(LVL_3, "}");
          }
        } // program_segmentation_flag == 0

        if (splice_info_section->segmentation_descriptor[i].segmentation_duration_flag == 1)
        {
          indent_print(LVL_3, "segmentation_duration : %d", splice_info_section->segmentation_descriptor[i].segmentation_duration);
        } // segmentation_duration_flag == 1
        indent_print(LVL_3, "segmentation_upid_type : %d", splice_info_section->segmentation_descriptor[i].segmentation_upid_type);
        indent_print(LVL_3, "segmentation_upid_length : %d", splice_info_section->segmentation_descriptor[i].segmentation_upid_length);

        // TODO(oguzhank): print segmentation_upid by its type!!
        for (int k = 0; k < splice_info_section->segmentation_descriptor[i].segmentation_upid_length; k++)
        {
          indent_print(LVL_3, "segmentation_upid[%d] : 0x%02X", k, splice_info_section->segmentation_descriptor[i].segmentation_upid_data[k]);
        }
        indent_print(LVL_3, "segmentation_type_id : %d", splice_info_section->segmentation_descriptor[i].segmentation_type_id);
        indent_print(LVL_3, "segment_num : %d", splice_info_section->segmentation_descriptor[i].segment_num);
        indent_print(LVL_3, "segments_expected : %d", splice_info_section->segmentation_descriptor[i].segments_expected);
        if (splice_info_section->segmentation_descriptor[i].segmentation_type_id == 0x34 || 
            splice_info_section->segmentation_descriptor[i].segmentation_type_id == 0x36)
        {
          indent_print(LVL_3, "sub_segment_num : %d", splice_info_section->segmentation_descriptor[i].sub_segment_num);
          indent_print(LVL_3, "sub_segments_expected : %d", splice_info_section->segmentation_descriptor[i].sub_segments_expected);
        }
      } // if segmentation_event_cancel_indicator == 0

      indent_print(LVL_2, "}"); // segmentation_descriptor()
    }
    indent_print(LVL_1, "}"); // splice_descriptor() 

    if (splice_info_section->alignment_stuffing_bytes_length > 0)
    {
      indent_print(LVL_1, "alignment_stuffing {");
    }
    for (int i = 0; i < splice_info_section->alignment_stuffing_bytes_length; i++) {
      indent_print(LVL_2, "alignment_stuffing[%d] : 0x%02X", i, splice_info_section->alignment_stuffing[i]);
    }
    if (splice_info_section->alignment_stuffing_bytes_length > 0)
    {
      indent_print(LVL_1, "}"); // alignment_stuffing() 
    }
    if (splice_info_section->encrypted_packet == 1)
    {
      indent_print(LVL_1, "e_crc_32 : %d", splice_info_section->e_crc_32);
    }
    indent_print(LVL_1, "crc_32 : %d", splice_info_section->crc_32);

    indent_print(LVL_0, "}"); // splice_info_section() 
    // next item
    parsed_list = parsed_list->next;
  }

}
